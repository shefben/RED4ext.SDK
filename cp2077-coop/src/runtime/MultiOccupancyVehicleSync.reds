// Multi-Occupancy Vehicle Synchronization System
// Handles multiple players in vehicles with proper seat assignments and synchronized interactions

// Multi-Occupancy Vehicle Manager
public class MultiOccupancyVehicleManager extends ScriptableSystem {
    private static let s_instance: ref<MultiOccupancyVehicleManager>;
    private let m_occupiedVehicles: array<ref<OccupiedVehicleState>>;
    private let m_seatReservations: array<ref<SeatReservation>>;
    private let m_syncTimer: Float = 0.0;
    private let m_occupancySyncInterval: Float = 0.1; // 10 FPS for occupancy updates
    private let m_localPlayer: wref<PlayerPuppet>;

    public static func GetInstance() -> ref<MultiOccupancyVehicleManager> {
        if !IsDefined(MultiOccupancyVehicleManager.s_instance) {
            MultiOccupancyVehicleManager.s_instance = new MultiOccupancyVehicleManager();
        }
        return MultiOccupancyVehicleManager.s_instance;
    }

    public func Initialize(player: ref<PlayerPuppet>) -> Void {
        this.m_localPlayer = player;
        LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Multi-Occupancy Vehicle Manager initialized");
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        this.m_syncTimer += deltaTime;

        if this.m_syncTimer >= this.m_occupancySyncInterval {
            this.SynchronizeOccupancyStates();
            this.ProcessSeatReservations();
            this.m_syncTimer = 0.0;
        }
    }

    private func SynchronizeOccupancyStates() -> Void {
        for vehicleState in this.m_occupiedVehicles {
            if vehicleState.HasOccupancyChanged() {
                let occupancyData = vehicleState.GetOccupancySyncData();
                Net_SendVehicleOccupancyUpdate(occupancyData);
                vehicleState.MarkOccupancySynced();
            }
        }
    }

    private func ProcessSeatReservations() -> Void {
        let expiredReservations: array<ref<SeatReservation>>;

        for reservation in this.m_seatReservations {
            if reservation.IsExpired() {
                ArrayPush(expiredReservations, reservation);
            }
        }

        for expiredReservation in expiredReservations {
            ArrayRemove(this.m_seatReservations, expiredReservation);
            this.NotifySeatReservationExpired(expiredReservation);
        }
    }

    // Player enters vehicle
    public func OnPlayerEnterVehicle(playerId: Uint32, vehicleId: Uint64, seatIndex: Int32) -> Bool {
        let vehicleState = this.FindOrCreateVehicleState(vehicleId);

        // Check if seat is available
        if !vehicleState.IsSeatAvailable(seatIndex) {
            LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Seat " + seatIndex + " in vehicle " + vehicleId + " is occupied");
            return false;
        }

        // Remove any existing reservation
        this.RemoveSeatReservation(vehicleId, seatIndex);

        // Assign seat to player
        vehicleState.AssignSeat(playerId, seatIndex);

        // Sync to other players
        let enterData: VehicleEnterData;
        enterData.playerId = playerId;
        enterData.vehicleId = vehicleId;
        enterData.seatIndex = seatIndex;
        enterData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendPlayerEnterVehicle(enterData);

        LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Player " + playerId + " entered vehicle " + vehicleId + " seat " + seatIndex);
        return true;
    }

    // Player exits vehicle
    public func OnPlayerExitVehicle(playerId: Uint32, vehicleId: Uint64) -> Bool {
        let vehicleState = this.FindVehicleState(vehicleId);
        if !IsDefined(vehicleState) {
            return false;
        }

        let seatIndex = vehicleState.GetPlayerSeat(playerId);
        if seatIndex == -1 {
            return false;
        }

        // Remove player from seat
        vehicleState.RemovePlayer(playerId);

        // Sync to other players
        let exitData: VehicleExitData;
        exitData.playerId = playerId;
        exitData.vehicleId = vehicleId;
        exitData.seatIndex = seatIndex;
        exitData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        Net_SendPlayerExitVehicle(exitData);

        // Clean up if vehicle is empty
        if vehicleState.IsEmpty() {
            this.RemoveVehicleState(vehicleId);
        }

        LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Player " + playerId + " exited vehicle " + vehicleId);
        return true;
    }

