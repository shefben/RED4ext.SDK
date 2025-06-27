#include "SnapshotHeap.hpp"
#include "../core/GameClock.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <malloc.h>
#include <vector>

namespace CoopNet
{
struct Entry
{
    uint64_t time;
    size_t bytes;
};
static std::vector<Entry> g_entries;

void SnapshotStore_Add(size_t bytes)
{
    g_entries.push_back({GameClock::GetTimeMs(), bytes});
}

void SnapshotStore_PurgeOld(float ageSec)
{
    uint64_t now = GameClock::GetTimeMs();
    g_entries.erase(std::remove_if(g_entries.begin(), g_entries.end(), [&](const Entry& e)
                                   { return now - e.time > static_cast<uint64_t>(ageSec * 1000.f); }),
                    g_entries.end());
}

size_t SnapshotStore_GetMemory()
{
    size_t total = 0;
    for (auto& e : g_entries)
        total += e.bytes;
    return total;
}

void SnapshotMemCheck()
{
#ifdef __GLIBC__
    struct mallinfo mi = mallinfo();
    size_t used = static_cast<size_t>(mi.uordblks);
#else
    size_t used = 0;
#endif
    used = std::max(used, SnapshotStore_GetMemory());
    if (used > (size_t(2) << 30))
    {
        SnapshotStore_PurgeOld(300.f);
        std::cerr << "[MemGuard] snapshot heap high, purged old baselines" << std::endl;
    }
}

} // namespace CoopNet
