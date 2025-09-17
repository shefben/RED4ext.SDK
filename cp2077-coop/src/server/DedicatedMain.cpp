#include "../core/GameClock.hpp"
#include "../core/Hash.hpp"
#include "../core/SaveFork.hpp"
#include "../core/SaveMigration.hpp"
#include "../core/SessionState.hpp"
#include "../net/Net.hpp"
#include "AdminController.hpp"
#include "ApartmentController.hpp"
#include "BillboardController.hpp"
#include "BreachController.hpp"
#include "CameraController.hpp"
#include "CarryController.hpp"
#include "DoorBreachController.hpp"
#include "ElevatorController.hpp"
#include "GlobalEventController.hpp"
#include "GrenadeController.hpp"
#include "Heartbeat.hpp"
#include "InfoServer.hpp"
#include "NpcController.hpp"
#include "PhaseGC.hpp"
#include "PoliceDispatch.hpp"
#include "QuestWatchdog.hpp"
#include "SectorLODController.hpp"
#include "ShardController.hpp"
#include "ServerConfig.hpp"
#include "SmartCamController.hpp"
#include "SnapshotHeap.hpp"
#include "WorldStateIO.hpp"
#include "StatusController.hpp"
#include "TextureGuard.hpp"
#include "TrafficController.hpp"
#include "VehicleController.hpp"
#include "VendorController.hpp"
#include "WebDash.hpp"
#include "../plugin/PluginManager.hpp"
#include "../core/TaskGraph.hpp"
#include "../net/Snapshot.hpp"
#include "../core/Red4extUtils.hpp"
#include <RED4ext/RED4ext.hpp>