    // Reserve seat for incoming player
    public func RequestSeatReservation(playerId: Uint32, vehicleId: Uint64, preferredSeat: Int32) -> ESeatReservationResult {
        let vehicleState = this.FindOrCreateVehicleState(vehicleId);

        // Check if preferred seat is available
        if preferredSeat != -1 && vehicleState.IsSeatAvailable(preferredSeat) {
            let reservation = new SeatReservation();
            reservation.Initialize(playerId, vehicleId, preferredSeat, 5.0); // 5 second reservation
            ArrayPush(this.m_seatReservations, reservation);

            let reservationData: SeatReservationData;
            reservationData.playerId = playerId;
            reservationData.vehicleId = vehicleId;
            reservationData.seatIndex = preferredSeat;
            reservationData.reservationDuration = 5.0;
            reservationData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

            Net_SendSeatReservation(reservationData);

            LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Seat " + preferredSeat + " reserved for player " + playerId);
            return ESeatReservationResult.Success;
        }

        // Find alternative seat
        let availableSeat = vehicleState.FindAvailableSeat();
        if availableSeat != -1 {
            let reservation = new SeatReservation();
            reservation.Initialize(playerId, vehicleId, availableSeat, 5.0);
            ArrayPush(this.m_seatReservations, reservation);

            let reservationData: SeatReservationData;
            reservationData.playerId = playerId;
            reservationData.vehicleId = vehicleId;
            reservationData.seatIndex = availableSeat;
            reservationData.reservationDuration = 5.0;
            reservationData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

            Net_SendSeatReservation(reservationData);

            LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Alternative seat " + availableSeat + " reserved for player " + playerId);
            return ESeatReservationResult.AlternativeSeat;
        }

        return ESeatReservationResult.VehicleFull;
    }

    // Network event handlers
    public func OnRemotePlayerEnterVehicle(enterData: VehicleEnterData) -> Void {
        let vehicleState = this.FindOrCreateVehicleState(enterData.vehicleId);
        vehicleState.AssignSeat(enterData.playerId, enterData.seatIndex);

        // Update visual representation for remote player
        this.UpdateRemotePlayerVehicleState(enterData.playerId, enterData.vehicleId, true);

        LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Remote player " + enterData.playerId + " entered vehicle " + enterData.vehicleId);
    }

    public func OnRemotePlayerExitVehicle(exitData: VehicleExitData) -> Void {
        let vehicleState = this.FindVehicleState(exitData.vehicleId);
        if IsDefined(vehicleState) {
            vehicleState.RemovePlayer(exitData.playerId);

            // Update visual representation for remote player
            this.UpdateRemotePlayerVehicleState(exitData.playerId, exitData.vehicleId, false);

            if vehicleState.IsEmpty() {
                this.RemoveVehicleState(exitData.vehicleId);
            }
        }

        LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Remote player " + exitData.playerId + " exited vehicle " + exitData.vehicleId);
    }

    public func OnRemoteSeatReservation(reservationData: SeatReservationData) -> Void {
        let reservation = new SeatReservation();
        reservation.Initialize(reservationData.playerId, reservationData.vehicleId,
                             reservationData.seatIndex, reservationData.reservationDuration);
        ArrayPush(this.m_seatReservations, reservation);

        LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Remote seat reservation: Player " + reservationData.playerId + " reserved seat " + reservationData.seatIndex);
    }

    public func OnRemoteVehicleOccupancyUpdate(occupancyData: VehicleOccupancyUpdateData) -> Void {
        let vehicleState = this.FindOrCreateVehicleState(occupancyData.vehicleId);
        vehicleState.UpdateFromRemote(occupancyData);
    }

    private func UpdateRemotePlayerVehicleState(playerId: Uint32, vehicleId: Uint64, isEntering: Bool) -> Void {
        // This would update the visual state of remote players in vehicles
        // Including animations, positioning, and vehicle control delegation
    }

    private func FindOrCreateVehicleState(vehicleId: Uint64) -> ref<OccupiedVehicleState> {
        let vehicleState = this.FindVehicleState(vehicleId);
        if !IsDefined(vehicleState) {
            vehicleState = new OccupiedVehicleState();
            vehicleState.Initialize(vehicleId);
            ArrayPush(this.m_occupiedVehicles, vehicleState);
        }
        return vehicleState;
    }

