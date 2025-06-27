#include "SaveFork.hpp"
// Saves are written with Content-Encoding: zstd
#include "../third_party/zstd/zstd.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace CoopNet
{
std::string GetSessionSavePath(uint32_t sessionId)
{
    std::filesystem::path dir = std::filesystem::path(kCoopSavePath) / std::to_string(sessionId);
    return dir.string();
}

void EnsureCoopSaveDirs()
{
    try
    {
        std::filesystem::path base(kCoopSavePath);
        if (std::filesystem::create_directories(base))
        {
            std::cout << "Created save directory: " << base << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to create save directory " << kCoopSavePath << ": " << e.what() << std::endl;
    }
}

bool LoadCarParking(uint32_t sessionId, uint32_t peerId, CarParking& out)
{
    namespace fs = std::filesystem;
    fs::path file =
        fs::path(kCoopSavePath) / std::to_string(sessionId) / ("phase_" + std::to_string(peerId) + ".json.zst");
    std::ifstream in(file, std::ios::binary);
    if (!in.is_open())
        return false;
    std::string zdata((std::istreambuf_iterator<char>(in)), {});
    std::vector<char> raw(2048);
    size_t size = ZSTD_decompress(raw.data(), raw.size(), zdata.data(), zdata.size());
    if (ZSTD_isError(size))
        return false;
    std::string json(raw.data(), size);
    if (json.find("\"CarParking\"") == std::string::npos)
        return false;
    int parsed = std::sscanf(
        json.c_str(), "{\"CarParking\":{\"vehTpl\":%u,\"pos\":[%f,%f,%f],\"rot\":[%f,%f,%f,%f],\"health\":%hu",
        &out.vehTpl, &out.pos.X, &out.pos.Y, &out.pos.Z, &out.rot.X, &out.rot.Y, &out.rot.Z, &out.rot.W, &out.health);
    return parsed >= 9;
}

void SaveCarParking(uint32_t sessionId, uint32_t peerId, const CarParking& cp)
{
    std::stringstream ss;
    ss << "{\"CarParking\":{\"vehTpl\":" << cp.vehTpl << ",\"pos\":[" << cp.pos.X << ',' << cp.pos.Y << ',' << cp.pos.Z
       << "],\"rot\":[" << cp.rot.X << ',' << cp.rot.Y << ',' << cp.rot.Z << ',' << cp.rot.W
       << "],\"health\":" << cp.health << "}}";
    SavePhase(sessionId, peerId, ss.str());
}

void SaveSession(uint32_t sessionId, const std::string& jsonBlob)
{
    try
    {
        EnsureCoopSaveDirs();
        namespace fs = std::filesystem;
        fs::path dir(kCoopSavePath);
        fs::path file = dir / (std::to_string(sessionId) + ".json.zst");

        std::vector<char> buf(ZSTD_compressBound(jsonBlob.size()));
        size_t z = ZSTD_compress(buf.data(), buf.size(), jsonBlob.data(), jsonBlob.size(), 3);
        if (ZSTD_isError(z))
            z = 0;
        buf.resize(z);

        for (int i = 5; i >= 1; --i)
        {
            fs::path prev = dir / (std::to_string(sessionId) + ".json.zst." + std::to_string(i));
            if (i == 5 && fs::exists(prev))
                fs::remove(prev);
            if (i > 1)
            {
                fs::path older = dir / (std::to_string(sessionId) + ".json.zst." + std::to_string(i - 1));
                if (fs::exists(older))
                    fs::rename(older, prev);
            }
        }
        if (fs::exists(file))
            fs::rename(file, dir / (std::to_string(sessionId) + ".json.zst.1"));

        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            std::cerr << "Failed to open session file " << file << std::endl;
            return;
        }

        out.write(buf.data(), buf.size());
        out.close();

        if (out.good())
        {
            std::cout << "Saved session to " << file << std::endl;
        }
        else
        {
            std::cerr << "Failed to write session file " << file << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error saving session: " << e.what() << std::endl;
    }
}

void SavePhase(uint32_t sessionId, uint32_t peerId, const std::string& jsonBlob)
{
    try
    {
        EnsureCoopSaveDirs();
        namespace fs = std::filesystem;
        fs::path dir = fs::path(kCoopSavePath) / std::to_string(sessionId);
        fs::create_directories(dir);
        fs::path file = dir / ("phase_" + std::to_string(peerId) + ".json.zst");

        std::vector<char> buf(ZSTD_compressBound(jsonBlob.size()));
        size_t z = ZSTD_compress(buf.data(), buf.size(), jsonBlob.data(), jsonBlob.size(), 3);
        if (ZSTD_isError(z))
            z = 0;
        buf.resize(z);

        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            std::cerr << "Failed to open phase file " << file << std::endl;
            return;
        }
        out.write(buf.data(), buf.size());
        out.close();
        if (out.good())
        {
            fs::path indexFile = dir / "phase_index.txt";
            std::unordered_set<uint32_t> ids;
            if (std::ifstream idxIn{indexFile}; idxIn.is_open())
            {
                uint32_t id;
                while (idxIn >> id)
                    ids.insert(id);
            }
            if (ids.insert(peerId).second)
            {
                std::ofstream idxOut(indexFile, std::ios::trunc);
                if (idxOut.is_open())
                {
                    for (auto id : ids)
                        idxOut << id << '\n';
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error saving phase: " << e.what() << std::endl;
    }
}
} // namespace CoopNet
