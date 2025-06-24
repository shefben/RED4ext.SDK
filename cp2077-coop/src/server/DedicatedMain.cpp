#include "../net/Net.hpp"
#include "../core/GameClock.hpp"
#include "NpcController.hpp"
#include <iostream>
#include <cstring>
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
    std::cout << "Dedicated up" << std::endl;

    // Main server loop placeholder
    for (int i = 0; i < 10; ++i)
    {
        CoopNet::GameClock::Tick(CoopNet::kFixedDeltaMs);
        CoopNet::NpcController_ServerTick(CoopNet::kFixedDeltaMs);
        Net_Poll(static_cast<uint32_t>(CoopNet::kFixedDeltaMs));
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(CoopNet::kFixedDeltaMs)));
    }

    Net_Shutdown();
    return 0;
}

