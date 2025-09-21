// Network and Player Synchronization Test Suite
// Validates core multiplayer functionality

#include "../net/Net.hpp"
#include "../net/Connection.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>

class NetworkTestSuite {
public:
    bool RunAllTests() {
        std::cout << "=== CP2077-Coop Network Test Suite ===" << std::endl;

        bool allPassed = true;

        allPassed &= TestNetworkInitialization();
        allPassed &= TestServerStartStop();
        allPassed &= TestPlayerConnectionManagement();
        allPassed &= TestPlayerSynchronization();
        allPassed &= TestConnectionStateTracking();
        allPassed &= TestChatBroadcast();
        allPassed &= TestPlayerKickBan();

        if (allPassed) {
            std::cout << "\n✅ All network tests PASSED!" << std::endl;
        } else {
            std::cout << "\n❌ Some network tests FAILED!" << std::endl;
        }

        return allPassed;
    }

private:
    bool TestNetworkInitialization() {
        std::cout << "\n--- Test: Network Initialization ---" << std::endl;

        // Test network init/shutdown
        Net_Init();
        std::cout << "✓ Net_Init completed" << std::endl;

        Net_Shutdown();
        std::cout << "✓ Net_Shutdown completed" << std::endl;

        std::cout << "✅ Network initialization test PASSED" << std::endl;
        return true;
    }

    bool TestServerStartStop() {
        std::cout << "\n--- Test: Server Start/Stop ---" << std::endl;

        // Initialize network first
        Net_Init();

        // Test server start
        bool startResult = Net_StartServer(27015, 8);
        if (!startResult) {
            std::cout << "❌ Failed to start server" << std::endl;
            return false;
        }
        std::cout << "✓ Server started on port 27015" << std::endl;

        // Test server info
        auto serverInfo = Net_GetServerInfo();
        std::cout << "✓ Server info: " << serverInfo.name
                  << " (" << serverInfo.playerCount << "/" << serverInfo.maxPlayers << ")" << std::endl;

        // Test server stop
        Net_StopServer();
        std::cout << "✓ Server stopped" << std::endl;

        Net_Shutdown();

        std::cout << "✅ Server start/stop test PASSED" << std::endl;
        return true;
    }

    bool TestPlayerConnectionManagement() {
        std::cout << "\n--- Test: Player Connection Management ---" << std::endl;

        Net_Init();
        Net_StartServer(27016, 4);

        // Test getting empty connections
        auto connections = Net_GetConnections();
        assert(connections.empty());
        std::cout << "✓ Initial connections list is empty" << std::endl;

        // Test peer ID generation
        uint32_t peerId = Net_GetPeerId();
        assert(peerId > 0);
        std::cout << "✓ Generated peer ID: " << peerId << std::endl;

        // Test connection finding (should return null for non-existent)
        CoopNet::Connection* conn = Net_FindConnection(999);
        assert(conn == nullptr);
        std::cout << "✓ Non-existent connection correctly returns null" << std::endl;

        Net_StopServer();
        Net_Shutdown();

        std::cout << "✅ Player connection management test PASSED" << std::endl;
        return true;
    }

    bool TestPlayerSynchronization() {
        std::cout << "\n--- Test: Player Synchronization ---" << std::endl;

        Net_Init();
        Net_StartServer(27017, 4);

        // Test player spawn broadcast
        CoopNet::TransformSnap spawnSnap;
        spawnSnap.pos = {100.0f, 200.0f, 10.0f};
        spawnSnap.vel = {0.0f, 0.0f, 0.0f};
        spawnSnap.rot = {0.0f, 0.0f, 0.0f, 1.0f};
        spawnSnap.health = 100;
        spawnSnap.armor = 50;
        spawnSnap.ownerId = 1;
        spawnSnap.seq = 1;

        Net_BroadcastAvatarSpawn(1, spawnSnap);
        std::cout << "✓ Avatar spawn broadcast completed" << std::endl;

        // Test player position tracking
        RED4ext::Vector3 testPos = {150.0f, 250.0f, 15.0f};
        Net_SetConnectionAvatarPos(1, testPos);

        RED4ext::Vector3 retrievedPos = Net_GetConnectionAvatarPos(1);
        // Note: Since connection doesn't exist, this will return default (0,0,0)
        std::cout << "✓ Avatar position set/get completed" << std::endl;

        // Test player update broadcast
        RED4ext::Vector3 newPos = {200.0f, 300.0f, 20.0f};
        RED4ext::Vector3 velocity = {5.0f, 0.0f, 0.0f};
        RED4ext::Quaternion rotation = {0.0f, 0.0f, 0.707f, 0.707f};

        Net_BroadcastPlayerUpdate(1, newPos, velocity, rotation, 90, 25);
        std::cout << "✓ Player update broadcast completed" << std::endl;

        // Test player join/leave
        Net_HandlePlayerJoin(1, "TestPlayer");
        std::cout << "✓ Player join handling completed" << std::endl;

        Net_HandlePlayerLeave(1, "Test disconnect");
        std::cout << "✓ Player leave handling completed" << std::endl;

        // Test avatar despawn
        Net_BroadcastAvatarDespawn(1);
        std::cout << "✓ Avatar despawn broadcast completed" << std::endl;

        Net_StopServer();
        Net_Shutdown();

        std::cout << "✅ Player synchronization test PASSED" << std::endl;
        return true;
    }

