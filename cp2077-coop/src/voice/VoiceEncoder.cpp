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

bool StartCapture(const char* deviceName)
{
    if (g_capturing)
        return true;

    std::cout << "[Voice] StartCapture dev=" << (deviceName ? deviceName : "default") << std::endl;

    const ALCchar* dev = (deviceName && *deviceName) ? deviceName : nullptr;
    g_capDev = alcCaptureOpenDevice(dev, 48000, AL_FORMAT_MONO16, 960 * 10);
    if (!g_capDev)
    {
        std::cerr << "Failed to open capture device" << std::endl;
        return false;
    }
    alcCaptureStart(g_capDev);

    int err = 0;
    g_encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK)
    {
        std::cerr << "Failed to init Opus encoder" << std::endl;
        alcCaptureCloseDevice(g_capDev);
        g_capDev = nullptr;
        return false;
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
    if (avail < 960)
        return 0;

    alcCaptureSamples(g_capDev, pcm, 960);
    int bytes = opus_encode(g_encoder, pcm, 960, outBuf, 256);
    if (bytes < 0)
        return 0;
    return bytes;
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
