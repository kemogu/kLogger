#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <cstdio>
#include <cstring>

// TODO: Solve multithread crashing.
// TODO: Add automatic flush().

// Project-specific headers
#include "Level.h"
#include "LogEntry.h"
#include "Color.h"

namespace KL {

/**
 * @class Logger
 * @brief High-performance, thread-safe, asynchronous logging system using the Singleton pattern.
 *
 * This logger writes colored output to the console and rotates log files based on line count.
 * It follows a producer-consumer model: application threads push log entries into a lock-free-style
 * queue while a dedicated background thread consumes them. This design ensures zero blocking on I/O.
 *
 * @note Fully compatible with C++17 (no C++20 features used).
 * @note Zero dynamic allocations in the hot path (timestamp formatting uses stack buffer).
 */
class Logger {
public:
    /**
     * @brief Returns the singleton instance (Meyers' Singleton - thread-safe since C++11).
     * @return Reference to the global Logger instance.
     */
    static Logger& get_instance() {
        static Logger instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Initializes the logger and starts the background worker thread.
     *
     * Safe to call multiple times. Should ideally be called once at program startup.
     *
     * @param folderPath Directory where log files will be stored. Empty = current working directory.
     * @param maxLinesPerFile Maximum lines per file before rotation (default: 100,000).
     */
    void init(const std::string& folderPath = "", size_t maxLinesPerFile = 100000)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mIsInitialized) {
            return;
        }

        mMaxLines = maxLinesPerFile;

        // Resolve log directory
        std::error_code ec;
        mLogDirectory = folderPath.empty() ? std::filesystem::current_path() : std::filesystem::path(folderPath);
        std::filesystem::create_directories(mLogDirectory, ec);
        if (ec) {
            std::cerr << "[Logger] Failed to create log directory: " << ec.message() << std::endl;
        }

        // Performance: disable stream synchronization with C stdio
        std::ios::sync_with_stdio(false);
        std::cout.tie(nullptr);

        #ifdef _WIN32
            // Enable ANSI color support on Windows 10+ consoles
            auto enableVT = []() {
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                if (hOut == INVALID_HANDLE_VALUE) return;
                DWORD dwMode = 0;
                if (!GetConsoleMode(hOut, &dwMode)) return;
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            };
            enableVT();
        #endif

        mIsRunning = true;
        mWorkerThread = std::thread(&Logger::process_queue, this);
        mIsInitialized = true;
    }

    /**
     * @brief Queues a log message for asynchronous processing.
     *
     * This is the main logging function. If the logger hasn't been initialized yet,
     * it will automatically initialize with default settings.
     *
     * @param level       Log severity level
     * @param msg         Log message (moved into the queue)
     * @param writeToFile Whether to write this entry to file (default: true)
     */
    void log(Level level, std::string msg, bool writeToFile = true)
    {
        if (!mIsInitialized) {
            init();  // Lazy initialization fallback
        }

        auto now = std::chrono::system_clock::now();

        {
            std::lock_guard<std::mutex> lock(mMutex);
            mLogEntryQueue.emplace(LogEntry{writeToFile, now, level, std::move(msg)});
        }

        mCV.notify_one();
    }

    /**
     * @brief Forces immediate flush of all queued logs and shuts down the worker thread.
     *
     * Called automatically from destructor, but can be called manually before program exit
     * if strict ordering or immediate flush is required.
     */
    void flush_and_shutdown()
    {
        shut_down();
    }

private:
    /// Private constructor - initializes member variables
    Logger()
        : mCurrentLineCount(0)
        , mMaxLines(100000)
        , mIsInitialized(false)
        , mIsRunning(false)
    {}

    /// Destructor - ensures clean shutdown
    ~Logger()
    {
        shut_down();
    }

