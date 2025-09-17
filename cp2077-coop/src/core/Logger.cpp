#include "Logger.hpp"

namespace CoopNet {
    // Static member definitions
    std::mutex Logger::s_logMutex;
    LogLevel Logger::s_logLevel = LogLevel::DEBUG;
    std::ofstream Logger::s_logFile;
    bool Logger::s_initialized = false;
}