namespace CoopNet
{
void BuildSnapshot(std::vector<EntitySnap>& out);
}
#include <cmath>
#include <cstring>
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--help") == 0)
        {
            return 0;
        }
    }

    CoopNet::ServerConfig_Load();
    CoopNet::ApartmentController_Load();
    CoopNet::QuestWatchdog_LoadCritical();
    CoopNet::QuestWatchdog_LoadRomance();
    CoopNet::QuestWatchdog_LoadMain();
    CoopNet::QuestWatchdog_LoadSide();
    Net_Init();
    CoopNet::MigrateSinglePlayerSave();
    CoopNet::CarParking park{};
    CoopNet::TransformSnap vs{};
    vs.pos = {0.f, 0.f, 0.f};
    vs.vel = {0.f, 0.f, 0.f};
    vs.rot = {0.f, 0.f, 0.f, 1.f};
    if (CoopNet::LoadCarParking(CoopNet::SessionState_GetId(), 1u, park) && park.health > 0)
    {
        vs.pos = park.pos;
        vs.rot = park.rot;
        vs.health = park.health;
        CoopNet::VehicleController_SpawnPhaseVehicle(park.vehTpl, 0u, vs, 0u);
    }
    else
    {
        CoopNet::VehicleController_SpawnPhaseVehicle(CoopNet::Fnv1a32("vehicle_caliburn"), 0u, vs, 0u);
    }
    CoopNet::WebDash_Start();
    CoopNet::AdminController_Start();
    CoopNet::InfoServer_Start();
    CoopNet::PluginManager_Init();
    // Declare time/weather/session vars before first use
    uint32_t sessionId = 0;
    uint64_t worldClock = 0;
    uint32_t sunAngle = 0;
    uint16_t particleSeed = 1u;
    uint8_t weatherId = 0u;
    uint8_t bdPhase = 0u;
    float worldTimer = 0.f;
    uint16_t lastSunDeg = 0u;
    uint8_t lastWeather = weatherId;
    CoopNet::WorldStatePacket saved{};
    if (CoopNet::LoadWorldState(saved))
    {
        sunAngle = static_cast<uint32_t>(saved.sunAngleDeg % 360) * 100u;
        particleSeed = saved.particleSeed;
        weatherId = saved.weatherId;
        lastSunDeg = saved.sunAngleDeg;
        lastWeather = weatherId;
        CoopNet::SessionState_UpdateWeather(saved.sunAngleDeg, saved.weatherId, saved.particleSeed);
    }
    std::cout << "Dedicated up" << std::endl;
    CoopNet::TaskGraph taskGraph;
    size_t maxWorkers = std::max<size_t>(1, std::thread::hardware_concurrency() - 1);
    taskGraph.Start(maxWorkers);

    // Main server loop
    bool running = true;
    uint32_t idleTicks = 0;
    float frameAccum = 0.f;
    int frameCount = 0;
    float goodTime = 0.f;
    float hbTimer = 0.f;
    float memTimer = 0.f;
    float latencyTimer = 0.f;
    float scaleTimer = 0.f;
    float scaleAccum = 0.f;
    int scaleFrames = 0;
    float fastUnder = 0.f;
    float tickMs = CoopNet::GameClock::GetTickMs();
    bool validated = false;
    bool hbSent = false;
    auto last = std::chrono::steady_clock::now();

    while (running)
    {
        auto begin = std::chrono::steady_clock::now();
        if (sessionId == 0)
            sessionId = CoopNet::SessionState_GetId();
        if (sessionId && !validated)
        {
            CoopNet::ValidateSessionState(sessionId);
            validated = true;
            std::string hjson = "{\"id\":" + std::to_string(sessionId) + "}";
            CoopNet::Heartbeat_Send(hjson);
            hbSent = true;
        }

        CoopNet::GameClock::Tick(tickMs);
        worldClock += static_cast<uint64_t>(tickMs);
        sunAngle = (sunAngle + static_cast<uint32_t>(tickMs)) % 36000;
        worldTimer += tickMs / 1000.f;
        uint16_t deg = static_cast<uint16_t>((sunAngle + 50) / 100);
        if (deg >= 360)
            deg = 0;
        bool changed = std::abs(static_cast<int>(deg) - static_cast<int>(lastSunDeg)) >= 5 || weatherId != lastWeather;
        if (worldTimer >= 30.f || changed)
        {
            worldTimer = 0.f;
            lastSunDeg = deg;
            if (weatherId != lastWeather)
            {
                lastWeather = weatherId;
                particleSeed = static_cast<uint16_t>(std::rand());
            }
            Net_BroadcastWorldState(deg, weatherId, particleSeed);
            CoopNet::SessionState_UpdateWeather(deg, weatherId, particleSeed);
        }
        CoopNet::ElevatorController_ServerTick(tickMs);
        if (!CoopNet::ElevatorController_IsPaused())
        {
            taskGraph.Submit([tickMs]
                            { CoopNet::NpcController_ServerTick(tickMs); });
            taskGraph.Submit([tickMs]
                            { CoopNet::VehicleController_PhysicsStep(tickMs); });
            CoopNet::VehicleController_ServerTick(tickMs);
            CoopNet::BreachController_ServerTick(tickMs);
            CoopNet::ShardController_ServerTick(tickMs);
            CoopNet::VendorController_Tick(tickMs, worldClock);
            CoopNet::BillboardController_Tick(tickMs);
            CoopNet::DoorBreachController_Tick(tickMs);
            CoopNet::CamController_Tick(tickMs);
            CoopNet::CarryController_Tick(tickMs);
            CoopNet::GrenadeController_Tick(tickMs);
            CoopNet::PoliceDispatch_Tick(tickMs);
            CoopNet::StatusController_Tick(tickMs);
            CoopNet::TrafficController_Tick(tickMs);
            RED4EXT_EXECUTE("GameModeManager", "TickDM", nullptr, static_cast<uint32_t>(tickMs));
        }
        Net_Poll(static_cast<uint32_t>(tickMs));
        taskGraph.Submit([]
                        {
                            std::vector<CoopNet::EntitySnap> tmp;
                            CoopNet::BuildSnapshot(tmp);
                        });
        CoopNet::QuestWatchdog_Tick(tickMs);
        CoopNet::PhaseGC_Tick(CoopNet::GameClock::GetCurrentTick());
        CoopNet::AdminController_Tick(tickMs);
        CoopNet::PluginManager_Tick(tickMs / 1000.f);
        hbTimer += tickMs / 1000.f;
        memTimer += tickMs / 1000.f;
        CoopNet::TextureGuard_Tick(tickMs / 1000.f);
        CoopNet::SectorLODController_Tick(tickMs / 1000.f);
        latencyTimer += tickMs / 1000.f;
        for (auto* c : Net_GetConnections())
        {
            uint64_t now = CoopNet::GameClock::GetTimeMs();
            if (now - c->lastBWCheckMs >= 30000)
            {
                c->lastBWCheckMs = now;
                bool poor = c->rttMs > 250.f || c->packetLoss > 0.15f;
                if (poor && !c->lowBWMode)
                {
                    c->lowBWMode = true;
                    Net_SendLowBWMode(c, true);
                }
                else if (!poor && c->lowBWMode)
                {
                    c->lowBWMode = false;
                    Net_SendLowBWMode(c, false);
                }
            }
        }
        if (latencyTimer >= 10.f)
        {
            float sum = 0.f;
            int cnt = 0;
            for (auto* c : Net_GetConnections())
            {
                sum += c->GetAverageRtt();
                ++cnt;
            }
            if (cnt > 0)
            {
                float avgRtt = sum / cnt;
                if (avgRtt > 200.f && tickMs < 40.f)
                {
                    tickMs = 40.f;
                    CoopNet::GameClock::SetTickMs(tickMs);
                    Net_BroadcastTickRateChange(static_cast<uint16_t>(tickMs));
                }
                else if (avgRtt < 120.f && tickMs > 25.f)
                {
                    tickMs = 25.f;
                    CoopNet::GameClock::SetTickMs(tickMs);
                    Net_BroadcastTickRateChange(static_cast<uint16_t>(tickMs));
                }
            }
            latencyTimer = 0.f;
        }
        // Heartbeat every six minutes so master server can prune stale hosts
        if (hbTimer >= 360.f)
        {
            hbTimer = 0.f;
            size_t count = Net_GetConnections().size();
            uint32_t id = CoopNet::SessionState_GetId();
            std::string json = "{\"id\":" + std::to_string(id) + ",\"cur\":" + std::to_string(count) + ",\"max\":4,\"password\":false,\"mode\":\"Coop\"}";
            CoopNet::Heartbeat_Send(json);
        }
        if (memTimer >= 60.f)
        {
            memTimer = 0.f;
            CoopNet::SnapshotMemCheck();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(tickMs)));

        auto end = std::chrono::steady_clock::now();
        float frameMs = std::chrono::duration<float, std::milli>(end - begin).count();
        frameAccum += frameMs;
        frameCount++;
        scaleTimer += frameMs / 1000.f;
        scaleAccum += frameMs;
        scaleFrames++;
        if (scaleTimer >= 5.f)
        {
            float avgFrame = scaleAccum / scaleFrames;
            scaleTimer = 0.f;
            scaleAccum = 0.f;
            scaleFrames = 0;
            if (avgFrame > 30.f && taskGraph.GetWorkerCount() < maxWorkers)
            {
                taskGraph.Resize(taskGraph.GetWorkerCount() + 1);
                std::cout << "Autoscale workers=" << taskGraph.GetWorkerCount() << std::endl;
                fastUnder = 0.f;
            }
            else if (avgFrame < 15.f)
            {
                fastUnder += 5.f;
                if (fastUnder >= 30.f && taskGraph.GetWorkerCount() > 1)
                {
                    taskGraph.Resize(taskGraph.GetWorkerCount() - 1);
                    std::cout << "Autoscale workers=" << taskGraph.GetWorkerCount() << std::endl;
                    fastUnder = 0.f;
                }
            }
            else
            {
                fastUnder = 0.f;
            }
        }
        if (frameAccum >= 1000.f)
        {
            float avg = frameAccum / frameCount;
            frameAccum = 0.f;
            frameCount = 0;
            if (avg > 25.f && tickMs < 40.f)
            {
                tickMs = 40.f;
                CoopNet::GameClock::SetTickMs(tickMs);
                Net_BroadcastTickRateChange(static_cast<uint16_t>(tickMs));
                goodTime = 0.f;
            }
            else
            {
                if (avg < 12.f)
                    goodTime += 1.f;
                else
                    goodTime = 0.f;
                if (goodTime >= 2.f && tickMs > 25.f)
                {
                    tickMs = 25.f;
                    CoopNet::GameClock::SetTickMs(tickMs);
                    Net_BroadcastTickRateChange(static_cast<uint16_t>(tickMs));
                }
            }
        }

        if (Net_GetConnections().empty())
        {
            if (++idleTicks > 300)
                running = false;
        }
        else
        {
            idleTicks = 0;
        }
    }

    taskGraph.Stop();
    CoopNet::PluginManager_Shutdown();
    CoopNet::SaveSessionState(sessionId);
    CoopNet::SaveWorldState({lastSunDeg, lastWeather, particleSeed});
    if (hbSent)
        CoopNet::Heartbeat_Disconnect(sessionId);
    CoopNet::AdminController_Stop();
    CoopNet::InfoServer_Stop();
    CoopNet::WebDash_Stop();
    Net_Shutdown();
    return 0;
}
