#include "../../third_party/zstd/zstd.h"
#include "../server/QuestWatchdog.hpp"
#include "Net.hpp"
#include "../core/Red4extUtils.hpp"
#include <cstring>
#include <vector>

namespace CoopNet
{

std::vector<uint8_t> BuildPhaseBundle(uint32_t phaseId)
{
    QuestFullSyncPacket qs{};
    QuestWatchdog_BuildFullSync(phaseId, qs);
    size_t rawSize = sizeof(QuestFullSyncPacket);
    std::vector<uint8_t> raw(rawSize);
    std::memcpy(raw.data(), &qs, rawSize);
    size_t bound = ZSTD_compressBound(rawSize);
    std::vector<uint8_t> out(bound);
    size_t z = ZSTD_compress(out.data(), bound, raw.data(), rawSize, 1);
    if (ZSTD_isError(z))
        return {};
    out.resize(z);
    return out;
}

void ApplyPhaseBundle(uint32_t phaseId, const uint8_t* buf, size_t len)
{
    size_t expected = ZSTD_getFrameContentSize(buf, len);
    if (expected == ZSTD_CONTENTSIZE_ERROR)
        return;
    if (expected == ZSTD_CONTENTSIZE_UNKNOWN)
        expected = 1024 * 1024; // 1MB fallback
    if (expected > 10 * 1024 * 1024)
        return; // sanity cap 10MB
    std::vector<uint8_t> raw(expected);
    size_t size = ZSTD_decompress(raw.data(), raw.size(), buf, len);
    if (ZSTD_isError(size) || size < sizeof(QuestFullSyncPacket))
        return;
    QuestFullSyncPacket pkt{};
    std::memcpy(&pkt, raw.data(), sizeof(QuestFullSyncPacket));
    // TODO: Fix RED4ext API compatibility
    // The Function type and GetFunction signature have changed in newer RED4ext versions
    // This needs to be updated to use the correct CGlobalFunction* and CName-based API
    /*
    if (RED4ext::CGlobalFunction* fn = RED4ext::CRTTISystem::Get()->GetFunction(RED4ext::CName("ApplyFullSync")))
    {
        RED4EXT_EXECUTE("QuestSync", "ApplyFullSync", nullptr, &pkt);
    }
    */
}

} // namespace CoopNet
