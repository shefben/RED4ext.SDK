// Dynamic gang warfare and territory control system for open world multiplayer

public enum GangFaction {
    Maelstrom = 0,
    Valentinos = 1,
    TigerClaws = 2,
    SixthStreet = 3,
    VoodooBoys = 4,
    Animals = 5,
    Scavengers = 6,
    Tygers = 7,
    Moxes = 8,
    Wraiths = 9,
    Militech = 10,
    Arasaka = 11,
    Independent = 12
}

public enum TerritoryStatus {
    Neutral = 0,
    Contested = 1,      // Battle in progress
    Controlled = 2,     // Firmly controlled by faction
    Unstable = 3,       // Recently changed hands
    Fortified = 4       // Heavily defended
}

public enum WarfareEventType {
    TerritoryAssault = 0,
    DefenseOperation = 1,
    SupplyRaid = 2,
    IntelGathering = 3,
    Sabotage = 4,
    PeaceTalks = 5,
    PowerStruggle = 6,
    ResourceControl = 7
}

public struct Territory {
    public var territoryId: String;
    public var displayName: String;
    public var districtName: String;
    public var centerPosition: Vector3;
    public var boundaryPoints: array<Vector3>;
    public var controllingFaction: GangFaction;
    public var status: TerritoryStatus;
    public var controlStrength: Float;     // 0.0 to 1.0
    public var economicValue: Int32;       // Value in eddies per hour
    public var strategicImportance: Int32; // 1-10 importance rating
    public var defenseLevel: Int32;        // Defensive capabilities
    public var lastAttackTime: Float;
    public var activeEvents: array<String>; // Event IDs happening here
    public var influencingPlayers: array<String>; // Players who've affected this territory
}

public struct GangWarEvent {
    public var eventId: String;
    public var eventType: WarfareEventType;
    public var targetTerritoryId: String;
    public var attackingFaction: GangFaction;
    public var defendingFaction: GangFaction;
    public var startTime: Float;
    public var duration: Float;
    public var participants: array<String>; // Player UUIDs
    public var objectives: array<WarObjective>;
    public var rewards: WarRewards;
    public var isActive: Bool;
    public var difficulty: Int32; // 1-5 difficulty level
    public var requiresCoordination: Bool; // Needs multiple players
}

public struct WarObjective {
    public var objectiveId: String;
    public var description: String;
    public var type: String; // "eliminate", "capture", "defend", "hack", "collect"
    public var targetPosition: Vector3;
    public var targetCount: Int32;
    public var currentProgress: Int32;
    public var timeLimit: Float;
    public var isCompleted: Bool;
    public var assignedPlayers: array<String>;
}

public struct WarRewards {
    public var experiencePoints: Int32;
    public var streetCredGain: Int32;
    public var eddiesReward: Int32;
    public var territoryInfluence: Float;
    public var factionReputation: array<FactionRep>;
    public var uniqueItems: array<String>;
    public var permanentBonuses: array<String>;
}

public struct FactionRep {
    public var faction: GangFaction;
    public var reputationChange: Int32;
}

public struct PlayerWarStatus {
    public var playerId: String;
    public var currentFactionAlliance: GangFaction;
    public var factionReputations: array<FactionRep>;
    public var activeEvents: array<String>;
    public var territoriesInfluenced: array<String>;
    public var totalWarScore: Int32;
    public var lastActivity: Float;
}

public class GangWarfare {
    private static var isInitialized: Bool = false;
    private static var territories: array<Territory>;
    private static var activeEvents: array<GangWarEvent>;
    private static var playerWarStatus: array<PlayerWarStatus>;
    private static var warfareUI: ref<GangWarfareUI>;
    private static var updateInterval: Float = 30.0; // Update every 30 seconds
    private static var lastUpdateTime: Float = 0.0;
    private static var globalWarState: Float = 0.5; // 0.0 = peace, 1.0 = total war
    
