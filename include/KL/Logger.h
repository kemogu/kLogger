#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>             // For I/O operations
#include <string>               // For text operations
#include <sstream>              // For std::stringstream   
#include <chrono>               // For time timestamp
#include <ctime>                // For localtime functions
#include <fstream>              // For file operations
#include <filesystem>           // For creating and reading path, files (NOTE: Reguired C++ 17)
#include <thread>               // For worker thread logic.
#include <mutex>                // For adding lines to files and writing stream to terminal
#include <queue>                // For working thread logic.
#include <condition_variable>   // For awake to worker thread.
#include <atomic>               // For wake and awake to worker thread.
#include <algorithm>            // For replace function.
#include <iomanip>              // For std::put_time, std::setw, std::setfill

#include "Level.h"
#include "Color.h"
#include "LogEntry.h"

namespace KL {
    /**
     * @class Logger
     * 
     * @brief A thread-safe, asynchronous Logger class implemented using the Singleton pattern.
     * 
     * This logger supports writing to both the console (with colors) and rotating log files.
     * It utilizes a worker thread to process log entries from a queue to avoid blocking
     * the main execution thread.
     */
    class Logger {
        public:
            /**
             * @brief Retrieves the singleton instance of the Logger.
             * 
             * Implements Meyers' Singleton Pattern to ensure only one instance exists
             * and is initialized safely.
             * 
             * @return Logger& Reference to the static Logger instance.
             */
            static Logger& get_instance() {
                static Logger instance;
                return instance;
            } // End function get_instance
            
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

            /**
             * @brief Initializes the logger configuration and starts the worker thread.
             * 
             * If the folder path is not provided, the current working directory is used.
             * Creates the directory if it does not exist.
             * 
             * @param folderPath Path to the directory where log files will be stored. Default is current path.
             * @param maxLinesPerFile Maximum number of lines per log file before rotation. Default is 100,000.
             */
            void init(const std::string& folderPath = "", size_t maxLinesPerFile = 100000) {
                std::lock_guard<std::mutex> lock(mMutex);
                if (true == mIsInitialized) return;

                mMaxLines = maxLinesPerFile;

                if (folderPath.empty()) {
                    mLogDirectory = std::filesystem::current_path();
                } else {
                    mLogDirectory = folderPath;

                    if (false == std::filesystem::exists(mLogDirectory)) {
                        std::filesystem::create_directories(mLogDirectory);
                    }
                }

                mIsInitialized = true;
                mIsRunning = true;
                mWorkerThread = std::thread(&Logger::process_queue, this);
            } // End function init

            /**
             * @brief Pushes a new log message to the processing queue.
             * 
             * This method is thread-safe. If the logger is not initialized, 
             * it attempts to initialize it with default values.
             * 
             * @param level The severity level of the log (INFO, WARNING, ERROR).
             * @param msg The actual log message.
             * @param writeToFile Flag indicating whether this message should be written to the file.
             */
            void log(Level level, std::string msg, bool writeToFile) {
                
                if (false == mIsInitialized) {
                    std::cout << "Logger initializing with default values. " << "\n"; 
                    init();
                }
                    
                auto now = std::chrono::system_clock::now();  
                {
                    std::lock_guard<std::mutex> lock(mMutex);
                    mLogEntryQueue.push(LogEntry{ writeToFile, now, level, std::move(msg) });
                }
                mCV.notify_one();
            } // End function log
        private:

            // Default constructor 
            Logger() : mCurrentLineCount(0), mMaxLines(100000), mIsInitialized(false), mIsRunning(false) {}
            
            // Destructor
            ~Logger() {
                shut_down();
            } // End function ~Logger
            
            /**
             * @brief Stops the worker thread and closes the open file stream.
             */
            void shut_down() {
                mIsRunning = false;
                mCV.notify_all();
                if (mWorkerThread.joinable()) mWorkerThread.join();
                if (mFileStream.is_open()) {
                    mFileStream.flush();
                    mFileStream.close();  
                } 
            } // End function shut_down
            
            /**
             * @brief Converts the Log Level enum to its string representation.
             * 
             * @param level The Level enum value.
             * @return std::string String representation (e.g., "INFO").
             */
            std::string level_to_string(Level level) {
                switch (level) {
                    case Level::INFO: return "INFO";
                    case Level::WARNING: return "WARNING";
                    case Level::ERROR: return "ERROR";
                    default: return "UNKNOWN";
                }
            } // End function level_to_string
            
            /**
             * @brief This function returns date and timestamp according now.
             * 
             * @return time stamp
             */
            std::string get_time_stamp_for_now() {
                const auto now = std::chrono::system_clock::now();
                const auto inTime = std::chrono::system_clock::to_time_t(now);
                const auto msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
                const auto ms = msSinceEpoch % 1000;

                std::tm buf{};
                #if defined(_WIN32)
                    if (localtime_s(&buf, &inTime) != 0) {
                        throw std::runtime_error("localtime_s failed");
                    }
                #else
                    if (localtime_r(&inTime, &buf) == nullptr) {
                        throw std::runtime_error("localtime_r failed");
                    }
                #endif
                
                // Formatted D-M-Y H:M:S.MS
                std::ostringstream oss;
                oss << std::put_time(&buf, "%d-%m-%Y %H:%M:%S");
                oss << '.' << std::setw(3) << std::setfill('0') << ms.count();

                return oss.str();
            }

