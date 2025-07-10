#pragma once
#include <cstdint>
#include "VoiceEncoder.hpp"

namespace CoopVoice
{
void PushPacket(uint16_t seq, const uint8_t* data, uint16_t size);
int DecodeFrame(int16_t* pcmOut);
uint16_t ConsumeDropPct();
void Reset();
void SetVolume(float volume);
void SetCodec(Codec codec);
} // namespace CoopVoice
