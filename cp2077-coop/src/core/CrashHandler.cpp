#include <iostream>
#include <filesystem>
#include <string>

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

    // FIXME(next ticket): generate minidump at dumpPath
    // FIXME(next ticket): copy last 1 MB of network log to logPath
    // FIXME(next ticket): compress dumpPath and logPath into zipPath

    std::cout << "Crash report archive: " << zipPath.string() << std::endl;

    // Trigger UI prompt in scripts
    // CrashReportPrompt.Show(zipPath.string());
}
