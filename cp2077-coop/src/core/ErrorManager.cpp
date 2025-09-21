#include "ErrorManager.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <regex>
#include <spdlog/spdlog.h>
#include "Logger.hpp"

// Undefine conflicting macros from Logger.hpp
#ifdef LogDebug
#undef LogDebug
#endif
#ifdef LogInfo
#undef LogInfo
#endif
#ifdef LogWarning
#undef LogWarning
#endif
#ifdef LogError
#undef LogError
#endif

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
// #include <imagehlp.h> // Conflicts with dbghelp.h
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "imagehlp.lib")
#else
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace CoopNet {

// Exception implementation
CoopNetException::CoopNetException(ErrorCategory category, ErrorSeverity severity,
                                  const std::string& message, const std::string& details)
    : m_category(category), m_severity(severity), m_message(message), m_details(details) {
    m_errorId = ErrorManager::Instance().ReportError(category, severity, message, details);
}

const char* CoopNetException::what() const noexcept {
    return m_message.c_str();
}

// Error context RAII implementation
ErrorContext::ErrorContext(const std::string& key, const std::string& value) : m_key(key) {
    auto& errorManager = ErrorManager::Instance();
    auto context = errorManager.GetContext();

    auto it = context.find(key);
    if (it != context.end()) {
        m_hadPreviousValue = true;
        m_previousValue = it->second;
    } else {
        m_hadPreviousValue = false;
    }

    errorManager.SetContextValue(key, value);
}

ErrorContext::~ErrorContext() {
    auto& errorManager = ErrorManager::Instance();

    if (m_hadPreviousValue) {
        errorManager.SetContextValue(m_key, m_previousValue);
    } else {
        errorManager.RemoveContextValue(m_key);
    }
}

// Main ErrorManager implementation
ErrorManager& ErrorManager::Instance() {
    static ErrorManager instance;
    return instance;
}

bool ErrorManager::Initialize(const LoggingConfig& config) {
    if (m_initialized) {
        return true;
    }

    std::lock_guard<std::mutex> lock(m_configMutex);

    m_config = config;

    // Create log directory if it doesn't exist
    if (!ErrorUtils::CreateDirectoryIfNotExists(m_config.logDirectory)) {
        return false;
    }

    // Initialize crash handling
    if (m_config.enableStackTrace) {
        RegisterCrashHandler();
    }

    // Open log file
    if (static_cast<uint8_t>(m_config.outputTargets & LogTarget::File) != 0) {
        if (!OpenLogFile()) {
            return false;
        }
    }

    // Set default error handling strategies
    m_defaultStrategies[ErrorCategory::System] = ErrorHandlingStrategy::Escalate;
    m_defaultStrategies[ErrorCategory::Network] = ErrorHandlingStrategy::Retry;
    m_defaultStrategies[ErrorCategory::Audio] = ErrorHandlingStrategy::Fallback;
    m_defaultStrategies[ErrorCategory::Performance] = ErrorHandlingStrategy::Ignore;
    m_defaultStrategies[ErrorCategory::UI] = ErrorHandlingStrategy::Fallback;
    m_defaultStrategies[ErrorCategory::Game] = ErrorHandlingStrategy::Retry;
    m_defaultStrategies[ErrorCategory::Database] = ErrorHandlingStrategy::Retry;
    m_defaultStrategies[ErrorCategory::Security] = ErrorHandlingStrategy::Terminate;

    // Reset statistics
    ResetStatistics();

    // Start processing thread if async logging is enabled
    if (m_config.enableAsync) {
        m_shouldStop.store(false);
        m_processingThread = std::thread(&ErrorManager::ProcessingLoop, this);
    }

    m_initialized = true;

    LogInfo("ErrorManager", "Error management system initialized");
    return true;
}

void ErrorManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    LogInfo("ErrorManager", "Shutting down error management system");

    // Stop processing thread
    m_shouldStop.store(true);
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }

    // Flush remaining logs
    FlushLogs();

    // Close log file
    CloseLogFile();

    // Unregister crash handler
    if (m_crashHandlerRegistered) {
        UnregisterCrashHandler();
    }

    // Clear all data
    {
        std::lock_guard<std::mutex> errorLock(m_errorMutex);
        m_errorHistory.clear();
        while (!m_errorQueue.empty()) {
            m_errorQueue.pop();
        }
    }

    {
        std::lock_guard<std::mutex> logLock(m_logMutex);
        while (!m_logQueue.empty()) {
            m_logQueue.pop();
        }
    }

    m_initialized = false;
}

