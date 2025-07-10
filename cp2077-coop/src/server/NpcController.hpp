#pragma once
#include "../net/Snapshot.hpp"

namespace CoopNet {

void NpcController_ServerTick(float dt);
void NpcController_ClientApplySnap(const NpcSnap& snap);
void NpcController_Despawn(uint32_t id);
void NpcController_OnPlayerEnterSector(uint32_t peerId, uint64_t hash);
uint32_t NpcController_GetSectorSeed(uint64_t hash);
void NpcController_ApplyCrowdSeed(uint64_t hash, uint32_t seed);
const NpcSnap& NpcController_GetSnap();

}
