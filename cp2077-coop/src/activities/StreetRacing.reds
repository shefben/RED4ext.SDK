// Street racing circuits with betting and customization system

public enum RaceType {
    Circuit = 0,          // Closed loop circuit racing
    Sprint = 1,           // Point-to-point racing  
    Drift = 2,            // Drift competition
    Drag = 3,             // Straight line acceleration
    TimeAttack = 4,       // Solo time trial
    Elimination = 5,      // Last place eliminated each lap
    Pursuit = 6,          // Cops vs racers
    Freestyle = 7,        // Open world exploration race
    Demolition = 8        // Combat racing with weapons
}

public enum RaceStatus {
    Open = 0,             // Accepting participants
    Countdown = 1,        // Race starting soon
    InProgress = 2,       // Race is active
    Finished = 3,         // Race completed
    Cancelled = 4         // Race was cancelled
}

public enum VehicleClass {
    Street = 0,           // Standard street vehicles
    Sport = 1,            // Sports cars
    Super = 2,            // Supercars  
    Hyper = 3,            // Hypercars
    Motorcycle = 4,       // Motorcycles
    Truck = 5,            // Heavy vehicles
    Custom = 6,           // Custom/modified vehicles
    Nomad = 7            // Nomad clan vehicles
}

public struct StreetRace {
    public var raceId: String;
    public var raceName: String;
    public var raceType: RaceType;
    public var status: RaceStatus;
    public var circuitId: String;
    public var organizerId: String;
    public var maxParticipants: Int32;
    public var entryFee: Int32;
    public var prizePool: Int32;
    public var allowedVehicleClass: VehicleClass;
    public var startTime: Float;
    public var duration: Float;
    public var participants: array<RaceParticipant>;
    public var spectators: array<String>; // Player UUIDs watching
    public var bets: array<RaceBet>;
    public var rules: RaceRules;
    public var results: RaceResults;
    public var weather: String;
    public var timeOfDay: String;
}

public struct RaceParticipant {
    public var playerId: String;
    public var vehicleId: String;
    public var vehicleName: String;
    public var vehicleClass: VehicleClass;
    public var startPosition: Int32; // Grid position
    public var currentPosition: Int32;
    public var currentLap: Int32;
    public var bestLapTime: Float;
    public var totalTime: Float;
    public var isFinished: Bool;
    public var isDisqualified: Bool;
    public var penaltyTime: Float;
    public var checkpoints: array<Bool>; // Checkpoint completion status
    public var reputation: Int32; // Racing reputation
}

public struct RaceBet {
    public var betId: String;
    public var bettorId: String;
    public var targetPlayerId: String; // Who they're betting on
    public var betAmount: Int32;
    public var betType: String; // "win", "place", "show", "exact_finish"
    public var odds: Float;
    public var potentialPayout: Int32;
    public var isWinning: Bool;
}

public struct RaceRules {
    public var allowRespawn: Bool;
    public var allowRepairs: Bool;
    public var allowWeapons: Bool;
    public var damageEnabled: Bool;
    public var trafficEnabled: Bool;
    public var policeEnabled: Bool;
    public var maxLaps: Int32;
    public var maxDuration: Float; // Maximum race duration
    public var checkpointRequirement: Bool; // Must hit all checkpoints
    public var collisionPenalty: Float; // Time penalty for collisions
    public var shortcuts: Bool; // Allow shortcut routes
}

public struct RaceResults {
    public var isComplete: Bool;
    public var positions: array<String>; // Player IDs in finish order
    public var fastestLapTime: Float;
    public var fastestLapPlayer: String;
    public var totalPayouts: Int32;
    public var spectatorCount: Int32;
    public var averageSpeed: Float;
    public var crashCount: Int32;
    public var raceRating: Float; // 1-10 excitement rating
}

