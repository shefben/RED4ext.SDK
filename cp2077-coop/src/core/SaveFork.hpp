#pragma once

// Utility functions that rewrite save file paths for co-op sessions.

#include <string>
#include <cstdint>

namespace CoopNet
{
constexpr const char* kCoopSavePath = "SavedGames/Coop/";

std::string GetSessionSavePath(uint32_t sessionId);
void EnsureCoopSaveDirs();
void SaveSession(uint32_t sessionId, const std::string& jsonBlob);
}