    bool TestConnectionStateTracking() {
        std::cout << "\n--- Test: Connection State Tracking ---" << std::endl;

        // Test connection state enum values
        assert(static_cast<int>(CoopNet::ConnectionState::Disconnected) == 0);
        assert(static_cast<int>(CoopNet::ConnectionState::Handshaking) == 1);
        assert(static_cast<int>(CoopNet::ConnectionState::Connected) == 2);
        assert(static_cast<int>(CoopNet::ConnectionState::Lobby) == 3);
        assert(static_cast<int>(CoopNet::ConnectionState::InGame) == 4);
        assert(static_cast<int>(CoopNet::ConnectionState::Disconnecting) == 5);

        std::cout << "✓ Connection state enum values correct" << std::endl;

        // Test connection object creation
        CoopNet::Connection testConn;
        assert(testConn.GetState() == CoopNet::ConnectionState::Disconnected);
        assert(testConn.peerId == 0);
        assert(testConn.peer == nullptr);

        std::cout << "✓ Connection object initialization correct" << std::endl;

        std::cout << "✅ Connection state tracking test PASSED" << std::endl;
        return true;
    }

    bool TestChatBroadcast() {
        std::cout << "\n--- Test: Chat Broadcast ---" << std::endl;

        Net_Init();
        Net_StartServer(27018, 4);

        // Test chat message broadcast
        std::string testMessage = "Hello multiplayer world!";
        Net_BroadcastChat(testMessage);
        std::cout << "✓ Chat broadcast completed" << std::endl;

        // Test killfeed broadcast
        std::string killfeedMsg = "Player1 eliminated Player2";
        Net_BroadcastKillfeed(killfeedMsg);
        std::cout << "✓ Killfeed broadcast completed" << std::endl;

        // Test chat message wrapper
        Net_BroadcastChatMessage("Server announcement");
        std::cout << "✓ Chat message wrapper completed" << std::endl;

        Net_StopServer();
        Net_Shutdown();

        std::cout << "✅ Chat broadcast test PASSED" << std::endl;
        return true;
    }

    bool TestPlayerKickBan() {
        std::cout << "\n--- Test: Player Kick/Ban System ---" << std::endl;

        Net_Init();
        Net_StartServer(27019, 4);

        // Test ban check for non-banned player
        bool isBanned = Net_IsPlayerBanned(123);
        assert(!isBanned);
        std::cout << "✓ Non-banned player check correct" << std::endl;

        // Test banning a player
        Net_BanPlayer(123, "Test ban");
        std::cout << "✓ Player ban completed" << std::endl;

        // Test ban check for banned player
        isBanned = Net_IsPlayerBanned(123);
        assert(isBanned);
        std::cout << "✓ Banned player check correct" << std::endl;

        // Test kicking a player (won't actually kick since connection doesn't exist)
        Net_KickPlayer(456, "Test kick");
        std::cout << "✓ Player kick attempt completed" << std::endl;

        // Test password functionality
        Net_SetServerPassword("testpass123");
        std::cout << "✓ Server password set" << std::endl;

        Net_SetServerPassword("");
        std::cout << "✓ Server password removed" << std::endl;

        Net_StopServer();
        Net_Shutdown();

        std::cout << "✅ Player kick/ban test PASSED" << std::endl;
        return true;
    }
};

// Test runner function
extern "C" int RunNetworkTests() {
    NetworkTestSuite testSuite;
    bool success = testSuite.RunAllTests();
    return success ? 0 : 1;
}

// Standalone test executable
#ifdef NETWORK_TEST_STANDALONE
int main() {
    return RunNetworkTests();
}
#endif