public struct RaceCircuit {
    public var circuitId: String;
    public var circuitName: String;
    public var description: String;
    public var district: String;
    public var raceType: RaceType;
    public var difficulty: Int32; // 1-10 difficulty
    public var checkpoints: array<Vector3>;
    public var startLine: Vector3;
    public var finishLine: Vector3;
    public var circuitLength: Float; // Meters
    public var recordTime: Float;
    public var recordHolder: String;
    public var surfaceType: String; // "asphalt", "dirt", "mixed"
    public var hazards: array<String>; // Environmental hazards
    public var recommendedClass: VehicleClass;
    public var isUnderground: Bool; // Underground illegal racing
    public var heatLevel: Int32; // Police attention level
}

public struct VehicleCustomization {
    public var vehicleId: String;
    public var ownerId: String;
    public var modifications: array<VehicleMod>;
    public var performance: VehiclePerformance;
    public var appearance: VehicleAppearance;
    public var totalValue: Int32;
    public var legalStatus: String; // "legal", "questionable", "illegal"
}

public struct VehicleMod {
    public var modType: String; // "engine", "turbo", "suspension", "tires", "armor"
    public var modName: String;
    public var performanceGain: Float;
    public var cost: Int32;
    public var legality: String;
    public var description: String;
}

public struct VehiclePerformance {
    public var acceleration: Float; // 0-100 rating
    public var topSpeed: Float;
    public var handling: Float;
    public var braking: Float;
    public var durability: Float;
    public var weight: Float;
    public var powerRating: Int32; // Horsepower equivalent
}

public struct VehicleAppearance {
    public var paintColor: String;
    public var paintFinish: String; // "matte", "metallic", "chrome", "neon"
    public var decals: array<String>;
    public var wheels: String;
    public var bodyKit: String;
    public var lighting: String; // Underglow, headlights, etc.
    public var windows: String; // Tinting level
}

public class StreetRacing {
    private static var isInitialized: Bool = false;
    private static var activeRaces: array<StreetRace>;
    private static var raceCircuits: array<RaceCircuit>;
    private static var playerVehicles: array<VehicleCustomization>;
    private static var racingUI: ref<StreetRacingUI>;
    private static var bettingUI: ref<RaceBettingUI>;
    private static var updateInterval: Float = 1.0; // Update every second during races
    private static var lastUpdateTime: Float = 0.0;
    
    // Network callbacks
    private static cb func OnRaceCreated(data: String) -> Void;
    private static cb func OnPlayerJoinRace(data: String) -> Void;
    private static cb func OnRaceStarted(data: String) -> Void;
    private static cb func OnCheckpointHit(data: String) -> Void;
    private static cb func OnRaceFinished(data: String) -> Void;
    private static cb func OnBetPlaced(data: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_RACING", "Initializing street racing system...");
        
        // Initialize race circuits
        StreetRacing.InitializeCircuits();
        
        // Initialize player vehicles
        StreetRacing.InitializePlayerVehicles();
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("race_created", StreetRacing.OnRaceCreated);
        NetworkingSystem.RegisterCallback("race_player_join", StreetRacing.OnPlayerJoinRace);
        NetworkingSystem.RegisterCallback("race_started", StreetRacing.OnRaceStarted);
        NetworkingSystem.RegisterCallback("race_checkpoint_hit", StreetRacing.OnCheckpointHit);
        NetworkingSystem.RegisterCallback("race_finished", StreetRacing.OnRaceFinished);
        NetworkingSystem.RegisterCallback("race_bet_placed", StreetRacing.OnBetPlaced);
        
        // Start update loop
        StreetRacing.StartUpdateLoop();
        
        isInitialized = true;
        LogChannel(n"COOP_RACING", "Street racing system initialized with " + ToString(ArraySize(raceCircuits)) + " circuits");
    }
    
