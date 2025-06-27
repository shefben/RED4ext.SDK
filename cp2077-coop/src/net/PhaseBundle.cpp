#include "../../third_party/zstd/zstd.h"
#include "../server/QuestWatchdog.hpp"
#include "Net.hpp"
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
    std::vector<uint8_t> raw(16384);
    size_t size = ZSTD_decompress(raw.data(), raw.size(), buf, len);
    if (ZSTD_isError(size) || size < sizeof(QuestFullSyncPacket))
        return;
    QuestFullSyncPacket pkt{};
    std::memcpy(&pkt, raw.data(), sizeof(QuestFullSyncPacket));
    if (RED4ext::Function* fn = RED4ext::CRTTISystem::Get()->GetFunction("QuestSync", "ApplyFullSync"))
    {
        RED4ext::ExecuteFunction("QuestSync", "ApplyFullSync", nullptr, &pkt);
    }
}

} // namespace CoopNet
