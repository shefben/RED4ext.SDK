#include "VoiceDecoder.hpp"
#include <AL/al.h>
#include <AL/alc.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <opus/opus.h>
#include <vector>

namespace CoopVoice
{
struct JitterPkt
{
    uint16_t seq;
    uint16_t size;
    uint8_t data[256];
};

static std::vector<JitterPkt> g_buffer;
static OpusDecoder* g_decoder = nullptr;
static uint16_t g_lastSeq = 0;
static uint32_t g_recv = 0;
static uint32_t g_dropped = 0;
static uint64_t g_lastWarn = 0;
static ALCdevice* g_dev = nullptr;
static ALCcontext* g_ctx = nullptr;
static ALuint g_source = 0;
static ALuint g_buffers[4];
static int g_bufIndex = 0;

void PushPacket(uint16_t seq, const uint8_t* data, uint16_t size)
{
    JitterPkt p{};
    p.seq = seq;
    p.size = std::min<size_t>(size, sizeof(p.data));
    std::memcpy(p.data, data, p.size);
    auto it = std::lower_bound(g_buffer.begin(), g_buffer.end(), seq,
                               [](const JitterPkt& a, uint16_t s) { return a.seq < s; });
    g_buffer.insert(it, p);
    g_recv++;
    while (g_buffer.size() > 120)
    {
        g_buffer.erase(g_buffer.begin());
        g_dropped++;
        uint64_t now =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        if (now - g_lastWarn > 60000)
        {
            std::cerr << "[Voice] backlog purge" << std::endl;
            g_lastWarn = now;
        }
    }
}

static bool NextPacket(JitterPkt& out)
{
    if (g_buffer.empty())
        return false;
    while (!g_buffer.empty() && (uint16_t)(g_lastSeq + 1 - g_buffer.front().seq) > 10)
        g_buffer.erase(g_buffer.begin());
    if (g_buffer.empty())
        return false;
    out = g_buffer.front();
    g_lastSeq = out.seq;
    g_buffer.erase(g_buffer.begin());
    return true;
}

static bool EnsureAL()
{
    if (g_dev)
        return true;
    g_dev = alcOpenDevice("Generic Software");
    if (!g_dev)
        return false;
    g_ctx = alcCreateContext(g_dev, nullptr);
    if (!g_ctx)
    {
        alcCloseDevice(g_dev);
        g_dev = nullptr;
        return false;
    }
    alcMakeContextCurrent(g_ctx);
    alGenSources(1, &g_source);
    alGenBuffers(4, g_buffers);
    g_bufIndex = 0;
    return true;
}

int DecodeFrame(int16_t* pcmOut)
{
    if (!g_decoder)
    {
        int err = 0;
        g_decoder = opus_decoder_create(48000, 1, &err);
        if (err != OPUS_OK)
            return 0;
    }

    JitterPkt pkt{};
    if (!NextPacket(pkt))
        return 0;

    int samples = opus_decode(g_decoder, pkt.data, pkt.size, pcmOut, 960, 0);
    if (samples < 0)
        return 0;

    if (!EnsureAL())
        return samples;

    ALint processed = 0;
    alGetSourcei(g_source, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0)
    {
        ALuint buf;
        alSourceUnqueueBuffers(g_source, 1, &buf);
    }
    ALint queued = 0;
    alGetSourcei(g_source, AL_BUFFERS_QUEUED, &queued);
    if (queued > 8)
        return samples; // drop to avoid latency

    alBufferData(g_buffers[g_bufIndex], AL_FORMAT_MONO16, pcmOut, samples * sizeof(int16_t), 48000);
    alSourceQueueBuffers(g_source, 1, &g_buffers[g_bufIndex]);
    g_bufIndex = (g_bufIndex + 1) % 4;
    ALint state = 0;
    alGetSourcei(g_source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING)
        alSourcePlay(g_source);
    return samples;
}

uint16_t ConsumeDropPct()
{
    uint32_t total = g_recv + g_dropped;
    uint16_t pct = 0;
    if (total > 0)
        pct = static_cast<uint16_t>((g_dropped * 100) / total);
    g_recv = 0;
    g_dropped = 0;
    return pct;
}
} // namespace CoopVoice
