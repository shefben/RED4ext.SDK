#pragma once
#include "VoiceEncoder.hpp"
#include <cstdint>

namespace CoopVoice
{
void PushPacket(uint16_t seq, const uint8_t* data, uint16_t size);
int DecodeFrame(int16_t* pcmOut);
void QueuePCM(const int16_t* pcm, int samples);
uint16_t ConsumeDropPct();
void Reset();
void SetVolume(float volume);
void SetCodec(Codec codec);
} // namespace CoopVoice