            /**
             * @brief This function returns the date and timestamp according to log entry.
             * 
             * @param log
             * 
             * @return time stamp
             */
            std::string get_time_stamp_for_log(const LogEntry& log) {
                const auto inTime = std::chrono::system_clock::to_time_t(log.timeStamp);
                const auto msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(log.timeStamp.time_since_epoch());
                const auto ms = msSinceEpoch % 1000;

                std::tm buf{};
                #if defined(_WIN32)
                    if (localtime_s(&buf, &inTime) != 0) {
                        throw std::runtime_error("localtime_s failed");
                    }
                #else
                    if (localtime_r(&inTime, &buf) == nullptr) {
                        throw std::runtime_error("localtime_r failed");
                    }
                #endif
                
                // Formatted D-M-Y H:M:S.MS
                std::ostringstream oss;
                oss << std::put_time(&buf, "%d-%m-%Y %H:%M:%S");
                oss << '.' << std::setw(3) << std::setfill('0') << ms.count();

                return oss.str();
            } // End function get_time_stamp_for_log

            /**
             * @brief Formats the log message for final output.
             * 
             * Combines timestamp, level, and message body.
             * Example: [Date-Time][INFO][Message]
             * 
             * @param log The LogEntry to format.
             * @return std::string The fully formatted log string.
             */
            std::string get_formatted_message(const LogEntry& log) {
                std::ostringstream formattedMsg;
                formattedMsg << "[" << get_time_stamp_for_log(log) << "]" << "[" << level_to_string(log.level) << "]" << "[" << log.msg << "]";
                return formattedMsg.str();
            } // End function get_formatted_message
            
            /**
             * @brief Closes the current file (if open) and creates a new one with a timestamped name.
             * 
             * The filename format is: klog_d-m-y-H-M-S.MS.txt
             */
            void create_new_file() {
                try {
                    if (mFileStream.is_open()) {
                        mFileStream.close();
                    }
                    
                    std::string timeStampStr = get_time_stamp_for_now();
                    std::replace(timeStampStr.begin(), timeStampStr.end(), ':', '-');
                    std::replace(timeStampStr.begin(), timeStampStr.end(), ' ', '-');
                    std::string fileName = "klog_" + timeStampStr + ".txt";
                    std::filesystem::path fullPath = mLogDirectory / fileName;

                    mFileStream.open(fullPath, std::ios::out | std::ios::app);
                    mCurrentLineCount = 0;
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                }
            } // End function create_new_file
            
            /**
             * @brief Writes the formatted message to the log file.
             * 
             * Handles file rotation if the maximum line count is reached or the file is closed.
             * 
             * @param msg The formatted message string.
             */
            void write_to_file(const std::string& msg) {
                bool needNewFile = ((false == mFileStream.is_open()) || mCurrentLineCount >= mMaxLines);
                if (true == needNewFile) {
                    create_new_file();
                }

                if (true == mFileStream.is_open()) {
                    mFileStream << msg << "\n";
                    mCurrentLineCount ++;
                }
            } // End function write_to_file
            
            /**
             * @brief Retrieves the ANSI color code corresponding to the log level.
             * 
             * @param level The log level.
             * @return std::string ANSI color escape code.
             */
            std::string get_color_code(Level level) {
                switch (level) {
                    case Level::INFO:       return Color::GREEN;
                    case Level::WARNING:    return Color::YELLOW;
                    case Level::ERROR:      return Color::RED;
                    default:                return "";
                }
            } // End function get_color_code
            
            /**
             * @brief Writes the formatted message to the standard output (Terminal).
             * 
             * Applies color coding based on the severity level. 
             * ERROR level uses std::cerr, others use std::cout.
             * 
             * @param level The log level (determines color and stream).
             * @param msg The formatted message string.
             */
            void write_to_terminal(Level level, const std::string& msg) {
                std::string colorCode = get_color_code(level);
                std::string coloredMsg = colorCode + msg + Color::RESET + "\n";

                if (Level::ERROR == level) {
                    std::cerr << coloredMsg;
                } else {
                    std::cout << coloredMsg;
                }
            } // End function write_to_terminal

            /**
             * @brief The main loop for the worker thread.
             * 
             * Continuously waits for log entries in the queue. When awakened,
             * it swaps the queue to a local buffer (for minimal lock duration)
             * and processes the logs by writing to file and/or terminal.
             */
            void process_queue() {
                std::queue<LogEntry> localQueue;
                while (true) {
                    
                    {
                        std::unique_lock<std::mutex> lock(mMutex);
                        mCV.wait(lock, [this] {
                            return !mLogEntryQueue.empty() || !mIsRunning;
                        });
    
                        bool shouldWorkerThreadWork = (!mIsRunning && mLogEntryQueue.empty());
                        if (shouldWorkerThreadWork) break;
    
                        std::swap(localQueue, mLogEntryQueue);
                    }

                    if (localQueue.empty()) continue;

                    while (!localQueue.empty()) {
                        LogEntry log = std::move(localQueue.front());
                        localQueue.pop();
                        
                        std::string formattedMessage = get_formatted_message(log);
                        if (log.writeToFile)
                            write_to_file(formattedMessage);
                        write_to_terminal(log.level, formattedMessage);
                    }
                }                
            } // End function process_queue
            
            std::queue<LogEntry> mLogEntryQueue;    // Queue to hold pending log entries.
            std::condition_variable mCV;            // Condition variable to wake up worker thread.
            std::mutex mMutex;                      // Mutex for thread safety.
            std::thread mWorkerThread;              // Background thread for processing logs.
            std::atomic<bool> mIsRunning;           // Atomic flag to control the worker loop.
            std::atomic<bool> mIsInitialized;       // Atomic flag to check initialization status.

            std::ofstream mFileStream;              // File stream for writing logs.
            std::filesystem::path mLogDirectory;    // Directory path for log files.
            size_t mMaxLines;                       // Max lines per file limit.
            size_t mCurrentLineCount;               // Current line count in the active file.
    };
}

#endif //! LOGGER_H