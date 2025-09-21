#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <string>
#include <memory>

namespace RED4ext
{
    // Location enums matching REDscript
    enum class LocationType : uint8_t
    {
        Apartment = 0,
        Store = 1,
        Custom = 2,
        Notable = 3
    };

    enum class InstanceType : uint8_t
    {
        Apartment = 0,
        Store = 1,
        Custom = 2
    };

    enum class LocationAccessLevel : uint8_t
    {
        Public = 0,
        Friends = 1,
        Guild = 2,
        Private = 3,
        Admin = 4
    };

    enum class ApartmentEntryResult : uint8_t
    {
        Success = 0,
        NotFound = 1,
        AccessDenied = 2,
        InstanceFull = 3,
        AlreadyInside = 4
    };

    enum class StoreEntryResult : uint8_t
    {
        Success = 0,
        NotFound = 1,
        Closed = 2,
        InstanceFull = 3
    };

    enum class LocationCreationResult : uint8_t
    {
        Success = 0,
        InsufficientPermissions = 1,
        InvalidConfiguration = 2,
        DuplicateId = 3,
        StorageFull = 4
    };

    enum class InstanceEntryResult : uint8_t
    {
        Success = 0,
        Full = 1,
        AccessDenied = 2,
        InvalidInstance = 3
    };

    enum class PlayerLocationContext : uint8_t
    {
        OpenWorld = 0,
        ApartmentInstance = 1,
        CustomInstance = 2,
        StoreInstance = 3
    };

    // Data structures
    struct Vector3
    {
        float x, y, z;

        Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
        Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    };

    struct ApartmentPermissions
    {
        bool allowFriends;
        bool allowGuildMembers;
        bool allowPublic;
        std::vector<uint32_t> allowedPlayers;
        std::vector<uint32_t> blockedPlayers;

        ApartmentPermissions()
            : allowFriends(true), allowGuildMembers(false), allowPublic(false) {}
    };

    struct LocationConfig
    {
        std::string id;
        std::string name;
        std::string description;
        Vector3 entrancePosition;
        Vector3 interiorPosition;
        uint32_t maxPlayers;

        LocationConfig()
            : maxPlayers(8) {}

        LocationConfig(const std::string& id_, const std::string& name_, const std::string& desc_,
                      const Vector3& entrance_, const Vector3& interior_, uint32_t maxPlayers_)
            : id(id_), name(name_), description(desc_), entrancePosition(entrance_),
              interiorPosition(interior_), maxPlayers(maxPlayers_) {}
    };

    struct ApartmentConfig
    {
        std::string id;
        std::string name;
        std::string description;
        Vector3 entrancePosition;
        Vector3 interiorPosition;
        uint32_t maxPlayers;
        uint32_t purchaseCost;
        ApartmentPermissions defaultPermissions;

        ApartmentConfig()
            : maxPlayers(8), purchaseCost(0) {}
    };

    struct StoreConfig
    {
        std::string id;
        std::string name;
        std::string description;
        Vector3 entrancePosition;
        Vector3 interiorPosition;
        uint32_t maxPlayers;
        std::string storeType;
        std::string operatingHours;

        StoreConfig()
            : maxPlayers(10), operatingHours("24/7") {}
    };

    struct InstanceUpdateData
    {
        std::string instanceId;
        std::string locationId;
        InstanceType instanceType;
        uint32_t ownerId;
        std::vector<uint32_t> playerIds;
        std::chrono::steady_clock::time_point timestamp;

        InstanceUpdateData()
            : instanceType(InstanceType::Custom), ownerId(0),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct ApartmentPermissionData
    {
        std::string apartmentId;
        uint32_t ownerId;
        ApartmentPermissions permissions;
        std::chrono::steady_clock::time_point timestamp;

        ApartmentPermissionData()
            : ownerId(0), timestamp(std::chrono::steady_clock::now()) {}
    };