    // Network callbacks
    private static cb func OnTerritoryEventStart(data: String) -> Void;
    private static cb func OnTerritoryEventUpdate(data: String) -> Void;
    private static cb func OnPlayerJoinEvent(data: String) -> Void;
    private static cb func OnTerritoryCapture(data: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_WARFARE", "Initializing gang warfare system...");
        
        // Initialize territories
        GangWarfare.InitializeTerritories();
        
        // Initialize player war status
        GangWarfare.InitializePlayerStatus();
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("warfare_event_start", GangWarfare.OnTerritoryEventStart);
        NetworkingSystem.RegisterCallback("warfare_event_update", GangWarfare.OnTerritoryEventUpdate);
        NetworkingSystem.RegisterCallback("warfare_player_join", GangWarfare.OnPlayerJoinEvent);
        NetworkingSystem.RegisterCallback("warfare_territory_capture", GangWarfare.OnTerritoryCapture);
        
        // Start update loop
        GangWarfare.StartUpdateLoop();
        
        isInitialized = true;
        LogChannel(n"COOP_WARFARE", "Gang warfare system initialized with " + ToString(ArraySize(territories)) + " territories");
    }
    
    private static func InitializeTerritories() -> Void {
        ArrayClear(territories);
        
        // Watson District
        GangWarfare.CreateTerritory("watson_northside", "Northside Industrial", "Watson", 
            new Vector3(-1200.0, 1300.0, 50.0), GangFaction.Maelstrom, 1500, 8);
        
        GangWarfare.CreateTerritory("watson_kabuki", "Kabuki Market", "Watson",
            new Vector3(-1100.0, 1100.0, 30.0), GangFaction.TigerClaws, 2000, 7);
        
        // Westbrook District  
        GangWarfare.CreateTerritory("westbrook_japantown", "Japantown", "Westbrook",
            new Vector3(-800.0, -200.0, 40.0), GangFaction.TigerClaws, 2500, 9);
        
        GangWarfare.CreateTerritory("westbrook_charter_hill", "Charter Hill", "Westbrook",
            new Vector3(-600.0, -400.0, 60.0), GangFaction.Arasaka, 3000, 10);
        
        // Heywood District
        GangWarfare.CreateTerritory("heywood_valentino", "Valentino Territory", "Heywood",
            new Vector3(800.0, -800.0, 25.0), GangFaction.Valentinos, 1800, 7);
        
        GangWarfare.CreateTerritory("heywood_sixth_street", "Sixth Street Domain", "Heywood",
            new Vector3(1000.0, -600.0, 30.0), GangFaction.SixthStreet, 1600, 6);
        
        // Pacifica District
        GangWarfare.CreateTerritory("pacifica_voodoo", "Voodoo Boys Haven", "Pacifica",
            new Vector3(-2000.0, -1500.0, 20.0), GangFaction.VoodooBoys, 2200, 8);
        
        GangWarfare.CreateTerritory("pacifica_animals", "Animal Territory", "Pacifica",
            new Vector3(-1800.0, -1300.0, 15.0), GangFaction.Animals, 1400, 5);
        
        // Santo Domingo District
        GangWarfare.CreateTerritory("santo_sixth_street", "Sixth Street Industrial", "Santo Domingo",
            new Vector3(1200.0, 800.0, 45.0), GangFaction.SixthStreet, 1700, 6);
        
        // Badlands
        GangWarfare.CreateTerritory("badlands_wraiths", "Wraith Territory", "Badlands",
            new Vector3(2500.0, 2000.0, 100.0), GangFaction.Wraiths, 1200, 4);
        
        LogChannel(n"COOP_WARFARE", "Initialized " + ToString(ArraySize(territories)) + " territories");
    }
    
    private static func CreateTerritory(id: String, name: String, district: String, center: Vector3, faction: GangFaction, value: Int32, importance: Int32) -> Void {
        let territory: Territory;
        territory.territoryId = id;
        territory.displayName = name;
        territory.districtName = district;
        territory.centerPosition = center;
        territory.controllingFaction = faction;
        territory.status = TerritoryStatus.Controlled;
        territory.controlStrength = 0.8; // Start with strong control
        territory.economicValue = value;
        territory.strategicImportance = importance;
        territory.defenseLevel = 3; // Medium defense
        territory.lastAttackTime = 0.0;
        
        // Generate boundary points (simple square for now)
        let size = 200.0;
        ArrayPush(territory.boundaryPoints, new Vector3(center.X - size, center.Y - size, center.Z));
        ArrayPush(territory.boundaryPoints, new Vector3(center.X + size, center.Y - size, center.Z));
        ArrayPush(territory.boundaryPoints, new Vector3(center.X + size, center.Y + size, center.Z));
        ArrayPush(territory.boundaryPoints, new Vector3(center.X - size, center.Y + size, center.Z));
        
        ArrayPush(territories, territory);
    }
    
