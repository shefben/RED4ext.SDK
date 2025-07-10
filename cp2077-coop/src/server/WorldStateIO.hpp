#pragma once
#include "../net/Packets.hpp"

namespace CoopNet {

bool LoadWorldState(WorldStatePacket& out);
void SaveWorldState(const WorldStatePacket& state);

} // namespace CoopNet
