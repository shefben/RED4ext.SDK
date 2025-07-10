#include "WorldStateIO.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace CoopNet {

static const char* kWorldStatePath = "server/world_state.json";

bool LoadWorldState(WorldStatePacket& out)
{
    std::ifstream in(kWorldStatePath);
    if (!in.is_open())
        return false;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (std::sscanf(json.c_str(), "{\"sun\":%hu,\"id\":%hhu,\"seed\":%hu}", &out.sunAngleDeg, &out.weatherId, &out.particleSeed) == 3)
        return true;
    return false;
}

void SaveWorldState(const WorldStatePacket& state)
{
    try {
        std::filesystem::create_directories("server");
        std::ofstream outFile(kWorldStatePath, std::ios::trunc);
        if (!outFile.is_open())
            return;
        outFile << "{\"sun\":" << state.sunAngleDeg << ",\"id\":" << static_cast<int>(state.weatherId)
                << ",\"seed\":" << state.particleSeed << "}\n";
    } catch (const std::exception& e) {
        std::cerr << "SaveWorldState error: " << e.what() << std::endl;
    }
}

} // namespace CoopNet
