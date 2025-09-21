#pragma once

#include <RED4ext/RED4ext.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <string>
#include <fstream>
#include <exception>

namespace CoopNet {

// Error severity levels
enum class ErrorSeverity : uint8_t {
    Debug = 0,          // Development information
    Info = 1,           // General information
    Warning = 2,        // Potential issues
    Error = 3,          // Recoverable errors
    Critical = 4,       // Critical errors that affect functionality
    Fatal = 5           // Unrecoverable errors
};

// Error categories
enum class ErrorCategory : uint8_t {
    System = 0,         // System-level errors
    Network = 1,        // Network communication errors
    Audio = 2,          // Voice/audio errors
    Performance = 3,    // Performance-related issues
    UI = 4,             // User interface errors
    Game = 5,           // Game integration errors
    Database = 6,       // Database operation errors
    Security = 7,       // Security and validation errors
    Custom = 255        // Custom application errors
};

// Error handling strategies
enum class ErrorHandlingStrategy : uint8_t {
    Ignore = 0,         // Log but don't take action
    Retry = 1,          // Attempt to retry the operation
    Fallback = 2,       // Use fallback mechanism
    Escalate = 3,       // Escalate to higher severity
    Terminate = 4       // Terminate the operation/system
};

// Log output targets
enum class LogTarget : uint8_t {
    Console = 0x01,     // Console output
    File = 0x02,        // File logging
    Network = 0x04,     // Network logging
    Debugger = 0x08,    // Debug output
    UI = 0x10,          // UI notifications
    All = 0xFF          // All targets
};

// Bitwise operators for LogTarget enum class
inline LogTarget operator|(LogTarget lhs, LogTarget rhs) {
    return static_cast<LogTarget>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline LogTarget operator&(LogTarget lhs, LogTarget rhs) {
    return static_cast<LogTarget>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline LogTarget& operator|=(LogTarget& lhs, LogTarget rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline LogTarget& operator&=(LogTarget& lhs, LogTarget rhs) {
    lhs = lhs & rhs;
    return lhs;
}

inline bool operator!=(LogTarget lhs, LogTarget rhs) {
    return static_cast<uint8_t>(lhs) != static_cast<uint8_t>(rhs);
}

// Forward declarations
struct ErrorInfo;
struct LogEntry;
struct ErrorHandler;
struct LoggingConfig;
struct ErrorStatistics;

// Error information structure
struct ErrorInfo {
    uint64_t errorId;
    ErrorSeverity severity;
    ErrorCategory category;
    std::string errorCode;
    std::string message;
    std::string details;
    std::string sourceFile;
    std::string sourceFunction;
    uint32_t sourceLine;
    std::chrono::steady_clock::time_point timestamp;
    uint32_t threadId;
    std::string stackTrace;
    std::unordered_map<std::string, std::string> context;
    uint32_t occurrenceCount;
    std::chrono::steady_clock::time_point firstOccurrence;
    std::chrono::steady_clock::time_point lastOccurrence;
};

// Log entry structure
struct LogEntry {
    uint64_t entryId;
    ErrorSeverity level;
    ErrorCategory category;
    std::string logger;
    std::string message;
    std::chrono::steady_clock::time_point timestamp;
    uint32_t threadId;
    std::unordered_map<std::string, std::string> metadata;
};

// Error handler configuration
struct ErrorHandler {
    std::string handlerName;
    ErrorCategory category;
    ErrorSeverity minSeverity;
    ErrorHandlingStrategy strategy;
    uint32_t maxRetries;
    std::chrono::milliseconds retryDelay;
    std::function<bool(const ErrorInfo&)> customHandler;
    std::function<bool(const ErrorInfo&)> fallbackHandler;
    bool isActive;
    uint64_t handledCount;
};

// Logging configuration
struct LoggingConfig {
    ErrorSeverity minLogLevel = ErrorSeverity::Info;
    LogTarget outputTargets = LogTarget::All;
    std::string logDirectory = "logs/";
    std::string logFileName = "coopnet.log";
    uint64_t maxLogFileSize = 10 * 1024 * 1024; // 10MB
    uint32_t maxLogFiles = 5;
    bool enableRotation = true;
    bool enableCompression = true;
    bool enableAsync = true;
    bool enableStackTrace = true;
    bool enableTimestamps = true;
    bool enableThreadInfo = true;
    std::string logFormat = "[{timestamp}] [{level}] [{category}] {message}";
    uint32_t flushInterval = 1000; // milliseconds
    uint32_t bufferSize = 8192;
};

// Error statistics
struct ErrorStatistics {
    std::unordered_map<ErrorSeverity, uint64_t> errorCounts;
    std::unordered_map<ErrorCategory, uint64_t> categoryCounts;
    uint64_t totalErrors = 0;
    uint64_t totalWarnings = 0;
    uint64_t totalCriticalErrors = 0;
    uint64_t totalFatalErrors = 0;
    uint64_t handledErrors = 0;
    uint64_t unhandledErrors = 0;
    std::chrono::steady_clock::time_point sessionStart;
    std::chrono::steady_clock::time_point lastError;
    float errorsPerMinute = 0.0f;
};

// Exception wrapper for better error handling
class CoopNetException : public std::exception {
public:
    CoopNetException(ErrorCategory category, ErrorSeverity severity,
                    const std::string& message, const std::string& details = "");

    const char* what() const noexcept override;
    ErrorCategory GetCategory() const { return m_category; }
    ErrorSeverity GetSeverity() const { return m_severity; }
    const std::string& GetDetails() const { return m_details; }
    uint64_t GetErrorId() const { return m_errorId; }

private:
    ErrorCategory m_category;
    ErrorSeverity m_severity;
    std::string m_message;
    std::string m_details;
    uint64_t m_errorId;
};

// Main error management system
class ErrorManager {
public:
    static ErrorManager& Instance();

    // System lifecycle
    bool Initialize(const LoggingConfig& config = LoggingConfig{});
    void Shutdown();

    // Error reporting
    uint64_t ReportError(ErrorCategory category, ErrorSeverity severity,
                        const std::string& message, const std::string& details = "",
                        const std::string& sourceFile = "", const std::string& sourceFunction = "",
                        uint32_t sourceLine = 0);

    uint64_t ReportException(const std::exception& ex, ErrorCategory category = ErrorCategory::System,
                           const std::string& context = "");

    bool HandleError(uint64_t errorId);
    bool HandleError(const ErrorInfo& error);

    // Logging
    void Log(ErrorSeverity level, ErrorCategory category, const std::string& logger,
            const std::string& message, const std::unordered_map<std::string, std::string>& metadata = {});

    void LogDebug(const std::string& logger, const std::string& message);
    void LogInfo(const std::string& logger, const std::string& message);
    void LogWarning(const std::string& logger, const std::string& message);
    void LogError(const std::string& logger, const std::string& message);
    void LogCritical(const std::string& logger, const std::string& message);
    void LogFatal(const std::string& logger, const std::string& message);

    // Error handler management
    bool RegisterErrorHandler(const ErrorHandler& handler);
    bool UnregisterErrorHandler(const std::string& handlerName);
    void SetDefaultStrategy(ErrorCategory category, ErrorHandlingStrategy strategy);
    ErrorHandlingStrategy GetDefaultStrategy(ErrorCategory category) const;

    // Error retrieval and analysis
    ErrorInfo GetError(uint64_t errorId) const;
    std::vector<ErrorInfo> GetRecentErrors(uint32_t count = 100) const;
    std::vector<ErrorInfo> GetErrorsByCategory(ErrorCategory category, uint32_t count = 100) const;
    std::vector<ErrorInfo> GetErrorsBySeverity(ErrorSeverity severity, uint32_t count = 100) const;

    // Statistics and reporting
    ErrorStatistics GetStatistics() const;
    void ResetStatistics();
    std::string GenerateErrorReport() const;
    std::string GenerateSessionSummary() const;

    // Configuration management
    void UpdateConfig(const LoggingConfig& config);
    LoggingConfig GetConfig() const;
    void SetLogLevel(ErrorSeverity level);
    ErrorSeverity GetLogLevel() const;
    void SetLogTargets(LogTarget targets);
    LogTarget GetLogTargets() const;

    // File management
    bool RotateLogFile();
    bool CompressOldLogs();
    void CleanupOldLogs();
    std::vector<std::string> GetLogFiles() const;

    // Advanced features
    void EnableStackTrace(bool enabled);
    bool IsStackTraceEnabled() const;
    void SetContextValue(const std::string& key, const std::string& value);
    void RemoveContextValue(const std::string& key);
    std::unordered_map<std::string, std::string> GetContext() const;

    // Crash handling
    void RegisterCrashHandler();
    void UnregisterCrashHandler();
    bool GenerateCrashDump(const std::string& filename = "");
    void SetCrashDumpDirectory(const std::string& directory);

    // Network logging
    void EnableNetworkLogging(bool enabled, const std::string& endpoint = "");
    bool SendErrorToServer(const ErrorInfo& error);

    // Error suppression and filtering
    void SuppressError(const std::string& errorCode, std::chrono::milliseconds duration = std::chrono::milliseconds(0));
    void UnsuppressError(const std::string& errorCode);
    bool IsErrorSuppressed(const std::string& errorCode) const;
    void SetErrorFilter(std::function<bool(const ErrorInfo&)> filter);

    // Event callbacks
    using ErrorEventCallback = std::function<void(const ErrorInfo&)>;
    void RegisterErrorCallback(ErrorSeverity minSeverity, ErrorEventCallback callback);
    void UnregisterErrorCallback();

private:
    ErrorManager() = default;
    ~ErrorManager() = default;
    ErrorManager(const ErrorManager&) = delete;
    ErrorManager& operator=(const ErrorManager&) = delete;

    // Core processing
    void ProcessingLoop();
    void ProcessLogQueue();
    void ProcessErrorQueue();
    void FlushLogs();
    void UpdateStatistics();

    // Log file management
    bool OpenLogFile();
    void CloseLogFile();
    bool ShouldRotateLog() const;
    std::string GenerateLogFileName(uint32_t index = 0) const;
    void WriteLogEntry(const LogEntry& entry);
    std::string FormatLogEntry(const LogEntry& entry) const;

    // Error processing
    uint64_t GenerateErrorId();
    uint64_t GenerateLogEntryId();
    bool ExecuteErrorHandler(const ErrorInfo& error, const ErrorHandler& handler);
    std::string CaptureStackTrace() const;
    uint32_t GetCurrentThreadId() const;

    // Utility methods
    std::string GetSeverityString(ErrorSeverity severity) const;
    std::string GetCategoryString(ErrorCategory category) const;
    std::string GetTimestampString(std::chrono::steady_clock::time_point timestamp) const;
    std::string FormatMessage(const std::string& format, const std::unordered_map<std::string, std::string>& values) const;

    // Data storage
    LoggingConfig m_config;
    ErrorStatistics m_statistics;
    std::unordered_map<uint64_t, ErrorInfo> m_errorHistory;
    std::unordered_map<std::string, ErrorHandler> m_errorHandlers;
    std::unordered_map<ErrorCategory, ErrorHandlingStrategy> m_defaultStrategies;
    std::unordered_map<std::string, std::string> m_globalContext;

    // Suppression and filtering
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_suppressedErrors;
    std::function<bool(const ErrorInfo&)> m_errorFilter;

    // Queues
    std::queue<LogEntry> m_logQueue;
    std::queue<ErrorInfo> m_errorQueue;

    // Synchronization
    mutable std::mutex m_logMutex;
    mutable std::mutex m_errorMutex;
    mutable std::mutex m_configMutex;
    mutable std::mutex m_statsMutex;
    mutable std::mutex m_handlerMutex;

    // System state
    bool m_initialized = false;
    bool m_asyncLogging = true;
    bool m_networkLoggingEnabled = false;
    std::string m_networkEndpoint;

    // File handles
    std::ofstream m_logFile;
    std::string m_currentLogFileName;
    uint64_t m_currentLogSize = 0;

    // Threading
    std::thread m_processingThread;
    std::atomic<bool> m_shouldStop{false};

    // ID generation
    std::atomic<uint64_t> m_nextErrorId{1};
    std::atomic<uint64_t> m_nextLogEntryId{1};

    // Event callbacks
    ErrorEventCallback m_errorCallback;
    ErrorSeverity m_callbackMinSeverity = ErrorSeverity::Error;
    std::mutex m_callbackMutex;

    // Crash handling
    bool m_crashHandlerRegistered = false;
    std::string m_crashDumpDirectory = "crashdumps/";

    // Performance tracking
    std::chrono::steady_clock::time_point m_lastFlush;
    std::chrono::steady_clock::time_point m_lastStatUpdate;
};

// Convenient macros for error reporting
#define COOP_LOG_DEBUG(logger, message) \
    CoopNet::ErrorManager::Instance().LogDebug(logger, message)

#define COOP_LOG_INFO(logger, message) \
    CoopNet::ErrorManager::Instance().LogInfo(logger, message)

#define COOP_LOG_WARNING(logger, message) \
    CoopNet::ErrorManager::Instance().LogWarning(logger, message)

#define COOP_LOG_ERROR(logger, message) \
    CoopNet::ErrorManager::Instance().LogError(logger, message)

#define COOP_LOG_CRITICAL(logger, message) \
    CoopNet::ErrorManager::Instance().LogCritical(logger, message)

#define COOP_LOG_FATAL(logger, message) \
    CoopNet::ErrorManager::Instance().LogFatal(logger, message)

#define COOP_REPORT_ERROR(category, severity, message, details) \
    CoopNet::ErrorManager::Instance().ReportError(category, severity, message, details, __FILE__, __FUNCTION__, __LINE__)

#define COOP_REPORT_EXCEPTION(ex, category, context) \
    CoopNet::ErrorManager::Instance().ReportException(ex, category, context)

// RAII class for error context management
class ErrorContext {
public:
    ErrorContext(const std::string& key, const std::string& value);
    ~ErrorContext();

private:
    std::string m_key;
    bool m_hadPreviousValue;
    std::string m_previousValue;
};

// Utility functions for error management
namespace ErrorUtils {
    std::string GetSeverityName(ErrorSeverity severity);
    std::string GetCategoryName(ErrorCategory category);
    std::string GetStrategyName(ErrorHandlingStrategy strategy);

    // Error code generation
    std::string GenerateErrorCode(ErrorCategory category, const std::string& operation, uint32_t code = 0);

    // Stack trace utilities
    std::vector<std::string> ParseStackTrace(const std::string& stackTrace);
    std::string FormatStackTrace(const std::vector<std::string>& frames);

    // File utilities
    bool CreateDirectoryIfNotExists(const std::string& path);
    uint64_t GetFileSize(const std::string& filename);
    bool CompressFile(const std::string& source, const std::string& destination);

    // System error mapping
    std::string GetSystemErrorMessage(int errorCode);
    ErrorSeverity MapSystemErrorToSeverity(int errorCode);
}

} // namespace CoopNet