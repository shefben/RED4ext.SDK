#include "SaveMigration.hpp"
#include "SaveFork.hpp"
#include "Hash.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace CoopNet {
namespace fs = std::filesystem;

static fs::path GetVanillaDir()
{
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    if (!home)
        return {};
    fs::path base(home);
    base /= "Saved Games/CD Projekt Red/Cyberpunk 2077";
    return base;
}

bool MigrateSinglePlayerSave()
{
    try {
        fs::path coopDir(kCoopSavePath);
        if (fs::exists(coopDir) && !fs::is_empty(coopDir))
            return true; // already migrated or have saves
        fs::path srcDir = GetVanillaDir();
        if (srcDir.empty() || !fs::exists(srcDir))
            return false;
        fs::path newest;
        fs::file_time_type newestTime;
        for (auto& p : fs::directory_iterator(srcDir))
        {
            if (p.path().extension() == ".sav")
            {
                if (newest.empty() || p.last_write_time() > newestTime)
                {
                    newest = p.path();
                    newestTime = p.last_write_time();
                }
            }
        }
        if (newest.empty())
            return false;
        std::ifstream in(newest, std::ios::binary);
        if (!in.is_open())
            return false;
        std::string data((std::istreambuf_iterator<char>(in)), {});
        uint32_t crc = Fnv1a32(data.c_str());
        std::string outJson = "{\"version\":1,\"checksum\":" + std::to_string(crc) + "}";
        uint32_t sid = SessionState_GetId();
        if (sid == 0)
            sid = 1;
        SaveSession(sid, outJson);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "MigrateSinglePlayerSave error: " << e.what() << std::endl;
        return false;
    }
}

static size_t g_snapIndex = 0;

void SaveRollbackSnapshot(uint32_t sessionId, const std::string& jsonBlob)
{
    try {
        EnsureCoopSaveDirs();
        fs::path dir = fs::path(kCoopSavePath) / "snapshots";
        fs::create_directories(dir);
        fs::path file = dir / (std::to_string(sessionId) + "_snap" + std::to_string(g_snapIndex) + ".json");
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (out.is_open())
            out << jsonBlob;
        g_snapIndex = (g_snapIndex + 1) % 20;
    } catch (const std::exception& e) {
        std::cerr << "SaveRollbackSnapshot error: " << e.what() << std::endl;
    }
}

bool ValidateSessionState(uint32_t sessionId)
{
    try {
        fs::path file = fs::path(kCoopSavePath) / (std::to_string(sessionId) + ".json");
        std::ifstream in(file, std::ios::binary);
        if (!in.is_open() || in.peek() == EOF)
        {
            fs::path dir = fs::path(kCoopSavePath) / "snapshots";
            for (int i = 19; i >= 0; --i)
            {
                fs::path snap = dir / (std::to_string(sessionId) + "_snap" + std::to_string(i) + ".json");
                if (fs::exists(snap))
                {
                    std::cerr << "Session corrupt, rolling back to " << snap << std::endl;
                    fs::copy_file(snap, file, fs::copy_options::overwrite_existing);
                    return false;
                }
            }
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ValidateSessionState error: " << e.what() << std::endl;
        return false;
    }
}

} // namespace CoopNet

