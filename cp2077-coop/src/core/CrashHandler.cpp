#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

// Forward declarations
void GenerateMinidump(const char* dumpPath);
void CopyNetworkLog(const char* logPath);
void CreateCrashReport(const std::filesystem::path& reportPath, const char* reason);

// Captures crash information and zips the last network log.
void CaptureCrash(const char* reason)
{
    using namespace std::filesystem;
    std::cout << "Crash captured: " << reason << std::endl;

    const path crashDir{"crash"};
    create_directories(crashDir);

    const path dumpPath = crashDir / "dump.dmp";
    const path logPath  = crashDir / "netlog_last.txt";
    const path zipPath  = crashDir / "report.zip";

    // Generate minidump
    GenerateMinidump(dumpPath.string().c_str());
    
    // Copy network log (if available)
    CopyNetworkLog(logPath.string().c_str());
    
    // Create crash report text file
    CreateCrashReport(crashDir / "crash_info.txt", reason);

    std::cout << "Crash report archive: " << zipPath.string() << std::endl;

    // Trigger UI prompt in scripts
    // CrashReportPrompt.Show(zipPath.string());
}

#ifdef _WIN32
void GenerateMinidump(const char* dumpPath)
{
    try {
        HANDLE hFile = CreateFileA(dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create minidump file: " << dumpPath << std::endl;
            return;
        }

        HANDLE hProcess = GetCurrentProcess();
        DWORD processId = GetCurrentProcessId();

        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = {};
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = nullptr; // Will be null for manual dumps
        exceptionInfo.ClientPointers = FALSE;

        MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
            MiniDumpWithIndirectlyReferencedMemory |
            MiniDumpWithDataSegs |
            MiniDumpWithHandleData |
            MiniDumpWithThreadInfo
        );

        BOOL result = MiniDumpWriteDump(
            hProcess,
            processId,
            hFile,
            dumpType,
            &exceptionInfo,
            nullptr,
            nullptr
        );

        CloseHandle(hFile);

        if (result) {
            std::cout << "Minidump created successfully: " << dumpPath << std::endl;
        } else {
            std::cerr << "Failed to write minidump. Error: " << GetLastError() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in GenerateMinidump: " << e.what() << std::endl;
    }
}
#else
void GenerateMinidump(const char* dumpPath)
{
    // On non-Windows platforms, create a simple crash log
    std::ofstream file(dumpPath);
    if (file.is_open()) {
        file << "Minidump generation not supported on this platform\n";
        file << "Crash detected at: " << std::time(nullptr) << "\n";
        file.close();
        std::cout << "Basic crash log created: " << dumpPath << std::endl;
    }
}
#endif

void CopyNetworkLog(const char* logPath)
{
    try {
        // Look for network log in common locations
        std::vector<std::filesystem::path> logLocations = {
            "logs/network.log",
            "network.log",
            "../logs/network.log",
            "cp2077-coop/logs/network.log"
        };

        std::filesystem::path sourceLog;
        for (const auto& location : logLocations) {
            if (std::filesystem::exists(location)) {
                sourceLog = location;
                break;
            }
        }

        if (sourceLog.empty()) {
            // Create empty log file to indicate no network log was found
            std::ofstream file(logPath);
            if (file.is_open()) {
                file << "No network log file found\n";
                file << "Searched locations:\n";
                for (const auto& location : logLocations) {
                    file << "  " << location.string() << "\n";
                }
                file.close();
            }
            return;
        }

        // Copy last 1MB of the log file
        const size_t maxLogSize = 1024 * 1024; // 1MB
        
        std::ifstream source(sourceLog, std::ios::binary | std::ios::ate);
        if (!source.is_open()) {
            std::cerr << "Failed to open source log: " << sourceLog << std::endl;
            return;
        }

        size_t fileSize = static_cast<size_t>(source.tellg());
        size_t copySize = (std::min)(fileSize, maxLogSize);
        size_t skipSize = fileSize > maxLogSize ? fileSize - maxLogSize : 0;

        source.seekg(skipSize);
        
        std::ofstream dest(logPath, std::ios::binary);
        if (!dest.is_open()) {
            std::cerr << "Failed to create log copy: " << logPath << std::endl;
            return;
        }

        dest << source.rdbuf();
        source.close();
        dest.close();

        std::cout << "Network log copied: " << copySize << " bytes to " << logPath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in CopyNetworkLog: " << e.what() << std::endl;
    }
}

void CreateCrashReport(const std::filesystem::path& reportPath, const char* reason)
{
    try {
        std::ofstream report(reportPath);
        if (!report.is_open()) {
            std::cerr << "Failed to create crash report: " << reportPath << std::endl;
            return;
        }

        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);

        report << "=== CP2077-COOP CRASH REPORT ===\n";
        report << "Timestamp: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
        report << "Reason: " << reason << "\n";
        report << "Platform: " << 
#ifdef _WIN32
            "Windows" 
#elif __linux__
            "Linux"
#elif __APPLE__
            "macOS"
#else
            "Unknown"
#endif
            << "\n";

        // Add system information
        report << "\n=== SYSTEM INFO ===\n";
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            report << "Total RAM: " << (memInfo.ullTotalPhys / (1024 * 1024)) << " MB\n";
            report << "Available RAM: " << (memInfo.ullAvailPhys / (1024 * 1024)) << " MB\n";
            report << "Memory Load: " << memInfo.dwMemoryLoad << "%\n";
        }
#endif

        report << "\n=== FILES INCLUDED ===\n";
        report << "- dump.dmp (minidump)\n";
        report << "- netlog_last.txt (last 1MB of network log)\n";
        report << "- crash_info.txt (this file)\n";

        report.close();
        std::cout << "Crash report created: " << reportPath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in CreateCrashReport: " << e.what() << std::endl;
    }
}