    private static func InitializeCircuits() -> Void {
        ArrayClear(raceCircuits);
        
        // Watson District Circuits
        StreetRacing.CreateCircuit("watson_industrial", "Industrial Complex Circuit", "Watson", RaceType.Circuit, 6,
            4200.0, VehicleClass.Street, false, 3, "Weaving through industrial machinery and shipping containers");
        
        StreetRacing.CreateCircuit("watson_northside", "Northside Speedway", "Watson", RaceType.Sprint, 4,
            2800.0, VehicleClass.Sport, false, 2, "High-speed run through residential areas");
        
        // Westbrook District Circuits  
        StreetRacing.CreateCircuit("westbrook_japantown", "Neon Circuit", "Westbrook", RaceType.Circuit, 7,
            3600.0, VehicleClass.Super, true, 5, "Underground racing through glittering neon-lit streets");
        
        StreetRacing.CreateCircuit("westbrook_downtown", "Corporate Run", "Westbrook", RaceType.Sprint, 8,
            5200.0, VehicleClass.Hyper, false, 4, "Racing past corporate towers and luxury districts");
        
        // Heywood District Circuits
        StreetRacing.CreateCircuit("heywood_drift", "Barrio Drift Track", "Heywood", RaceType.Drift, 5,
            2400.0, VehicleClass.Sport, false, 3, "Tight corners perfect for drift competitions");
        
        StreetRacing.CreateCircuit("heywood_drag", "Valentino Strip", "Heywood", RaceType.Drag, 3,
            800.0, VehicleClass.Custom, true, 6, "Straight-line acceleration battle on gang territory");
        
        // Pacifica District Circuits
        StreetRacing.CreateCircuit("pacifica_coastal", "Coastal Highway", "Pacifica", RaceType.Circuit, 4,
            6800.0, VehicleClass.Street, false, 2, "Scenic coastal racing with ocean views");
        
        StreetRacing.CreateCircuit("pacifica_combat", "Wasteland Warrior", "Pacifica", RaceType.Demolition, 9,
            4500.0, VehicleClass.Truck, false, 7, "Combat racing through abandoned construction sites");
        
        // Santo Domingo District Circuits
        StreetRacing.CreateCircuit("santo_nomad", "Nomad Trails", "Santo Domingo", RaceType.Freestyle, 6,
            8200.0, VehicleClass.Nomad, false, 4, "Off-road racing through industrial wasteland");
        
        // Badlands Circuits
        StreetRacing.CreateCircuit("badlands_canyon", "Canyon Carver", "Badlands", RaceType.Circuit, 8,
            12400.0, VehicleClass.Sport, false, 5, "Dangerous mountain racing with steep drops");
        
        StreetRacing.CreateCircuit("badlands_pursuit", "Outlaw Chase", "Badlands", RaceType.Pursuit, 10,
            15600.0, VehicleClass.Custom, false, 8, "High-stakes chase racing with NCPD interference");
        
        LogChannel(n"COOP_RACING", "Initialized " + ToString(ArraySize(raceCircuits)) + " racing circuits");
    }
    
    private static func CreateCircuit(id: String, name: String, district: String, type: RaceType, difficulty: Int32, 
                                     length: Float, recommendedClass: VehicleClass, underground: Bool, heat: Int32, desc: String) -> Void {
        let circuit: RaceCircuit;
        circuit.circuitId = id;
        circuit.circuitName = name;
        circuit.description = desc;
        circuit.district = district;
        circuit.raceType = type;
        circuit.difficulty = difficulty;
        circuit.circuitLength = length;
        circuit.recommendedClass = recommendedClass;
        circuit.isUnderground = underground;
        circuit.heatLevel = heat;
        circuit.recordTime = 0.0; // No record yet
        circuit.recordHolder = "";
        
        // Generate checkpoints based on circuit type and length
        StreetRacing.GenerateCheckpoints(circuit);
        
        // Add hazards based on district and difficulty
        StreetRacing.AddCircuitHazards(circuit);
        
        ArrayPush(raceCircuits, circuit);
    }
    
    private static func GenerateCheckpoints(circuit: ref<RaceCircuit>) -> Void {
        let checkpointCount = Cast<Int32>(circuit.circuitLength / 1000.0) + 2; // Roughly every km + start/finish
        
        // Generate positions around circuit (simplified - real implementation would use actual map data)
        let centerX = RandRangeF(-2000.0, 2000.0);
        let centerY = RandRangeF(-2000.0, 2000.0);
        let radius = circuit.circuitLength / (2.0 * 3.14159);
        
        for i in Range(checkpointCount) {
            let angle = (Cast<Float>(i) / Cast<Float>(checkpointCount)) * 2.0 * 3.14159;
            let x = centerX + CosF(angle) * radius;
            let y = centerY + SinF(angle) * radius;
            let z = RandRangeF(20.0, 100.0); // Varying elevation
            
            ArrayPush(circuit.checkpoints, new Vector3(x, y, z));
        }
        
        // Set start and finish lines
        if ArraySize(circuit.checkpoints) > 0 {
            circuit.startLine = circuit.checkpoints[0];
            circuit.finishLine = circuit.checkpoints[ArraySize(circuit.checkpoints) - 1];
        }
    }
    
