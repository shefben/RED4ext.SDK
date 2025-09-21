// Custom Location Management System
// Admin-configurable apartments, stores, and custom locations with instanced spaces and ownership

// Custom Location Manager - manages all custom locations and instanced spaces
public class CustomLocationManager extends ScriptableSystem {
    private static let s_instance: ref<CustomLocationManager>;
    private let m_customApartments: array<ref<CustomApartment>>;
    private let m_customStores: array<ref<CustomStore>>;
    private let m_customLocations: array<ref<CustomLocation>>;
    private let m_instancedSpaces: array<ref<InstancedSpace>>;
    private let m_locationDatabase: ref<LocationDatabase>;
    private let m_apartmentInstances: array<ref<ApartmentInstance>>;
    private let m_locationSyncTimer: Float = 0.0;
    private let m_locationSyncInterval: Float = 1.0; // 1 FPS for location updates
    private let m_localPlayer: wref<PlayerPuppet>;

    public static func GetInstance() -> ref<CustomLocationManager> {
        if !IsDefined(CustomLocationManager.s_instance) {
            CustomLocationManager.s_instance = new CustomLocationManager();
        }
        return CustomLocationManager.s_instance;
    }

    public func Initialize(player: ref<PlayerPuppet>) -> Void {
        this.m_localPlayer = player;
        this.InitializeLocationDatabase();
        this.LoadCustomLocations();
        LogChannel(n"LocationManager", s"[LocationManager] Custom Location Manager initialized");
    }

    private func InitializeLocationDatabase() -> Void {
        this.m_locationDatabase = new LocationDatabase();
        this.m_locationDatabase.Initialize();

        // Load existing singleplayer locations automatically
        this.ScanExistingLocations();

        LogChannel(n"LocationManager", s"[LocationManager] Location database initialized");
    }

    private func ScanExistingLocations() -> Void {
        // Automatically detect and catalog all existing singleplayer locations
        this.ScanApartmentLocations();
        this.ScanStoreLocations();
        this.ScanNotableLocations();
    }

    private func ScanApartmentLocations() -> Void {
        // Scan for existing apartment locations from singleplayer
        let apartmentSystem = GameInstance.GetApartmentSystem(GetGameInstance());
        let existingApartments = apartmentSystem.GetAllApartments();

        for apartment in existingApartments {
            this.RegisterExistingApartment(apartment);
        }

        LogChannel(n"LocationManager", s"[LocationManager] Scanned " + ArraySize(existingApartments) + " existing apartments");
    }

    private func ScanStoreLocations() -> Void {
        // Scan for existing store/vendor locations from singleplayer
        let vendorSystem = GameInstance.GetVendorSystem(GetGameInstance());
        let existingVendors = vendorSystem.GetAllVendors();

        for vendor in existingVendors {
            this.RegisterExistingStore(vendor);
        }

        LogChannel(n"LocationManager", s"[LocationManager] Scanned existing stores");
    }

    private func ScanNotableLocations() -> Void {
        // Scan for other notable locations (bars, clubs, corpo buildings, etc.)
        let questSystem = GameInstance.GetQuestSystem(GetGameInstance());
        let notableLocations = questSystem.GetNotableLocations();

        for location in notableLocations {
            this.RegisterNotableLocation(location);
        }

        LogChannel(n"LocationManager", s"[LocationManager] Scanned notable locations");
    }

    private func LoadCustomLocations() -> Void {
        // Load admin-configured custom locations
        this.LoadCustomApartments();
        this.LoadCustomStores();
        this.LoadCustomLocationsFromConfig();
    }

    private func LoadCustomApartments() -> Void {
        // Load custom apartment definitions from admin configuration
        let configPath = "config/custom_apartments.json";
        let apartmentConfigs = this.LoadLocationConfig(configPath);

        for config in apartmentConfigs {
            let customApartment = new CustomApartment();
            customApartment.InitializeFromConfig(config);
            ArrayPush(this.m_customApartments, customApartment);

            LogChannel(n"LocationManager", s"[LocationManager] Loaded custom apartment: " + customApartment.GetName());
        }
    }

