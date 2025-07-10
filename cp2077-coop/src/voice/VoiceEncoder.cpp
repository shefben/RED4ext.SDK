#include "VoiceEncoder.hpp"
#include "OpusEncoder.hpp"
#include "../net/Net.hpp"
#include <AL/alc.h>
#include <cstring>
#include <iostream>

namespace CoopVoice
{
static bool g_capturing = false;
static ALCdevice* g_capDev = nullptr;
static uint32_t g_sampleRate = 48000;
static uint32_t g_bitrate = 24000;
static int g_frameSamples = 960;
static Codec g_codec = Codec::Opus;

bool StartCapture(const char* deviceName, uint32_t sampleRate, uint32_t bitrate, Codec codec)
{
    if (g_capturing)
        return true;

    g_sampleRate = sampleRate;
    g_bitrate = bitrate;
    g_frameSamples = static_cast<int>(g_sampleRate / 50);
    g_codec = codec;

    std::cout << "[Voice] StartCapture dev=" << (deviceName ? deviceName : "default") << " sr=" << g_sampleRate
              << " br=" << g_bitrate << std::endl;

    const ALCchar* dev = (deviceName && *deviceName) ? deviceName : nullptr;
    g_capDev = alcCaptureOpenDevice(dev, g_sampleRate, AL_FORMAT_MONO16, g_frameSamples * 10);
    if (!g_capDev)
    {
        ALenum err = alcGetError(nullptr);
        std::cerr << "[Voice] Failed to open capture device '" << (deviceName ? deviceName : "default")
                  << "' sr=" << g_sampleRate << " err=" << err << std::endl;
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

    if (g_codec == Codec::Opus)
    {
        if (!Opus_Init(g_sampleRate, g_bitrate))
        {
            alcCaptureCloseDevice(g_capDev);
            g_capDev = nullptr;
            return false;
        }
    }

    g_capturing = true;
    return true;
}

int EncodeFrame(int16_t* pcm, uint8_t* outBuf)
{
    if (!g_capturing || !g_capDev)
        return 0;

    ALCint avail = 0;
    alcGetIntegerv(g_capDev, ALC_CAPTURE_SAMPLES, 1, &avail);
    if (avail < g_frameSamples)
        return 0;

    alcCaptureSamples(g_capDev, pcm, g_frameSamples);
    if (g_codec == Codec::Opus)
    {
        return Opus_EncodeFrame(pcm, g_frameSamples, outBuf, kOpusFrameBytes);
    }
    std::memcpy(outBuf, pcm, g_frameSamples * sizeof(int16_t));
    return g_frameSamples * sizeof(int16_t);
}

uint16_t GetFrameSamples()
{
    return static_cast<uint16_t>(g_frameSamples);
}

uint16_t GetFrameBytes()
{
    if (g_codec == Codec::Opus)
        return kOpusFrameBytes;
    return static_cast<uint16_t>(g_frameSamples * sizeof(int16_t));
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
    if (g_codec == Codec::Opus)
        Opus_Shutdown();
    g_capturing = false;
}

void SetCodec(Codec codec)
{
    g_codec = codec;
}

} // namespace CoopVoice
