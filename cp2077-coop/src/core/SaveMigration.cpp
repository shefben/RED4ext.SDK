#include "SaveMigration.hpp"
#include "Hash.hpp"
#include "SaveFork.hpp"
#include "SessionState.hpp"
#include "../net/Snapshot.hpp"
#include "../third_party/zstd/zstd.h"
#ifdef HAVE_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#endif
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace CoopNet
{
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
    try
    {
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
        MergeSinglePlayerData(sid);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "MigrateSinglePlayerSave error: " << e.what() << std::endl;
        return false;
    }
}

static size_t g_snapIndex = 0;

void SaveRollbackSnapshot(uint32_t sessionId, const std::string& jsonBlob)
{
    try
    {
        EnsureCoopSaveDirs();
        fs::path dir = fs::path(kCoopSavePath) / "snapshots";
        fs::create_directories(dir);
        fs::path file = dir / (std::to_string(sessionId) + "_snap" + std::to_string(g_snapIndex) + ".json");
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (out.is_open())
            out << jsonBlob;
        g_snapIndex = (g_snapIndex + 1) % 20;
    }
    catch (const std::exception& e)
    {
        std::cerr << "SaveRollbackSnapshot error: " << e.what() << std::endl;
    }
}

bool ValidateSessionState(uint32_t sessionId)
{
    try
    {
        fs::path file = fs::path(kCoopSavePath) / (std::to_string(sessionId) + ".json.zst");
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
    }
    catch (const std::exception& e)
    {
        std::cerr << "ValidateSessionState error: " << e.what() << std::endl;
        return false;
    }
}

struct SingleSave
{
    uint32_t xp = 0;
    std::unordered_map<std::string, uint32_t> quests;
    std::vector<CoopNet::ItemSnap> inventory;
};

#ifdef HAVE_RAPIDJSON
static bool LoadJsonFile(const fs::path& path, rapidjson::Document& doc)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        return false;
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (path.extension() == ".zst")
    {
        size_t expected = ZSTD_getFrameContentSize(data.data(), data.size());
        if (expected == ZSTD_CONTENTSIZE_ERROR)
            return false;
        if (expected == ZSTD_CONTENTSIZE_UNKNOWN)
            expected = 1024 * 1024; // 1MB fallback
        if (expected > 10 * 1024 * 1024)
            return false;
        std::vector<char> buf(expected);
        size_t out = ZSTD_decompress(buf.data(), buf.size(), data.data(), data.size());
        if (ZSTD_isError(out))
            return false;
        data.assign(buf.data(), out);
    }
    doc.Parse(data.c_str());
    return !doc.HasParseError();
}

static bool LoadSingleSave(const fs::path& path, SingleSave& out)
{
    rapidjson::Document doc;
    if (!LoadJsonFile(path, doc))
        return false;
    if (doc.HasMember("xp"))
        out.xp = doc["xp"].GetUint();
    if (doc.HasMember("quests"))
    {
        for (auto it = doc["quests"].MemberBegin(); it != doc["quests"].MemberEnd(); ++it)
            out.quests[it->name.GetString()] = it->value.GetUint();
    }
    if (doc.HasMember("inventory"))
    {
        for (auto& e : doc["inventory"].GetArray())
        {
            ItemSnap it{e["itemId"].GetUint(), static_cast<uint16_t>(e["qty"].GetUint())};
            out.inventory.push_back(it);
        }
    }
    return true;
}

static void MergeSaves(const rapidjson::Document& coop,
                       const SingleSave& sp,
                       rapidjson::Document& out,
                       std::vector<std::string>& warnings)
{
    using namespace rapidjson;
    out.SetObject();
    auto& alloc = out.GetAllocator();

    uint32_t coopXP = coop["xp"].GetUint();
    uint32_t finalXP = std::max(coopXP, sp.xp);
    out.AddMember("xp", finalXP, alloc);

    Value quests(kObjectType);
    for (auto itr = coop["quests"].MemberBegin(); itr != coop["quests"].MemberEnd(); ++itr)
    {
        std::string name = itr->name.GetString();
        uint32_t stage = itr->value.GetUint();
        auto it = sp.quests.find(name);
        uint32_t base = it == sp.quests.end() ? 0 : it->second;
        if (stage != base && base != 0)
        {
            std::string w = "Quest " + name + " stage mismatch " + std::to_string(base) + " vs " + std::to_string(stage);
            warnings.push_back(w);
        }
        quests.AddMember(Value(name.c_str(), alloc), stage > base ? stage : base, alloc);
    }
    out.AddMember("quests", quests, alloc);

    Value inv(kArrayType);
    for (auto& c : coop["inventory"].GetArray())
    {
        uint32_t id = c["itemId"].GetUint();
        uint16_t qty = static_cast<uint16_t>(c["qty"].GetUint());
        bool merged = false;
        for (auto& item : sp.inventory)
        {
            if (item.itemId == id)
            {
                if (item.quantity != qty)
                {
                    std::string w = "Item " + std::to_string(id) + " qty mismatch " + std::to_string(item.quantity) + " vs " + std::to_string(qty);
                    warnings.push_back(w);
                }
                uint16_t finalQty = std::max(item.quantity, qty);
                Value entry(kObjectType);
                entry.AddMember("itemId", id, alloc);
                entry.AddMember("qty", finalQty, alloc);
                inv.PushBack(entry, alloc);
                merged = true;
                break;
            }
        }
        if (!merged)
            inv.PushBack(c, alloc);
    }
    for (auto& item : sp.inventory)
    {
        bool found = false;
        for (auto& c : coop["inventory"].GetArray())
        {
            if (c["itemId"].GetUint() == item.itemId)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            Value entry(kObjectType);
            entry.AddMember("itemId", item.itemId, alloc);
            entry.AddMember("qty", item.quantity, alloc);
            inv.PushBack(entry, alloc);
        }
    }
    out.AddMember("inventory", inv, alloc);
}

bool MergeSinglePlayerData(uint32_t sessionId)
{
    try
    {
        fs::path coopFile = fs::path(kCoopSavePath) / (std::to_string(sessionId) + ".json.zst");
        rapidjson::Document coopDoc;
        if (!LoadJsonFile(coopFile, coopDoc))
            return false;

        fs::path srcDir = GetVanillaDir();
        if (srcDir.empty() || !fs::exists(srcDir))
            return false;
        fs::path latest;
        fs::file_time_type newest;
        for (auto& p : fs::directory_iterator(srcDir))
        {
            if (p.path().extension() == ".json")
            {
                if (latest.empty() || p.last_write_time() > newest)
                {
                    latest = p.path();
                    newest = p.last_write_time();
                }
            }
        }
        if (latest.empty())
            return false;

        SingleSave sp{};
        if (!LoadSingleSave(latest, sp))
            return false;

        rapidjson::Document merged;
        std::vector<std::string> warns;
        MergeSaves(coopDoc, sp, merged, warns);

        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> wr(buf);
        merged.Accept(wr);
        SaveSession(sessionId, buf.GetString());

        for (auto& w : warns)
            std::cerr << "[Merge] " << w << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "MergeSinglePlayerData error: " << e.what() << std::endl;
        return false;
    }
}

#else // !HAVE_RAPIDJSON

bool MergeSinglePlayerData(uint32_t sessionId, const std::string& singlePlayerPath)
{
    std::cerr << "[SaveMigration] RapidJSON not available, save migration disabled" << std::endl;
    return false;
}

#endif // HAVE_RAPIDJSON

} // namespace CoopNet