    /// Signals worker thread to exit and joins it
    void shut_down()
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mIsRunning = false;
        }

        mCV.notify_all();

        if (mWorkerThread.joinable()) {
            mWorkerThread.join();
        }

        if (mFileStream.is_open()) {
            mFileStream.flush();
            mFileStream.close();
        }
    }

    /**
     * @brief Formats a time_point into a fixed-size char buffer using snprintf (zero allocation).
     *
     * Format: DD-MM-YYYY HH:MM:SS.mmm
     *
     * @param tp     Time point to format
     * @param buffer Destination buffer (must be at least 64 bytes)
     * @param size   Size of the destination buffer
     */
    void format_timestamp(const std::chrono::system_clock::time_point& tp, char* buffer, size_t size) const
    {
        const auto time_t_val = std::chrono::system_clock::to_time_t(tp);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;

        std::tm tm_val{};
        #if defined(_WIN32)
                localtime_s(&tm_val, &time_t_val);
        #else
                localtime_r(&time_t_val, &tm_val);
        #endif

        std::snprintf(buffer, size, "%02d-%02d-%04d %02d:%02d:%02d.%03d",
                      tm_val.tm_mday, tm_val.tm_mon + 1, tm_val.tm_year + 1900,
                      tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec,
                      static_cast<int>(ms.count()));
    }

    /// Converts Level enum to string literal
    constexpr const char* level_to_string(Level level) const noexcept
    {
        switch (level) {
            case Level::INFO:    return "INFO";
            case Level::WARNING: return "WARNING";
            case Level::ERROR:   return "ERROR";
            default:             return "UNKNOWN";
        }
    }

    /// Returns ANSI color escape sequence for the given level
    constexpr const char* get_color_code(Level level) const noexcept
    {
        switch (level) {
            case Level::INFO:    return "\033[92m"; // Bright Green
            case Level::WARNING: return "\033[93m"; // Bright Yellow
            case Level::ERROR:   return "\033[91m"; // Bright Red
            default:             return "\033[0m";  // Reset
        }
    }

    /// Background thread main loop - processes queued log entries
    void process_queue()
    {
        std::queue<LogEntry> localQueue;

        char timeBuffer[64]{};   // Stack-allocated timestamp buffer
        std::string lineBuffer;
        lineBuffer.reserve(512); // Pre-allocate for typical log size

        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mCV.wait(lock, [this] { return !mLogEntryQueue.empty() || !mIsRunning; });

                if (!mIsRunning && mLogEntryQueue.empty()) {
                    break;
                }

                std::swap(localQueue, mLogEntryQueue);  // Release lock as fast as possible
            }

            while (!localQueue.empty())
            {
                const auto& entry = localQueue.front();
                const Level& level = entry.level;

                // Format timestamp without allocation
                format_timestamp(entry.timeStamp, timeBuffer, sizeof(timeBuffer));

                // Build final line (single allocation at most)
                lineBuffer.clear();
                lineBuffer += '[';
                lineBuffer += timeBuffer;
                lineBuffer += "][";
                lineBuffer += level_to_string(level);
                lineBuffer += "][";
                lineBuffer += entry.msg;
                lineBuffer += ']';

                // Write to file if requested
                if (entry.writeToFile) {
                    write_to_file(lineBuffer);
                }

                // Write to console with color
                if (Level::ERROR == level) {
                    std::cerr << get_color_code(level) << lineBuffer << Color::RESET << std::endl;
                }
                else {
                    std::cout << get_color_code(level) << lineBuffer << Color::RESET << "\n";
                }

                localQueue.pop();
            }
        }
    }

    /// Writes a line to the current log file, creating a new one if necessary
    void write_to_file(const std::string& msg)
    {
        if (!mFileStream.is_open() || mCurrentLineCount >= mMaxLines) {
            create_new_file();
        }

        if (mFileStream.is_open()) {
            mFileStream << msg << '\n';
            ++mCurrentLineCount;
        }
        // If file still not open → silently drop (disk full, permission, etc.)
        // Critical applications may want to log this to stderr
    }

    /// Closes current file and opens a new one with timestamped name
    void create_new_file()
    {
        if (mFileStream.is_open()) {
            mFileStream.flush();
            mFileStream.close();
        }

        const auto now = std::chrono::system_clock::now();
        const auto time_t_val = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm_val{};
        #if defined(_WIN32)
                localtime_s(&tm_val, &time_t_val);
        #else
                localtime_r(&time_t_val, &tm_val);
        #endif

        char filename[128];
        std::snprintf(filename, sizeof(filename),
                      "klog_%02d-%02d-%04d-%02d-%02d-%02d-%03d.txt",
                      tm_val.tm_mday, tm_val.tm_mon + 1, tm_val.tm_year + 1900,
                      tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec,
                      static_cast<int>(ms.count()));

        const std::filesystem::path fullPath = mLogDirectory / filename;
        mFileStream.open(fullPath, std::ios::out | std::ios::app);

        if (!mFileStream.is_open()) {
            std::cerr << "[Logger] CRITICAL: Failed to open log file: " << fullPath << std::endl;
        }

        mCurrentLineCount = 0;
    }

    // Member variables
    std::queue<LogEntry> mLogEntryQueue;
    std::condition_variable mCV;
    std::mutex mMutex;
    std::thread mWorkerThread;

    std::atomic<bool> mIsRunning {false};
    bool mIsInitialized {false};  // Protected by mMutex

    std::ofstream mFileStream;
    std::filesystem::path mLogDirectory;
    size_t mMaxLines{100000};
    size_t mCurrentLineCount{0};
};

} // namespace KL

#endif // LOGGER_H