#include "AssetStreamer.hpp"
#include "../../third_party/zstd/zstd.h"
#include <RED4ext/RED4ext.hpp>
#include <openssl/sha.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <chrono>
#include <algorithm>

namespace CoopNet
{
namespace fs = std::filesystem;

static std::unordered_map<uint16_t, std::string> s_bundleSha;
static std::mutex s_shaMutex;
static constexpr uint64_t kBundleLimit = 128ull * 1024ull * 1024ull; // 128 MB

static uint64_t DirSize(const fs::path& p)
{
    uint64_t total = 0;
    for (auto& f : fs::recursive_directory_iterator(p))
        if (f.is_regular_file())
            total += f.file_size();
    return total;
}

static void EnforceBundleLimit()
{
    fs::path base = fs::path("runtime_cache") / "plugins";
    if (!fs::exists(base))
        return;
    struct Entry
    {
        fs::path path;
        uint64_t size;
        fs::file_time_type mtime;
    };
    std::vector<Entry> ent;
    uint64_t total = 0;
    for (auto& dir : fs::directory_iterator(base))
    {
        if (!dir.is_directory())
            continue;
        uint64_t sz = DirSize(dir.path());
        ent.push_back({dir.path(), sz, fs::last_write_time(dir.path())});
        total += sz;
    }
    std::sort(ent.begin(), ent.end(), [](const Entry& a, const Entry& b) { return a.mtime < b.mtime; });
    for (const auto& e : ent)
    {
        if (total <= kBundleLimit)
            break;
        fs::remove_all(e.path);
        total -= e.size;
        std::cerr << "[AssetCache] purged bundle " << e.path.filename().string() << std::endl;
    }
}

AssetStreamer::AssetStreamer() = default;
AssetStreamer::~AssetStreamer()
{
    Stop();
}

void AssetStreamer::Start()
{
    if (m_running)
        return;
    m_running = true;
    m_thread = std::jthread([this] { Worker(); });
}

void AssetStreamer::Stop()
{
    if (!m_running)
        return;
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void AssetStreamer::Submit(Task&& t)
{
    m_tasks.Push(t);
}

bool AssetStreamer::Poll(Result& out)
{
    return m_results.Pop(out);
}

size_t AssetStreamer::GetPending() const
{
    return m_tasks.Empty() ? 0 : 1; // rough indicator
}

void AssetStreamer::Worker()
{
    while (m_running)
    {
        Task t;
        if (m_tasks.Pop(t))
        {
            bool ok = Process(t);
            m_results.Push({t.pluginId, ok});
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

bool AssetStreamer::Process(const Task& t)
{
    std::vector<uint8_t> raw(5u * 1024u * 1024u);
    size_t size = ZSTD_decompress(raw.data(), raw.size(), t.data.data(), t.data.size());
    if (ZSTD_isError(size))
    {
        std::cerr << "Bundle decompress failed for plugin " << t.pluginId << ": " << ZSTD_getErrorName(size) << std::endl;
        return false;
    }
    raw.resize(size);
    unsigned char sha[SHA256_DIGEST_LENGTH];
    SHA256(t.data.data(), t.data.size(), sha);
    std::string s(reinterpret_cast<char*>(sha), SHA256_DIGEST_LENGTH);
    {
        std::lock_guard lock(s_shaMutex);
        if (s_bundleSha[t.pluginId] == s)
            return true;
        s_bundleSha[t.pluginId] = s;
    }
    fs::path base = fs::path("runtime_cache") / "plugins" / std::to_string(t.pluginId);
    fs::create_directories(base);
    const uint8_t* p = raw.data();
    const uint8_t* end = raw.data() + raw.size();
    while (p + 2 <= end)
    {
        uint16_t pathLen;
        memcpy(&pathLen, p, 2);
        p += 2;
        if (p + pathLen > end)
            break;
        std::string rel(reinterpret_cast<const char*>(p), pathLen);
        p += pathLen;
        if (p + 4 > end)
            break;
        uint32_t len;
        memcpy(&len, p, 4);
        p += 4;
        if (p + len > end)
            break;
        fs::path out = base / rel;
        fs::create_directories(out.parent_path());
        std::ofstream f(out, std::ios::binary);
        f.write(reinterpret_cast<const char*>(p), len);
        p += len;
    }
    fs::last_write_time(base, fs::file_time_type::clock::now());
    EnforceBundleLimit();
    return true;
}

static AssetStreamer g_streamer;

AssetStreamer& GetAssetStreamer()
{
    return g_streamer;
}

} // namespace CoopNet

