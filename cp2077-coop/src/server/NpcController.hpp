#pragma once
#include "../net/Snapshot.hpp"

namespace CoopNet {

void NpcController_ServerTick(float dt);
void NpcController_ClientApplySnap(const NpcSnap& snap);

}
