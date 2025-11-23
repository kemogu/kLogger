#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <string>
#include "Level.h"

namespace KL {
    struct LogEntry {
        bool writeToFile;
        Level level;
        std::string msg;
    };
}

#endif //! LOGENTRY_H