    private static func InitializePlayerStatus() -> Void {
        ArrayClear(playerWarStatus);
        
        let connectedPlayers = NetworkingSystem.GetConnectedPlayerIds();
        for playerId in connectedPlayers {
            let status: PlayerWarStatus;
            status.playerId = playerId;
            status.currentFactionAlliance = GangFaction.Independent;
            status.totalWarScore = 0;
            status.lastActivity = GetGameTime();
            
            // Initialize neutral reputation with all factions
            for i in Range(13) { // Number of factions
                let rep: FactionRep;
                rep.faction = IntToEnum(i, GangFaction.Independent);
                rep.reputationChange = 0;
                ArrayPush(status.factionReputations, rep);
            }
            
            ArrayPush(playerWarStatus, status);
        }
    }
    
    public static func StartTerritoryEvent(territoryId: String, eventType: WarfareEventType, attackingFaction: GangFaction) -> String {
        let territory = GangWarfare.GetTerritory(territoryId);
        if Equals(territory.territoryId, "") {
            LogChannel(n"COOP_WARFARE", "Territory not found: " + territoryId);
            return "";
        }
        
        if territory.status == TerritoryStatus.Contested {
            LogChannel(n"COOP_WARFARE", "Territory already contested: " + territoryId);
            return "";
        }
        
        // Create new warfare event
        let eventId = territoryId + "_" + ToString(Cast<Int32>(eventType)) + "_" + ToString(GetGameTime());
        let event: GangWarEvent;
        event.eventId = eventId;
        event.eventType = eventType;
        event.targetTerritoryId = territoryId;
        event.attackingFaction = attackingFaction;
        event.defendingFaction = territory.controllingFaction;
        event.startTime = GetGameTime();
        event.duration = GangWarfare.CalculateEventDuration(eventType, territory.strategicImportance);
        event.isActive = true;
        event.difficulty = GangWarfare.CalculateEventDifficulty(territory);
        event.requiresCoordination = event.difficulty >= 3;
        
        // Create objectives
        event.objectives = GangWarfare.GenerateEventObjectives(event);
        
        // Calculate rewards
        event.rewards = GangWarfare.CalculateEventRewards(event, territory);
        
        ArrayPush(activeEvents, event);
        
        // Update territory status
        territory.status = TerritoryStatus.Contested;
        ArrayPush(territory.activeEvents, eventId);
        GangWarfare.UpdateTerritory(territory);
        
        LogChannel(n"COOP_WARFARE", "Started warfare event: " + eventId + " in " + territory.displayName);
        
        // Broadcast event to all players
        let eventData = GangWarfare.SerializeWarEvent(event);
        NetworkingSystem.BroadcastMessage("warfare_event_start", eventData);
        
        // Show notification to nearby players
        GangWarfare.NotifyNearbyPlayers(territory, event);
        
        return eventId;
    }
    
