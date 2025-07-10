#pragma once
#include <cstdint>

namespace CoopVoice
{
bool OpusDecoder_Init(uint32_t sampleRate);
int Opus_DecodeFrame(const uint8_t* data, uint16_t size, int16_t* pcmOut, int frameSamples);
void OpusDecoder_Shutdown();
}
