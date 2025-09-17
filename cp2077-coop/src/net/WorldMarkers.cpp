#include "../../third_party/zstd/zstd.h"
#include "Net.hpp"
#include "../core/Red4extUtils.hpp"
#include <cstring>
#include <vector>
#include <algorithm>

namespace CoopNet
{

std::vector<uint8_t> BuildMarkerBlob()
{
    RED4ext::DynArray<RED4ext::Vector3> pos;
    RED4EXT_EXECUTE("WorldMarkerHelpers", "GatherPositions", nullptr, &pos);
    // Clamp to protocol's 16-bit count to avoid truncation
    uint32_t total = static_cast<uint32_t>(pos.size);
    uint16_t count = static_cast<uint16_t>(std::min<uint32_t>(total, 65535u));
    std::vector<uint8_t> raw(sizeof(uint16_t) + count * sizeof(RED4ext::Vector3));
    std::memcpy(raw.data(), &count, sizeof(uint16_t));
    if (count > 0)
        std::memcpy(raw.data() + sizeof(uint16_t), pos.Begin(), static_cast<size_t>(count) * sizeof(RED4ext::Vector3));
    size_t bound = ZSTD_compressBound(raw.size());
    std::vector<uint8_t> out(bound);
    size_t z = ZSTD_compress(out.data(), bound, raw.data(), raw.size(), 1);
    if (ZSTD_isError(z))
        return {};
    out.resize(z);
    return out;
}

void ApplyMarkerBlob(const uint8_t* buf, size_t len)
{
    size_t expected = ZSTD_getFrameContentSize(buf, len);
    if (expected == ZSTD_CONTENTSIZE_ERROR)
        return;
    if (expected == ZSTD_CONTENTSIZE_UNKNOWN)
        expected = 512 * 1024; // 512KB fallback
    if (expected > 10 * 1024 * 1024)
        return; // sanity cap
    std::vector<uint8_t> raw(expected);
    size_t size = ZSTD_decompress(raw.data(), raw.size(), buf, len);
    if (ZSTD_isError(size))
        return;
    raw.resize(size);
    if (raw.size() < sizeof(uint16_t))
        return;
    uint16_t count;
    std::memcpy(&count, raw.data(), sizeof(uint16_t));
    size_t need = sizeof(uint16_t) + static_cast<size_t>(count) * sizeof(RED4ext::Vector3);
    if (raw.size() < need)
        return;
    const RED4ext::Vector3* arr = reinterpret_cast<const RED4ext::Vector3*>(raw.data() + sizeof(uint16_t));
    for (uint16_t i = 0; i < count; ++i)
    {
        RED4EXT_EXECUTE("WorldMarkerHelpers", "ApplyPosition", nullptr, &arr[i]);
    }
}

} // namespace CoopNet
