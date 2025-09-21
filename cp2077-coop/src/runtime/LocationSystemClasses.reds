// Location System Supporting Classes
// Custom apartments, stores, instances, and location database components

// Custom Apartment Class
public class CustomApartment extends ScriptableComponent {
    private let m_id: String;
    private let m_name: String;
    private let m_description: String;
    private let m_entrancePosition: Vector3;
    private let m_interiorPosition: Vector3;
    private let m_maxPlayers: Int32;
    private let m_owners: array<Uint32>; // Multiple owners supported
    private let m_permissions: ApartmentPermissions;
    private let m_purchaseCost: Int32;
    private let m_isFromSingleplayer: Bool;

    public func InitializeFromConfig(config: ApartmentConfig) -> Void {
        this.m_id = config.id;
        this.m_name = config.name;
        this.m_description = config.description;
        this.m_entrancePosition = config.entrancePosition;
        this.m_interiorPosition = config.interiorPosition;
        this.m_maxPlayers = config.maxPlayers;
        this.m_permissions = config.defaultPermissions;
        this.m_purchaseCost = config.purchaseCost;
        this.m_isFromSingleplayer = false;
    }

    public func InitializeFromGameData(apartmentData: ApartmentData) -> Void {
        // Initialize from existing singleplayer apartment data
        this.m_id = apartmentData.id;
        this.m_name = apartmentData.displayName;
        this.m_description = apartmentData.description;
        this.m_entrancePosition = apartmentData.entrancePosition;
        this.m_interiorPosition = apartmentData.interiorSpawnPoint;
        this.m_maxPlayers = 8; // Default for singleplayer apartments
        this.m_isFromSingleplayer = true;

        // Set default permissions for singleplayer apartments
        let defaultPermissions: ApartmentPermissions;
        defaultPermissions.allowFriends = true;
        defaultPermissions.allowGuildMembers = false;
        defaultPermissions.allowPublic = false;
        this.m_permissions = defaultPermissions;
    }

    public func Update() -> Void {
        // Update apartment state if needed
    }

    public func CanPlayerEnter(playerId: Uint32, requestedOwnerId: Uint32) -> Bool {
        // If player is an owner, they can always enter
        if this.IsOwner(playerId) {
            return true;
        }

        // If requesting entry to specific owner's instance
        if requestedOwnerId != 0u {
            if !this.IsOwner(requestedOwnerId) {
                return false;
            }

            // Check permissions for that owner's instance
            return this.CheckPlayerPermissions(playerId, requestedOwnerId);
        }

        return false;
    }

    private func CheckPlayerPermissions(playerId: Uint32, ownerId: Uint32) -> Bool {
        // Check if player is blocked
        for blockedId in this.m_permissions.blockedPlayers {
            if blockedId == playerId {
                return false;
            }
        }

        // Check if player is explicitly allowed
        for allowedId in this.m_permissions.allowedPlayers {
            if allowedId == playerId {
                return true;
            }
        }

        // Check friend permissions
        if this.m_permissions.allowFriends && this.ArePlayersFriends(ownerId, playerId) {
            return true;
        }

        // Check guild permissions
        if this.m_permissions.allowGuildMembers && this.ArePlayersInSameGuild(ownerId, playerId) {
            return true;
        }

        // Check public access
        return this.m_permissions.allowPublic;
    }

    private func ArePlayersFriends(playerId1: Uint32, playerId2: Uint32) -> Bool {
        // This would check the friends system
        return false; // Placeholder
    }

    private func ArePlayersInSameGuild(playerId1: Uint32, playerId2: Uint32) -> Bool {
        // This would check the guild system
        return false; // Placeholder
    }

    public func AddOwner(playerId: Uint32) -> Bool {
        if this.IsOwner(playerId) {
            return false;
        }

        ArrayPush(this.m_owners, playerId);
        return true;
    }

    public func RemoveOwner(playerId: Uint32) -> Bool {
        return ArrayRemove(this.m_owners, playerId);
    }

    public func IsOwner(playerId: Uint32) -> Bool {
        for ownerId in this.m_owners {
            if ownerId == playerId {
                return true;
            }
        }
        return false;
    }