    public static func JoinWarfareEvent(playerId: String, eventId: String, side: GangFaction) -> Bool {
        let eventIndex = GangWarfare.FindEventIndex(eventId);
        if eventIndex == -1 {
            LogChannel(n"COOP_WARFARE", "Event not found: " + eventId);
            return false;
        }
        
        let event = activeEvents[eventIndex];
        if !event.isActive {
            LogChannel(n"COOP_WARFARE", "Event is not active: " + eventId);
            return false;
        }
        
        // Check if player is already in event
        for participantId in event.participants {
            if Equals(participantId, playerId) {
                LogChannel(n"COOP_WARFARE", "Player already in event: " + playerId);
                return false;
            }
        }
        
        // Validate side choice
        if side != event.attackingFaction && side != event.defendingFaction {
            LogChannel(n"COOP_WARFARE", "Invalid faction choice for event");
            return false;
        }
        
        // Add player to event
        ArrayPush(event.participants, playerId);
        activeEvents[eventIndex] = event;
        
        // Update player war status
        let playerStatus = GangWarfare.GetPlayerWarStatus(playerId);
        ArrayPush(playerStatus.activeEvents, eventId);
        playerStatus.currentFactionAlliance = side;
        playerStatus.lastActivity = GetGameTime();
        GangWarfare.UpdatePlayerWarStatus(playerStatus);
        
        LogChannel(n"COOP_WARFARE", "Player " + playerId + " joined warfare event " + eventId + " for " + ToString(Cast<Int32>(side)));
        
        // Broadcast player join
        let joinData = playerId + "|" + eventId + "|" + ToString(Cast<Int32>(side));
        NetworkingSystem.BroadcastMessage("warfare_player_join", joinData);
        
        // Assign objectives to player
        GangWarfare.AssignObjectivesToPlayer(playerId, event);
        
        return true;
    }
    
    public static func UpdateEventObjective(eventId: String, objectiveId: String, playerId: String, progress: Int32) -> Void {
        let eventIndex = GangWarfare.FindEventIndex(eventId);
        if eventIndex == -1 {
            return;
        }
        
        let event = activeEvents[eventIndex];
        
        // Find and update objective
        for i in Range(ArraySize(event.objectives)) {
            let objective = event.objectives[i];
            if Equals(objective.objectiveId, objectiveId) {
                objective.currentProgress += progress;
                objective.currentProgress = MinI(objective.currentProgress, objective.targetCount);
                
                if objective.currentProgress >= objective.targetCount {
                    objective.isCompleted = true;
                    LogChannel(n"COOP_WARFARE", "Objective completed: " + objectiveId + " by " + playerId);
                }
                
                event.objectives[i] = objective;
                break;
            }
        }
        
        activeEvents[eventIndex] = event;
        
        // Check if event is complete
        GangWarfare.CheckEventCompletion(eventIndex);
        
        // Broadcast update
        let updateData = eventId + "|" + objectiveId + "|" + playerId + "|" + ToString(progress);
        NetworkingSystem.BroadcastMessage("warfare_event_update", updateData);
    }
    
    private static func CheckEventCompletion(eventIndex: Int32) -> Void {
        let event = activeEvents[eventIndex];
        
        // Check if all objectives are completed
        let completedObjectives = 0;
        for objective in event.objectives {
            if objective.isCompleted {
                completedObjectives += 1;
            }
        }
        
        let allCompleted = completedObjectives >= ArraySize(event.objectives);
        let timeExpired = GetGameTime() >= (event.startTime + event.duration);
        
        if allCompleted || timeExpired {
            GangWarfare.CompleteWarfareEvent(eventIndex, allCompleted);
        }
    }
    
    private static func CompleteWarfareEvent(eventIndex: Int32, wasSuccessful: Bool) -> Void {
        let event = activeEvents[eventIndex];
        let territory = GangWarfare.GetTerritory(event.targetTerritoryId);
        
        LogChannel(n"COOP_WARFARE", "Completing warfare event: " + event.eventId + " (Success: " + ToString(wasSuccessful) + ")");
        
        if wasSuccessful {
            // Determine winner and apply territorial changes
            GangWarfare.ProcessTerritorialChange(event, territory);
        }
        
        // Distribute rewards to participants
        GangWarfare.DistributeEventRewards(event, wasSuccessful);
        
        // Update territory status
        territory.status = wasSuccessful ? TerritoryStatus.Unstable : TerritoryStatus.Controlled;
        territory.lastAttackTime = GetGameTime();
        
        // Remove event from territory
        for i in Range(ArraySize(territory.activeEvents)) {
            if Equals(territory.activeEvents[i], event.eventId) {
                ArrayErase(territory.activeEvents, i);
                break;
            }
        }
        
        GangWarfare.UpdateTerritory(territory);
        
        // Clean up event
        event.isActive = false;
        ArrayErase(activeEvents, eventIndex);
        
        // Update global war state
        GangWarfare.UpdateGlobalWarState();
        
        // Broadcast completion
        let completionData = event.eventId + "|" + ToString(wasSuccessful) + "|" + ToString(Cast<Int32>(event.attackingFaction));
        NetworkingSystem.BroadcastMessage("warfare_event_complete", completionData);
    }
    
