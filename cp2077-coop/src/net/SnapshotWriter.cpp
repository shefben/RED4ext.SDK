#include "../runtime/QuestSync.reds"
#include "../runtime/SpectatorCam.reds"
#include "Snapshot.hpp"
#include <vector>

namespace CoopNet
{
struct EntitySnap
{
    uint32_t id;
    uint32_t phaseId;
    TransformSnap snap;
};

std::vector<EntitySnap> g_entitySnaps;

void BuildSnapshot(std::vector<EntitySnap>& out)
{
    uint32_t local = QuestSync::localPhase;
    uint32_t spectate = SpectatorCam::spectatePhase;
    for (auto& e : g_entitySnaps)
    {
        if (e.phaseId != local && e.phaseId != spectate)
            continue; // PX-3
        out.push_back(e);
    }
}

} // namespace CoopNet
