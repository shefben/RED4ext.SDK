#include "../../third_party/zstd/zstd.h"
#include "Net.hpp"
#include <cstring>
#include <iostream>
#include <vector>

namespace CoopNet
{

std::vector<uint8_t> BuildMarkerBlob()
{
    RED4ext::DynArray<RED4ext::Vector3> pos;
    RED4ext::ExecuteFunction("WorldMarkerHelpers", "GatherPositions", nullptr, &pos);
    uint16_t count = static_cast<uint16_t>(pos.size);
    std::vector<uint8_t> raw(sizeof(uint16_t) + count * sizeof(RED4ext::Vector3));
    std::memcpy(raw.data(), &count, sizeof(uint16_t));
    if (count > 0)
        std::memcpy(raw.data() + sizeof(uint16_t), pos.Begin(), count * sizeof(RED4ext::Vector3));
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
    std::vector<uint8_t> raw(10240);
    size_t size = ZSTD_decompress(raw.data(), raw.size(), buf, len);
    if (ZSTD_isError(size))
        return;
    raw.resize(size);
    if (raw.size() < sizeof(uint16_t))
        return;
    uint16_t count;
    std::memcpy(&count, raw.data(), sizeof(uint16_t));
    const RED4ext::Vector3* arr = reinterpret_cast<const RED4ext::Vector3*>(raw.data() + sizeof(uint16_t));
    for (uint16_t i = 0; i < count; ++i)
    {
        RED4ext::ExecuteFunction("WorldMarkerHelpers", "ApplyPosition", nullptr, &arr[i]);
    }
}

} // namespace CoopNet
