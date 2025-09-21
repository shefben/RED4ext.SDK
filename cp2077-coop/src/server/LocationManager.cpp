#include "LocationManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace RED4ext
{
    // LocationManager Implementation
    LocationManager& LocationManager::GetInstance()
    {
        static LocationManager instance;
        return instance;
    }

    void LocationManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Clear existing data
        m_playerStates.clear();
        m_apartments.clear();
        m_stores.clear();
        m_customLocations.clear();
        m_instancedSpaces.clear();
        m_locationDatabase.clear();
        m_playerToInstances.clear();
        m_locationsByType.clear();

        // Initialize timing
        m_lastUpdate = std::chrono::steady_clock::now();
        m_lastCleanup = m_lastUpdate;
        m_updateInterval = 1.0f; // 1 FPS location updates

        // Initialize statistics
        m_totalInstancesCreated = 0;
        m_totalTeleports = 0;
        m_totalLocationsRegistered = 0;
        m_totalPermissionChanges = 0;

        // Initialize location type mappings
        m_locationsByType[LocationType::Apartment] = {};
        m_locationsByType[LocationType::Store] = {};
        m_locationsByType[LocationType::Custom] = {};
        m_locationsByType[LocationType::Notable] = {};

        // Load existing configurations
        LoadLocationConfigurations();
    }

    void LocationManager::Shutdown()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        m_playerStates.clear();
        m_apartments.clear();
        m_stores.clear();
        m_customLocations.clear();
        m_instancedSpaces.clear();
        m_locationDatabase.clear();
        m_playerToInstances.clear();
        m_locationsByType.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_locationEntryCallback = nullptr;
        m_apartmentAccessCallback = nullptr;
        m_instanceCreatedCallback = nullptr;
        m_locationCreatedCallback = nullptr;
        m_permissionChangedCallback = nullptr;
    }

    void LocationManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - m_lastUpdate).count() / 1000.0f;

        m_lastUpdate = currentTime;

        // Update location states
        UpdateLocationStates(deltaTime);

        // Update instanced spaces
        UpdateInstancedSpaces(deltaTime);

        // Process location expirations
        ProcessLocationExpirations();

        // Validate location states
        ValidateLocationStates();

        // Periodic cleanup (every 5 minutes)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 300) {
            CleanupEmptyInstances();
            m_lastCleanup = currentTime;
        }
    }

    void LocationManager::AddPlayer(uint32_t playerId, const std::string& playerName)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerState = std::make_unique<PlayerLocationState>();
        playerState->playerId = playerId;
        playerState->playerName = playerName;
        playerState->isConnected = true;
        playerState->syncPriority = 1.0f;
        playerState->currentContext = PlayerLocationContext::OpenWorld;

        m_playerStates[playerId] = std::move(playerState);
    }

    void LocationManager::RemovePlayer(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Remove player from any instances they're in
        RemovePlayerFromAllMappings(playerId);

        // Remove player state
        m_playerStates.erase(playerId);
    }

    bool LocationManager::RegisterCustomApartment(const ApartmentConfig& config)
    {
        if (!ValidateApartmentConfig(config)) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Check for duplicate ID
        if (m_apartments.find(config.id) != m_apartments.end()) {
            return false;
        }

        // Create apartment
        auto apartment = std::make_unique<CustomApartment>();
        apartment->id = config.id;
        apartment->name = config.name;
        apartment->description = config.description;
        apartment->entrancePosition = config.entrancePosition;
        apartment->interiorPosition = config.interiorPosition;
        apartment->maxPlayers = config.maxPlayers;
        apartment->permissions = config.defaultPermissions;
        apartment->purchaseCost = config.purchaseCost;
        apartment->isFromSingleplayer = false;
        apartment->lastUpdate = std::chrono::steady_clock::now();

        m_apartments[config.id] = std::move(apartment);

        // Update mappings
        UpdateLocationTypeMapping(config.id, LocationType::Apartment, true);

        // Add to location database
        LocationEntry entry;
        entry.id = config.id;
        entry.locationType = LocationType::Apartment;
        entry.position = config.entrancePosition;
        entry.name = config.name;
        entry.isActive = true;
        m_locationDatabase.push_back(entry);

        m_totalLocationsRegistered++;

        return true;
    }

    bool LocationManager::RegisterCustomStore(const StoreConfig& config)
    {
        if (!ValidateStoreConfig(config)) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Check for duplicate ID
        if (m_stores.find(config.id) != m_stores.end()) {
            return false;
        }

        // Create store
        auto store = std::make_unique<CustomStore>();
        store->id = config.id;
        store->name = config.name;
        store->description = config.description;
        store->entrancePosition = config.entrancePosition;
        store->interiorPosition = config.interiorPosition;
        store->maxPlayers = config.maxPlayers;
        store->storeType = config.storeType;
        store->operatingHours = config.operatingHours;
        store->isOpen = true;
        store->isFromSingleplayer = false;
        store->lastUpdate = std::chrono::steady_clock::now();

        m_stores[config.id] = std::move(store);

        // Update mappings
        UpdateLocationTypeMapping(config.id, LocationType::Store, true);

        // Add to location database
        LocationEntry entry;
        entry.id = config.id;
        entry.locationType = LocationType::Store;
        entry.position = config.entrancePosition;
        entry.name = config.name;
        entry.isActive = true;
        m_locationDatabase.push_back(entry);

        m_totalLocationsRegistered++;

        return true;
    }

    ApartmentEntryResult LocationManager::RequestApartmentEntry(uint32_t playerId, const std::string& apartmentId, uint32_t ownerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto apartmentIt = m_apartments.find(apartmentId);
        if (apartmentIt == m_apartments.end()) {
            return ApartmentEntryResult::NotFound;
        }

        auto& apartment = apartmentIt->second;

        // Check permissions
        if (!CanPlayerEnterApartment(playerId, apartmentId, ownerId)) {
            return ApartmentEntryResult::AccessDenied;
        }

        lock.unlock();

        // Create or find apartment instance for the owner
        auto* instance = FindOrCreateApartmentInstance(apartmentId, ownerId);
        if (!instance) {
            return ApartmentEntryResult::NotFound;
        }

        // Check if instance is full
        if (instance->players.size() >= instance->maxPlayers) {
            return ApartmentEntryResult::InstanceFull;
        }

        // Check if player is already in instance
        auto playerIt = std::find(instance->players.begin(), instance->players.end(), playerId);
        if (playerIt != instance->players.end()) {
            return ApartmentEntryResult::AlreadyInside;
        }

        // Add player to instance
        instance->players.push_back(playerId);
        instance->lastActivityTime = std::chrono::steady_clock::now();
        instance->hasStateChanged = true;

        // Update player state
        auto* playerState = GetPlayerLocationState(playerId);
        if (playerState) {
            playerState->currentContext = PlayerLocationContext::ApartmentInstance;
            playerState->currentInstanceId = instance->instanceId;
            playerState->currentLocationId = apartmentId;
            playerState->lastLocationUpdate = std::chrono::steady_clock::now();
            playerState->apartmentsVisited++;
        }

        // Update mappings
        UpdatePlayerToInstanceMapping(playerId, instance->instanceId, true);

        // Teleport player to instance
        TeleportPlayerToInstance(playerId, instance->instanceId, instance->spawnPoint);

        // Notify listeners
        NotifyLocationEntry(playerId, apartmentId, true);
        NotifyApartmentAccess(playerId, apartmentId, ApartmentEntryResult::Success);

        // Broadcast instance update
        InstanceUpdateData updateData;
        updateData.instanceId = instance->instanceId;
        updateData.locationId = apartmentId;
        updateData.instanceType = InstanceType::Apartment;
        updateData.ownerId = ownerId;
        updateData.playerIds = instance->players;
        updateData.timestamp = std::chrono::steady_clock::now();

        BroadcastInstanceUpdate(updateData);

        return ApartmentEntryResult::Success;
    }

    bool LocationManager::ExitApartment(uint32_t playerId)
    {
        auto* instance = FindPlayerCurrentInstance(playerId);
        if (!instance || instance->instanceType != InstanceType::Apartment) {
            return false;
        }

        // Remove player from instance
        auto playerIt = std::find(instance->players.begin(), instance->players.end(), playerId);
        if (playerIt != instance->players.end()) {
            instance->players.erase(playerIt);
            instance->hasStateChanged = true;

            if (!instance->players.empty()) {
                instance->lastActivityTime = std::chrono::steady_clock::now();
            }
        }

        // Update player state
        auto* playerState = GetPlayerLocationState(playerId);
        if (playerState) {
            playerState->currentContext = PlayerLocationContext::OpenWorld;
            playerState->currentInstanceId = "";
            playerState->currentLocationId = "";
            playerState->lastLocationUpdate = std::chrono::steady_clock::now();
        }

        // Update mappings
        UpdatePlayerToInstanceMapping(playerId, instance->instanceId, false);

        // Teleport player back to world
        TeleportPlayerToWorld(playerId, instance->exitLocation);

        // Notify listeners
        NotifyLocationEntry(playerId, instance->locationId, false);

        // Broadcast instance update
        InstanceUpdateData updateData;
        updateData.instanceId = instance->instanceId;
        updateData.locationId = instance->locationId;
        updateData.instanceType = instance->instanceType;
        updateData.ownerId = instance->ownerId;
        updateData.playerIds = instance->players;
        updateData.timestamp = std::chrono::steady_clock::now();

        BroadcastInstanceUpdate(updateData);

        return true;
    }

    bool LocationManager::SetApartmentPermissions(uint32_t ownerId, const std::string& apartmentId, const ApartmentPermissions& permissions)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto apartmentIt = m_apartments.find(apartmentId);
        if (apartmentIt == m_apartments.end()) {
            return false;
        }

        auto& apartment = apartmentIt->second;

        // Check if player is an owner
        auto ownerIt = std::find(apartment->owners.begin(), apartment->owners.end(), ownerId);
        if (ownerIt == apartment->owners.end()) {
            return false;
        }

        // Update permissions
        apartment->permissions = permissions;
        apartment->lastUpdate = std::chrono::steady_clock::now();
        m_totalPermissionChanges++;

        lock.unlock();

        // Notify listeners
        NotifyPermissionChanged(apartmentId, ownerId);

        // Broadcast permission update
        ApartmentPermissionData permissionData;
        permissionData.apartmentId = apartmentId;
        permissionData.ownerId = ownerId;
        permissionData.permissions = permissions;
        permissionData.timestamp = std::chrono::steady_clock::now();

        BroadcastApartmentPermissions(permissionData);

        return true;
    }

    StoreEntryResult LocationManager::RequestStoreEntry(uint32_t playerId, const std::string& storeId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto storeIt = m_stores.find(storeId);
        if (storeIt == m_stores.end()) {
            return StoreEntryResult::NotFound;
        }

        auto& store = storeIt->second;

        if (!store->isOpen) {
            return StoreEntryResult::Closed;
        }

        lock.unlock();

        // Create or find store instance
        auto* instance = FindOrCreateStoreInstance(storeId);
        if (!instance) {
            return StoreEntryResult::NotFound;
        }

        // Check if instance is full
        if (instance->players.size() >= instance->maxPlayers) {
            return StoreEntryResult::InstanceFull;
        }

        // Add player to instance
        instance->players.push_back(playerId);
        instance->lastActivityTime = std::chrono::steady_clock::now();
        instance->hasStateChanged = true;

        // Update player state
        auto* playerState = GetPlayerLocationState(playerId);
        if (playerState) {
            playerState->currentContext = PlayerLocationContext::StoreInstance;
            playerState->currentInstanceId = instance->instanceId;
            playerState->currentLocationId = storeId;
            playerState->lastLocationUpdate = std::chrono::steady_clock::now();
            playerState->storesVisited++;
        }

        // Update mappings
        UpdatePlayerToInstanceMapping(playerId, instance->instanceId, true);

        // Teleport player to instance
        TeleportPlayerToInstance(playerId, instance->instanceId, instance->spawnPoint);

        // Notify listeners
        NotifyLocationEntry(playerId, storeId, true);

        // Broadcast instance update
        InstanceUpdateData updateData;
        updateData.instanceId = instance->instanceId;
        updateData.locationId = storeId;
        updateData.instanceType = InstanceType::Store;
        updateData.ownerId = 0; // Stores have no owner
        updateData.playerIds = instance->players;
        updateData.timestamp = std::chrono::steady_clock::now();

        BroadcastInstanceUpdate(updateData);

        return StoreEntryResult::Success;
    }

    LocationCreationResult LocationManager::CreateCustomApartment(uint32_t adminId, const ApartmentConfig& config)
    {
        if (!IsPlayerAdmin(adminId)) {
            return LocationCreationResult::InsufficientPermissions;
        }

        if (!ValidateApartmentConfig(config)) {
            return LocationCreationResult::InvalidConfiguration;
        }

        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        if (m_apartments.find(config.id) != m_apartments.end()) {
            return LocationCreationResult::DuplicateId;
        }

        lock.unlock();

        // Register the apartment
        if (!RegisterCustomApartment(config)) {
            return LocationCreationResult::InvalidConfiguration;
        }

        // Save configuration
        SaveLocationConfiguration(LocationConfig{
            config.id, config.name, config.description,
            config.entrancePosition, config.interiorPosition,
            config.maxPlayers
        }, "config/apartments/");

        // Broadcast creation
        LocationCreationData creationData;
        creationData.locationType = LocationType::Apartment;
        creationData.locationId = config.id;
        creationData.config = SerializeConfig(LocationConfig{
            config.id, config.name, config.description,
            config.entrancePosition, config.interiorPosition,
            config.maxPlayers
        });
        creationData.creatorId = adminId;
        creationData.timestamp = std::chrono::steady_clock::now();

        BroadcastLocationCreation(creationData);
        NotifyLocationCreated(creationData);

        return LocationCreationResult::Success;
    }

    LocationCreationResult LocationManager::CreateCustomStore(uint32_t adminId, const StoreConfig& config)
    {
        if (!IsPlayerAdmin(adminId)) {
            return LocationCreationResult::InsufficientPermissions;
        }

        if (!ValidateStoreConfig(config)) {
            return LocationCreationResult::InvalidConfiguration;
        }

        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        if (m_stores.find(config.id) != m_stores.end()) {
            return LocationCreationResult::DuplicateId;
        }

        lock.unlock();

        // Register the store
        if (!RegisterCustomStore(config)) {
            return LocationCreationResult::InvalidConfiguration;
        }

        // Save configuration
        SaveLocationConfiguration(LocationConfig{
            config.id, config.name, config.description,
            config.entrancePosition, config.interiorPosition,
            config.maxPlayers
        }, "config/stores/");

        // Broadcast creation
        LocationCreationData creationData;
        creationData.locationType = LocationType::Store;
        creationData.locationId = config.id;
        creationData.config = SerializeConfig(LocationConfig{
            config.id, config.name, config.description,
            config.entrancePosition, config.interiorPosition,
            config.maxPlayers
        });
        creationData.creatorId = adminId;
        creationData.timestamp = std::chrono::steady_clock::now();

        BroadcastLocationCreation(creationData);
        NotifyLocationCreated(creationData);

        return LocationCreationResult::Success;
    }

    std::string LocationManager::CreateInstancedSpace(const std::string& locationId, InstanceType type, uint32_t ownerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_instancesMutex);

        std::string instanceId = GenerateInstanceId();

        auto instance = std::make_unique<InstancedSpace>();
        instance->instanceId = instanceId;
        instance->locationId = locationId;
        instance->instanceType = type;
        instance->ownerId = ownerId;
        instance->maxPlayers = 8; // Default
        instance->creationTime = std::chrono::steady_clock::now();
        instance->lastActivityTime = instance->creationTime;
        instance->hasStateChanged = true;

        m_instancedSpaces[instanceId] = std::move(instance);
        m_totalInstancesCreated++;

        lock.unlock();

        // Notify listeners
        NotifyInstanceCreated(instanceId, type, ownerId);

        return instanceId;
    }

    PlayerLocationState* LocationManager::GetPlayerLocationState(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    const PlayerLocationState* LocationManager::GetPlayerLocationState(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    CustomApartment* LocationManager::FindApartment(const std::string& apartmentId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_apartments.find(apartmentId);
        return (it != m_apartments.end()) ? it->second.get() : nullptr;
    }

    CustomStore* LocationManager::FindStore(const std::string& storeId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_stores.find(storeId);
        return (it != m_stores.end()) ? it->second.get() : nullptr;
    }

    std::vector<LocationEntry> LocationManager::GetNearbyLocations(const Vector3& position, float radius) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<LocationEntry> nearbyLocations;

        for (const auto& entry : m_locationDatabase) {
            if (!entry.isActive) continue;

            float distance = CalculateDistance(position, entry.position);
            if (distance <= radius) {
                nearbyLocations.push_back(entry);
            }
        }

        return nearbyLocations;
    }

    bool LocationManager::ValidateApartmentConfig(const ApartmentConfig& config) const
    {
        if (config.id.empty() || config.name.empty()) {
            return false;
        }

        if (config.maxPlayers == 0 || config.maxPlayers > 32) {
            return false;
        }

        if (!LocationUtils::IsValidPosition(config.entrancePosition) ||
            !LocationUtils::IsValidPosition(config.interiorPosition)) {
            return false;
        }

        return true;
    }

    bool LocationManager::ValidateStoreConfig(const StoreConfig& config) const
    {
        if (config.id.empty() || config.name.empty()) {
            return false;
        }

        if (config.maxPlayers == 0 || config.maxPlayers > 50) {
            return false;
        }

        if (!LocationUtils::IsValidPosition(config.entrancePosition) ||
            !LocationUtils::IsValidPosition(config.interiorPosition)) {
            return false;
        }

        return true;
    }

    bool LocationManager::CanPlayerEnterApartment(uint32_t playerId, const std::string& apartmentId, uint32_t ownerId) const
    {
        auto* apartment = const_cast<LocationManager*>(this)->FindApartment(apartmentId);
        if (!apartment) {
            return false;
        }

        // Check if player is an owner
        auto ownerIt = std::find(apartment->owners.begin(), apartment->owners.end(), playerId);
        if (ownerIt != apartment->owners.end()) {
            return true;
        }

        // Check if requesting entry to specific owner's instance
        if (ownerId != 0) {
            auto requestedOwnerIt = std::find(apartment->owners.begin(), apartment->owners.end(), ownerId);
            if (requestedOwnerIt == apartment->owners.end()) {
                return false;
            }

            // Check permissions for that owner's instance
            return CheckApartmentPermissions(playerId, ownerId, apartment->permissions);
        }

        return false;
    }

    void LocationManager::BroadcastInstanceUpdate(const InstanceUpdateData& instanceData)
    {
        SendInstanceUpdateToClients(instanceData);
    }

    void LocationManager::BroadcastApartmentPermissions(const ApartmentPermissionData& permissionData)
    {
        SendApartmentPermissionsToClients(permissionData);
    }

    void LocationManager::BroadcastLocationCreation(const LocationCreationData& creationData)
    {
        SendLocationCreationToClients(creationData);
    }

    void LocationManager::BroadcastPlayerTeleport(const PlayerInstanceTeleport& teleportData)
    {
        SendPlayerTeleportToClients(teleportData);
    }

    // Private implementation methods
    std::string LocationManager::GenerateInstanceId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis(1, UINT64_MAX);

        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        std::stringstream ss;
        ss << "instance_" << timestamp << "_" << dis(gen);
        return ss.str();
    }

    void LocationManager::UpdateLocationStates(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        // Update apartment states
        for (auto& [apartmentId, apartment] : m_apartments) {
            apartment->lastUpdate = currentTime;
            // Additional apartment-specific updates could go here
        }

        // Update store states
        for (auto& [storeId, store] : m_stores) {
            store->lastUpdate = currentTime;
            // Check operating hours, etc.
        }

        // Update custom location states
        for (auto& [locationId, location] : m_customLocations) {
            location->lastUpdate = currentTime;
        }

        // Update player states
        for (auto& [playerId, playerState] : m_playerStates) {
            // Check for player timeout (5 minutes of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - playerState->lastActivity).count();

            if (timeSinceActivity >= 5) {
                playerState->isConnected = false;
            }
        }
    }

    void LocationManager::UpdateInstancedSpaces(float deltaTime)
    {
        std::unique_lock<std::shared_mutex> lock(m_instancesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [instanceId, instance] : m_instancedSpaces) {
            // Update activity tracking
            if (!instance->players.empty()) {
                instance->lastActivityTime = currentTime;
            }

            // Broadcast updates if state changed
            if (instance->hasStateChanged) {
                InstanceUpdateData updateData;
                updateData.instanceId = instance->instanceId;
                updateData.locationId = instance->locationId;
                updateData.instanceType = instance->instanceType;
                updateData.ownerId = instance->ownerId;
                updateData.playerIds = instance->players;
                updateData.timestamp = currentTime;

                lock.unlock();
                BroadcastInstanceUpdate(updateData);
                lock.lock();

                instance->hasStateChanged = false;
            }
        }
    }

    void LocationManager::CleanupEmptyInstances()
    {
        std::unique_lock<std::shared_mutex> lock(m_instancesMutex);

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<std::string> expiredInstances;

        for (auto& [instanceId, instance] : m_instancedSpaces) {
            if (instance->players.empty()) {
                auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
                    currentTime - instance->lastActivityTime).count();

                if (timeSinceActivity >= 5) { // 5 minutes empty
                    expiredInstances.push_back(instanceId);
                }
            }
        }

        // Remove expired instances
        for (const auto& instanceId : expiredInstances) {
            m_instancedSpaces.erase(instanceId);
        }

        lock.unlock();

        // Broadcast instance removals
        for (const auto& instanceId : expiredInstances) {
            InstanceManagementUpdate updateData;
            updateData.instanceId = instanceId;
            updateData.isBeingDestroyed = true;
            updateData.updateTime = currentTime;
            // Would broadcast this to clients
        }
    }

    InstancedSpace* LocationManager::FindOrCreateApartmentInstance(const std::string& apartmentId, uint32_t ownerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_instancesMutex);

        // Find existing instance for this apartment owner
        for (auto& [instanceId, instance] : m_instancedSpaces) {
            if (instance->locationId == apartmentId &&
                instance->instanceType == InstanceType::Apartment &&
                instance->ownerId == ownerId) {
                return instance.get();
            }
        }

        // Create new instance
        std::string instanceId = GenerateInstanceId();
        auto instance = std::make_unique<InstancedSpace>();
        instance->instanceId = instanceId;
        instance->locationId = apartmentId;
        instance->instanceType = InstanceType::Apartment;
        instance->ownerId = ownerId;
        instance->maxPlayers = 8; // Default for apartments
        instance->creationTime = std::chrono::steady_clock::now();
        instance->lastActivityTime = instance->creationTime;
        instance->hasStateChanged = true;

        // Set spawn points from apartment configuration
        auto* apartment = FindApartment(apartmentId);
        if (apartment) {
            instance->spawnPoint = apartment->interiorPosition;
            instance->exitLocation = apartment->entrancePosition;
            instance->maxPlayers = apartment->maxPlayers;
        }

        auto* instancePtr = instance.get();
        m_instancedSpaces[instanceId] = std::move(instance);
        m_totalInstancesCreated++;

        return instancePtr;
    }

    InstancedSpace* LocationManager::FindOrCreateStoreInstance(const std::string& storeId)
    {
        std::unique_lock<std::shared_mutex> lock(m_instancesMutex);

        // Find existing store instance (stores use shared instances)
        for (auto& [instanceId, instance] : m_instancedSpaces) {
            if (instance->locationId == storeId && instance->instanceType == InstanceType::Store) {
                return instance.get();
            }
        }

        // Create new store instance
        std::string instanceId = GenerateInstanceId();
        auto instance = std::make_unique<InstancedSpace>();
        instance->instanceId = instanceId;
        instance->locationId = storeId;
        instance->instanceType = InstanceType::Store;
        instance->ownerId = 0; // Stores have no owner
        instance->maxPlayers = 10; // Default for stores
        instance->creationTime = std::chrono::steady_clock::now();
        instance->lastActivityTime = instance->creationTime;
        instance->hasStateChanged = true;

        // Set spawn points from store configuration
        auto* store = FindStore(storeId);
        if (store) {
            instance->spawnPoint = store->interiorPosition;
            instance->exitLocation = store->entrancePosition;
            instance->maxPlayers = store->maxPlayers;
        }

        auto* instancePtr = instance.get();
        m_instancedSpaces[instanceId] = std::move(instance);
        m_totalInstancesCreated++;

        return instancePtr;
    }

    InstancedSpace* LocationManager::FindPlayerCurrentInstance(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_instancesMutex);

        for (auto& [instanceId, instance] : m_instancedSpaces) {
            auto playerIt = std::find(instance->players.begin(), instance->players.end(), playerId);
            if (playerIt != instance->players.end()) {
                return instance.get();
            }
        }

        return nullptr;
    }

    void LocationManager::TeleportPlayerToInstance(uint32_t playerId, const std::string& instanceId, const Vector3& spawnPoint)
    {
        PlayerInstanceTeleport teleportData;
        teleportData.playerId = playerId;
        teleportData.instanceId = instanceId;
        teleportData.spawnPoint = spawnPoint;
        teleportData.isEntering = true;
        teleportData.timestamp = std::chrono::steady_clock::now();

        m_totalTeleports++;

        BroadcastPlayerTeleport(teleportData);
    }

    void LocationManager::TeleportPlayerToWorld(uint32_t playerId, const Vector3& exitLocation)
    {
        PlayerInstanceTeleport teleportData;
        teleportData.playerId = playerId;
        teleportData.instanceId = "";
        teleportData.spawnPoint = exitLocation;
        teleportData.isEntering = false;
        teleportData.timestamp = std::chrono::steady_clock::now();

        m_totalTeleports++;

        BroadcastPlayerTeleport(teleportData);
    }

    bool LocationManager::CheckApartmentPermissions(uint32_t playerId, uint32_t ownerId, const ApartmentPermissions& permissions) const
    {
        // Check if player is blocked
        auto blockedIt = std::find(permissions.blockedPlayers.begin(), permissions.blockedPlayers.end(), playerId);
        if (blockedIt != permissions.blockedPlayers.end()) {
            return false;
        }

        // Check if player is explicitly allowed
        auto allowedIt = std::find(permissions.allowedPlayers.begin(), permissions.allowedPlayers.end(), playerId);
        if (allowedIt != permissions.allowedPlayers.end()) {
            return true;
        }

        // Check friend permissions
        if (permissions.allowFriends && ArePlayersFriends(ownerId, playerId)) {
            return true;
        }

        // Check guild permissions
        if (permissions.allowGuildMembers && ArePlayersInSameGuild(ownerId, playerId)) {
            return true;
        }

        // Check public access
        return permissions.allowPublic;
    }

    bool LocationManager::ArePlayersFriends(uint32_t playerId1, uint32_t playerId2) const
    {
        // This would integrate with the friends system
        // Placeholder implementation
        return false;
    }

    bool LocationManager::ArePlayersInSameGuild(uint32_t playerId1, uint32_t playerId2) const
    {
        // This would integrate with the guild system
        // Placeholder implementation
        return false;
    }

    float LocationManager::CalculateDistance(const Vector3& pos1, const Vector3& pos2) const
    {
        float dx = pos1.x - pos2.x;
        float dy = pos1.y - pos2.y;
        float dz = pos1.z - pos2.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    void LocationManager::UpdatePlayerToInstanceMapping(uint32_t playerId, const std::string& instanceId, bool isActive)
    {
        auto& instanceList = m_playerToInstances[playerId];

        if (isActive) {
            if (std::find(instanceList.begin(), instanceList.end(), instanceId) == instanceList.end()) {
                instanceList.push_back(instanceId);
            }
        } else {
            instanceList.erase(std::remove(instanceList.begin(), instanceList.end(), instanceId), instanceList.end());
        }
    }

    void LocationManager::UpdateLocationTypeMapping(const std::string& locationId, LocationType type, bool isActive)
    {
        auto& locationList = m_locationsByType[type];

        if (isActive) {
            if (std::find(locationList.begin(), locationList.end(), locationId) == locationList.end()) {
                locationList.push_back(locationId);
            }
        } else {
            locationList.erase(std::remove(locationList.begin(), locationList.end(), locationId), locationList.end());
        }
    }

    void LocationManager::RemovePlayerFromAllMappings(uint32_t playerId)
    {
        // Remove player from any instances
        auto* instance = FindPlayerCurrentInstance(playerId);
        if (instance) {
            auto playerIt = std::find(instance->players.begin(), instance->players.end(), playerId);
            if (playerIt != instance->players.end()) {
                instance->players.erase(playerIt);
                instance->hasStateChanged = true;
            }
        }

        // Clear player to instance mapping
        m_playerToInstances.erase(playerId);
    }

    void LocationManager::ProcessLocationExpirations()
    {
        // Clean up expired location entries, configurations, etc.
        // This would handle any time-based location cleanup
    }

    void LocationManager::ValidateLocationStates()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (auto& [playerId, playerState] : m_playerStates) {
            // Validate and correct invalid location states
            if (!playerState->currentInstanceId.empty()) {
                auto* instance = const_cast<LocationManager*>(this)->FindInstancedSpace(playerState->currentInstanceId);
                if (!instance) {
                    // Instance no longer exists, reset player to open world
                    playerState->currentContext = PlayerLocationContext::OpenWorld;
                    playerState->currentInstanceId = "";
                    playerState->currentLocationId = "";
                }
            }
        }
    }

    void LocationManager::LoadLocationConfigurations()
    {
        // Load location configurations from persistent storage
        // This would read from config files and register locations
        // Placeholder for actual file loading implementation
    }

    bool LocationManager::SaveLocationConfiguration(const LocationConfig& config, const std::string& configPath)
    {
        // Save location configuration to persistent storage
        // This would write to config files
        // Placeholder for actual file saving implementation
        return true;
    }

    std::string LocationManager::SerializeConfig(const LocationConfig& config) const
    {
        // Serialize configuration to string format (JSON, etc.)
        // Placeholder implementation
        return "{}";
    }

    bool LocationManager::IsPlayerAdmin(uint32_t playerId) const
    {
        // This would check admin status from admin system
        // Placeholder implementation
        return false;
    }

    InstancedSpace* LocationManager::FindInstancedSpace(const std::string& instanceId)
    {
        std::shared_lock<std::shared_mutex> lock(m_instancesMutex);

        auto it = m_instancedSpaces.find(instanceId);
        return (it != m_instancedSpaces.end()) ? it->second.get() : nullptr;
    }

    uint32_t LocationManager::GetActivePlayerCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        uint32_t count = 0;
        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->isConnected) {
                count++;
            }
        }

        return count;
    }

    uint32_t LocationManager::GetTotalLocationsCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);
        return static_cast<uint32_t>(m_locationDatabase.size());
    }

    uint32_t LocationManager::GetActiveInstancesCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_instancesMutex);
        return static_cast<uint32_t>(m_instancedSpaces.size());
    }

    // Notification methods
    void LocationManager::NotifyLocationEntry(uint32_t playerId, const std::string& locationId, bool isEntering)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_locationEntryCallback) {
            m_locationEntryCallback(playerId, locationId, isEntering);
        }
    }

    void LocationManager::NotifyApartmentAccess(uint32_t playerId, const std::string& apartmentId, ApartmentEntryResult result)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_apartmentAccessCallback) {
            m_apartmentAccessCallback(playerId, apartmentId, result);
        }
    }

    void LocationManager::NotifyInstanceCreated(const std::string& instanceId, InstanceType type, uint32_t ownerId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_instanceCreatedCallback) {
            m_instanceCreatedCallback(instanceId, type, ownerId);
        }
    }

    void LocationManager::NotifyLocationCreated(const LocationCreationData& creationData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_locationCreatedCallback) {
            m_locationCreatedCallback(creationData);
        }
    }

    void LocationManager::NotifyPermissionChanged(const std::string& apartmentId, uint32_t ownerId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_permissionChangedCallback) {
            m_permissionChangedCallback(apartmentId, ownerId);
        }
    }

    void LocationManager::SendInstanceUpdateToClients(const InstanceUpdateData& instanceData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void LocationManager::SendApartmentPermissionsToClients(const ApartmentPermissionData& permissionData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void LocationManager::SendLocationCreationToClients(const LocationCreationData& creationData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void LocationManager::SendPlayerTeleportToClients(const PlayerInstanceTeleport& teleportData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    // Callback setters
    void LocationManager::SetLocationEntryCallback(LocationEntryCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_locationEntryCallback = callback;
    }

    void LocationManager::SetApartmentAccessCallback(ApartmentAccessCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_apartmentAccessCallback = callback;
    }

    void LocationManager::SetInstanceCreatedCallback(InstanceCreatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_instanceCreatedCallback = callback;
    }

    void LocationManager::SetLocationCreatedCallback(LocationCreatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_locationCreatedCallback = callback;
    }

    void LocationManager::SetPermissionChangedCallback(PermissionChangedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_permissionChangedCallback = callback;
    }

    // Additional method implementations
    void LocationManager::UpdatePlayerActivity(uint32_t playerId)
    {
        auto* playerState = GetPlayerLocationState(playerId);
        if (playerState) {
            playerState->lastActivity = std::chrono::steady_clock::now();
            playerState->isConnected = true;
        }
    }

    void LocationManager::SynchronizePlayerLocation(uint32_t playerId)
    {
        auto* playerState = GetPlayerLocationState(playerId);
        if (!playerState) {
            return;
        }

        // Create location sync data
        LocationStateUpdate syncData;
        syncData.playerId = playerId;
        syncData.context = playerState->currentContext;
        syncData.currentLocationId = playerState->currentLocationId;
        syncData.currentInstanceId = playerState->currentInstanceId;
        syncData.position = playerState->lastKnownPosition;
        syncData.updateTime = std::chrono::steady_clock::now();
        syncData.syncVersion = 1; // Version for compatibility

        // Would broadcast this update
    }

    void LocationManager::ForceSyncPlayer(uint32_t playerId)
    {
        SynchronizePlayerLocation(playerId);
    }

    void LocationManager::SetSyncPriority(uint32_t playerId, float priority)
    {
        auto* playerState = GetPlayerLocationState(playerId);
        if (playerState) {
            playerState->syncPriority = priority;
        }
    }

    bool LocationManager::RegisterCustomLocation(const LocationConfig& config)
    {
        if (!ValidateLocationConfig(config)) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Check for duplicate ID
        if (m_customLocations.find(config.id) != m_customLocations.end()) {
            return false;
        }

        // Create custom location
        auto location = std::make_unique<CustomLocation>();
        location->id = config.id;
        location->name = config.name;
        location->description = config.description;
        location->entrancePosition = config.entrancePosition;
        location->interiorPosition = config.interiorPosition;
        location->maxPlayers = config.maxPlayers;
        location->isFromSingleplayer = false;
        location->accessLevel = LocationAccessLevel::Public;
        location->lastUpdate = std::chrono::steady_clock::now();

        m_customLocations[config.id] = std::move(location);

        // Update mappings
        UpdateLocationTypeMapping(config.id, LocationType::Custom, true);

        // Add to location database
        LocationEntry entry;
        entry.id = config.id;
        entry.locationType = LocationType::Custom;
        entry.position = config.entrancePosition;
        entry.name = config.name;
        entry.isActive = true;
        m_locationDatabase.push_back(entry);

        m_totalLocationsRegistered++;

        return true;
    }

    bool LocationManager::ValidateLocationConfig(const LocationConfig& config) const
    {
        if (config.id.empty() || config.name.empty()) {
            return false;
        }

        if (config.maxPlayers == 0 || config.maxPlayers > 100) {
            return false;
        }

        if (!LocationUtils::IsValidPosition(config.entrancePosition) ||
            !LocationUtils::IsValidPosition(config.interiorPosition)) {
            return false;
        }

        return true;
    }

    LocationCreationResult LocationManager::CreateCustomLocation(uint32_t adminId, const LocationConfig& config)
    {
        if (!IsPlayerAdmin(adminId)) {
            return LocationCreationResult::InsufficientPermissions;
        }

        if (!ValidateLocationConfig(config)) {
            return LocationCreationResult::InvalidConfiguration;
        }

        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        if (m_customLocations.find(config.id) != m_customLocations.end()) {
            return LocationCreationResult::DuplicateId;
        }

        lock.unlock();

        // Register the location
        if (!RegisterCustomLocation(config)) {
            return LocationCreationResult::InvalidConfiguration;
        }

        // Save configuration
        SaveLocationConfiguration(config, "config/locations/");

        // Broadcast creation
        LocationCreationData creationData;
        creationData.locationType = LocationType::Custom;
        creationData.locationId = config.id;
        creationData.config = SerializeConfig(config);
        creationData.creatorId = adminId;
        creationData.timestamp = std::chrono::steady_clock::now();

        BroadcastLocationCreation(creationData);
        NotifyLocationCreated(creationData);

        return LocationCreationResult::Success;
    }

    std::unordered_map<LocationType, uint32_t> LocationManager::GetLocationTypeStats() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::unordered_map<LocationType, uint32_t> stats;
        for (const auto& [type, locationList] : m_locationsByType) {
            stats[type] = static_cast<uint32_t>(locationList.size());
        }

        return stats;
    }

    // Utility functions implementation
    namespace LocationUtils
    {
        std::string LocationTypeToString(LocationType type)
        {
            switch (type) {
                case LocationType::Apartment: return "Apartment";
                case LocationType::Store: return "Store";
                case LocationType::Custom: return "Custom";
                case LocationType::Notable: return "Notable";
                default: return "Unknown";
            }
        }

        std::string InstanceTypeToString(InstanceType type)
        {
            switch (type) {
                case InstanceType::Apartment: return "Apartment";
                case InstanceType::Store: return "Store";
                case InstanceType::Custom: return "Custom";
                default: return "Unknown";
            }
        }

        std::string PlayerLocationContextToString(PlayerLocationContext context)
        {
            switch (context) {
                case PlayerLocationContext::OpenWorld: return "OpenWorld";
                case PlayerLocationContext::ApartmentInstance: return "ApartmentInstance";
                case PlayerLocationContext::CustomInstance: return "CustomInstance";
                case PlayerLocationContext::StoreInstance: return "StoreInstance";
                default: return "Unknown";
            }
        }

        bool IsValidPosition(const Vector3& position)
        {
            const float MAX_COORD = 100000.0f;
            return std::abs(position.x) < MAX_COORD &&
                   std::abs(position.y) < MAX_COORD &&
                   std::abs(position.z) < MAX_COORD;
        }

        bool IsValidLocationId(const std::string& locationId)
        {
            return !locationId.empty() && locationId.length() < 128;
        }

        float CalculateDistance(const Vector3& pos1, const Vector3& pos2)
        {
            float dx = pos1.x - pos2.x;
            float dy = pos1.y - pos2.y;
            float dz = pos1.z - pos2.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        Vector3 CalculateDirection(const Vector3& from, const Vector3& to)
        {
            Vector3 dir;
            dir.x = to.x - from.x;
            dir.y = to.y - from.y;
            dir.z = to.z - from.z;

            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (length > 0.0f) {
                dir.x /= length;
                dir.y /= length;
                dir.z /= length;
            }

            return dir;
        }

        bool ShouldSyncLocationState(const PlayerLocationState& oldState, const PlayerLocationState& newState)
        {
            // Sync if context changed
            if (oldState.currentContext != newState.currentContext) {
                return true;
            }

            // Sync if location changed
            if (oldState.currentLocationId != newState.currentLocationId) {
                return true;
            }

            // Sync if instance changed
            if (oldState.currentInstanceId != newState.currentInstanceId) {
                return true;
            }

            // Sync if position changed significantly (more than 5 meters)
            float positionDiff = CalculateDistance(oldState.lastKnownPosition, newState.lastKnownPosition);
            if (positionDiff > 5.0f) {
                return true;
            }

            return false;
        }

        uint32_t HashLocationState(const PlayerLocationState& state)
        {
            // Simple hash combining multiple state values
            uint32_t hash = 0;
            hash ^= static_cast<uint32_t>(state.currentContext) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<std::string>{}(state.currentLocationId) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<std::string>{}(state.currentInstanceId) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }
    }
}