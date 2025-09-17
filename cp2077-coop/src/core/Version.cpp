#include "Version.hpp"
#include "VersionGenerated.hpp"
#include "Hash.hpp"
#include <sstream>
#include <ctime>
#include <iomanip>

namespace CoopNet
{
    // Use generated constants from VersionGenerated.hpp
    
    // Generate CRC from critical version components
    uint32_t Version::GenerateCRC(uint32_t major, uint32_t minor, uint32_t patch, const std::string& gitHash)
    {
        std::stringstream ss;
        ss << major << "." << minor << "." << patch << "-" << gitHash;
        std::string versionStr = ss.str();
        return Fnv1a32(versionStr.c_str());
    }
    
    const Version& Version::Current()
    {
        static Version current = {
            VERSION_MAJOR,
            VERSION_MINOR, 
            VERSION_PATCH,
            VERSION_BUILD,
            GenerateCRC(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, std::string{GIT_HASH}),
            std::string(GIT_HASH),
            std::string(BUILD_DATE)
        };
        return current;
    }
    
    bool Version::IsCompatibleWith(const Version& other) const
    {
        // Major version must match exactly
        if (major != other.major)
            return false;
            
        // Minor version differences are acceptable for backwards compatibility
        // within reasonable range (e.g., 1.0.x can connect to 1.1.x)
        if (std::abs(static_cast<int>(minor) - static_cast<int>(other.minor)) > 1)
            return false;
            
        return true;
    }
    
    std::string Version::ToString() const
    {
        std::stringstream ss;
        ss << major << "." << minor << "." << patch << "." << build;
        if (!gitHash.empty() && gitHash != "unknown")
        {
            ss << "-" << gitHash.substr(0, 8); // Short hash
        }
        return ss.str();
    }
    
    
    uint32_t GetBuildCRC()
    {
        return VERSION_CRC;
    }
    
    bool ValidateRemoteVersion(uint32_t remoteCRC)
    {
        // For now, require exact CRC match for compatibility
        // In production, this could be more flexible based on version rules
        return remoteCRC == VERSION_CRC;
    }
}