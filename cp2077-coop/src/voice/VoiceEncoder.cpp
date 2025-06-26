#include "VoiceEncoder.hpp"
#include "../net/Net.hpp"
#include <cstring>
#include <iostream>
#include <opus/opus.h>

namespace CoopVoice
{
static bool g_capturing = false;
static OpusEncoder* g_encoder = nullptr;

bool StartCapture(const char* deviceName)
{
    std::cout << "[Voice] StartCapture dev=" << (deviceName ? deviceName : "default") << std::endl;
    int err = 0;
    g_encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK)
    {
        std::cerr << "Failed to init Opus encoder" << std::endl;
        return false;
    }
    g_capturing = true;
    (void)deviceName; // device handling not yet implemented
    return true;
}

int EncodeFrame(int16_t* pcm, uint8_t* outBuf)
{
    if (!g_capturing || !g_encoder)
        return 0;
    int bytes = opus_encode(g_encoder, pcm, 960, outBuf, 256);
    if (bytes < 0)
        return 0;
    return bytes;
}

} // namespace CoopVoice
