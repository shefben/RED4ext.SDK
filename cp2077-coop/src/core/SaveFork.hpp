#pragma once

// Utility functions that rewrite save file paths for co-op sessions.

#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>

#include <cstdint>
#include <string>

namespace CoopNet
{
constexpr const char* kCoopSavePath = "SavedGames/Coop/";

std::string GetSessionSavePath(uint32_t sessionId);
void EnsureCoopSaveDirs();
void SaveSession(uint32_t sessionId, const std::string& jsonBlob);
void SavePhase(uint32_t sessionId, uint32_t peerId, const std::string& jsonBlob);

struct CarParking
{
    uint32_t vehTpl = 0;
    RED4ext::Vector3 pos{};
    RED4ext::Quaternion rot{};
    uint16_t health = 0;
};

bool LoadCarParking(uint32_t sessionId, uint32_t peerId, CarParking& out);
void SaveCarParking(uint32_t sessionId, uint32_t peerId, const CarParking& cp);
bool LoadArcadeHighScore(uint32_t cabId, uint32_t& peerId, uint32_t& score);
void SaveArcadeHighScore(uint32_t cabId, uint32_t peerId, uint32_t score);
} // namespace CoopNet