    private static func AddCircuitHazards(circuit: ref<RaceCircuit>) -> Void {
        // Add hazards based on district and difficulty
        if StrContains(circuit.district, "Badlands") {
            ArrayPush(circuit.hazards, "steep_drops");
            ArrayPush(circuit.hazards, "loose_rocks");
            if circuit.difficulty >= 7 {
                ArrayPush(circuit.hazards, "sandstorms");
            }
        }
        
        if StrContains(circuit.district, "Watson") {
            ArrayPush(circuit.hazards, "heavy_traffic");
            ArrayPush(circuit.hazards, "construction");
        }
        
        if StrContains(circuit.district, "Pacifica") {
            ArrayPush(circuit.hazards, "gang_interference");
            if circuit.difficulty >= 6 {
                ArrayPush(circuit.hazards, "debris");
            }
        }
        
        if circuit.heatLevel >= 5 {
            ArrayPush(circuit.hazards, "police_pursuit");
        }
        
        if circuit.isUnderground {
            ArrayPush(circuit.hazards, "low_visibility");
            ArrayPush(circuit.hazards, "narrow_passages");
        }
    }
    
    private static func InitializePlayerVehicles() -> Void {
        ArrayClear(playerVehicles);
        
        // Initialize vehicles for connected players
        let connectedPlayers = NetworkingSystem.GetConnectedPlayerIds();
        for playerId in connectedPlayers {
            StreetRacing.CreateDefaultVehicle(playerId);
        }
    }
    
    private static func CreateDefaultVehicle(playerId: String) -> Void {
        let vehicle: VehicleCustomization;
        vehicle.vehicleId = playerId + "_default";
        vehicle.ownerId = playerId;
        vehicle.totalValue = 50000; // 50k eddies base value
        vehicle.legalStatus = "legal";
        
        // Default performance
        vehicle.performance.acceleration = 60.0;
        vehicle.performance.topSpeed = 180.0;
        vehicle.performance.handling = 55.0;
        vehicle.performance.braking = 50.0;
        vehicle.performance.durability = 70.0;
        vehicle.performance.weight = 1500.0;
        vehicle.performance.powerRating = 300;
        
        // Default appearance
        vehicle.appearance.paintColor = "blue";
        vehicle.appearance.paintFinish = "metallic";
        vehicle.appearance.wheels = "stock";
        vehicle.appearance.bodyKit = "stock";
        vehicle.appearance.lighting = "stock";
        vehicle.appearance.windows = "clear";
        
        ArrayPush(playerVehicles, vehicle);
    }
    
    public static func CreateRace(organizerId: String, circuitId: String, raceType: RaceType, maxParticipants: Int32, entryFee: Int32, allowedClass: VehicleClass) -> String {
        let circuit = StreetRacing.GetCircuit(circuitId);
        if Equals(circuit.circuitId, "") {
            LogChannel(n"COOP_RACING", "Circuit not found: " + circuitId);
            return "";
        }
        
        // Generate race ID
        let raceId = circuitId + "_" + organizerId + "_" + ToString(GetGameTime());
        
        let race: StreetRace;
        race.raceId = raceId;
        race.raceName = circuit.circuitName + " Race";
        race.raceType = raceType;
        race.status = RaceStatus.Open;
        race.circuitId = circuitId;
        race.organizerId = organizerId;
        race.maxParticipants = maxParticipants;
        race.entryFee = entryFee;
        race.prizePool = entryFee * maxParticipants; // Simple prize pool calculation
        race.allowedVehicleClass = allowedClass;
        race.startTime = GetGameTime() + 120.0; // 2 minutes to join
        race.duration = 0.0; // Will be set when race starts
        race.weather = "clear"; // Default weather
        race.timeOfDay = "day"; // Default time
        
        // Set default rules
        StreetRacing.SetDefaultRaceRules(race, circuit);
        
        ArrayPush(activeRaces, race);
        
        LogChannel(n"COOP_RACING", "Created race: " + raceId + " on " + circuit.circuitName);
        
        // Broadcast race creation
        let raceData = StreetRacing.SerializeRace(race);
        NetworkingSystem.BroadcastMessage("race_created", raceData);
        
        return raceId;
    }
    