void ErrorManager::ProcessingLoop() {
    auto lastFlush = std::chrono::steady_clock::now();
    auto lastStatsUpdate = std::chrono::steady_clock::now();

    while (!m_shouldStop.load()) {
        auto now = std::chrono::steady_clock::now();

        // Process queues
        ProcessLogQueue();
        ProcessErrorQueue();

        // Periodic flushing
        auto flushElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlush);
        if (flushElapsed.count() >= m_config.flushInterval) {
            FlushLogs();
            lastFlush = now;
        }

        // Update statistics
        auto statsElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsUpdate);
        if (statsElapsed.count() >= 60) { // Update every minute
            UpdateStatistics();
            lastStatsUpdate = now;
        }

        // Check for log rotation
        if (m_config.enableRotation && ShouldRotateLog()) {
            RotateLogFile();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

uint64_t ErrorManager::ReportError(ErrorCategory category, ErrorSeverity severity,
                                  const std::string& message, const std::string& details,
                                  const std::string& sourceFile, const std::string& sourceFunction,
                                  uint32_t sourceLine) {
    ErrorInfo error;
    error.errorId = GenerateErrorId();
    error.severity = severity;
    error.category = category;
    error.errorCode = ErrorUtils::GenerateErrorCode(category, sourceFunction);
    error.message = message;
    error.details = details;
    error.sourceFile = sourceFile;
    error.sourceFunction = sourceFunction;
    error.sourceLine = sourceLine;
    error.timestamp = std::chrono::steady_clock::now();
    error.threadId = GetCurrentThreadId();
    error.context = m_globalContext;
    error.occurrenceCount = 1;
    error.firstOccurrence = error.timestamp;
    error.lastOccurrence = error.timestamp;

    // Capture stack trace if enabled
    if (m_config.enableStackTrace && severity >= ErrorSeverity::Error) {
        error.stackTrace = CaptureStackTrace();
    }

    // Check if error is suppressed
    if (IsErrorSuppressed(error.errorCode)) {
        return error.errorId;
    }

    // Apply error filter
    if (m_errorFilter && !m_errorFilter(error)) {
        return error.errorId;
    }

    // Store error in history
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);

        // Check for duplicate errors
        for (auto& [id, existingError] : m_errorHistory) {
            if (existingError.errorCode == error.errorCode &&
                existingError.message == error.message) {
                existingError.occurrenceCount++;
                existingError.lastOccurrence = error.timestamp;
                return existingError.errorId;
            }
        }

        m_errorHistory[error.errorId] = error;

        if (m_config.enableAsync) {
            m_errorQueue.push(error);
        } else {
            HandleError(error);
        }
    }

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_statistics.errorCounts[severity]++;
        m_statistics.categoryCounts[category]++;
        m_statistics.totalErrors++;

        if (severity == ErrorSeverity::Warning) m_statistics.totalWarnings++;
        else if (severity == ErrorSeverity::Critical) m_statistics.totalCriticalErrors++;
        else if (severity == ErrorSeverity::Fatal) m_statistics.totalFatalErrors++;

        m_statistics.lastError = error.timestamp;
    }

    // Trigger callback if registered
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (m_errorCallback && severity >= m_callbackMinSeverity) {
            try {
                m_errorCallback(error);
            } catch (const std::exception& e) {
                // Avoid recursive error reporting
                // spdlog::error("Exception in error callback: {}", e.what());
            }
        }
    }

    // Log the error
    Log(severity, category, "ErrorManager", message + (details.empty() ? "" : " - " + details));

    return error.errorId;
}

uint64_t ErrorManager::ReportException(const std::exception& ex, ErrorCategory category,
                                      const std::string& context) {
    std::string message = "Exception caught: " + std::string(ex.what());
    std::string details = context;

    // Check if it's a CoopNetException for better handling
    if (const auto* coopEx = dynamic_cast<const CoopNetException*>(&ex)) {
        category = coopEx->GetCategory();
        details += " | " + coopEx->GetDetails();
    }

    return ReportError(category, ErrorSeverity::Error, message, details);
}

