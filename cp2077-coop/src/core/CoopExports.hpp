#pragma once

#include <RED4ext/RED4ext.hpp>
#include <cstdint>
#include <string>

namespace CoopNet {

// Export macros for compatibility
#define COOP_EXPORT __declspec(dllexport)

// Forward declarations
class NetworkManager;
class PlayerManager;
class SessionManager;

// Core API functions for RED4ext integration
extern "C" {
    COOP_EXPORT bool InitializeCoopSystem();
    COOP_EXPORT void ShutdownCoopSystem();
    COOP_EXPORT bool IsCoopSystemActive();

    // Network functions
    COOP_EXPORT uint32_t Net_GetConnectedPlayerCount();
    COOP_EXPORT uint32_t Net_GetLocalPeerId();
    COOP_EXPORT bool Net_IsHost();
    COOP_EXPORT bool Net_IsConnected();

    // Session functions
    COOP_EXPORT bool Session_Create(const char* sessionName);
    COOP_EXPORT bool Session_Join(const char* sessionId);
    COOP_EXPORT void Session_Leave();
    COOP_EXPORT const char* Session_GetId();
}

// C++ API
namespace API {
    bool Initialize();
    void Shutdown();
    bool IsActive();

    NetworkManager* GetNetworkManager();
    PlayerManager* GetPlayerManager();
    SessionManager* GetSessionManager();
}

} // namespace CoopNet