    private func LoadCustomStores() -> Void {
        // Load custom store definitions from admin configuration
        let configPath = "config/custom_stores.json";
        let storeConfigs = this.LoadLocationConfig(configPath);

        for config in storeConfigs {
            let customStore = new CustomStore();
            customStore.InitializeFromConfig(config);
            ArrayPush(this.m_customStores, customStore);

            LogChannel(n"LocationManager", s"[LocationManager] Loaded custom store: " + customStore.GetName());
        }
    }

    private func LoadCustomLocationsFromConfig() -> Void {
        // Load general custom locations from admin configuration
        let configPath = "config/custom_locations.json";
        let locationConfigs = this.LoadLocationConfig(configPath);

        for config in locationConfigs {
            let customLocation = new CustomLocation();
            customLocation.InitializeFromConfig(config);
            ArrayPush(this.m_customLocations, customLocation);

            LogChannel(n"LocationManager", s"[LocationManager] Loaded custom location: " + customLocation.GetName());
        }
    }

    private func LoadLocationConfig(configPath: String) -> array<LocationConfig> {
        // Load location configuration from file (placeholder - would integrate with file system)
        let configs: array<LocationConfig>;
        return configs; // Placeholder
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        this.m_locationSyncTimer += deltaTime;

        if this.m_locationSyncTimer >= this.m_locationSyncInterval {
            this.UpdateLocationStates();
            this.UpdateInstancedSpaces();
            this.CleanupEmptyInstances();
            this.m_locationSyncTimer = 0.0;
        }
    }

    private func UpdateLocationStates() -> Void {
        // Update custom locations
        for location in this.m_customLocations {
            location.Update();
        }

        // Update custom apartments
        for apartment in this.m_customApartments {
            apartment.Update();
        }

        // Update custom stores
        for store in this.m_customStores {
            store.Update();
        }
    }

    private func UpdateInstancedSpaces() -> Void {
        for instance in this.m_instancedSpaces {
            instance.Update();

            if instance.HasStateChanged() {
                let instanceData = instance.GetSyncData();
                Net_SendInstanceUpdate(instanceData);
                instance.MarkSynced();
            }
        }
    }

    private func CleanupEmptyInstances() -> Void {
        let emptyInstances: array<ref<InstancedSpace>>;

        for instance in this.m_instancedSpaces {
            if instance.IsEmpty() && instance.HasBeenEmptyForMinutes(5) {
                ArrayPush(emptyInstances, instance);
            }
        }

        for emptyInstance in emptyInstances {
            this.RemoveInstancedSpace(emptyInstance.GetInstanceId());
            LogChannel(n"LocationManager", s"[LocationManager] Cleaned up empty instance: " + emptyInstance.GetInstanceId());
        }
    }

    // Apartment Management
    public func RequestApartmentEntry(playerId: Uint32, apartmentId: String, ownerId: Uint32) -> EApartmentEntryResult {
        let apartment = this.FindApartment(apartmentId);
        if !IsDefined(apartment) {
            return EApartmentEntryResult.NotFound;
        }

        // Check permissions
        if !apartment.CanPlayerEnter(playerId, ownerId) {
            return EApartmentEntryResult.AccessDenied;
        }

        // Create or find apartment instance for the owner
        let instance = this.FindOrCreateApartmentInstance(apartmentId, ownerId);

        // Add player to instance
        let entryResult = instance.AddPlayer(playerId);
        if Equals(entryResult, EInstanceEntryResult.Success) {
            // Teleport player to instance
            this.TeleportPlayerToInstance(playerId, instance);

            // Notify other players in instance
            this.NotifyInstancePlayerJoined(instance.GetInstanceId(), playerId);

            LogChannel(n"LocationManager", s"[LocationManager] Player " + playerId + " entered apartment " + apartmentId + " (Owner: " + ownerId + ")");
            return EApartmentEntryResult.Success;
        }

        return EApartmentEntryResult.InstanceFull;
    }

