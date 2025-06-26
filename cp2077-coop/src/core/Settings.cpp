#include "Settings.hpp"
#include "SaveFork.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace CoopNet {
void SaveSettings(const std::string& json)
{
    try {
        EnsureCoopSaveDirs();
        const std::filesystem::path file = std::filesystem::path(kCoopSavePath) / "settings.json";
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "Failed to open settings file" << std::endl;
            return;
        }
        out << json;
    } catch (const std::exception& e) {
        std::cerr << "SaveSettings error: " << e.what() << std::endl;
    }
}
}
