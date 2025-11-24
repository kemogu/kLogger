#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <string>           // For std::string
#include <chrono>           // For std::chrono::syttem_clock::time_point

#include "Level.h"

namespace KL {
    struct LogEntry {
        bool writeToFile;
        std::chrono::system_clock::time_point timeStamp;
        Level level;
        std::string msg;
    };
}

#endif //! LOGENTRY_H