#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>             // For I/O operations
#include <string>               // For text operations   
#include <chrono>               // For time timestamp
#include <fstream>              // For file operations
#include <filesystem>           // For creating and reading path, files (NOTE: Reguired C++ 17)
#include <thread>               // For worker thread logic.
#include <mutex>                // For adding lines to files and writing stream to terminal
#include <queue>                // For working thread logic.
#include <condition_variable>   // For awake to worker thread.
#include <atomic>               // For wake and awake to worker thread.
#include <algorithm>            // For replace function.

#include "Level.h"
#include "Color.h"
#include "LogEntry.h"

namespace KL {
    class Logger {
        public:
            /**
             * @brief Meyers' Singleton Pattern
             */
            static Logger& get_instance() {
                static Logger instance;
                return instance;
            } // End function get_instance
            
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

            /**
             * @brief
             * @param folderPath
             * @param maxLinesPerFile
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

            void log(Level level, const std::string& msg, bool writeToFile) {
                std::lock_guard<std::mutex> lock(mMutex);

                std::string timeStampStr = get_time_stamp();
                std::string levelStr = level_to_string(level);
                std::string formattedMsg = get_formatted_message(timeStampStr, levelStr, msg);

                LogEntry log;
                log.writeToFile = writeToFile;
                log.level = level;
                log.msg = formattedMsg;

                mLogEntryQueue.push(log);
                mCV.notify_one();
            } // End function log
        private:

            // Default constructor 
            Logger() : mCurrentLineCount(0), mMaxLines(100000), mIsInitialized(false) {}
            
            // Destructor
            ~Logger() {
                shut_down();
            } // End function ~Logger

            void shut_down() {
                mIsRunning = false;
                mCV.notify_all();
                if (mWorkerThread.joinable()) mWorkerThread.join();
                if (mFileStream.is_open()) mFileStream.close();
            } // End function shut_down

            std::string level_to_string(Level level) {
                switch (level) {
                    case Level::INFO: return "INFO";
                    case Level::WARNING: return "WARNING";
                    case Level::ERROR: return "ERROR";
                    default: return "UNKNOWN";
                }
            } // End function level_to_string

            /**
             * @brief This function return the date and timestamp.
             * 
             * @return time stamp
             */
            std::string get_time_stamp() {
                const auto now = std::chrono::system_clock::now();
                const auto inTime = std::chrono::system_clock::to_time_t(now);
                const auto msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
                const auto ms = msSinceEpoch % 1000;

                std::tm buf{};
                #if defined(_WIN32)
                    if (localtime_s(&buf, &in_time_t) != 0) {
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
            } // End function get_time_stamp

            /**
             * @brief
             * 
             * @param
             * @param
             * @param
             * 
             * @return formatted message
             */
            std::string get_formatted_message(const std::string& timeStampStr, const std::string& levelStr, const std::string& msgStr) {
                std::string formattedStr = "[" + timeStampStr + "]" + "[" + levelStr + "]" + "[" + msgStr + "]";
                return formattedStr;
            } // End function get_formatted_message

            void create_new_file() {
                if (mFileStream.is_open()) {
                    mFileStream.close();
                }
                
                std::string timeStampStr = get_time_stamp();
                std::replace(timeStampStr.begin(), timeStampStr.end(), ":", "-");
                std::replace(timeStampStr.begin(), timeStampStr.end(), " ", "-");
                std::string fileName = "klog_" + timeStampStr + ".txt";
                std::filesystem::path fullPath = mLogDirectory / fileName;

                mFileStream.open(fullPath, std::ios::out | std::ios::app);
                mCurrentLineCount = 0;
            } // End function create_new_file

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

            std::string get_color_code(Level level) {
                switch (level) {
                    case Level::INFO:       return Color::GREEN;
                    case Level::WARNING:    return Color::YELLOW;
                    case Level::ERROR:      return Color::RED;
                    default:                return "";
                }
            } // End function get_color_code

            void write_to_terminal(Level level, const std::string& msg) {
                std::string colorCode = get_color_code(level);
                std::string coloredMsg = colorCode + msg + Color::RESET + "/n";

                if (Level::ERROR == level) {
                    std::cerr << coloredMsg;
                } else {
                    std::cout << coloredMsg;
                }
            } // End function write_to_terminal

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
    
                        if (log.writeToFile)
                            write_to_file(log.msg);
                        else
                            write_to_terminal(log.level, log.msg);
                    }
                }                
            } // End function process_queue
            
            std::queue<LogEntry> mLogEntryQueue;
            std::condition_variable mCV;
            std::mutex mMutex;
            std::thread mWorkerThread;
            std::atomic<bool> mIsRunning;
            std::atomic<bool> mIsInitialized;

            std::ofstream mFileStream;
            std::filesystem::path mLogDirectory;
            size_t mMaxLines;
            size_t mCurrentLineCount;
    };
}

#endif //! LOGGER_H