bool ErrorManager::HandleError(const ErrorInfo& error) {
    // Find appropriate handler
    ErrorHandler* handler = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_handlerMutex);

        for (auto& [name, h] : m_errorHandlers) {
            if (h.isActive && h.category == error.category && error.severity >= h.minSeverity) {
                handler = &h;
                break;
            }
        }
    }

    bool handled = false;

    if (handler) {
        handled = ExecuteErrorHandler(error, *handler);

        if (handled) {
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_statistics.handledErrors++;
        }
    } else {
        // Use default strategy
        auto strategyIt = m_defaultStrategies.find(error.category);
        if (strategyIt != m_defaultStrategies.end()) {
            ErrorHandler defaultHandler;
            defaultHandler.strategy = strategyIt->second;
            defaultHandler.maxRetries = 3;
            defaultHandler.retryDelay = std::chrono::milliseconds(100);

            handled = ExecuteErrorHandler(error, defaultHandler);
        }
    }

    if (!handled) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_statistics.unhandledErrors++;
    }

    return handled;
}

bool ErrorManager::ExecuteErrorHandler(const ErrorInfo& error, const ErrorHandler& handler) {
    switch (handler.strategy) {
        case ErrorHandlingStrategy::Ignore:
            return true;

        case ErrorHandlingStrategy::Retry:
            // Retry logic would be implemented here
            LogInfo("ErrorManager", "Retrying operation for error: " + error.message);
            return true;

        case ErrorHandlingStrategy::Fallback:
            if (handler.fallbackHandler) {
                return handler.fallbackHandler(error);
            }
            LogWarning("ErrorManager", "Using fallback for error: " + error.message);
            return true;

        case ErrorHandlingStrategy::Escalate:
            {
                ErrorInfo escalated = error;
                escalated.severity = static_cast<ErrorSeverity>(std::min(
                    static_cast<int>(ErrorSeverity::Fatal),
                    static_cast<int>(error.severity) + 1));
                ReportError(escalated.category, escalated.severity,
                          "Escalated: " + escalated.message, escalated.details);
            }
            return true;

        case ErrorHandlingStrategy::Terminate:
            LogFatal("ErrorManager", "Terminating due to critical error: " + error.message);
            // In a real implementation, this might trigger a graceful shutdown
            return true;

        default:
            return false;
    }
}

void ErrorManager::Log(ErrorSeverity level, ErrorCategory category, const std::string& logger,
                      const std::string& message, const std::unordered_map<std::string, std::string>& metadata) {
    if (level < m_config.minLogLevel) {
        return;
    }

    LogEntry entry;
    entry.entryId = GenerateLogEntryId();
    entry.level = level;
    entry.category = category;
    entry.logger = logger;
    entry.message = message;
    entry.timestamp = std::chrono::steady_clock::now();
    entry.threadId = GetCurrentThreadId();
    entry.metadata = metadata;

    if (m_config.enableAsync) {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_logQueue.push(entry);
    } else {
        WriteLogEntry(entry);
    }
}

void ErrorManager::WriteLogEntry(const LogEntry& entry) {
    std::string formattedEntry = FormatLogEntry(entry);

    // Output to configured targets
    if (static_cast<uint8_t>(m_config.outputTargets & LogTarget::Console) != 0) {
        std::cout << formattedEntry << std::endl;
    }

    if (static_cast<uint8_t>(m_config.outputTargets & LogTarget::File) != 0 && m_logFile.is_open()) {
        m_logFile << formattedEntry << std::endl;
        m_currentLogSize += formattedEntry.length() + 1;
    }

    if (static_cast<uint8_t>(m_config.outputTargets & LogTarget::Debugger) != 0) {
#ifdef _WIN32
        OutputDebugStringA((formattedEntry + "\n").c_str());
#endif
    }

    // Network logging
    if (static_cast<uint8_t>(m_config.outputTargets & LogTarget::Network) != 0 && m_networkLoggingEnabled) {
        // Network logging would be implemented here
    }
}

std::string ErrorManager::FormatLogEntry(const LogEntry& entry) const {
    std::string format = m_config.logFormat;

    // Replace placeholders
    std::unordered_map<std::string, std::string> values;
    values["timestamp"] = GetTimestampString(entry.timestamp);
    values["level"] = GetSeverityString(entry.level);
    values["category"] = GetCategoryString(entry.category);
    values["logger"] = entry.logger;
    values["message"] = entry.message;
    values["thread"] = std::to_string(entry.threadId);

    return FormatMessage(format, values);
}