    public func ExitApartment(playerId: Uint32) -> Bool {
        let instance = this.FindPlayerApartmentInstance(playerId);
        if !IsDefined(instance) {
            return false;
        }

        // Remove player from instance
        instance.RemovePlayer(playerId);

        // Teleport player back to world
        this.TeleportPlayerToWorld(playerId, instance.GetExitLocation());

        // Notify other players in instance
        this.NotifyInstancePlayerLeft(instance.GetInstanceId(), playerId);

        LogChannel(n"LocationManager", s"[LocationManager] Player " + playerId + " exited apartment instance " + instance.GetInstanceId());
        return true;
    }

    public func SetApartmentPermissions(ownerId: Uint32, apartmentId: String, permissions: ApartmentPermissions) -> Bool {
        let apartment = this.FindApartment(apartmentId);
        if !IsDefined(apartment) || !apartment.IsOwner(ownerId) {
            return false;
        }

        apartment.SetPermissions(permissions);

        // Sync permissions to all clients
        let permissionData: ApartmentPermissionData;
        permissionData.apartmentId = apartmentId;
        permissionData.ownerId = ownerId;
        permissionData.permissions = permissions;
        permissionData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendApartmentPermissions(permissionData);

        LogChannel(n"LocationManager", s"[LocationManager] Apartment permissions updated for " + apartmentId);
        return true;
    }

    // Custom Location Management (Admin Functions)
    public func CreateCustomApartment(adminId: Uint32, config: ApartmentConfig) -> ELocationCreationResult {
        if !this.IsPlayerAdmin(adminId) {
            return ELocationCreationResult.InsufficientPermissions;
        }

        // Validate configuration
        if !this.ValidateApartmentConfig(config) {
            return ELocationCreationResult.InvalidConfiguration;
        }

        // Check for duplicate ID
        if IsDefined(this.FindApartment(config.id)) {
            return ELocationCreationResult.DuplicateId;
        }

        // Create apartment
        let customApartment = new CustomApartment();
        customApartment.InitializeFromConfig(config);
        ArrayPush(this.m_customApartments, customApartment);

        // Save to persistent storage
        this.SaveApartmentConfig(config);

        // Sync to all clients
        let creationData: LocationCreationData;
        creationData.locationType = ELocationType.Apartment;
        creationData.locationId = config.id;
        creationData.config = this.SerializeConfig(config);
        creationData.creatorId = adminId;
        creationData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendLocationCreation(creationData);

        LogChannel(n"LocationManager", s"[LocationManager] Admin " + adminId + " created custom apartment: " + config.id);
        return ELocationCreationResult.Success;
    }

    public func CreateCustomStore(adminId: Uint32, config: StoreConfig) -> ELocationCreationResult {
        if !this.IsPlayerAdmin(adminId) {
            return ELocationCreationResult.InsufficientPermissions;
        }

        if !this.ValidateStoreConfig(config) {
            return ELocationCreationResult.InvalidConfiguration;
        }

        if IsDefined(this.FindStore(config.id)) {
            return ELocationCreationResult.DuplicateId;
        }

        let customStore = new CustomStore();
        customStore.InitializeFromConfig(config);
        ArrayPush(this.m_customStores, customStore);

        this.SaveStoreConfig(config);

        let creationData: LocationCreationData;
        creationData.locationType = ELocationType.Store;
        creationData.locationId = config.id;
        creationData.config = this.SerializeConfig(config);
        creationData.creatorId = adminId;
        creationData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendLocationCreation(creationData);

        LogChannel(n"LocationManager", s"[LocationManager] Admin " + adminId + " created custom store: " + config.id);
        return ELocationCreationResult.Success;
    }

