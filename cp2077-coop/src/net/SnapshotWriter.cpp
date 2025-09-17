// #include "../runtime/QuestSync.reds" // REMOVED: Cannot include .reds in C++
// #include "../runtime/SpectatorCam.reds" // REMOVED: Cannot include .reds in C++
#include "Snapshot.hpp"
#include <vector>
#include <mutex>

// Temporary namespaces for missing .reds includes
namespace QuestSync {
    static uint32_t localPhase = 0;
}

namespace SpectatorCam {
    static uint32_t spectatePhase = 0;
}

namespace CoopNet
{

std::vector<EntitySnap> g_entitySnaps;
std::mutex g_entitySnapsMutex;

void AddEntitySnap(uint32_t id, uint32_t phaseId, const TransformSnap& snap)
{
    std::lock_guard<std::mutex> lock(g_entitySnapsMutex);
    g_entitySnaps.push_back({id, phaseId, snap});
}

void ClearEntitySnaps()
{
    std::lock_guard<std::mutex> lock(g_entitySnapsMutex);
    g_entitySnaps.clear();
}

void BuildSnapshot(std::vector<EntitySnap>& out)
{
    uint32_t local = QuestSync::localPhase;
    uint32_t spectate = SpectatorCam::spectatePhase;
    std::lock_guard<std::mutex> lock(g_entitySnapsMutex);
    for (auto& e : g_entitySnaps)
    {
        if (e.phaseId != local && e.phaseId != spectate)
            continue; // PX-3
        out.push_back(e);
    }
}

} // namespace CoopNet
