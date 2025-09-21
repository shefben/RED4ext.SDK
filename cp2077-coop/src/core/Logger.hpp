#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <cstdio>
#include <cstdarg>

// Windows.h defines ERROR as a macro, so we need to undefine it
#ifdef ERROR
#undef ERROR
#endif

namespace CoopNet {
    
    enum class LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };
    
    class Logger {
    private:
        static std::mutex s_logMutex;
        static LogLevel s_logLevel;
        static std::ofstream s_logFile;
        static bool s_initialized;
        
        static std::string GetTimestamp() {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return ss.str();
        }
        
        static const char* LevelToString(LogLevel level) {
            switch (level) {
                case LogLevel::DEBUG: return "DEBUG";
                case LogLevel::INFO: return "INFO";
                case LogLevel::WARNING: return "WARN";
                case LogLevel::ERROR: return "ERROR";
                default: return "UNKNOWN";
            }
        }
        
    public:
        static void Initialize() {
            std::lock_guard<std::mutex> lock(s_logMutex);
            if (!s_initialized) {
                s_logFile.open("cp2077_coop.log", std::ios::app);
                s_logLevel = LogLevel::DEBUG;
                s_initialized = true;
                Log(LogLevel::INFO, "Logger initialized");
            }
        }
        
        static void Shutdown() {
            std::lock_guard<std::mutex> lock(s_logMutex);
            if (s_initialized) {
                Log(LogLevel::INFO, "Logger shutting down");
                s_logFile.close();
                s_initialized = false;
            }
        }
        
        static void Log(LogLevel level, const std::string& message) {
            if (level < s_logLevel) return;
            
            std::lock_guard<std::mutex> lock(s_logMutex);
            std::string timestamp = GetTimestamp();
            std::string levelStr = LevelToString(level);
            std::string fullMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
            
            // Output to console
            if (level >= LogLevel::ERROR) {
                std::cerr << fullMessage << std::endl;
            } else {
                std::cout << fullMessage << std::endl;
            }
            
            // Output to file
            if (s_logFile.is_open()) {
                s_logFile << fullMessage << std::endl;
                s_logFile.flush();
            }
        }
        
        template<typename... Args>
        static void LogFormatted(LogLevel level, Args... args) {
            if (level < s_logLevel) return;

            std::stringstream ss;
            ((ss << args << " "), ...);
            std::string message = ss.str();

            // Remove trailing space
            if (!message.empty() && message.back() == ' ') {
                message.pop_back();
            }

            Log(level, message);
        }
    };
    
    // Static member definitions are in Logger.cpp
}

// Convenience macros for easy logging
#define LogDebug(msg) CoopNet::Logger::Log(CoopNet::LogLevel::DEBUG, msg)
#define LogInfo(...) CoopNet::Logger::LogFormatted(CoopNet::LogLevel::INFO, __VA_ARGS__)
#define LogWarning(...) CoopNet::Logger::LogFormatted(CoopNet::LogLevel::WARNING, __VA_ARGS__)
#define LogError(...) CoopNet::Logger::LogFormatted(CoopNet::LogLevel::ERROR, __VA_ARGS__)

// Printf-style logging functions with proper variadic support
inline void LogPrintfImpl(CoopNet::LogLevel level, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    CoopNet::Logger::Log(level, std::string(buffer));
}

// Formatted logging macros with proper variadic support
#define LogDebugF(...) LogPrintfImpl(CoopNet::LogLevel::DEBUG, __VA_ARGS__)
#define LogInfoF(...) LogPrintfImpl(CoopNet::LogLevel::INFO, __VA_ARGS__)
#define LogWarningF(...) LogPrintfImpl(CoopNet::LogLevel::WARNING, __VA_ARGS__)
#define LogErrorF(...) LogPrintfImpl(CoopNet::LogLevel::ERROR, __VA_ARGS__)