    private func FindVehicleState(vehicleId: Uint64) -> ref<OccupiedVehicleState> {
        for vehicleState in this.m_occupiedVehicles {
            if vehicleState.GetVehicleId() == vehicleId {
                return vehicleState;
            }
        }
        return null;
    }

    private func RemoveVehicleState(vehicleId: Uint64) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_occupiedVehicles)) {
            if this.m_occupiedVehicles[i].GetVehicleId() == vehicleId {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_occupiedVehicles, this.m_occupiedVehicles[index]);
        }
    }

    private func RemoveSeatReservation(vehicleId: Uint64, seatIndex: Int32) -> Void {
        let toRemove: array<ref<SeatReservation>>;

        for reservation in this.m_seatReservations {
            if reservation.GetVehicleId() == vehicleId && reservation.GetSeatIndex() == seatIndex {
                ArrayPush(toRemove, reservation);
            }
        }

        for reservation in toRemove {
            ArrayRemove(this.m_seatReservations, reservation);
        }
    }

    private func NotifySeatReservationExpired(reservation: ref<SeatReservation>) -> Void {
        LogChannel(n"VehicleOccupancy", s"[VehicleOccupancy] Seat reservation expired: Player " + reservation.GetPlayerId() + " Vehicle " + reservation.GetVehicleId() + " Seat " + reservation.GetSeatIndex());
    }

    // Public API
    public func GetVehicleOccupancy(vehicleId: Uint64) -> array<Uint32> {
        let vehicleState = this.FindVehicleState(vehicleId);
        if IsDefined(vehicleState) {
            return vehicleState.GetOccupants();
        }
        return [];
    }

    public func IsVehicleOccupied(vehicleId: Uint64) -> Bool {
        let vehicleState = this.FindVehicleState(vehicleId);
        return IsDefined(vehicleState) && !vehicleState.IsEmpty();
    }

    public func GetDriverId(vehicleId: Uint64) -> Uint32 {
        let vehicleState = this.FindVehicleState(vehicleId);
        if IsDefined(vehicleState) {
            return vehicleState.GetDriver();
        }
        return 0u;
    }

    public func GetAvailableSeats(vehicleId: Uint64) -> array<Int32> {
        let vehicleState = this.FindVehicleState(vehicleId);
        if IsDefined(vehicleState) {
            return vehicleState.GetAvailableSeats();
        }
        return [];
    }
}

// Occupied Vehicle State Tracker
public class OccupiedVehicleState extends ScriptableComponent {
    private let m_vehicleId: Uint64;
    private let m_seatAssignments: array<SeatAssignment>; // Index = seat number, value = player ID (0 = empty)
    private let m_maxSeats: Int32;
    private let m_driverId: Uint32;
    private let m_hasOccupancyChanged: Bool;

    public func Initialize(vehicleId: Uint64) -> Void {
        this.m_vehicleId = vehicleId;
        this.m_maxSeats = this.DetermineMaxSeats(vehicleId);
        this.InitializeSeatAssignments();
        this.m_driverId = 0u;
        this.m_hasOccupancyChanged = false;
    }

    private func DetermineMaxSeats(vehicleId: Uint64) -> Int32 {
        // Determine vehicle seat capacity from vehicle data
        // This would query the vehicle's record for seat count
        return 4; // Default to 4 seats (driver + 3 passengers)
    }

    private func InitializeSeatAssignments() -> Void {
        // Initialize all seats as empty
        for i in Range(this.m_maxSeats) {
            let assignment: SeatAssignment;
            assignment.seatIndex = i;
            assignment.playerId = 0u;
            assignment.isOccupied = false;
            assignment.seatType = this.GetSeatType(i);
            ArrayPush(this.m_seatAssignments, assignment);
        }
    }

    private func GetSeatType(seatIndex: Int32) -> ESeatType {
        switch seatIndex {
            case 0: return ESeatType.Driver;
            case 1: return ESeatType.Passenger;
            default: return ESeatType.Rear;
        }
    }

    public func AssignSeat(playerId: Uint32, seatIndex: Int32) -> Bool {
        if seatIndex < 0 || seatIndex >= this.m_maxSeats {
            return false;
        }

        if this.m_seatAssignments[seatIndex].isOccupied {
            return false;
        }

        this.m_seatAssignments[seatIndex].playerId = playerId;
        this.m_seatAssignments[seatIndex].isOccupied = true;

        // Set driver if assigning driver seat
        if seatIndex == 0 {
            this.m_driverId = playerId;
        }

        this.m_hasOccupancyChanged = true;
        return true;
    }

