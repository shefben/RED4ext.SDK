#include "GameClock.hpp"
#include <algorithm>
#include <mutex>
#include <chrono>

namespace CoopNet
{

static std::mutex g_clockMutex;
float GameClock::s_accumulator = 0.0f;
uint64_t GameClock::s_tick = 0;
float GameClock::currentTickMs = kDefaultDeltaMs;

void GameClock::Tick(float dtMs)
{
    std::lock_guard lock(g_clockMutex);
    s_accumulator += dtMs;
    while (s_accumulator >= currentTickMs)
    {
        s_accumulator -= currentTickMs;
        ++s_tick;
    }
}

uint64_t GameClock::GetCurrentTick()
{
    std::lock_guard lock(g_clockMutex);
    return s_tick;
}

float GameClock::GetTickAlpha(float nowMs)
{
    std::lock_guard lock(g_clockMutex);
    float total = s_accumulator + nowMs;
    if (total < 0.0f)
        total = 0.0f;
    if (total > currentTickMs)
        total = currentTickMs;
    return total / currentTickMs;
}

float GameClock::GetTickMs()
{
    std::lock_guard lock(g_clockMutex);
    return currentTickMs;
}

void GameClock::SetTickMs(float ms)
{
    std::lock_guard lock(g_clockMutex);
    currentTickMs = std::clamp(ms, 20.f, 50.f);
}

uint64_t GameClock::GetTimeMs()
{
    auto now = std::chrono::steady_clock::now();
    auto epoch = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
    return static_cast<uint64_t>(millis.count());
}

} // namespace CoopNet