std::string ErrorManager::FormatMessage(const std::string& format,
                                       const std::unordered_map<std::string, std::string>& values) const {
    std::string result = format;

    for (const auto& [key, value] : values) {
        std::string placeholder = "{" + key + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    return result;
}

bool ErrorManager::OpenLogFile() {
    m_currentLogFileName = GenerateLogFileName();

    m_logFile.open(m_currentLogFileName, std::ios::app);
    if (!m_logFile.is_open()) {
        return false;
    }

    m_currentLogSize = ErrorUtils::GetFileSize(m_currentLogFileName);
    return true;
}

void ErrorManager::CloseLogFile() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

bool ErrorManager::ShouldRotateLog() const {
    return m_currentLogSize >= m_config.maxLogFileSize;
}

bool ErrorManager::RotateLogFile() {
    if (!m_logFile.is_open()) {
        return false;
    }

    CloseLogFile();

    // Move current log file
    std::string rotatedName = GenerateLogFileName(1);

    try {
        std::filesystem::rename(m_currentLogFileName, rotatedName);
    } catch (const std::exception& e) {
        LogError("ErrorManager", "Failed to rotate log file: " + std::string(e.what()));
        return false;
    }

    // Compress old log if enabled
    if (m_config.enableCompression) {
        std::string compressedName = rotatedName + ".gz";
        if (ErrorUtils::CompressFile(rotatedName, compressedName)) {
            std::filesystem::remove(rotatedName);
        }
    }

    // Clean up old logs
    CleanupOldLogs();

    // Open new log file
    return OpenLogFile();
}

void ErrorManager::CleanupOldLogs() {
    try {
        std::vector<std::filesystem::path> logFiles;

        for (const auto& entry : std::filesystem::directory_iterator(m_config.logDirectory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(m_config.logFileName) == 0) {
                    logFiles.push_back(entry.path());
                }
            }
        }

        // Sort by modification time (newest first)
        std::sort(logFiles.begin(), logFiles.end(),
                 [](const std::filesystem::path& a, const std::filesystem::path& b) {
                     return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                 });

        // Remove old files beyond the limit
        if (logFiles.size() > m_config.maxLogFiles) {
            for (size_t i = m_config.maxLogFiles; i < logFiles.size(); ++i) {
                std::filesystem::remove(logFiles[i]);
            }
        }
    } catch (const std::exception& e) {
        LogError("ErrorManager", "Failed to cleanup old logs: " + std::string(e.what()));
    }
}

std::string ErrorManager::GenerateLogFileName(uint32_t index) const {
    std::string baseName = m_config.logFileName;

    if (index > 0) {
        size_t dotPos = baseName.find_last_of('.');
        if (dotPos != std::string::npos) {
            baseName = baseName.substr(0, dotPos) + "." + std::to_string(index) + baseName.substr(dotPos);
        } else {
            baseName += "." + std::to_string(index);
        }
    }

    return m_config.logDirectory + baseName;
}

std::string ErrorManager::CaptureStackTrace() const {
#ifdef _WIN32
    const int maxFrames = 64;
    void* stack[maxFrames];

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    WORD frames = CaptureStackBackTrace(0, maxFrames, stack, NULL);

    std::stringstream trace;

    for (WORD i = 0; i < frames; ++i) {
        DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);

        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbol)) {
            trace << "[" << i << "] " << symbol->Name << " + 0x" << std::hex << displacement << std::dec << "\n";
        } else {
            trace << "[" << i << "] 0x" << std::hex << address << std::dec << "\n";
        }
    }

    SymCleanup(process);
    return trace.str();
#else
    const int maxFrames = 64;
    void* buffer[maxFrames];

    int frames = backtrace(buffer, maxFrames);
    char** symbols = backtrace_symbols(buffer, frames);

    std::stringstream trace;
    for (int i = 0; i < frames; ++i) {
        trace << "[" << i << "] " << symbols[i] << "\n";
    }

    free(symbols);
    return trace.str();
#endif
}

uint32_t ErrorManager::GetCurrentThreadId() const {
#ifdef _WIN32
    return ::GetCurrentThreadId();
#else
    return static_cast<uint32_t>(pthread_self());
#endif
}

