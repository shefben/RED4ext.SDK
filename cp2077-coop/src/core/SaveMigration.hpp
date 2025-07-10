#pragma once
#include <string>
#include <cstdint>

namespace CoopNet {
// Detect vanilla save and migrate to coop directory if none exists.
bool MigrateSinglePlayerSave();

// Writes a rolling snapshot for rollback safety
void SaveRollbackSnapshot(uint32_t sessionId, const std::string& jsonBlob);

// Validate session file and restore from snapshot on failure
bool ValidateSessionState(uint32_t sessionId);

// Merge inventory and quest data from a single-player save
bool MergeSinglePlayerData(uint32_t sessionId);
}
