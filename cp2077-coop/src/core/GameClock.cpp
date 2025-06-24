#include "GameClock.hpp"

namespace CoopNet
{

float GameClock::s_accumulator = 0.0f;
uint64_t GameClock::s_tick = 0;

void GameClock::Tick(float dtMs)
{
    s_accumulator += dtMs;
    while (s_accumulator >= kFixedDeltaMs)
    {
        s_accumulator -= kFixedDeltaMs;
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
    if (total > kFixedDeltaMs)
        total = kFixedDeltaMs;
    return total / kFixedDeltaMs;
}

} // namespace CoopNet
