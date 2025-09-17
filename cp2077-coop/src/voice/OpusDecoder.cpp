#include "OpusDecoder.hpp"
#include <opus.h>

namespace CoopVoice
{
static OpusDecoder* g_decoder = nullptr;

bool OpusDecoder_Init(uint32_t sampleRate)
{
    int err = 0;
    g_decoder = opus_decoder_create(sampleRate, 1, &err);
    return err == OPUS_OK;
}

int Opus_DecodeFrame(const uint8_t* data, uint16_t size, int16_t* pcmOut, int frameSamples)
{
    if (!g_decoder)
        return 0;
    int samples = opus_decode(g_decoder, data, size, pcmOut, frameSamples, 0);
    if (samples < 0)
        return 0;
    return samples;
}

void OpusDecoder_Shutdown()
{
    if (g_decoder)
    {
        opus_decoder_destroy(g_decoder);
        g_decoder = nullptr;
    }
}

} // namespace CoopVoice
