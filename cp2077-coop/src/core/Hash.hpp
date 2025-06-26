#pragma once
#include <cstdint>

namespace CoopNet
{
// Helper used by sector streaming to match worldStreaming::SectorID hashing
// in the game. Simple FNV-1a 64-bit implementation.
inline uint64_t Fnv1a64(const char* str)
{
    uint64_t hash = 14695981039346656037ull;
    while (*str)
    {
        hash ^= static_cast<uint8_t>(*str++);
        hash *= 1099511628211ull;
    }
    return hash;
}

inline uint32_t Fnv1a32(const char* str)
{
    uint32_t hash = 2166136261u;
    while (*str)
    {
        hash ^= static_cast<uint8_t>(*str++);
        hash *= 16777619u;
    }
    return hash;
}

inline uint64_t Fnv1a64Pos(float x, float y)
{
    uint64_t hash = 14695981039346656037ull;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&x);
    for (size_t i = 0; i < sizeof(float); ++i)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    bytes = reinterpret_cast<const uint8_t*>(&y);
    for (size_t i = 0; i < sizeof(float); ++i)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}
} // namespace CoopNet

