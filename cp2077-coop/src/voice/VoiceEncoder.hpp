#pragma once
#include <cstdint>

namespace CoopVoice
{
bool StartCapture(const char* deviceName, uint32_t sampleRate, uint32_t bitrate);
int EncodeFrame(int16_t* pcm, uint8_t* outBuf);
void StopCapture();
} // namespace CoopVoice