    private static func ProcessTerritorialChange(event: GangWarEvent, territory: Territory) -> Void {
        if event.eventType == WarfareEventType.TerritoryAssault {
            // Territory changes hands
            let oldFaction = territory.controllingFaction;
            territory.controllingFaction = event.attackingFaction;
            territory.controlStrength = 0.6; // Weaker initial control
            
            LogChannel(n"COOP_WARFARE", "Territory " + territory.displayName + " captured by " + ToString(Cast<Int32>(event.attackingFaction)) + " from " + ToString(Cast<Int32>(oldFaction)));
            
            // Broadcast territory capture
            let captureData = territory.territoryId + "|" + ToString(Cast<Int32>(oldFaction)) + "|" + ToString(Cast<Int32>(event.attackingFaction));
            NetworkingSystem.BroadcastMessage("warfare_territory_capture", captureData);
            
        } else if event.eventType == WarfareEventType.DefenseOperation {
            // Strengthen existing control
            territory.controlStrength = MinF(1.0, territory.controlStrength + 0.2);
            territory.defenseLevel = MinI(5, territory.defenseLevel + 1);
            
        } else if event.eventType == WarfareEventType.Sabotage {
            // Weaken control
            territory.controlStrength = MaxF(0.1, territory.controlStrength - 0.3);
            territory.defenseLevel = MaxI(1, territory.defenseLevel - 1);
        }
        
        GangWarfare.UpdateTerritory(territory);
    }
    
    private static func DistributeEventRewards(event: GangWarEvent, wasSuccessful: Bool) -> Void {
        let baseRewards = event.rewards;
        let successMultiplier = wasSuccessful ? 1.0 : 0.3; // Reduced rewards for failure
        
        for participantId in event.participants {
            let rewards: WarRewards;
            rewards.experiencePoints = Cast<Int32>(Cast<Float>(baseRewards.experiencePoints) * successMultiplier);
            rewards.streetCredGain = Cast<Int32>(Cast<Float>(baseRewards.streetCredGain) * successMultiplier);
            rewards.eddiesReward = Cast<Int32>(Cast<Float>(baseRewards.eddiesReward) * successMultiplier);
            rewards.territoryInfluence = baseRewards.territoryInfluence * successMultiplier;
            
            // Apply rewards to player
            GangWarfare.ApplyPlayerRewards(participantId, rewards, event);
            
            LogChannel(n"COOP_WARFARE", "Distributed rewards to " + participantId + ": " + ToString(rewards.experiencePoints) + " XP, " + ToString(rewards.eddiesReward) + " eddies");
        }
    }
    
    private static func ApplyPlayerRewards(playerId: String, rewards: WarRewards, event: GangWarEvent) -> Void {
        // Apply experience and street cred
        let playerData = NetworkingSystem.GetPlayerData(playerId);
        if IsDefined(playerData) {
            playerData.AddExperience(rewards.experiencePoints);
            playerData.AddStreetCred(rewards.streetCredGain);
            playerData.AddMoney(rewards.eddiesReward);
        }
        
        // Update war status
        let warStatus = GangWarfare.GetPlayerWarStatus(playerId);
        warStatus.totalWarScore += rewards.experiencePoints / 10;
        
        // Update faction reputation
        for reputationChange in rewards.factionReputation {
            GangWarfare.UpdatePlayerFactionReputation(warStatus, reputationChange.faction, reputationChange.reputationChange);
        }
        
        GangWarfare.UpdatePlayerWarStatus(warStatus);
        
        // Give unique items if any
        for itemId in rewards.uniqueItems {
            GangWarfare.GivePlayerItem(playerId, itemId);
        }
    }
    
    // Automatic event generation
    private static func StartUpdateLoop() -> Void {
        GangWarfare.UpdateWarfareState();
        DelaySystem.DelayCallback(GangWarfare.UpdateLoop, updateInterval);
    }
    
