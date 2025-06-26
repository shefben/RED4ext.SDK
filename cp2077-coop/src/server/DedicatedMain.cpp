#include "../core/GameClock.hpp"
#include "../core/SessionState.hpp"
#include "../net/Net.hpp"
#include "BreachController.hpp"
#include "ElevatorController.hpp"
#include "NpcController.hpp"
#include "VehicleController.hpp"
#include "AdminController.hpp"
#include "Heartbeat.hpp"
#include "WebDash.hpp"
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

    Net_Init();
    CoopNet::WebDash_Start();
    std::cout << "Dedicated up" << std::endl;
    uint32_t sessionId = static_cast<uint32_t>(std::time(nullptr));

    // Main server loop
    bool running = true;
    uint32_t idleTicks = 0;
    float frameAccum = 0.f;
    int frameCount = 0;
    float goodTime = 0.f;
    float hbTimer = 0.f;
    float tickMs = CoopNet::GameClock::GetTickMs();
    auto last = std::chrono::steady_clock::now();

    while (running)
    {
        auto begin = std::chrono::steady_clock::now();
        CoopNet::GameClock::Tick(tickMs);
        CoopNet::ElevatorController_ServerTick(tickMs);
        if (!CoopNet::ElevatorController_IsPaused())
        {
            CoopNet::NpcController_ServerTick(tickMs);
            CoopNet::VehicleController_ServerTick(tickMs);
            CoopNet::BreachController_ServerTick(tickMs);
        }
        Net_Poll(static_cast<uint32_t>(tickMs));
        CoopNet::AdminController_PollConsole();
        hbTimer += tickMs / 1000.f;
        if (hbTimer >= 30.f)
        {
            hbTimer = 0.f;
            CoopNet::Heartbeat_Send("{\"id\":1}");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(tickMs)));

        auto end = std::chrono::steady_clock::now();
        float frameMs = std::chrono::duration<float, std::milli>(end - begin).count();
        frameAccum += frameMs;
        frameCount++;
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

    CoopNet::SaveSessionState(sessionId);
    CoopNet::WebDash_Stop();
    Net_Shutdown();
    return 0;
}
