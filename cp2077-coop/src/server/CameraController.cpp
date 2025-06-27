#include "CameraController.hpp"
#include "../net/Net.hpp"
#include <unordered_map>

namespace CoopNet
{
struct CamState
{
    uint32_t peerId;
    float timer;
};
static std::unordered_map<uint32_t, CamState> g_cams;

void CamController_Hijack(uint32_t camId, uint32_t peerId)
{
    g_cams[camId] = {peerId, 0.f};
    Net_BroadcastCamHijack(camId, peerId);
}

void CamController_Stop(uint32_t camId)
{
    g_cams.erase(camId);
}

void CamController_Tick(float dt)
{
    for (auto& kv : g_cams)
    {
        kv.second.timer += dt;
        if (kv.second.timer >= 500.f)
        {
            kv.second.timer = 0.f;
            Net_BroadcastCamFrameStart(kv.first);
        }
    }
}
} // namespace CoopNet
