#pragma once
#include <cstdint>
namespace CoopNet
{
void Journal_Log(uint64_t tick, uint32_t peerId, const char* action, uint32_t entityId, int32_t delta);
}
