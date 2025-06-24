#include "SaveFork.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>


namespace CoopNet
{
std::string GetSessionSavePath(uint32_t sessionId)
{
    std::filesystem::path dir = std::filesystem::path(kCoopSavePath) / std::to_string(sessionId);
    return dir.string();
}

void EnsureCoopSaveDirs()
{
    try {
        std::filesystem::path base(kCoopSavePath);
        if (std::filesystem::create_directories(base)) {
            std::cout << "Created save directory: " << base << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create save directory " << kCoopSavePath << ": " << e.what() << std::endl;
    }
}

void SaveSession(uint32_t sessionId, const std::string& jsonBlob)
{
    try {
        EnsureCoopSaveDirs();
        const std::filesystem::path file =
            std::filesystem::path(kCoopSavePath) / (std::to_string(sessionId) + ".json");

        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "Failed to open session file " << file << std::endl;
            return;
        }

        out << jsonBlob;
        out.close();

        if (out.good()) {
            std::cout << "Saved session to " << file << std::endl;
        } else {
            std::cerr << "Failed to write session file " << file << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving session: " << e.what() << std::endl;
    }
}
}
