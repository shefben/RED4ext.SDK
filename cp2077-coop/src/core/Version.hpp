#pragma once
#include <cstdint>
#include <string>

namespace CoopNet
{
    // Version information structure
    struct Version
    {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
        uint32_t build;
        uint32_t crc;
        std::string gitHash;
        std::string buildDate;
        
        // Generate CRC from version components and source files
        static uint32_t GenerateCRC(uint32_t major, uint32_t minor, uint32_t patch, const std::string& gitHash);
        
        // Get current version
        static const Version& Current();
        
        // Check compatibility between versions
        bool IsCompatibleWith(const Version& other) const;
        
        // Get version string for display
        std::string ToString() const;
    };
    
    // These are generated at build time in VersionGenerated.hpp
    
    // Get current build CRC for network validation
    uint32_t GetBuildCRC();
    
    // Validate remote version compatibility
    bool ValidateRemoteVersion(uint32_t remoteCRC);
}