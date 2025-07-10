#pragma once
#include <cstdint>

namespace CoopVoice
{
enum class Codec : uint8_t
{
    PCM = 0,
    Opus = 1
};

constexpr uint16_t kOpusFrameBytes = 256;
constexpr uint16_t kPCMFrameBytes = 2048;

uint16_t GetFrameSamples();
uint16_t GetFrameBytes();
bool StartCapture(const char* deviceName, uint32_t sampleRate, uint32_t bitrate, Codec codec);
int EncodeFrame(int16_t* pcm, uint8_t* outBuf);
void StopCapture();
void SetCodec(Codec codec);
} // namespace CoopVoice