ErrorStatistics ErrorManager::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_statistics;
}

void ErrorManager::UpdateStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - m_statistics.sessionStart);

    if (elapsed.count() > 0) {
        m_statistics.errorsPerMinute = static_cast<float>(m_statistics.totalErrors) / elapsed.count();
    }
}

std::string ErrorManager::GetSeverityString(ErrorSeverity severity) const {
    return ErrorUtils::GetSeverityName(severity);
}

std::string ErrorManager::GetCategoryString(ErrorCategory category) const {
    return ErrorUtils::GetCategoryName(category);
}

std::string ErrorManager::GetTimestampString(std::chrono::steady_clock::time_point timestamp) const {
    // For simplicity, just use current system time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

uint64_t ErrorManager::GenerateErrorId() {
    return m_nextErrorId.fetch_add(1);
}

uint64_t ErrorManager::GenerateLogEntryId() {
    return m_nextLogEntryId.fetch_add(1);
}

// Utility functions implementation
namespace ErrorUtils {
    std::string GetSeverityName(ErrorSeverity severity) {
        switch (severity) {
            case ErrorSeverity::Debug: return "DEBUG";
            case ErrorSeverity::Info: return "INFO";
            case ErrorSeverity::Warning: return "WARNING";
            case ErrorSeverity::Error: return "ERROR";
            case ErrorSeverity::Critical: return "CRITICAL";
            case ErrorSeverity::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    std::string GetCategoryName(ErrorCategory category) {
        switch (category) {
            case ErrorCategory::System: return "SYSTEM";
            case ErrorCategory::Network: return "NETWORK";
            case ErrorCategory::Audio: return "AUDIO";
            case ErrorCategory::Performance: return "PERFORMANCE";
            case ErrorCategory::UI: return "UI";
            case ErrorCategory::Game: return "GAME";
            case ErrorCategory::Database: return "DATABASE";
            case ErrorCategory::Security: return "SECURITY";
            case ErrorCategory::Custom: return "CUSTOM";
            default: return "UNKNOWN";
        }
    }

    bool CreateDirectoryIfNotExists(const std::string& path) {
        try {
            return std::filesystem::create_directories(path);
        } catch (const std::exception&) {
            return false;
        }
    }

    uint64_t GetFileSize(const std::string& filename) {
        try {
            return std::filesystem::file_size(filename);
        } catch (const std::exception&) {
            return 0;
        }
    }

    std::string GenerateErrorCode(ErrorCategory category, const std::string& operation, uint32_t code) {
        std::stringstream ss;
        ss << GetCategoryName(category) << "_" << operation;
        if (code > 0) {
            ss << "_" << std::setfill('0') << std::setw(4) << code;
        }
        return ss.str();
    }

    bool CompressFile(const std::string& source, const std::string& destination) {
        // Compression would require a compression library like zlib
        // For now, return false to indicate unsupported
        return false;
    }
}

// Missing ErrorManager function implementations (Note: No Initialize() override needed - header only has Initialize(const LoggingConfig&))

void ErrorManager::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    m_statistics = {};
    m_statistics.sessionStart = std::chrono::steady_clock::now();
}

void ErrorManager::FlushLogs() {
    if (m_logFile.is_open()) {
        m_logFile.flush();
    }
}

void ErrorManager::ProcessLogQueue() {
    std::queue<LogEntry> logQueue;

    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        logQueue.swap(m_logQueue);
    }

    while (!logQueue.empty()) {
        WriteLogEntry(logQueue.front());
        logQueue.pop();
    }
}

void ErrorManager::ProcessErrorQueue() {
    std::queue<ErrorInfo> errorQueue;

    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        errorQueue.swap(m_errorQueue);
    }

    while (!errorQueue.empty()) {
        HandleError(errorQueue.front());
        errorQueue.pop();
    }
}

bool ErrorManager::IsErrorSuppressed(const std::string& errorCode) const {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    // Check if error code is in suppression map
    auto it = m_suppressedErrors.find(errorCode);
    if (it != m_suppressedErrors.end()) {
        auto now = std::chrono::steady_clock::now();
        // Check if suppression has expired (0 duration means permanent)
        if (it->second != std::chrono::steady_clock::time_point{} && now > it->second) {
            // Suppression has expired - should clean up but can't modify in const function
            return false;
        }
        return true;
    }

    return false;
}

