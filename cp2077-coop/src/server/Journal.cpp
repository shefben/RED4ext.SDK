#include "Journal.hpp"
#include "../core/GameClock.hpp"
#include "../third_party/zstd/zstd.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <mutex>

namespace CoopNet
{
static uint32_t g_index = 0;
static std::mutex g_logMutex;
static std::unique_ptr<std::ofstream> g_log;
static const char* kDir = "logs/journal";
static std::string GetLogPath()
{
    std::filesystem::create_directories(kDir);
    return std::string(kDir) + "/journal.log";
}

static void RotateIfNeeded()
{
    auto path = GetLogPath();
    if (std::filesystem::exists(path) && std::filesystem::file_size(path) >= 10u * 1024u * 1024u)
    {
        std::ifstream in(path, std::ios::binary);
        std::vector<char> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::vector<char> comp(ZSTD_compressBound(buf.size()));
        size_t z = ZSTD_compress(comp.data(), comp.size(), buf.data(), buf.size(), 3);
        if (!ZSTD_isError(z))
        {
            std::string out = std::string(kDir) + "/journal.log." + std::to_string(g_index++) + ".zst";
            std::ofstream fout(out, std::ios::binary);
            fout.write(comp.data(), z);
        }
        in.close();
        std::filesystem::remove(path);
        g_log.reset();
    }
}

void Journal_Log(uint64_t tick, uint32_t peerId, const char* action, uint32_t entityId, int32_t delta)
{
    try
    {
        std::lock_guard lock(g_logMutex);
        RotateIfNeeded();
        if (!g_log || !g_log->is_open())
        {
            g_log = std::make_unique<std::ofstream>(GetLogPath(), std::ios::app);
        }
        if (!g_log->is_open())
            return;
        (*g_log) << "{" << "\"tick\":" << tick << ",\"peerId\":" << peerId << ",\"action\":\"" << action
                 << "\",\"entityId\":" << entityId << ",\"delta\":" << delta << "}\n";
        g_log->flush();
    }
    catch (...)
    {
    }
}

} // namespace CoopNet