    public func CreateCustomLocation(adminId: Uint32, config: LocationConfig) -> ELocationCreationResult {
        if !this.IsPlayerAdmin(adminId) {
            return ELocationCreationResult.InsufficientPermissions;
        }

        if !this.ValidateLocationConfig(config) {
            return ELocationCreationResult.InvalidConfiguration;
        }

        if IsDefined(this.FindCustomLocation(config.id)) {
            return ELocationCreationResult.DuplicateId;
        }

        let customLocation = new CustomLocation();
        customLocation.InitializeFromConfig(config);
        ArrayPush(this.m_customLocations, customLocation);

        this.SaveLocationConfig(config);

        let creationData: LocationCreationData;
        creationData.locationType = ELocationType.Custom;
        creationData.locationId = config.id;
        creationData.config = this.SerializeConfig(config);
        creationData.creatorId = adminId;
        creationData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendLocationCreation(creationData);

        LogChannel(n"LocationManager", s"[LocationManager] Admin " + adminId + " created custom location: " + config.id);
        return ELocationCreationResult.Success;
    }

    // Store Management
    public func RequestStoreEntry(playerId: Uint32, storeId: String) -> EStoreEntryResult {
        let store = this.FindStore(storeId);
        if !IsDefined(store) {
            return EStoreEntryResult.NotFound;
        }

        if !store.IsOpen() {
            return EStoreEntryResult.Closed;
        }

        // Create shared store instance
        let instance = this.FindOrCreateStoreInstance(storeId);

        let entryResult = instance.AddPlayer(playerId);
        if Equals(entryResult, EInstanceEntryResult.Success) {
            this.TeleportPlayerToInstance(playerId, instance);
            this.NotifyInstancePlayerJoined(instance.GetInstanceId(), playerId);

            LogChannel(n"LocationManager", s"[LocationManager] Player " + playerId + " entered store " + storeId);
            return EStoreEntryResult.Success;
        }

        return EStoreEntryResult.InstanceFull;
    }

    // Instance Management
    private func FindOrCreateApartmentInstance(apartmentId: String, ownerId: Uint32) -> ref<ApartmentInstance> {
        // Find existing instance for this apartment owner
        for instance in this.m_apartmentInstances {
            if Equals(instance.GetApartmentId(), apartmentId) && instance.GetOwnerId() == ownerId {
                return instance;
            }
        }

        // Create new instance
        let newInstance = new ApartmentInstance();
        newInstance.Initialize(apartmentId, ownerId);
        ArrayPush(this.m_apartmentInstances, newInstance);

        LogChannel(n"LocationManager", s"[LocationManager] Created apartment instance: " + apartmentId + " for owner " + ownerId);
        return newInstance;
    }

    private func FindOrCreateStoreInstance(storeId: String) -> ref<InstancedSpace> {
        // Stores use shared instances
        for instance in this.m_instancedSpaces {
            if Equals(instance.GetLocationId(), storeId) && Equals(instance.GetInstanceType(), EInstanceType.Store) {
                return instance;
            }
        }

        let newInstance = new InstancedSpace();
        newInstance.Initialize(storeId, EInstanceType.Store, 0u); // No owner for stores
        ArrayPush(this.m_instancedSpaces, newInstance);

        LogChannel(n"LocationManager", s"[LocationManager] Created store instance: " + storeId);
        return newInstance;
    }

    private func TeleportPlayerToInstance(playerId: Uint32, instance: ref<InstancedSpace>) -> Void {
        // Teleport player to instanced space
        let teleportData: PlayerInstanceTeleport;
        teleportData.playerId = playerId;
        teleportData.instanceId = instance.GetInstanceId();
        teleportData.spawnPoint = instance.GetSpawnPoint();
        teleportData.isEntering = true;
        teleportData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendPlayerInstanceTeleport(teleportData);
    }

    private func TeleportPlayerToWorld(playerId: Uint32, exitLocation: Vector3) -> Void {
        // Teleport player back to open world
        let teleportData: PlayerInstanceTeleport;
        teleportData.playerId = playerId;
        teleportData.instanceId = "";
        teleportData.spawnPoint = exitLocation;
        teleportData.isEntering = false;
        teleportData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendPlayerInstanceTeleport(teleportData);
    }

    private func NotifyInstancePlayerJoined(instanceId: String, playerId: Uint32) -> Void {
        let notificationData: InstancePlayerNotification;
        notificationData.instanceId = instanceId;
        notificationData.playerId = playerId;
        notificationData.isJoining = true;
        notificationData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendInstancePlayerNotification(notificationData);
    }