    private static func UpdateLoop() -> Void {
        if !isInitialized {
            return;
        }
        
        let currentTime = GetGameTime();
        if (currentTime - lastUpdateTime) >= updateInterval {
            GangWarfare.UpdateWarfareState();
            lastUpdateTime = currentTime;
        }
        
        // Continue loop
        DelaySystem.DelayCallback(GangWarfare.UpdateLoop, updateInterval);
    }
    
    private static func UpdateWarfareState() -> Void {
        // Check for expired events
        for i in Range(ArraySize(activeEvents)) {
            let event = activeEvents[i];
            if event.isActive && GetGameTime() >= (event.startTime + event.duration) {
                GangWarfare.CheckEventCompletion(i);
            }
        }
        
        // Generate random events based on current state
        GangWarfare.GenerateRandomEvents();
        
        // Update territory stability
        GangWarfare.UpdateTerritoryStability();
        
        // Check for faction power shifts
        GangWarfare.CheckFactionPowerShifts();
    }
    
    private static func GenerateRandomEvents() -> Void {
        if ArraySize(activeEvents) >= 5 {
            return; // Too many active events
        }
        
        let eventChance = globalWarState * 0.1; // Higher war state = more events
        if RandF() > eventChance {
            return;
        }
        
        // Select random territory
        let territoryIndex = RandRange(0, ArraySize(territories));
        let territory = territories[territoryIndex];
        
        if territory.status == TerritoryStatus.Contested {
            return; // Already has active event
        }
        
        // Select random attacking faction (different from controlling faction)
        let attackingFaction: GangFaction;
        do {
            attackingFaction = IntToEnum(RandRange(0, 13), GangFaction.Independent);
        } while (attackingFaction == territory.controllingFaction);
        
        // Select event type based on territory and faction
        let eventType = GangWarfare.SelectRandomEventType(territory, attackingFaction);
        
        // Start the event
        GangWarfare.StartTerritoryEvent(territory.territoryId, eventType, attackingFaction);
        
        LogChannel(n"COOP_WARFARE", "Generated random warfare event in " + territory.displayName);
    }
    
    private static func UpdateTerritoryStability() -> Void {
        for i in Range(ArraySize(territories)) {
            let territory = territories[i];
            
            // Gradually stabilize territories over time
            if territory.status == TerritoryStatus.Unstable {
                let timeSinceAttack = GetGameTime() - territory.lastAttackTime;
                if timeSinceAttack > 300.0 { // 5 minutes
                    territory.status = TerritoryStatus.Controlled;
                    territory.controlStrength = MinF(1.0, territory.controlStrength + 0.1);
                    territories[i] = territory;
                }
            }
            
            // Strengthen control over time if no attacks
            if territory.status == TerritoryStatus.Controlled && ArraySize(territory.activeEvents) == 0 {
                let timeSinceAttack = GetGameTime() - territory.lastAttackTime;
                if timeSinceAttack > 600.0 { // 10 minutes
                    territory.controlStrength = MinF(1.0, territory.controlStrength + 0.05);
                    if territory.controlStrength >= 0.95 && territory.defenseLevel >= 4 {
                        territory.status = TerritoryStatus.Fortified;
                    }
                    territories[i] = territory;
                }
            }
        }
    }
    
    // Utility functions
    private static func GetTerritory(territoryId: String) -> Territory {
        for territory in territories {
            if Equals(territory.territoryId, territoryId) {
                return territory;
            }
        }
        
        let emptyTerritory: Territory;
        return emptyTerritory;
    }
    
    private static func UpdateTerritory(updatedTerritory: Territory) -> Void {
        for i in Range(ArraySize(territories)) {
            if Equals(territories[i].territoryId, updatedTerritory.territoryId) {
                territories[i] = updatedTerritory;
                break;
            }
        }
    }
    
    private static func GetPlayerWarStatus(playerId: String) -> PlayerWarStatus {
        for status in playerWarStatus {
            if Equals(status.playerId, playerId) {
                return status;
            }
        }
        
        let emptyStatus: PlayerWarStatus;
        return emptyStatus;
    }
    
    private static func UpdatePlayerWarStatus(updatedStatus: PlayerWarStatus) -> Void {
        for i in Range(ArraySize(playerWarStatus)) {
            if Equals(playerWarStatus[i].playerId, updatedStatus.playerId) {
                playerWarStatus[i] = updatedStatus;
                break;
            }
        }
    }
    
