#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../src/core/SessionState.hpp"

// NOTE: This utility uses rapidjson for simplicity. The header is expected to be
// provided by the build environment.
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace rapidjson;

struct SingleSave
{
    uint32_t xp = 0;
    std::unordered_map<std::string, uint32_t> quests;
    std::vector<CoopNet::ItemSnap> inventory;
};

static bool LoadJson(const std::string& path, Document& doc)
{
    std::ifstream in(path);
    if (!in.is_open())
        return false;
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    doc.Parse(data.c_str());
    return !doc.HasParseError();
}

static void SaveJson(const std::string& path, const Document& doc)
{
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    const_cast<Document&>(doc).Accept(writer);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << buffer.GetString();
}

static void MergeSaves(const Document& coop, const SingleSave& sp, Document& out, std::string& summary)
{
    out.SetObject();
    auto& alloc = out.GetAllocator();

    // XP
    uint32_t coopXP = coop["xp"].GetUint();
    uint32_t finalXP = std::max(coopXP, sp.xp);
    out.AddMember("xp", finalXP, alloc);
    if (finalXP != sp.xp)
        summary += "XP updated\n";

    // Quests
    Value quests(kObjectType);
    unsigned questDiff = 0;
    for (auto itr = coop["quests"].MemberBegin(); itr != coop["quests"].MemberEnd(); ++itr)
    {
        std::string name = itr->name.GetString();
        uint32_t stage = itr->value.GetUint();
        auto it = sp.quests.find(name);
        uint32_t base = it == sp.quests.end() ? 0 : it->second;
        if (stage > base)
            questDiff++;
        quests.AddMember(Value(name.c_str(), alloc), stage > base ? stage : base, alloc);
    }
    out.AddMember("quests", quests, alloc);
    if (questDiff)
    {
        summary += std::to_string(questDiff) + " quest stages updated\n";
    }

    // Inventory
    Value inv(kArrayType);
    Value conflicts(kArrayType);
    unsigned added = 0;
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
                    std::string desc = "Item " + std::to_string(id) + " qty " + std::to_string(item.quantity) + " vs " + std::to_string(qty);
                    conflicts.PushBack(Value(desc.c_str(), alloc), alloc);
                }
                merged = true;
                Value entry(kObjectType);
                uint16_t finalQty = std::max(item.quantity, qty);
                entry.AddMember("itemId", id, alloc);
                entry.AddMember("qty", finalQty, alloc);
                inv.PushBack(entry, alloc);
                break;
            }
        }
        if (!merged)
        {
            inv.PushBack(c, alloc);
        }
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
            added++;
        }
    }
    out.AddMember("inventory", inv, alloc);
    if (conflicts.Size() > 0)
        out.AddMember("conflicts", conflicts, alloc);
    if (added)
        summary += std::to_string(added) + " items added\n";
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: coop_merge <session.json> <singleplayerSave.dat>\n";
        return 1;
    }

    Document coopDoc;
    if (!LoadJson(argv[1], coopDoc))
    {
        std::cerr << "Failed to read session JSON" << std::endl;
        return 1;
    }

    // Stub: treat the singleplayer save as JSON as well
    Document spDoc;
    if (!LoadJson(argv[2], spDoc))
    {
        std::cerr << "Failed to read singleplayer save" << std::endl;
        return 1;
    }

    SingleSave sp;
    sp.xp = spDoc["xp"].GetUint();
    for (auto itr = spDoc["quests"].MemberBegin(); itr != spDoc["quests"].MemberEnd(); ++itr)
        sp.quests[itr->name.GetString()] = itr->value.GetUint();
    for (auto& e : spDoc["inventory"].GetArray())
    {
        CoopNet::ItemSnap it{e["itemId"].GetUint(), static_cast<uint16_t>(e["qty"].GetUint())};
        sp.inventory.push_back(it);
    }

    Document merged;
    std::string summary;
    MergeSaves(coopDoc, sp, merged, summary);
    SaveJson("merged.dat", merged);
    std::cout << summary;
    return 0;
}
