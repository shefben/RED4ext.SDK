#include "GameProcess.hpp"
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <filesystem>
extern char **environ;
#endif

namespace CoopNet {

bool GameProcess_Launch(const std::string& exe, const std::string& args)
{
#ifdef _WIN32
    // Sanitize args to avoid control characters
    for (char ch : args) {
        if (static_cast<unsigned char>(ch) < 32 && ch != '\t' && ch != '\n')
            return false;
    }
    STARTUPINFOA si{}; PROCESS_INFORMATION pi{};
    std::string cmdLine = args; // mutable buffer required by CreateProcessA
    // Pass application name explicitly to avoid shell interpretation
    if (!CreateProcessA(exe.c_str(), cmdLine.empty() ? nullptr : cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
        return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    namespace fs = std::filesystem;
    fs::create_directories("logs/server");
    std::string logPath = "logs/server/" + exe + ".log";
    int fd = open(logPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
        return false;

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, fd, STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, fd, STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, fd);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);

    pid_t pid = 0;
    const char* argv[] = { exe.c_str(), args.c_str(), nullptr };
    int ret = posix_spawnp(&pid, exe.c_str(), &actions, &attr, (char* const*)argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attr);

    if (ret == 0)
    {
        int status = 0;
        pid_t res = waitpid(pid, &status, WNOHANG);
        if (res == pid)
            return false; // exited immediately
        return true;
    }
    return false;
#endif
}

}
