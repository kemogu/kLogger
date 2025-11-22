#ifndef COLOR_H
#define COLOR_H

#include <string>

namespace KL {
    namespace Color {
        // ANSI escape codes.
        static const std::string RED     = "\033[31m";
        static const std::string GREEN   = "\033[32m";
        static const std::string YELLOW  = "\033[33m";
        static const std::string RESET   = "\033[0m";
    };
}

#endif //! COLOR_H