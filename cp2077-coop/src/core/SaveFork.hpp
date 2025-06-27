#pragma once

// Utility functions that rewrite save file paths for co-op sessions.

#include <cstdint>
#include <string>

namespace CoopNet
{
constexpr const char* kCoopSavePath = "SavedGames/Coop/";

std::string GetSessionSavePath(uint32_t sessionId);
void EnsureCoopSaveDirs();
void SaveSession(uint32_t sessionId, const std::string& jsonBlob);
void SavePhase(uint32_t sessionId, uint32_t peerId, const std::string& jsonBlob);
} // namespace CoopNet