    public func SetPermissions(permissions: ApartmentPermissions) -> Void {
        this.m_permissions = permissions;
    }

    // Getters
    public func GetId() -> String { return this.m_id; }
    public func GetName() -> String { return this.m_name; }
    public func GetDescription() -> String { return this.m_description; }
    public func GetEntrancePosition() -> Vector3 { return this.m_entrancePosition; }
    public func GetInteriorPosition() -> Vector3 { return this.m_interiorPosition; }
    public func GetMaxPlayers() -> Int32 { return this.m_maxPlayers; }
    public func GetOwners() -> array<Uint32> { return this.m_owners; }
    public func GetPermissions() -> ApartmentPermissions { return this.m_permissions; }
    public func GetPurchaseCost() -> Int32 { return this.m_purchaseCost; }
    public func IsFromSingleplayer() -> Bool { return this.m_isFromSingleplayer; }
}

// Custom Store Class
public class CustomStore extends ScriptableComponent {
    private let m_id: String;
    private let m_name: String;
    private let m_description: String;
    private let m_entrancePosition: Vector3;
    private let m_interiorPosition: Vector3;
    private let m_maxPlayers: Int32;
    private let m_storeType: String;
    private let m_operatingHours: String;
    private let m_isOpen: Bool;
    private let m_isFromSingleplayer: Bool;

    public func InitializeFromConfig(config: StoreConfig) -> Void {
        this.m_id = config.id;
        this.m_name = config.name;
        this.m_description = config.description;
        this.m_entrancePosition = config.entrancePosition;
        this.m_interiorPosition = config.interiorPosition;
        this.m_maxPlayers = config.maxPlayers;
        this.m_storeType = config.storeType;
        this.m_operatingHours = config.operatingHours;
        this.m_isOpen = true;
        this.m_isFromSingleplayer = false;
    }

    public func InitializeFromGameData(vendorData: VendorData) -> Void {
        // Initialize from existing singleplayer vendor data
        this.m_id = vendorData.id;
        this.m_name = vendorData.displayName;
        this.m_description = vendorData.description;
        this.m_entrancePosition = vendorData.position;
        this.m_interiorPosition = vendorData.interiorSpawnPoint;
        this.m_maxPlayers = 10; // Default for stores
        this.m_storeType = vendorData.vendorType;
        this.m_operatingHours = "24/7"; // Most game stores are always open
        this.m_isOpen = vendorData.isActive;
        this.m_isFromSingleplayer = true;
    }

    public func Update() -> Void {
        // Update store state (opening hours, etc.)
        this.UpdateOperatingStatus();
    }

    private func UpdateOperatingStatus() -> Void {
        // Check if store should be open based on operating hours
        // For singleplayer stores, follow their existing logic
        if this.m_isFromSingleplayer {
            // Use singleplayer store opening logic
        } else {
            // Use custom operating hours logic
            this.m_isOpen = this.IsWithinOperatingHours();
        }
    }

    private func IsWithinOperatingHours() -> Bool {
        // Parse operating hours and check current game time
        // For now, custom stores are always open
        return true; // Placeholder
    }

    // Getters
    public func GetId() -> String { return this.m_id; }
    public func GetName() -> String { return this.m_name; }
    public func GetDescription() -> String { return this.m_description; }
    public func GetEntrancePosition() -> Vector3 { return this.m_entrancePosition; }
    public func GetInteriorPosition() -> Vector3 { return this.m_interiorPosition; }
    public func GetMaxPlayers() -> Int32 { return this.m_maxPlayers; }
    public func GetStoreType() -> String { return this.m_storeType; }
    public func IsOpen() -> Bool { return this.m_isOpen; }
    public func IsFromSingleplayer() -> Bool { return this.m_isFromSingleplayer; }
}

// Custom Location Class
public class CustomLocation extends ScriptableComponent {
    private let m_id: String;
    private let m_name: String;
    private let m_description: String;
    private let m_entrancePosition: Vector3;
    private let m_interiorPosition: Vector3;
    private let m_maxPlayers: Int32;
    private let m_locationType: String;
    private let m_isFromSingleplayer: Bool;
    private let m_accessLevel: ELocationAccessLevel;