void ErrorManager::SetContextValue(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_globalContext[key] = value;
}

void ErrorManager::RemoveContextValue(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_globalContext.erase(key);
}

std::unordered_map<std::string, std::string> ErrorManager::GetContext() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_globalContext;
}

void ErrorManager::RegisterCrashHandler() {
    // Crash handler registration simplified due to static function access issues
    m_crashHandlerRegistered = true;
    LogInfo("ErrorManager", "Crash handler registration requested (simplified implementation)");
}

void ErrorManager::UnregisterCrashHandler() {
    // Crash handler unregistration simplified
    m_crashHandlerRegistered = false;
    LogInfo("ErrorManager", "Crash handler unregistration requested (simplified implementation)");
}

// Crash handlers removed due to access issues with private members
// Would need to be implemented with public interface or friend functions

void ErrorManager::LogDebug(const std::string& logger, const std::string& message) {
    Log(ErrorSeverity::Debug, ErrorCategory::System, logger, message);
}

void ErrorManager::LogInfo(const std::string& logger, const std::string& message) {
    Log(ErrorSeverity::Info, ErrorCategory::System, logger, message);
}

void ErrorManager::LogWarning(const std::string& logger, const std::string& message) {
    Log(ErrorSeverity::Warning, ErrorCategory::System, logger, message);
}

void ErrorManager::LogError(const std::string& logger, const std::string& message) {
    Log(ErrorSeverity::Error, ErrorCategory::System, logger, message);
}

void ErrorManager::LogCritical(const std::string& logger, const std::string& message) {
    Log(ErrorSeverity::Critical, ErrorCategory::System, logger, message);
}

void ErrorManager::LogFatal(const std::string& logger, const std::string& message) {
    Log(ErrorSeverity::Fatal, ErrorCategory::System, logger, message);
}

bool ErrorManager::RegisterErrorHandler(const ErrorHandler& handler) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);

    if (m_errorHandlers.find(handler.handlerName) != m_errorHandlers.end()) {
        return false; // Handler already exists
    }

    m_errorHandlers[handler.handlerName] = handler;
    return true;
}

bool ErrorManager::UnregisterErrorHandler(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);

    auto it = m_errorHandlers.find(name);
    if (it == m_errorHandlers.end()) {
        return false;
    }

    m_errorHandlers.erase(it);
    return true;
}

void ErrorManager::RegisterErrorCallback(ErrorSeverity minSeverity, ErrorEventCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_errorCallback = callback;
    m_callbackMinSeverity = minSeverity;
}

void ErrorManager::SetErrorFilter(std::function<bool(const ErrorInfo&)> filter) {
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_errorFilter = filter;
}

void ErrorManager::SuppressError(const std::string& errorCode, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    auto now = std::chrono::steady_clock::now();
    if (duration.count() > 0) {
        m_suppressedErrors[errorCode] = now + duration;
    } else {
        // Permanent suppression
        m_suppressedErrors[errorCode] = std::chrono::steady_clock::time_point{};
    }
}

void ErrorManager::UnsuppressError(const std::string& errorCode) {
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_suppressedErrors.erase(errorCode);
}

std::vector<ErrorInfo> ErrorManager::GetRecentErrors(uint32_t count) const {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    std::vector<ErrorInfo> recent;
    recent.reserve(std::min(count, static_cast<uint32_t>(m_errorHistory.size())));

    // Get the most recent errors (unordered_map doesn't have rbegin/rend)
    // Convert to vector first, then sort by timestamp
    std::vector<ErrorInfo> allErrors;
    for (const auto& [id, error] : m_errorHistory) {
        allErrors.push_back(error);
    }

    // Sort by timestamp (most recent first)
    std::sort(allErrors.begin(), allErrors.end(),
             [](const ErrorInfo& a, const ErrorInfo& b) {
                 return a.timestamp > b.timestamp;
             });

    // Take the requested count
    size_t takeCount = std::min(static_cast<size_t>(count), allErrors.size());
    recent.assign(allErrors.begin(), allErrors.begin() + takeCount);

    return recent;
}

ErrorInfo ErrorManager::GetError(uint64_t errorId) const {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    auto it = m_errorHistory.find(errorId);
    if (it != m_errorHistory.end()) {
        return it->second;
    }
    return ErrorInfo{}; // Return empty error info if not found
}

