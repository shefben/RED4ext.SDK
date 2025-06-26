#pragma once

#include <cstdint>

namespace CoopNet
{
// Fixed-step simulation timing.
// Call Tick(dtMs) each frame to accumulate elapsed time.
// GetCurrentTick() returns the deterministic tick index.
// GetTickAlpha(nowMs) yields interpolation alpha within the current tick.

constexpr float kDefaultDeltaMs = 32.f;

class GameClock
{
public:
    static void Tick(float dtMs);
    static uint64_t GetCurrentTick();
    static float GetTickAlpha(float nowMs);
    static float GetTickMs();
    static void SetTickMs(float ms);
    static float currentTickMs; // exposed for scripts

private:
    static float s_accumulator;
    static uint64_t s_tick;
};

} // namespace CoopNet
