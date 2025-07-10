#pragma once
#include <cstdint>

namespace CoopVoice
{
void PushPacket(uint16_t seq, const uint8_t* data, uint16_t size);
int DecodeFrame(int16_t* pcmOut);
uint16_t ConsumeDropPct();
void Reset();
} // namespace CoopVoice
