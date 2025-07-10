#pragma once
#include <cstdint>

namespace CoopVoice
{
bool Opus_Init(uint32_t sampleRate, uint32_t bitrate);
int Opus_EncodeFrame(const int16_t* pcm, int frameSamples, uint8_t* outBuf, uint16_t maxBytes);
void Opus_Shutdown();
}
