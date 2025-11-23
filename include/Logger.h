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

#include "Level.h"
#include "Color.h"
#include "LogEntry.h"

// TODO: Add worker thread logic.
// TODO: Add filename validation.

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
            } // End function init

            void log(Level level, const std::string& msg, bool writeToFile) {
                std::lock_guard<std::mutex> lock(mMutex);

                std::string timeStampStr = get_time_stamp();
                std::string levelStr = level_to_string(level);
                std::string formattedMsg = get_formatted_message(timeStampStr, levelStr, msg);

                if (true == writeToFile) {
                    write_to_file(formattedMsg);
                }
                
                write_to_terminal(level, formattedMsg);
            } // End function log
        private:

            // Default constructor 
            Logger() : mCurrentLineCount(0), mMaxLines(100000), mIsInitialized(false) {}
            
            // Destructor
            ~Logger() {
                if (mFileStream.is_open()) mFileStream.close();
            } // End function ~Logger

            void shut_down() {
                // TODO: Write shut_down function.
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
                // TODO: Add milisecond.
                std::chrono::time_point now = std::chrono::system_clock::now();
                auto inTime = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&inTime), "%d.%m.%Y %H:%M:%S");

                return ss.str();
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