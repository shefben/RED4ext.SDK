
// Auto-generated version information - DO NOT EDIT
#pragma once
#include <cstdint>

namespace CoopNet
{
    // Version constants
    constexpr uint32_t VERSION_MAJOR = 1;
    constexpr uint32_t VERSION_MINOR = 0;
    constexpr uint32_t VERSION_PATCH = 0;
    constexpr uint32_t VERSION_BUILD = 0;
    
    // Build information
    constexpr const char* GIT_HASH = "unknown";
    constexpr const char* BUILD_DATE = "2025-09-16 22:10:11 UTC";
    
    // Generate CRC at compile time
    constexpr uint32_t FNV1A_OFFSET = 2166136261u;
    constexpr uint32_t FNV1A_PRIME = 16777619u;
    
    constexpr uint32_t fnv1a_hash(const char* str, uint32_t hash = FNV1A_OFFSET)
    {
        return *str ? fnv1a_hash(str + 1, (hash ^ *str) * FNV1A_PRIME) : hash;
    }
    
    constexpr uint32_t VERSION_CRC = fnv1a_hash("1.0.0-unknown");
}
