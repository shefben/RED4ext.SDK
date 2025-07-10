#pragma once
#include <cstdint>

namespace CoopVoice
{
constexpr uint16_t kMaxFrameBytes = 256;
uint16_t GetFrameSamples();
bool StartCapture(const char* deviceName, uint32_t sampleRate, uint32_t bitrate);
int EncodeFrame(int16_t* pcm, uint8_t* outBuf);
void StopCapture();
} // namespace CoopVoice
