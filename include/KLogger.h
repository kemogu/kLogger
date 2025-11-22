#ifndef KLOGGER_H
#define KLOGGER_H

#include <iostream>     // For I/O operations
#include <string>       // For text operations   
#include <chrono>       // For time timestamp
#include <fstream>      // For file operations
#include <filesystem>   // For creating and reading path, files (NOTE: Reguired C++ 17)
#include <mutex>        // For adding lines to files and writing stream to terminal

namespace KL {
    
    enum class Level {
        INFO,
        WARNING,
        ERROR
    };

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

                // TODO: Add time stamp.


            } // End function log
        private:

            // Default constructor 
            Logger() : mCurrentLineCount(0), mMaxLines(100000), mIsInitialized(false) {}
            
            // Destructor
            ~Logger() {
                if (mFileStream.is_open()) mFileStream.close();
            } // End function ~Logger

            std::string level_to_string(Level level) {
                switch (level) {
                    case Level::INFO: return "INFO";
                    case Level::WARNING: return "WARNING";
                    case Level::ERROR: return "ERROR";
                    default: return "UNKNOWN";
                }
            } // End function level_to_string

            std::string get_time_stamp() {
                std::chrono::time_point now = std::chrono::system_clock::now();
                auto inTime = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&inTime), "%d.%m.%Y %H:%M:%S");

                return ss.str();
            } // End function get_time_stamp

            std::mutex mMutex;
            std::ofstream mFileStream;
            std::filesystem::path mLogDirectory;
            size_t mMaxLines;
            size_t mCurrentLineCount;
            bool mIsInitialized;
    };
}

#endif //! KLOGGER_H