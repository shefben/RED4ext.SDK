#include "VoiceEncoder.hpp"
#include "../net/Net.hpp"
#include <cstring>
#include <iostream>
#include <opus/opus.h>
#include <AL/alc.h>

namespace CoopVoice
{
static bool g_capturing = false;
static OpusEncoder* g_encoder = nullptr;
static ALCdevice* g_capDev = nullptr;
static uint32_t g_sampleRate = 48000;
static uint32_t g_bitrate = 24000;
static int g_frameSamples = 960;

bool StartCapture(const char* deviceName, uint32_t sampleRate, uint32_t bitrate)
{
    if (g_capturing)
        return true;

    g_sampleRate = sampleRate;
    g_bitrate = bitrate;
    g_frameSamples = static_cast<int>(g_sampleRate / 50);

    std::cout << "[Voice] StartCapture dev=" << (deviceName ? deviceName : "default") << " sr=" << g_sampleRate << " br=" << g_bitrate << std::endl;

    const ALCchar* dev = (deviceName && *deviceName) ? deviceName : nullptr;
    g_capDev = alcCaptureOpenDevice(dev, g_sampleRate, AL_FORMAT_MONO16, g_frameSamples * 10);
    if (!g_capDev)
    {
        std::cerr << "Failed to open capture device" << std::endl;
        if (g_sampleRate != 48000)
        {
            g_sampleRate = 48000;
            g_frameSamples = 960;
            g_capDev = alcCaptureOpenDevice(dev, g_sampleRate, AL_FORMAT_MONO16, g_frameSamples * 10);
        }
        if (!g_capDev)
            return false;
    }
    alcCaptureStart(g_capDev);

    int err = 0;
    g_encoder = opus_encoder_create(g_sampleRate, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK)
    {
        std::cerr << "Failed to init Opus encoder" << std::endl;
        alcCaptureCloseDevice(g_capDev);
        g_capDev = nullptr;
        return false;
    }
    if (opus_encoder_ctl(g_encoder, OPUS_SET_BITRATE(g_bitrate)) != OPUS_OK)
    {
        opus_encoder_ctl(g_encoder, OPUS_SET_BITRATE(24000));
        g_bitrate = 24000;
    }

    g_capturing = true;
    return true;
}

int EncodeFrame(int16_t* pcm, uint8_t* outBuf)
{
    if (!g_capturing || !g_encoder || !g_capDev)
        return 0;

    ALCint avail = 0;
    alcGetIntegerv(g_capDev, ALC_CAPTURE_SAMPLES, 1, &avail);
    if (avail < g_frameSamples)
        return 0;

    alcCaptureSamples(g_capDev, pcm, g_frameSamples);
    int bytes = opus_encode(g_encoder, pcm, g_frameSamples, outBuf, 256);
    if (bytes < 0)
        return 0;
    return bytes;
}

uint16_t GetFrameSamples()
{
    return static_cast<uint16_t>(g_frameSamples);
}

void StopCapture()
{
    if (!g_capturing)
        return;
    if (g_capDev)
    {
        alcCaptureStop(g_capDev);
        alcCaptureCloseDevice(g_capDev);
        g_capDev = nullptr;
    }
    if (g_encoder)
    {
        opus_encoder_destroy(g_encoder);
        g_encoder = nullptr;
    }
    g_capturing = false;
}

} // namespace CoopVoice