    public func RemovePlayer(playerId: Uint32) -> Bool {
        let removed = false;

        for assignment in this.m_seatAssignments {
            if assignment.playerId == playerId {
                assignment.playerId = 0u;
                assignment.isOccupied = false;

                // Clear driver if removing driver
                if assignment.seatIndex == 0 {
                    this.m_driverId = 0u;
                }

                removed = true;
                this.m_hasOccupancyChanged = true;
                break;
            }
        }

        return removed;
    }

    public func IsSeatAvailable(seatIndex: Int32) -> Bool {
        if seatIndex < 0 || seatIndex >= this.m_maxSeats {
            return false;
        }

        return !this.m_seatAssignments[seatIndex].isOccupied;
    }

    public func FindAvailableSeat() -> Int32 {
        for assignment in this.m_seatAssignments {
            if !assignment.isOccupied {
                return assignment.seatIndex;
            }
        }
        return -1; // No available seats
    }

    public func GetPlayerSeat(playerId: Uint32) -> Int32 {
        for assignment in this.m_seatAssignments {
            if assignment.playerId == playerId {
                return assignment.seatIndex;
            }
        }
        return -1; // Player not in vehicle
    }

    public func IsEmpty() -> Bool {
        for assignment in this.m_seatAssignments {
            if assignment.isOccupied {
                return false;
            }
        }
        return true;
    }

    public func UpdateFromRemote(occupancyData: VehicleOccupancyUpdateData) -> Void {
        // Update from remote occupancy data
        for i in Range(ArraySize(occupancyData.seatAssignments)) {
            if i < ArraySize(this.m_seatAssignments) {
                let remoteAssignment = occupancyData.seatAssignments[i];
                this.m_seatAssignments[i].playerId = remoteAssignment.playerId;
                this.m_seatAssignments[i].isOccupied = remoteAssignment.isOccupied;
            }
        }

        this.m_driverId = occupancyData.driverId;
    }

    public func GetOccupancySyncData() -> VehicleOccupancyUpdateData {
        let syncData: VehicleOccupancyUpdateData;
        syncData.vehicleId = this.m_vehicleId;
        syncData.seatAssignments = this.m_seatAssignments;
        syncData.driverId = this.m_driverId;
        syncData.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));

        return syncData;
    }

    // Getters
    public func GetVehicleId() -> Uint64 { return this.m_vehicleId; }
    public func GetDriver() -> Uint32 { return this.m_driverId; }
    public func GetMaxSeats() -> Int32 { return this.m_maxSeats; }
    public func HasOccupancyChanged() -> Bool { return this.m_hasOccupancyChanged; }
    public func MarkOccupancySynced() -> Void { this.m_hasOccupancyChanged = false; }

    public func GetOccupants() -> array<Uint32> {
        let occupants: array<Uint32>;
        for assignment in this.m_seatAssignments {
            if assignment.isOccupied {
                ArrayPush(occupants, assignment.playerId);
            }
        }
        return occupants;
    }

    public func GetAvailableSeats() -> array<Int32> {
        let availableSeats: array<Int32>;
        for assignment in this.m_seatAssignments {
            if !assignment.isOccupied {
                ArrayPush(availableSeats, assignment.seatIndex);
            }
        }
        return availableSeats;
    }
}

// Seat Reservation System
public class SeatReservation extends ScriptableComponent {
    private let m_playerId: Uint32;
    private let m_vehicleId: Uint64;
    private let m_seatIndex: Int32;
    private let m_expirationTime: Float;

    public func Initialize(playerId: Uint32, vehicleId: Uint64, seatIndex: Int32, duration: Float) -> Void {
        this.m_playerId = playerId;
        this.m_vehicleId = vehicleId;
        this.m_seatIndex = seatIndex;
        this.m_expirationTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance())) + duration;
    }

    public func IsExpired() -> Bool {
        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime(GetGameInstance()));
        return currentTime >= this.m_expirationTime;
    }

    // Getters
    public func GetPlayerId() -> Uint32 { return this.m_playerId; }
    public func GetVehicleId() -> Uint64 { return this.m_vehicleId; }
    public func GetSeatIndex() -> Int32 { return this.m_seatIndex; }
}

