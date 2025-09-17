#include "OpusEncoder.hpp"
#include <opus.h>
#include <iostream>

namespace CoopVoice
{
static OpusEncoder* g_encoder = nullptr;
static uint32_t g_bitrate = 24000;

bool Opus_Init(uint32_t sampleRate, uint32_t bitrate)
{
    int err = 0;
    g_encoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK)
    {
        std::cerr << "[Voice] Failed to init Opus encoder sr=" << sampleRate << " err=" << err << std::endl;
        g_encoder = nullptr;
        return false;
    }
    g_bitrate = bitrate;
    if (opus_encoder_ctl(g_encoder, OPUS_SET_BITRATE(g_bitrate)) != OPUS_OK)
    {
        opus_encoder_ctl(g_encoder, OPUS_SET_BITRATE(24000));
        g_bitrate = 24000;
    }
    return true;
}

int Opus_EncodeFrame(const int16_t* pcm, int frameSamples, uint8_t* outBuf, uint16_t maxBytes)
{
    if (!g_encoder)
        return 0;
    int bytes = opus_encode(g_encoder, pcm, frameSamples, outBuf, maxBytes);
    if (bytes < 0)
        return 0;
    return bytes;
}

void Opus_Shutdown()
{
    if (g_encoder)
    {
        opus_encoder_destroy(g_encoder);
        g_encoder = nullptr;
    }
}

} // namespace CoopVoice