std::vector<ErrorInfo> ErrorManager::GetErrorsByCategory(ErrorCategory category, uint32_t maxCount) const {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    std::vector<ErrorInfo> filtered;
    filtered.reserve(maxCount);

    for (const auto& [id, error] : m_errorHistory) {
        if (error.category == category && filtered.size() < maxCount) {
            filtered.push_back(error);
        }
    }

    return filtered;
}

// Additional ErrorManager function implementations to match header

bool ErrorManager::HandleError(uint64_t errorId) {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    auto it = m_errorHistory.find(errorId);
    if (it != m_errorHistory.end()) {
        return HandleError(it->second);
    }
    return false;
}

// Duplicate GetErrorsByCategory function removed - already implemented above

std::vector<ErrorInfo> ErrorManager::GetErrorsBySeverity(ErrorSeverity severity, uint32_t count) const {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    std::vector<ErrorInfo> filtered;
    filtered.reserve(count);

    for (const auto& [id, error] : m_errorHistory) {
        if (error.severity == severity && filtered.size() < count) {
            filtered.push_back(error);
        }
    }

    return filtered;
}

std::string ErrorManager::GenerateErrorReport() const {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    std::stringstream report;
    report << "Error Report\n";
    report << "============\n\n";

    auto stats = GetStatistics();
    report << "Total Errors: " << stats.totalErrors << "\n";
    report << "Total Warnings: " << stats.totalWarnings << "\n";
    report << "Critical Errors: " << stats.totalCriticalErrors << "\n";
    report << "Fatal Errors: " << stats.totalFatalErrors << "\n\n";

    return report.str();
}

std::string ErrorManager::GenerateSessionSummary() const {
    std::lock_guard<std::mutex> lock(m_errorMutex);

    std::stringstream summary;
    summary << "Session Summary\n";
    summary << "===============\n\n";

    auto stats = GetStatistics();
    summary << "Session Started: " << GetTimestampString(stats.sessionStart) << "\n";
    summary << "Total Errors: " << stats.totalErrors << "\n";
    summary << "Errors per Minute: " << stats.errorsPerMinute << "\n\n";

    return summary.str();
}

void ErrorManager::UpdateConfig(const LoggingConfig& config) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config = config;
}

LoggingConfig ErrorManager::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config;
}

void ErrorManager::SetLogLevel(ErrorSeverity level) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.minLogLevel = level;
}

ErrorSeverity ErrorManager::GetLogLevel() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.minLogLevel;
}

void ErrorManager::SetLogTargets(LogTarget targets) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.outputTargets = targets;
}

LogTarget ErrorManager::GetLogTargets() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.outputTargets;
}

bool ErrorManager::CompressOldLogs() {
    // Compression implementation would go here
    return true;
}

std::vector<std::string> ErrorManager::GetLogFiles() const {
    std::vector<std::string> logFiles;
    // Implementation would scan log directory and return file list
    return logFiles;
}

void ErrorManager::EnableStackTrace(bool enabled) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.enableStackTrace = enabled;
}

bool ErrorManager::IsStackTraceEnabled() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.enableStackTrace;
}

bool ErrorManager::GenerateCrashDump(const std::string& filename) {
    // Crash dump generation would be implemented here
    return true;
}

void ErrorManager::SetCrashDumpDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_crashDumpDirectory = directory;
}

void ErrorManager::EnableNetworkLogging(bool enabled, const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_networkLoggingEnabled = enabled;
    m_networkEndpoint = endpoint;
}

bool ErrorManager::SendErrorToServer(const ErrorInfo& error) {
    // Network error reporting would be implemented here
    return true;
}

void ErrorManager::UnregisterErrorCallback() {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_errorCallback = nullptr;
}

void ErrorManager::SetDefaultStrategy(ErrorCategory category, ErrorHandlingStrategy strategy) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    m_defaultStrategies[category] = strategy;
}

ErrorHandlingStrategy ErrorManager::GetDefaultStrategy(ErrorCategory category) const {
    std::lock_guard<std::mutex> lock(m_handlerMutex);

    auto it = m_defaultStrategies.find(category);
    return (it != m_defaultStrategies.end()) ? it->second : ErrorHandlingStrategy::Ignore;
}

} // namespace CoopNet