    public func InitializeFromConfig(config: LocationConfig) -> Void {
        this.m_id = config.id;
        this.m_name = config.name;
        this.m_description = config.description;
        this.m_entrancePosition = config.entrancePosition;
        this.m_interiorPosition = config.interiorPosition;
        this.m_maxPlayers = config.maxPlayers;
        this.m_isFromSingleplayer = false;
        this.m_accessLevel = ELocationAccessLevel.Public;
    }

    public func InitializeFromGameData(locationData: NotableLocationData) -> Void {
        // Initialize from existing singleplayer location data
        this.m_id = locationData.id;
        this.m_name = locationData.displayName;
        this.m_description = locationData.description;
        this.m_entrancePosition = locationData.entrancePosition;
        this.m_interiorPosition = locationData.interiorSpawnPoint;
        this.m_maxPlayers = 15; // Default for notable locations
        this.m_locationType = locationData.locationType;
        this.m_isFromSingleplayer = true;
        this.m_accessLevel = ELocationAccessLevel.Public;
    }

    public func Update() -> Void {
        // Update location state if needed
    }

    // Getters
    public func GetId() -> String { return this.m_id; }
    public func GetName() -> String { return this.m_name; }
    public func GetDescription() -> String { return this.m_description; }
    public func GetEntrancePosition() -> Vector3 { return this.m_entrancePosition; }
    public func GetInteriorPosition() -> Vector3 { return this.m_interiorPosition; }
    public func GetMaxPlayers() -> Int32 { return this.m_maxPlayers; }
    public func GetLocationType() -> String { return this.m_locationType; }
    public func IsFromSingleplayer() -> Bool { return this.m_isFromSingleplayer; }
}

// Instanced Space Class
public class InstancedSpace extends ScriptableComponent {
    private let m_instanceId: String;
    private let m_locationId: String;
    private let m_instanceType: EInstanceType;
    private let m_ownerId: Uint32; // 0 for shared spaces
    private let m_players: array<Uint32>;
    private let m_maxPlayers: Int32;
    private let m_spawnPoint: Vector3;
    private let m_exitLocation: Vector3;
    private let m_creationTime: Float;
    private let m_lastActivityTime: Float;
    private let m_hasStateChanged: Bool;

    public func Initialize(locationId: String, instanceType: EInstanceType, ownerId: Uint32) -> Void {
        this.m_instanceId = this.GenerateInstanceId();
        this.m_locationId = locationId;
        this.m_instanceType = instanceType;
        this.m_ownerId = ownerId;
        this.m_maxPlayers = 8; // Default
        this.m_creationTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        this.m_lastActivityTime = this.m_creationTime;
        this.m_hasStateChanged = true;
    }