    private static func SetDefaultRaceRules(race: ref<StreetRace>, circuit: RaceCircuit) -> Void {
        race.rules.allowRespawn = true;
        race.rules.allowRepairs = false;
        race.rules.allowWeapons = race.raceType == RaceType.Demolition;
        race.rules.damageEnabled = true;
        race.rules.trafficEnabled = !circuit.isUnderground;
        race.rules.policeEnabled = circuit.heatLevel >= 4;
        race.rules.maxLaps = race.raceType == RaceType.Circuit ? 3 : 1;
        race.rules.maxDuration = circuit.circuitLength / 50.0 * Cast<Float>(race.rules.maxLaps); // Rough estimate
        race.rules.checkpointRequirement = true;
        race.rules.collisionPenalty = 5.0; // 5 second penalty
        race.rules.shortcuts = circuit.difficulty <= 5; // Allow shortcuts on easier tracks
    }
    
    public static func JoinRace(raceId: String, playerId: String, vehicleId: String) -> Bool {
        let raceIndex = StreetRacing.FindActiveRaceIndex(raceId);
        if raceIndex == -1 {
            LogChannel(n"COOP_RACING", "Race not found: " + raceId);
            return false;
        }
        
        let race = activeRaces[raceIndex];
        
        if race.status != RaceStatus.Open {
            LogChannel(n"COOP_RACING", "Race not open for joining");
            return false;
        }
        
        if ArraySize(race.participants) >= race.maxParticipants {
            LogChannel(n"COOP_RACING", "Race is full");
            return false;
        }
        
        // Check if player already joined
        for participant in race.participants {
            if Equals(participant.playerId, playerId) {
                LogChannel(n"COOP_RACING", "Player already in race");
                return false;
            }
        }
        
        // Validate vehicle
        let vehicle = StreetRacing.GetPlayerVehicle(playerId, vehicleId);
        if Equals(vehicle.vehicleId, "") {
            LogChannel(n"COOP_RACING", "Vehicle not found: " + vehicleId);
            return false;
        }
        
        let vehicleClass = StreetRacing.DetermineVehicleClass(vehicle);
        if vehicleClass != race.allowedVehicleClass && race.allowedVehicleClass != VehicleClass.Custom {
            LogChannel(n"COOP_RACING", "Vehicle class not allowed in this race");
            return false;
        }
        
        // Check entry fee
        let playerData = NetworkingSystem.GetPlayerData(playerId);
        if IsDefined(playerData) && playerData.GetMoney() < race.entryFee {
            LogChannel(n"COOP_RACING", "Insufficient funds for entry fee");
            return false;
        }
        
        // Deduct entry fee
        if IsDefined(playerData) {
            playerData.RemoveMoney(race.entryFee);
        }
        
        // Add participant
        let participant: RaceParticipant;
        participant.playerId = playerId;
        participant.vehicleId = vehicleId;
        participant.vehicleName = vehicle.vehicleId; // Simplified
        participant.vehicleClass = vehicleClass;
        participant.startPosition = ArraySize(race.participants) + 1;
        participant.currentPosition = participant.startPosition;
        participant.currentLap = 0;
        participant.bestLapTime = 0.0;
        participant.totalTime = 0.0;
        participant.isFinished = false;
        participant.isDisqualified = false;
        participant.penaltyTime = 0.0;
        participant.reputation = StreetRacing.GetPlayerRacingReputation(playerId);
        
        // Initialize checkpoint status
        let circuit = StreetRacing.GetCircuit(race.circuitId);
        for i in Range(ArraySize(circuit.checkpoints)) {
            ArrayPush(participant.checkpoints, false);
        }
        
        ArrayPush(race.participants, participant);
        activeRaces[raceIndex] = race;
        
        LogChannel(n"COOP_RACING", "Player " + playerId + " joined race " + raceId);
        
        // Broadcast player join
        let joinData = raceId + "|" + playerId + "|" + vehicleId;
        NetworkingSystem.BroadcastMessage("race_player_join", joinData);
        
        return true;
    }
    