    private static func FindEventIndex(eventId: String) -> Int32 {
        for i in Range(ArraySize(activeEvents)) {
            if Equals(activeEvents[i].eventId, eventId) {
                return i;
            }
        }
        return -1;
    }
    
    // Additional utility functions would be implemented here...
    
    // Network event handlers
    private static cb func OnTerritoryEventStart(data: String) -> Void {
        LogChannel(n"COOP_WARFARE", "Received territory event start: " + data);
        let event = GangWarfare.DeserializeWarEvent(data);
        ArrayPush(activeEvents, event);
    }
    
    private static cb func OnTerritoryEventUpdate(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 4 {
            let eventId = parts[0];
            let objectiveId = parts[1];
            let playerId = parts[2];
            let progress = StringToInt(parts[3]);
            
            GangWarfare.UpdateEventObjective(eventId, objectiveId, playerId, progress);
        }
    }
    
    private static cb func OnPlayerJoinEvent(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 3 {
            let playerId = parts[0];
            let eventId = parts[1];
            let side = IntToEnum(StringToInt(parts[2]), GangFaction.Independent);
            
            GangWarfare.JoinWarfareEvent(playerId, eventId, side);
        }
    }
    
    private static cb func OnTerritoryCapture(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 3 {
            let territoryId = parts[0];
            let oldFaction = IntToEnum(StringToInt(parts[1]), GangFaction.Independent);
            let newFaction = IntToEnum(StringToInt(parts[2]), GangFaction.Independent);
            
            LogChannel(n"COOP_WARFARE", "Territory " + territoryId + " captured by " + ToString(Cast<Int32>(newFaction)));
        }
    }
    
    // Serialization functions
    private static func SerializeWarEvent(event: GangWarEvent) -> String {
        let data = event.eventId + "|" + ToString(Cast<Int32>(event.eventType)) + "|" + event.targetTerritoryId;
        data += "|" + ToString(Cast<Int32>(event.attackingFaction)) + "|" + ToString(Cast<Int32>(event.defendingFaction));
        data += "|" + ToString(event.startTime) + "|" + ToString(event.duration) + "|" + ToString(event.isActive);
        return data;
    }
    
    private static func DeserializeWarEvent(data: String) -> GangWarEvent {
        let event: GangWarEvent;
        let parts = StrSplit(data, "|");
        
        if ArraySize(parts) >= 8 {
            event.eventId = parts[0];
            event.eventType = IntToEnum(StringToInt(parts[1]), WarfareEventType.TerritoryAssault);
            event.targetTerritoryId = parts[2];
            event.attackingFaction = IntToEnum(StringToInt(parts[3]), GangFaction.Independent);
            event.defendingFaction = IntToEnum(StringToInt(parts[4]), GangFaction.Independent);
            event.startTime = StringToFloat(parts[5]);
            event.duration = StringToFloat(parts[6]);
            event.isActive = StringToBool(parts[7]);
        }
        
        return event;
    }
    
    // Public API
    public static func GetTerritories() -> array<Territory> {
        return territories;
    }
    
    public static func GetActiveEvents() -> array<GangWarEvent> {
        return activeEvents;
    }
    
    public static func GetPlayerTerritoryInfluence(playerId: String) -> array<String> {
        let status = GangWarfare.GetPlayerWarStatus(playerId);
        return status.territoriesInfluenced;
    }
    
    public static func GetFactionControlledTerritories(faction: GangFaction) -> array<Territory> {
        let controlledTerritories: array<Territory>;
        for territory in territories {
            if territory.controllingFaction == faction {
                ArrayPush(controlledTerritories, territory);
            }
        }
        return controlledTerritories;
    }
    
    public static func GetNearbyTerritories(position: Vector3, radius: Float) -> array<Territory> {
        let nearbyTerritories: array<Territory>;
        for territory in territories {
            let distance = Vector3.Distance(position, territory.centerPosition);
            if distance <= radius {
                ArrayPush(nearbyTerritories, territory);
            }
        }
        return nearbyTerritories;
    }
}