// Data Structures
public struct SeatAssignment {
    public let seatIndex: Int32;
    public let playerId: Uint32;
    public let isOccupied: Bool;
    public let seatType: ESeatType;
}

public struct VehicleEnterData {
    public let playerId: Uint32;
    public let vehicleId: Uint64;
    public let seatIndex: Int32;
    public let timestamp: Float;
}

public struct VehicleExitData {
    public let playerId: Uint32;
    public let vehicleId: Uint64;
    public let seatIndex: Int32;
    public let timestamp: Float;
}

public struct SeatReservationData {
    public let playerId: Uint32;
    public let vehicleId: Uint64;
    public let seatIndex: Int32;
    public let reservationDuration: Float;
    public let timestamp: Float;
}

public struct VehicleOccupancyUpdateData {
    public let vehicleId: Uint64;
    public let seatAssignments: array<SeatAssignment>;
    public let driverId: Uint32;
    public let timestamp: Float;
}

// Enumerations
public enum ESeatType : Uint8 {
    Driver = 0,
    Passenger = 1,
    Rear = 2,
    Special = 3
}

public enum ESeatReservationResult : Uint8 {
    Success = 0,
    AlternativeSeat = 1,
    VehicleFull = 2,
    InvalidSeat = 3,
    AlreadyReserved = 4
}

// Native function declarations
native func Net_SendPlayerEnterVehicle(enterData: VehicleEnterData) -> Void;
native func Net_SendPlayerExitVehicle(exitData: VehicleExitData) -> Void;
native func Net_SendSeatReservation(reservationData: SeatReservationData) -> Void;
native func Net_SendVehicleOccupancyUpdate(occupancyData: VehicleOccupancyUpdateData) -> Void;

// Integration with vehicle system
@wrapMethod(VehicleComponent)
protected cb func OnMountingEvent(evt: ref<MountingEvent>) -> Bool {
    let result = wrappedMethod(evt);

    if evt.isInstant && IsDefined(evt.character) {
        let player = evt.character as PlayerPuppet;
        if IsDefined(player) {
            let playerId = Cast<Uint32>(player.GetEntityID());
            let vehicleId = Cast<Uint64>(this.GetVehicle().GetEntityID());
            let seatIndex = Cast<Int32>(evt.slotId);

            let occupancyManager = MultiOccupancyVehicleManager.GetInstance();
            if IsDefined(occupancyManager) {
                occupancyManager.OnPlayerEnterVehicle(playerId, vehicleId, seatIndex);
            }
        }
    }

    return result;
}

@wrapMethod(VehicleComponent)
protected cb func OnUnmountingEvent(evt: ref<UnmountingEvent>) -> Bool {
    let result = wrappedMethod(evt);

    if evt.isInstant && IsDefined(evt.character) {
        let player = evt.character as PlayerPuppet;
        if IsDefined(player) {
            let playerId = Cast<Uint32>(player.GetEntityID());
            let vehicleId = Cast<Uint64>(this.GetVehicle().GetEntityID());

            let occupancyManager = MultiOccupancyVehicleManager.GetInstance();
            if IsDefined(occupancyManager) {
                occupancyManager.OnPlayerExitVehicle(playerId, vehicleId);
            }
        }
    }

    return result;
}

// Network callbacks
@addMethod(PlayerPuppet)
public func OnNetworkPlayerEnterVehicle(enterData: VehicleEnterData) -> Void {
    MultiOccupancyVehicleManager.GetInstance().OnRemotePlayerEnterVehicle(enterData);
}

@addMethod(PlayerPuppet)
public func OnNetworkPlayerExitVehicle(exitData: VehicleExitData) -> Void {
    MultiOccupancyVehicleManager.GetInstance().OnRemotePlayerExitVehicle(exitData);
}

@addMethod(PlayerPuppet)
public func OnNetworkSeatReservation(reservationData: SeatReservationData) -> Void {
    MultiOccupancyVehicleManager.GetInstance().OnRemoteSeatReservation(reservationData);
}

@addMethod(PlayerPuppet)
public func OnNetworkVehicleOccupancyUpdate(occupancyData: VehicleOccupancyUpdateData) -> Void {
    MultiOccupancyVehicleManager.GetInstance().OnRemoteVehicleOccupancyUpdate(occupancyData);
}