    public static func PlaceBet(raceId: String, bettorId: String, targetPlayerId: String, betAmount: Int32, betType: String) -> Bool {
        let raceIndex = StreetRacing.FindActiveRaceIndex(raceId);
        if raceIndex == -1 {
            LogChannel(n"COOP_RACING", "Race not found for betting: " + raceId);
            return false;
        }
        
        let race = activeRaces[raceIndex];
        
        if race.status != RaceStatus.Open && race.status != RaceStatus.Countdown {
            LogChannel(n"COOP_RACING", "Betting closed for this race");
            return false;
        }
        
        // Validate bettor has funds
        let playerData = NetworkingSystem.GetPlayerData(bettorId);
        if !IsDefined(playerData) || playerData.GetMoney() < betAmount {
            LogChannel(n"COOP_RACING", "Insufficient funds for bet");
            return false;
        }
        
        // Validate target player is in race
        let validTarget = false;
        for participant in race.participants {
            if Equals(participant.playerId, targetPlayerId) {
                validTarget = true;
                break;
            }
        }
        
        if !validTarget {
            LogChannel(n"COOP_RACING", "Bet target not in race");
            return false;
        }
        
        // Calculate odds
        let odds = StreetRacing.CalculateBettingOdds(race, targetPlayerId, betType);
        
        // Create bet
        let bet: RaceBet;
        bet.betId = raceId + "_" + bettorId + "_" + ToString(GetGameTime());
        bet.bettorId = bettorId;
        bet.targetPlayerId = targetPlayerId;
        bet.betAmount = betAmount;
        bet.betType = betType;
        bet.odds = odds;
        bet.potentialPayout = Cast<Int32>(Cast<Float>(betAmount) * odds);
        bet.isWinning = false;
        
        ArrayPush(race.bets, bet);
        activeRaces[raceIndex] = race;
        
        // Deduct bet amount
        playerData.RemoveMoney(betAmount);
        
        LogChannel(n"COOP_RACING", "Bet placed: " + bettorId + " bet " + ToString(betAmount) + " on " + targetPlayerId);
        
        // Broadcast bet
        let betData = StreetRacing.SerializeBet(bet);
        NetworkingSystem.BroadcastMessage("race_bet_placed", betData);
        
        return true;
    }
    
    public static func StartRace(raceId: String) -> Bool {
        let raceIndex = StreetRacing.FindActiveRaceIndex(raceId);
        if raceIndex == -1 {
            return false;
        }
        
        let race = activeRaces[raceIndex];
        
        if race.status != RaceStatus.Open && race.status != RaceStatus.Countdown {
            LogChannel(n"COOP_RACING", "Race cannot be started from current status");
            return false;
        }
        
        if ArraySize(race.participants) < 2 {
            LogChannel(n"COOP_RACING", "Not enough participants to start race");
            return false;
        }
        
        // Update race status
        race.status = RaceStatus.InProgress;
        race.startTime = GetGameTime();
        
        // Position players at start line
        StreetRacing.PositionPlayersAtStart(race);
        
        activeRaces[raceIndex] = race;
        
        LogChannel(n"COOP_RACING", "Started race: " + raceId + " with " + ToString(ArraySize(race.participants)) + " participants");
        
        // Broadcast race start
        let startData = raceId + "|" + ToString(race.startTime);
        NetworkingSystem.BroadcastMessage("race_started", startData);
        
        return true;
    }
    