    private func GenerateInstanceId() -> String {
        // Generate unique instance ID
        let timestamp = Cast<Int64>(EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance())) * 1000.0);
        return "instance_" + ToString(timestamp);
    }

    public func Update() -> Void {
        // Update instance state
        if ArraySize(this.m_players) > 0 {
            this.m_lastActivityTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        }
    }

    public func AddPlayer(playerId: Uint32) -> EInstanceEntryResult {
        if ArraySize(this.m_players) >= this.m_maxPlayers {
            return EInstanceEntryResult.Full;
        }

        if this.HasPlayer(playerId) {
            return EInstanceEntryResult.Success; // Already in instance
        }

        ArrayPush(this.m_players, playerId);
        this.m_hasStateChanged = true;
        this.m_lastActivityTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        return EInstanceEntryResult.Success;
    }

    public func RemovePlayer(playerId: Uint32) -> Bool {
        let removed = ArrayRemove(this.m_players, playerId);
        if removed {
            this.m_hasStateChanged = true;
            if ArraySize(this.m_players) > 0 {
                this.m_lastActivityTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
            }
        }
        return removed;
    }

    public func HasPlayer(playerId: Uint32) -> Bool {
        for id in this.m_players {
            if id == playerId {
                return true;
            }
        }
        return false;
    }

    public func IsEmpty() -> Bool {
        return ArraySize(this.m_players) == 0;
    }

    public func HasBeenEmptyForMinutes(minutes: Float) -> Bool {
        if !this.IsEmpty() {
            return false;
        }

        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        let emptyDuration = currentTime - this.m_lastActivityTime;
        return emptyDuration >= (minutes * 60.0);
    }

    public func UpdateFromRemote(instanceData: InstanceUpdateData) -> Void {
        this.m_players = instanceData.playerIds;
        this.m_lastActivityTime = instanceData.timestamp;
    }

    public func GetSyncData() -> InstanceUpdateData {
        let syncData: InstanceUpdateData;
        syncData.instanceId = this.m_instanceId;
        syncData.locationId = this.m_locationId;
        syncData.instanceType = this.m_instanceType;
        syncData.ownerId = this.m_ownerId;
        syncData.playerIds = this.m_players;
        syncData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        return syncData;
    }

    // Getters
    public func GetInstanceId() -> String { return this.m_instanceId; }
    public func GetLocationId() -> String { return this.m_locationId; }
    public func GetInstanceType() -> EInstanceType { return this.m_instanceType; }
    public func GetOwnerId() -> Uint32 { return this.m_ownerId; }
    public func GetPlayers() -> array<Uint32> { return this.m_players; }
    public func GetPlayerCount() -> Int32 { return ArraySize(this.m_players); }
    public func GetMaxPlayers() -> Int32 { return this.m_maxPlayers; }
    public func GetSpawnPoint() -> Vector3 { return this.m_spawnPoint; }
    public func GetExitLocation() -> Vector3 { return this.m_exitLocation; }
    public func HasStateChanged() -> Bool { return this.m_hasStateChanged; }
    public func MarkSynced() -> Void { this.m_hasStateChanged = false; }
}

// Apartment Instance Class (specialized instanced space for apartments)
public class ApartmentInstance extends InstancedSpace {
    private let m_apartmentId: String;

    public func Initialize(apartmentId: String, ownerId: Uint32) -> Void {
        this.m_apartmentId = apartmentId;
        super.Initialize(apartmentId, EInstanceType.Apartment, ownerId);
    }

    public func GetApartmentId() -> String { return this.m_apartmentId; }
}

// Location Database Class
public class LocationDatabase extends ScriptableComponent {
    private let m_allLocations: array<ref<LocationEntry>>;
    private let m_locationsByType: array<ref<LocationTypeGroup>>;

    public func Initialize() -> Void {
        // Initialize location database
        this.InitializeLocationTypes();
    }

    private func InitializeLocationTypes() -> Void {
        // Create location type groups
        let apartmentGroup = new LocationTypeGroup();
        apartmentGroup.Initialize(ELocationType.Apartment);
        ArrayPush(this.m_locationsByType, apartmentGroup);

        let storeGroup = new LocationTypeGroup();
        storeGroup.Initialize(ELocationType.Store);
        ArrayPush(this.m_locationsByType, storeGroup);

        let customGroup = new LocationTypeGroup();
        customGroup.Initialize(ELocationType.Custom);
        ArrayPush(this.m_locationsByType, customGroup);

        let notableGroup = new LocationTypeGroup();
        notableGroup.Initialize(ELocationType.Notable);
        ArrayPush(this.m_locationsByType, notableGroup);
    }

    public func RegisterLocation(locationId: String, locationType: ELocationType, position: Vector3, name: String) -> Void {
        let entry = new LocationEntry();
        entry.Initialize(locationId, locationType, position, name);
        ArrayPush(this.m_allLocations, entry);

        // Add to type-specific group
        let typeGroup = this.GetLocationTypeGroup(locationType);
        if IsDefined(typeGroup) {
            typeGroup.AddLocation(entry);
        }
    }

    private func GetLocationTypeGroup(locationType: ELocationType) -> ref<LocationTypeGroup> {
        for group in this.m_locationsByType {
            if Equals(group.GetLocationType(), locationType) {
                return group;
            }
        }
        return null;
    }

    public func FindLocation(locationId: String) -> ref<LocationEntry> {
        for location in this.m_allLocations {
            if Equals(location.GetId(), locationId) {
                return location;
            }
        }
        return null;
    }

