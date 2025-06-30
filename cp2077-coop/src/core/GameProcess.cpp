#include "GameProcess.hpp"
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace CoopNet {

bool GameProcess_Launch(const std::string& exe, const std::string& args)
{
#ifdef _WIN32
    std::string cmd = exe + " " + args;
    STARTUPINFOA si{}; PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
        return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    pid_t pid = 0;
    std::string cmdExe = exe;
    std::string argStr = args;
    const char* argv[] = { cmdExe.c_str(), argStr.c_str(), nullptr };
    int ret = posix_spawnp(&pid, cmdExe.c_str(), nullptr, nullptr, (char* const*)argv, environ);
    if (ret != 0)
        return false;
    int status = 0;
    pid_t wp = waitpid(pid, &status, WNOHANG);
    if (wp == -1 || (wp == pid && (!WIFEXITED(status) || WEXITSTATUS(status) != 0)))
        return false;
    return true;
#endif
}

}