    public static func HitCheckpoint(raceId: String, playerId: String, checkpointIndex: Int32) -> Void {
        let raceIndex = StreetRacing.FindActiveRaceIndex(raceId);
        if raceIndex == -1 {
            return;
        }
        
        let race = activeRaces[raceIndex];
        
        if race.status != RaceStatus.InProgress {
            return;
        }
        
        // Find participant
        for i in Range(ArraySize(race.participants)) {
            let participant = race.participants[i];
            if Equals(participant.playerId, playerId) {
                // Validate checkpoint sequence
                if checkpointIndex < 0 || checkpointIndex >= ArraySize(participant.checkpoints) {
                    return;
                }
                
                // Mark checkpoint as hit
                participant.checkpoints[checkpointIndex] = true;
                
                // Check if this completes a lap
                if StreetRacing.AllCheckpointsHit(participant.checkpoints) {
                    StreetRacing.CompleteLap(race, i);
                }
                
                race.participants[i] = participant;
                break;
            }
        }
        
        activeRaces[raceIndex] = race;
        
        // Broadcast checkpoint hit
        let checkpointData = raceId + "|" + playerId + "|" + ToString(checkpointIndex);
        NetworkingSystem.BroadcastMessage("race_checkpoint_hit", checkpointData);
    }
    
    private static func CompleteLap(race: ref<StreetRace>, participantIndex: Int32) -> Void {
        let participant = race.participants[participantIndex];
        participant.currentLap += 1;
        
        let lapTime = GetGameTime() - race.startTime; // Simplified lap timing
        if participant.bestLapTime == 0.0 || lapTime < participant.bestLapTime {
            participant.bestLapTime = lapTime;
        }
        
        // Reset checkpoints for next lap
        for i in Range(ArraySize(participant.checkpoints)) {
            participant.checkpoints[i] = false;
        }
        
        // Check if race is finished
        if participant.currentLap >= race.rules.maxLaps {
            participant.isFinished = true;
            participant.totalTime = GetGameTime() - race.startTime;
            
            LogChannel(n"COOP_RACING", "Player " + participant.playerId + " finished race in " + ToString(participant.totalTime) + " seconds");
        }
        
        race.participants[participantIndex] = participant;
        
        // Check if race is complete
        StreetRacing.CheckRaceCompletion(race);
    }
    
    private static func CheckRaceCompletion(race: ref<StreetRace>) -> Void {
        let finishedCount = 0;
        for participant in race.participants {
            if participant.isFinished || participant.isDisqualified {
                finishedCount += 1;
            }
        }
        
        // Race ends when all participants finish or time limit reached
        let timeExpired = GetGameTime() >= (race.startTime + race.rules.maxDuration);
        let allFinished = finishedCount >= ArraySize(race.participants);
        
        if allFinished || timeExpired {
            StreetRacing.FinishRace(race);
        }
    }
    
    private static func FinishRace(race: ref<StreetRace>) -> Void {
        race.status = RaceStatus.Finished;
        
        // Calculate results
        StreetRacing.CalculateRaceResults(race);
        
        // Process bets
        StreetRacing.ProcessBettingPayouts(race);
        
        // Distribute prize money
        StreetRacing.DistributePrizeMoney(race);
        
        // Update player racing reputations
        StreetRacing.UpdateRacingReputations(race);
        
        LogChannel(n"COOP_RACING", "Race finished: " + race.raceId);
        
        // Broadcast race finish
        let finishData = StreetRacing.SerializeRaceResults(race);
        NetworkingSystem.BroadcastMessage("race_finished", finishData);
    }
    
    // Update loop for active races
    private static func StartUpdateLoop() -> Void {
        StreetRacing.UpdateRaces();
        DelaySystem.DelayCallback(StreetRacing.UpdateLoop, updateInterval);
    }
    
    private static func UpdateLoop() -> Void {
        if !isInitialized {
            return;
        }
        
        let currentTime = GetGameTime();
        if (currentTime - lastUpdateTime) >= updateInterval {
            StreetRacing.UpdateRaces();
            lastUpdateTime = currentTime;
        }
        
        DelaySystem.DelayCallback(StreetRacing.UpdateLoop, updateInterval);
    }
    
