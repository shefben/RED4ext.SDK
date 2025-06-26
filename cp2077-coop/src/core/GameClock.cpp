#include "GameClock.hpp"
#include <algorithm>

namespace CoopNet
{

float GameClock::s_accumulator = 0.0f;
uint64_t GameClock::s_tick = 0;
float GameClock::currentTickMs = kDefaultDeltaMs;

void GameClock::Tick(float dtMs)
{
    s_accumulator += dtMs;
    while (s_accumulator >= currentTickMs)
    {
        s_accumulator -= currentTickMs;
        ++s_tick;
    }
}

uint64_t GameClock::GetCurrentTick()
{
    return s_tick;
}

float GameClock::GetTickAlpha(float nowMs)
{
    float total = s_accumulator + nowMs;
    if (total < 0.0f)
        total = 0.0f;
    if (total > currentTickMs)
        total = currentTickMs;
    return total / currentTickMs;
}

float GameClock::GetTickMs()
{
    return currentTickMs;
}

void GameClock::SetTickMs(float ms)
{
    currentTickMs = std::clamp(ms, 20.f, 50.f);
}

} // namespace CoopNet