    private func NotifyInstancePlayerLeft(instanceId: String, playerId: Uint32) -> Void {
        let notificationData: InstancePlayerNotification;
        notificationData.instanceId = instanceId;
        notificationData.playerId = playerId;
        notificationData.isJoining = false;
        notificationData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendInstancePlayerNotification(notificationData);
    }

    // Helper methods
    private func RegisterExistingApartment(apartmentData: ApartmentData) -> Void {
        // Register existing singleplayer apartment for multiplayer use
        let apartment = new CustomApartment();
        apartment.InitializeFromGameData(apartmentData);
        ArrayPush(this.m_customApartments, apartment);
    }

    private func RegisterExistingStore(vendorData: VendorData) -> Void {
        // Register existing singleplayer store for multiplayer use
        let store = new CustomStore();
        store.InitializeFromGameData(vendorData);
        ArrayPush(this.m_customStores, store);
    }

    private func RegisterNotableLocation(locationData: NotableLocationData) -> Void {
        // Register existing singleplayer notable location
        let location = new CustomLocation();
        location.InitializeFromGameData(locationData);
        ArrayPush(this.m_customLocations, location);
    }

    private func FindApartment(apartmentId: String) -> ref<CustomApartment> {
        for apartment in this.m_customApartments {
            if Equals(apartment.GetId(), apartmentId) {
                return apartment;
            }
        }
        return null;
    }

    private func FindStore(storeId: String) -> ref<CustomStore> {
        for store in this.m_customStores {
            if Equals(store.GetId(), storeId) {
                return store;
            }
        }
        return null;
    }

    private func FindCustomLocation(locationId: String) -> ref<CustomLocation> {
        for location in this.m_customLocations {
            if Equals(location.GetId(), locationId) {
                return location;
            }
        }
        return null;
    }

    private func FindPlayerApartmentInstance(playerId: Uint32) -> ref<ApartmentInstance> {
        for instance in this.m_apartmentInstances {
            if instance.HasPlayer(playerId) {
                return instance;
            }
        }
        return null;
    }

    private func RemoveInstancedSpace(instanceId: String) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_instancedSpaces)) {
            if Equals(this.m_instancedSpaces[i].GetInstanceId(), instanceId) {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_instancedSpaces, this.m_instancedSpaces[index]);
        }
    }

    // Validation methods
    private func ValidateApartmentConfig(config: ApartmentConfig) -> Bool {
        return !Equals(config.id, "") && !Equals(config.name, "");
    }

    private func ValidateStoreConfig(config: StoreConfig) -> Bool {
        return !Equals(config.id, "") && !Equals(config.name, "");
    }

    private func ValidateLocationConfig(config: LocationConfig) -> Bool {
        return !Equals(config.id, "") && !Equals(config.name, "");
    }

    // Admin verification
    private func IsPlayerAdmin(playerId: Uint32) -> Bool {
        // This would check admin status from admin system
        return false; // Placeholder
    }

    // Persistence methods (placeholders)
    private func SaveApartmentConfig(config: ApartmentConfig) -> Void { }
    private func SaveStoreConfig(config: StoreConfig) -> Void { }
    private func SaveLocationConfig(config: LocationConfig) -> Void { }
    private func SerializeConfig(config: ref<ISerializable>) -> String { return ""; }

    // Network event handlers
    public func OnRemoteLocationCreation(creationData: LocationCreationData) -> Void {
        LogChannel(n"LocationManager", s"[LocationManager] Remote location creation: " + creationData.locationId);
        // Handle remote location creation
    }

    public func OnRemoteInstanceUpdate(instanceData: InstanceUpdateData) -> Void {
        // Handle remote instance updates
        let instance = this.FindInstancedSpace(instanceData.instanceId);
        if IsDefined(instance) {
            instance.UpdateFromRemote(instanceData);
        }
    }

    private func FindInstancedSpace(instanceId: String) -> ref<InstancedSpace> {
        for instance in this.m_instancedSpaces {
            if Equals(instance.GetInstanceId(), instanceId) {
                return instance;
            }
        }
        return null;
    }

    // Public API
    public func GetAvailableApartments() -> array<ref<CustomApartment>> {
        return this.m_customApartments;
    }

    public func GetAvailableStores() -> array<ref<CustomStore>> {
        return this.m_customStores;
    }

    public func GetCustomLocations() -> array<ref<CustomLocation>> {
        return this.m_customLocations;
    }

    public func GetPlayerLocation(playerId: Uint32) -> EPlayerLocationContext {
        // Determine if player is in an instance or open world
        if IsDefined(this.FindPlayerApartmentInstance(playerId)) {
            return EPlayerLocationContext.ApartmentInstance;
        }

        for instance in this.m_instancedSpaces {
            if instance.HasPlayer(playerId) {
                return EPlayerLocationContext.CustomInstance;
            }
        }

        return EPlayerLocationContext.OpenWorld;
    }
}