    public func GetLocationsByType(locationType: ELocationType) -> array<ref<LocationEntry>> {
        let typeGroup = this.GetLocationTypeGroup(locationType);
        if IsDefined(typeGroup) {
            return typeGroup.GetLocations();
        }
        return [];
    }

    public func GetNearbyLocations(position: Vector3, radius: Float) -> array<ref<LocationEntry>> {
        let nearbyLocations: array<ref<LocationEntry>>;

        for location in this.m_allLocations {
            let distance = Vector4.Distance(position, location.GetPosition());
            if distance <= radius {
                ArrayPush(nearbyLocations, location);
            }
        }

        return nearbyLocations;
    }
}

// Location Entry Class
public class LocationEntry extends ScriptableComponent {
    private let m_id: String;
    private let m_locationType: ELocationType;
    private let m_position: Vector3;
    private let m_name: String;
    private let m_isActive: Bool;

    public func Initialize(id: String, locationType: ELocationType, position: Vector3, name: String) -> Void {
        this.m_id = id;
        this.m_locationType = locationType;
        this.m_position = position;
        this.m_name = name;
        this.m_isActive = true;
    }

    // Getters
    public func GetId() -> String { return this.m_id; }
    public func GetLocationType() -> ELocationType { return this.m_locationType; }
    public func GetPosition() -> Vector3 { return this.m_position; }
    public func GetName() -> String { return this.m_name; }
    public func IsActive() -> Bool { return this.m_isActive; }
}

// Location Type Group Class
public class LocationTypeGroup extends ScriptableComponent {
    private let m_locationType: ELocationType;
    private let m_locations: array<ref<LocationEntry>>;

    public func Initialize(locationType: ELocationType) -> Void {
        this.m_locationType = locationType;
    }

    public func AddLocation(location: ref<LocationEntry>) -> Void {
        ArrayPush(this.m_locations, location);
    }

    public func RemoveLocation(locationId: String) -> Bool {
        let index = -1;
        for i in Range(ArraySize(this.m_locations)) {
            if Equals(this.m_locations[i].GetId(), locationId) {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_locations, this.m_locations[index]);
            return true;
        }
        return false;
    }

    // Getters
    public func GetLocationType() -> ELocationType { return this.m_locationType; }
    public func GetLocations() -> array<ref<LocationEntry>> { return this.m_locations; }
}

// Additional Enums
public enum ELocationAccessLevel : Uint8 {
    Public = 0,
    Friends = 1,
    Guild = 2,
    Private = 3,
    Admin = 4
}

// Additional Data Structures
public struct ApartmentData {
    public let id: String;
    public let displayName: String;
    public let description: String;
    public let entrancePosition: Vector3;
    public let interiorSpawnPoint: Vector3;
}

public struct VendorData {
    public let id: String;
    public let displayName: String;
    public let description: String;
    public let position: Vector3;
    public let interiorSpawnPoint: Vector3;
    public let vendorType: String;
    public let isActive: Bool;
}

public struct NotableLocationData {
    public let id: String;
    public let displayName: String;
    public let description: String;
    public let entrancePosition: Vector3;
    public let interiorSpawnPoint: Vector3;
    public let locationType: String;
}

public struct InstanceUpdateData {
    public let instanceId: String;
    public let locationId: String;
    public let instanceType: EInstanceType;
    public let ownerId: Uint32;
    public let playerIds: array<Uint32>;
    public let timestamp: Float;
}

public struct ApartmentPermissionData {
    public let apartmentId: String;
    public let ownerId: Uint32;
    public let permissions: ApartmentPermissions;
    public let timestamp: Float;
}

public struct LocationCreationData {
    public let locationType: ELocationType;
    public let locationId: String;
    public let config: String; // Serialized config
    public let creatorId: Uint32;
    public let timestamp: Float;
}

public struct PlayerInstanceTeleport {
    public let playerId: Uint32;
    public let instanceId: String;
    public let spawnPoint: Vector3;
    public let isEntering: Bool;
    public let timestamp: Float;
}

public struct InstancePlayerNotification {
    public let instanceId: String;
    public let playerId: Uint32;
    public let isJoining: Bool;
    public let timestamp: Float;
}