    struct LocationCreationData
    {
        LocationType locationType;
        std::string locationId;
        std::string config; // Serialized config
        uint32_t creatorId;
        std::chrono::steady_clock::time_point timestamp;

        LocationCreationData()
            : locationType(LocationType::Custom), creatorId(0),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct PlayerInstanceTeleport
    {
        uint32_t playerId;
        std::string instanceId;
        Vector3 spawnPoint;
        bool isEntering;
        std::chrono::steady_clock::time_point timestamp;

        PlayerInstanceTeleport()
            : playerId(0), isEntering(true),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct InstancePlayerNotification
    {
        std::string instanceId;
        uint32_t playerId;
        bool isJoining;
        std::chrono::steady_clock::time_point timestamp;

        InstancePlayerNotification()
            : playerId(0), isJoining(true),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    // Location classes
    struct CustomApartment
    {
        std::string id;
        std::string name;
        std::string description;
        Vector3 entrancePosition;
        Vector3 interiorPosition;
        uint32_t maxPlayers;
        std::vector<uint32_t> owners;
        ApartmentPermissions permissions;
        uint32_t purchaseCost;
        bool isFromSingleplayer;
        std::chrono::steady_clock::time_point lastUpdate;

        CustomApartment()
            : maxPlayers(8), purchaseCost(0), isFromSingleplayer(false),
              lastUpdate(std::chrono::steady_clock::now()) {}
    };

    struct CustomStore
    {
        std::string id;
        std::string name;
        std::string description;
        Vector3 entrancePosition;
        Vector3 interiorPosition;
        uint32_t maxPlayers;
        std::string storeType;
        std::string operatingHours;
        bool isOpen;
        bool isFromSingleplayer;
        std::chrono::steady_clock::time_point lastUpdate;

        CustomStore()
            : maxPlayers(10), operatingHours("24/7"), isOpen(true),
              isFromSingleplayer(false), lastUpdate(std::chrono::steady_clock::now()) {}
    };

    struct CustomLocation
    {
        std::string id;
        std::string name;
        std::string description;
        Vector3 entrancePosition;
        Vector3 interiorPosition;
        uint32_t maxPlayers;
        std::string locationType;
        bool isFromSingleplayer;
        LocationAccessLevel accessLevel;
        std::chrono::steady_clock::time_point lastUpdate;

        CustomLocation()
            : maxPlayers(15), isFromSingleplayer(false),
              accessLevel(LocationAccessLevel::Public),
              lastUpdate(std::chrono::steady_clock::now()) {}
    };

    struct InstancedSpace
    {
        std::string instanceId;
        std::string locationId;
        InstanceType instanceType;
        uint32_t ownerId; // 0 for shared spaces
        std::vector<uint32_t> players;
        uint32_t maxPlayers;
        Vector3 spawnPoint;
        Vector3 exitLocation;
        std::chrono::steady_clock::time_point creationTime;
        std::chrono::steady_clock::time_point lastActivityTime;
        bool hasStateChanged;

        InstancedSpace()
            : instanceType(InstanceType::Custom), ownerId(0), maxPlayers(8),
              creationTime(std::chrono::steady_clock::now()),
              lastActivityTime(std::chrono::steady_clock::now()),
              hasStateChanged(true) {}
    };

    struct LocationEntry
    {
        std::string id;
        LocationType locationType;
        Vector3 position;
        std::string name;
        bool isActive;

        LocationEntry()
            : locationType(LocationType::Custom), isActive(true) {}
    };

    struct PlayerLocationState
    {
        uint32_t playerId;
        std::string playerName;
        PlayerLocationContext currentContext;
        std::string currentInstanceId;
        std::string currentLocationId;
        Vector3 lastKnownPosition;
        std::chrono::steady_clock::time_point lastLocationUpdate;
        std::chrono::steady_clock::time_point lastActivity;
        bool isConnected;
        float syncPriority;

        // Statistics
        uint32_t instancesEntered;
        uint32_t apartmentsVisited;
        uint32_t storesVisited;
        uint32_t teleportCount;

        PlayerLocationState()
            : playerId(0), currentContext(PlayerLocationContext::OpenWorld),
              lastLocationUpdate(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()),
              isConnected(false), syncPriority(1.0f),
              instancesEntered(0), apartmentsVisited(0),
              storesVisited(0), teleportCount(0) {}
    };

    // Main location management class
    class LocationManager
    {
    public:
        static LocationManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Player management
        void AddPlayer(uint32_t playerId, const std::string& playerName);
        void RemovePlayer(uint32_t playerId);
        void UpdatePlayerActivity(uint32_t playerId);

        // Location management
        bool RegisterCustomApartment(const ApartmentConfig& config);
        bool RegisterCustomStore(const StoreConfig& config);
        bool RegisterCustomLocation(const LocationConfig& config);
        void ScanExistingLocations();

        // Apartment management
        ApartmentEntryResult RequestApartmentEntry(uint32_t playerId, const std::string& apartmentId, uint32_t ownerId);
        bool ExitApartment(uint32_t playerId);
        bool SetApartmentPermissions(uint32_t ownerId, const std::string& apartmentId, const ApartmentPermissions& permissions);
        bool AddApartmentOwner(const std::string& apartmentId, uint32_t playerId);
        bool RemoveApartmentOwner(const std::string& apartmentId, uint32_t playerId);

        // Store management
        StoreEntryResult RequestStoreEntry(uint32_t playerId, const std::string& storeId);
        bool ExitStore(uint32_t playerId);

        // Instance management
        std::string CreateInstancedSpace(const std::string& locationId, InstanceType type, uint32_t ownerId = 0);
        bool AddPlayerToInstance(uint32_t playerId, const std::string& instanceId);
        bool RemovePlayerFromInstance(uint32_t playerId, const std::string& instanceId);
        void CleanupEmptyInstances();

        // Admin functions
        LocationCreationResult CreateCustomApartment(uint32_t adminId, const ApartmentConfig& config);
        LocationCreationResult CreateCustomStore(uint32_t adminId, const StoreConfig& config);
        LocationCreationResult CreateCustomLocation(uint32_t adminId, const LocationConfig& config);

        // Query methods
        PlayerLocationState* GetPlayerLocationState(uint32_t playerId);
        const PlayerLocationState* GetPlayerLocationState(uint32_t playerId) const;
        CustomApartment* FindApartment(const std::string& apartmentId);
        CustomStore* FindStore(const std::string& storeId);
        CustomLocation* FindCustomLocation(const std::string& locationId);
        InstancedSpace* FindInstancedSpace(const std::string& instanceId);
        PlayerLocationContext GetPlayerLocationContext(uint32_t playerId) const;
        std::vector<LocationEntry> GetNearbyLocations(const Vector3& position, float radius) const;
        std::vector<uint32_t> GetPlayersInInstance(const std::string& instanceId) const;
        std::vector<std::string> GetPlayerOwnedApartments(uint32_t playerId) const;

        // Validation methods
        bool ValidateApartmentConfig(const ApartmentConfig& config) const;
        bool ValidateStoreConfig(const StoreConfig& config) const;
        bool ValidateLocationConfig(const LocationConfig& config) const;
        bool CanPlayerEnterApartment(uint32_t playerId, const std::string& apartmentId, uint32_t ownerId) const;
        bool IsPlayerAdmin(uint32_t playerId) const;

        // Synchronization
        void BroadcastInstanceUpdate(const InstanceUpdateData& instanceData);
        void BroadcastApartmentPermissions(const ApartmentPermissionData& permissionData);
        void BroadcastLocationCreation(const LocationCreationData& creationData);
        void BroadcastPlayerTeleport(const PlayerInstanceTeleport& teleportData);
        void BroadcastInstanceNotification(const InstancePlayerNotification& notificationData);
        void SynchronizePlayerLocation(uint32_t playerId);
        void ForceSyncPlayer(uint32_t playerId);
        void SetSyncPriority(uint32_t playerId, float priority);

        // Statistics and monitoring
        uint32_t GetActivePlayerCount() const;
        uint32_t GetTotalLocationsCount() const;
        uint32_t GetActiveInstancesCount() const;
        uint32_t GetApartmentCount() const;
        uint32_t GetStoreCount() const;
        std::unordered_map<LocationType, uint32_t> GetLocationTypeStats() const;
        std::unordered_map<InstanceType, uint32_t> GetInstanceTypeStats() const;

        // Event callbacks
        using LocationEntryCallback = std::function<void(uint32_t playerId, const std::string& locationId, bool isEntering)>;
        using ApartmentAccessCallback = std::function<void(uint32_t playerId, const std::string& apartmentId, ApartmentEntryResult result)>;
        using InstanceCreatedCallback = std::function<void(const std::string& instanceId, InstanceType type, uint32_t ownerId)>;
        using LocationCreatedCallback = std::function<void(const LocationCreationData& creationData)>;
        using PermissionChangedCallback = std::function<void(const std::string& apartmentId, uint32_t ownerId)>;

        void SetLocationEntryCallback(LocationEntryCallback callback);
        void SetApartmentAccessCallback(ApartmentAccessCallback callback);
        void SetInstanceCreatedCallback(InstanceCreatedCallback callback);
        void SetLocationCreatedCallback(LocationCreatedCallback callback);
        void SetPermissionChangedCallback(PermissionChangedCallback callback);

    private:
        LocationManager() = default;
        ~LocationManager() = default;
        LocationManager(const LocationManager&) = delete;
        LocationManager& operator=(const LocationManager&) = delete;

        // Internal data
        std::unordered_map<uint32_t, std::unique_ptr<PlayerLocationState>> m_playerStates;
        std::unordered_map<std::string, std::unique_ptr<CustomApartment>> m_apartments;
        std::unordered_map<std::string, std::unique_ptr<CustomStore>> m_stores;
        std::unordered_map<std::string, std::unique_ptr<CustomLocation>> m_customLocations;
        std::unordered_map<std::string, std::unique_ptr<InstancedSpace>> m_instancedSpaces;
        std::vector<LocationEntry> m_locationDatabase;
        std::unordered_map<uint32_t, std::vector<std::string>> m_playerToInstances;
        std::unordered_map<LocationType, std::vector<std::string>> m_locationsByType;

        // Thread safety
        mutable std::shared_mutex m_statesMutex;
        mutable std::mutex m_callbacksMutex;
        mutable std::shared_mutex m_instancesMutex;

        // Update timing
        std::chrono::steady_clock::time_point m_lastUpdate;
        std::chrono::steady_clock::time_point m_lastCleanup;
        float m_updateInterval;

        // Statistics
        uint32_t m_totalInstancesCreated;
        uint32_t m_totalTeleports;
        uint32_t m_totalLocationsRegistered;
        uint32_t m_totalPermissionChanges;

        // Event callbacks
        LocationEntryCallback m_locationEntryCallback;
        ApartmentAccessCallback m_apartmentAccessCallback;
        InstanceCreatedCallback m_instanceCreatedCallback;
        LocationCreatedCallback m_locationCreatedCallback;
        PermissionChangedCallback m_permissionChangedCallback;

        // Internal methods
        std::string GenerateInstanceId();
        void UpdateLocationStates(float deltaTime);
        void UpdateInstancedSpaces(float deltaTime);
        void ProcessLocationExpirations();
        void ValidateLocationStates();

        InstancedSpace* FindOrCreateApartmentInstance(const std::string& apartmentId, uint32_t ownerId);
        InstancedSpace* FindOrCreateStoreInstance(const std::string& storeId);
        InstancedSpace* FindPlayerCurrentInstance(uint32_t playerId);
        void TeleportPlayerToInstance(uint32_t playerId, const std::string& instanceId, const Vector3& spawnPoint);
        void TeleportPlayerToWorld(uint32_t playerId, const Vector3& exitLocation);

        bool CheckApartmentPermissions(uint32_t playerId, uint32_t ownerId, const ApartmentPermissions& permissions) const;
        bool ArePlayersFriends(uint32_t playerId1, uint32_t playerId2) const;
        bool ArePlayersInSameGuild(uint32_t playerId1, uint32_t playerId2) const;
        float CalculateDistance(const Vector3& pos1, const Vector3& pos2) const;

        void UpdatePlayerToInstanceMapping(uint32_t playerId, const std::string& instanceId, bool isActive);
        void UpdateLocationTypeMapping(const std::string& locationId, LocationType type, bool isActive);
        void RemovePlayerFromAllMappings(uint32_t playerId);

        void LoadLocationConfigurations();
        bool SaveLocationConfiguration(const LocationConfig& config, const std::string& configPath);
        std::string SerializeConfig(const LocationConfig& config) const;
        LocationConfig DeserializeConfig(const std::string& configData) const;

        void NotifyLocationEntry(uint32_t playerId, const std::string& locationId, bool isEntering);
        void NotifyApartmentAccess(uint32_t playerId, const std::string& apartmentId, ApartmentEntryResult result);
        void NotifyInstanceCreated(const std::string& instanceId, InstanceType type, uint32_t ownerId);
        void NotifyLocationCreated(const LocationCreationData& creationData);
        void NotifyPermissionChanged(const std::string& apartmentId, uint32_t ownerId);

        void SendInstanceUpdateToClients(const InstanceUpdateData& instanceData);
        void SendApartmentPermissionsToClients(const ApartmentPermissionData& permissionData);
        void SendLocationCreationToClients(const LocationCreationData& creationData);
        void SendPlayerTeleportToClients(const PlayerInstanceTeleport& teleportData);
        void SendInstanceNotificationToClients(const InstancePlayerNotification& notificationData);
    };

    // Utility functions for location management
    namespace LocationUtils
    {
        std::string LocationTypeToString(LocationType type);
        LocationType StringToLocationType(const std::string& typeStr);

        std::string InstanceTypeToString(InstanceType type);
        InstanceType StringToInstanceType(const std::string& typeStr);

        std::string PlayerLocationContextToString(PlayerLocationContext context);
        std::string ApartmentEntryResultToString(ApartmentEntryResult result);
        std::string StoreEntryResultToString(StoreEntryResult result);
        std::string LocationCreationResultToString(LocationCreationResult result);

        bool IsValidPosition(const Vector3& position);
        bool IsValidLocationId(const std::string& locationId);
        bool IsValidInstanceId(const std::string& instanceId);
        float CalculateDistance(const Vector3& pos1, const Vector3& pos2);
        Vector3 CalculateDirection(const Vector3& from, const Vector3& to);

        uint32_t HashLocationState(const PlayerLocationState& state);
        bool AreLocationStatesEquivalent(const InstanceUpdateData& data1, const InstanceUpdateData& data2, float tolerance = 0.1f);
        bool ShouldSyncLocationState(const PlayerLocationState& oldState, const PlayerLocationState& newState);
    }

    // Network message structures for client-server communication
    struct LocationStateUpdate
    {
        uint32_t playerId;
        PlayerLocationContext context;
        std::string currentLocationId;
        std::string currentInstanceId;
        Vector3 position;
        std::chrono::steady_clock::time_point updateTime;
        uint32_t syncVersion;
    };

    struct ApartmentAccessUpdate
    {
        uint32_t playerId;
        std::string apartmentId;
        uint32_t ownerId;
        ApartmentEntryResult result;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct StoreAccessUpdate
    {
        uint32_t playerId;
        std::string storeId;
        StoreEntryResult result;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct InstanceManagementUpdate
    {
        std::string instanceId;
        InstanceType type;
        std::vector<uint32_t> currentPlayers;
        bool isBeingDestroyed;
        std::chrono::steady_clock::time_point updateTime;
    };
}