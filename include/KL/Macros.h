#ifndef MACROS_H
#define MACROS_H

#include "Logger.h" // Logger sınıfının tanımını içerdiğinden emin olun

/**
 * @file LogMacros.h
 * @brief Helper macros for the KL::Logger class.
 * 
 * LOG_ prefix: Writes only to the terminal (Console).
 * FLOG_ prefix: Writes to both the terminal and the log file.
 */

// -----------------------------------------------------------------------------
// CONSOLE ONLY LOGGING MACROS (writeToFile = false)
// -----------------------------------------------------------------------------

/**
 * @brief Logs an INFO message to the console only.
 * @param msg The message string (std::string compatible).
 */
#define LOG_INFO(msg) \
    KL::Logger::get_instance().log(KL::Level::INFO, msg, false)

/**
 * @brief Logs a WARNING message to the console only.
 * @param msg The message string (std::string compatible).
 */
#define LOG_WARNING(msg) \
    KL::Logger::get_instance().log(KL::Level::WARNING, msg, false)

/**
 * @brief Logs an ERROR message to the console only.
 * @param msg The message string (std::string compatible).
 */
#define LOG_ERROR(msg) \
    KL::Logger::get_instance().log(KL::Level::ERROR, msg, false)


// -----------------------------------------------------------------------------
// FILE (+ CONSOLE) LOGGING MACROS (writeToFile = true)
// -----------------------------------------------------------------------------

/**
 * @brief Logs an INFO message to the console AND the log file.
 * @param msg The message string (std::string compatible).
 */
#define FLOG_INFO(msg) \
    KL::Logger::get_instance().log(KL::Level::INFO, msg, true)

/**
 * @brief Logs a WARNING message to the console AND the log file.
 * @param msg The message string (std::string compatible).
 */
#define FLOG_WARNING(msg) \
    KL::Logger::get_instance().log(KL::Level::WARNING, msg, true)

/**
 * @brief Logs an ERROR message to the console AND the log file.
 * @param msg The message string (std::string compatible).
 */
#define FLOG_ERROR(msg) \
    KL::Logger::get_instance().log(KL::Level::ERROR, msg, true)

#endif // MACROS_H