// Support Classes (continued in next file due to length limits...)

// Basic data structures and enums for the location system
public enum EApartmentEntryResult : Uint8 {
    Success = 0,
    NotFound = 1,
    AccessDenied = 2,
    InstanceFull = 3,
    AlreadyInside = 4
}

public enum EStoreEntryResult : Uint8 {
    Success = 0,
    NotFound = 1,
    Closed = 2,
    InstanceFull = 3
}

public enum ELocationCreationResult : Uint8 {
    Success = 0,
    InsufficientPermissions = 1,
    InvalidConfiguration = 2,
    DuplicateId = 3,
    StorageFull = 4
}

public enum EInstanceEntryResult : Uint8 {
    Success = 0,
    Full = 1,
    AccessDenied = 2,
    InvalidInstance = 3
}

public enum ELocationType : Uint8 {
    Apartment = 0,
    Store = 1,
    Custom = 2,
    Notable = 3
}

public enum EInstanceType : Uint8 {
    Apartment = 0,
    Store = 1,
    Custom = 2
}

public enum EPlayerLocationContext : Uint8 {
    OpenWorld = 0,
    ApartmentInstance = 1,
    CustomInstance = 2,
    StoreInstance = 3
}

// Data structures
public struct ApartmentPermissions {
    public let allowFriends: Bool;
    public let allowGuildMembers: Bool;
    public let allowPublic: Bool;
    public let allowedPlayers: array<Uint32>;
    public let blockedPlayers: array<Uint32>;
}

public struct LocationConfig {
    public let id: String;
    public let name: String;
    public let description: String;
    public let entrancePosition: Vector3;
    public let interiorPosition: Vector3;
    public let maxPlayers: Int32;
}

public struct ApartmentConfig {
    public let id: String;
    public let name: String;
    public let description: String;
    public let entrancePosition: Vector3;
    public let interiorPosition: Vector3;
    public let maxPlayers: Int32;
    public let purchaseCost: Int32;
    public let defaultPermissions: ApartmentPermissions;
}

public struct StoreConfig {
    public let id: String;
    public let name: String;
    public let description: String;
    public let entrancePosition: Vector3;
    public let interiorPosition: Vector3;
    public let maxPlayers: Int32;
    public let storeType: String;
    public let operatingHours: String;
}

// Native function declarations
native func Net_SendInstanceUpdate(instanceData: InstanceUpdateData) -> Void;
native func Net_SendApartmentPermissions(permissionData: ApartmentPermissionData) -> Void;
native func Net_SendLocationCreation(creationData: LocationCreationData) -> Void;
native func Net_SendPlayerInstanceTeleport(teleportData: PlayerInstanceTeleport) -> Void;
native func Net_SendInstancePlayerNotification(notificationData: InstancePlayerNotification) -> Void;

// Network callbacks
@addMethod(PlayerPuppet)
public func OnNetworkInstanceUpdate(instanceData: InstanceUpdateData) -> Void {
    CustomLocationManager.GetInstance().OnRemoteInstanceUpdate(instanceData);
}

@addMethod(PlayerPuppet)
public func OnNetworkLocationCreation(creationData: LocationCreationData) -> Void {
    CustomLocationManager.GetInstance().OnRemoteLocationCreation(creationData);
}