    private static func UpdateRaces() -> Void {
        for i in Range(ArraySize(activeRaces)) {
            let race = activeRaces[i];
            
            // Auto-start races when time comes
            if race.status == RaceStatus.Open && GetGameTime() >= race.startTime {
                if ArraySize(race.participants) >= 2 {
                    race.status = RaceStatus.Countdown;
                    // Give 10 second countdown
                    DelaySystem.DelayCallback(StreetRacing.StartRace, 10.0, race.raceId);
                } else {
                    // Cancel race if not enough participants
                    race.status = RaceStatus.Cancelled;
                    StreetRacing.RefundEntryFees(race);
                }
                
                activeRaces[i] = race;
            }
            
            // Check for race timeouts
            if race.status == RaceStatus.InProgress {
                let timeExpired = GetGameTime() >= (race.startTime + race.rules.maxDuration);
                if timeExpired {
                    StreetRacing.FinishRace(race);
                }
            }
            
            // Clean up old finished races
            if (race.status == RaceStatus.Finished || race.status == RaceStatus.Cancelled) &&
               GetGameTime() >= (race.startTime + 300.0) { // 5 minutes after finish
                ArrayErase(activeRaces, i);
                i -= 1; // Adjust index after erase
            }
        }
    }
    
    // Utility functions
    private static func GetCircuit(circuitId: String) -> RaceCircuit {
        for circuit in raceCircuits {
            if Equals(circuit.circuitId, circuitId) {
                return circuit;
            }
        }
        
        let emptyCircuit: RaceCircuit;
        return emptyCircuit;
    }
    
    private static func GetPlayerVehicle(playerId: String, vehicleId: String) -> VehicleCustomization {
        for vehicle in playerVehicles {
            if Equals(vehicle.ownerId, playerId) && Equals(vehicle.vehicleId, vehicleId) {
                return vehicle;
            }
        }
        
        let emptyVehicle: VehicleCustomization;
        return emptyVehicle;
    }
    
    private static func FindActiveRaceIndex(raceId: String) -> Int32 {
        for i in Range(ArraySize(activeRaces)) {
            if Equals(activeRaces[i].raceId, raceId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func DetermineVehicleClass(vehicle: VehicleCustomization) -> VehicleClass {
        let powerRating = vehicle.performance.powerRating;
        
        if powerRating < 200 {
            return VehicleClass.Street;
        } else if powerRating < 400 {
            return VehicleClass.Sport;
        } else if powerRating < 600 {
            return VehicleClass.Super;
        } else {
            return VehicleClass.Hyper;
        }
    }
    
    // Additional utility functions would be implemented here...
    
    // Public API
    public static func GetAvailableCircuits() -> array<RaceCircuit> {
        return raceCircuits;
    }
    
    public static func GetActiveRaces() -> array<StreetRace> {
        return activeRaces;
    }
    
    public static func GetPlayerVehicles(playerId: String) -> array<VehicleCustomization> {
        let playerVehicleList: array<VehicleCustomization>;
        for vehicle in playerVehicles {
            if Equals(vehicle.ownerId, playerId) {
                ArrayPush(playerVehicleList, vehicle);
            }
        }
        return playerVehicleList;
    }
    
    public static func GetRaceHistory(playerId: String) -> array<StreetRace> {
        // This would return completed races the player participated in
        let history: array<StreetRace>;
        // Implementation would query historical race data
        return history;
    }
    
    public static func GetPlayerRacingStats(playerId: String) -> PlayerRacingStats {
        // This would return comprehensive racing statistics
        let stats: PlayerRacingStats;
        // Implementation would calculate wins, losses, best times, etc.
        return stats;
    }
}

// Supporting structures
public struct PlayerRacingStats {
    public var playerId: String;
    public var racesEntered: Int32;
    public var racesWon: Int32;
    public var racesFinished: Int32;
    public var totalWinnings: Int32;
    public var bestLapTimes: array<Float>; // Per circuit
    public var reputation: Int32;
    public var favoriteCircuit: String;
    public var preferredVehicleClass: VehicleClass;
}