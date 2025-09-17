// Corporate raid system with planning phases and coordinated infiltration

public enum RaidType {
    DataTheft = 0,        // Steal corporate data
    Sabotage = 1,         // Destroy/disable systems  
    Assassination = 2,    // Eliminate target
    Extraction = 3,       // Rescue/extract person
    PlantEvidence = 4,    // Frame corporation
    TechHeist = 5,        // Steal prototype tech
    Infiltration = 6,     // Gather intelligence
    Blackmail = 7         // Obtain compromising info
}

public enum RaidPhase {
    Planning = 0,         // Planning and preparation
    Preparation = 1,      // Equipment and role assignment
    Infiltration = 2,     // Getting inside
    Execution = 3,        // Performing objectives
    Extraction = 4,       // Getting out safely
    Completed = 5,        // Mission complete
    Failed = 6           // Mission failed
}

public enum RaidRole {
    TeamLeader = 0,       // Coordinates team
    Netrunner = 1,        // Handles hacking/ICE
    TechSpecialist = 2,   // Deals with security systems
    Soldier = 3,          // Combat specialist
    Infiltrator = 4,      // Stealth and social engineering
    Medic = 5,           // Keeps team alive
    Driver = 6,          // Extraction and vehicle support
    FaceDancer = 7       // Social manipulation/disguise
}

public enum CorporateTarget {
    Arasaka = 0,
    Militech = 1,
    KangTao = 2,
    Zetatech = 3,
    Biotechnica = 4,
    PetroChemical = 5,
    Trauma = 6,
    Raven = 7,
    Kiroshi = 8,
    NetworkWatch = 9
}

public struct CorporateRaid {
    public var raidId: String;
    public var raidType: RaidType;
    public var targetCorporation: CorporateTarget;
    public var targetFacility: String;
    public var currentPhase: RaidPhase;
    public var planningStartTime: Float;
    public var raidStartTime: Float;
    public var maxDuration: Float;
    public var participants: array<RaidParticipant>;
    public var objectives: array<RaidObjective>;
    public var plan: RaidPlan;
    public var difficulty: Int32; // 1-10 difficulty scale
    public var heatLevel: Int32;  // Corporate attention level
    public var isActive: Bool;
    public var rewards: RaidRewards;
    public var failureConsequences: array<String>;
}

public struct RaidParticipant {
    public var playerId: String;
    public var assignedRole: RaidRole;
    public var isReady: Bool;
    public var equipmentLoadout: array<String>;
    public var specialSkills: array<String>;
    public var contribution: Float; // 0.0-1.0 performance rating
    public var isAlive: Bool;
    public var currentLocation: Vector3;
    public var status: String; // "planning", "ready", "infiltrating", "executing", "extracting", "down", "extracted"
}

public struct RaidObjective {
    public var objectiveId: String;
    public var description: String;
    public var type: String; // "primary", "secondary", "bonus"
    public var assignedRole: RaidRole;
    public var targetLocation: Vector3;
    public var requiredTime: Float;
    public var progressPercent: Float;
    public var isCompleted: Bool;
    public var isCritical: Bool; // Failure ends mission
    public var alertsGenerated: Int32;
    public var timeWindow: Float; // Time limit for objective
}

public struct RaidPlan {
    public var planId: String;
    public var approach: String; // "stealth", "loud", "mixed", "social"
    public var entryPoint: Vector3;
    public var exitPoint: Vector3;
    public var contingencyPlans: array<ContingencyPlan>;
    public var timeline: array<PlanPhase>;
    public var requiredEquipment: array<String>;
    public var estimatedDuration: Float;
    public var riskAssessment: Float; // 0.0-1.0
    public var successProbability: Float; // 0.0-1.0
}

public struct ContingencyPlan {
    public var triggerId: String;
    public var condition: String; // What triggers this plan
    public var actions: array<String>;
    public var fallbackRole: RaidRole;
    public var timeToExecute: Float;
}

public struct PlanPhase {
    public var phaseId: String;
    public var phaseName: String;
    public var startTime: Float; // Minutes from raid start
    public var duration: Float;
    public var involvedRoles: array<RaidRole>;
    public var objectives: array<String>;
    public var requirements: array<String>;
}

public struct RaidRewards {
    public var baseXP: Int32;
    public var bonusXP: Int32;
    public var eddiesReward: Int32;
    public var streetCredGain: Int32;
    public var corporateIntel: array<String>;
    public var uniqueItems: array<String>;
    public var corporateReputation: array<CorpRep>;
    public var blackmarketAccess: array<String>;
}

public struct CorpRep {
    public var corporation: CorporateTarget;
    public var reputationChange: Int32; // Can be negative
}

public struct FacilityLayout {
    public var facilityId: String;
    public var facilityName: String;
    public var corporation: CorporateTarget;
    public var floors: array<FacilityFloor>;
    public var securityLevel: Int32; // 1-10
    public var guardPatrols: array<PatrolRoute>;
    public var securitySystems: array<SecuritySystem>;
    public var entryPoints: array<Vector3>;
    public var keyAreas: array<KeyArea>;
}

public struct FacilityFloor {
    public var floorNumber: Int32;
    public var areas: array<String>;
    public var securityCheckpoints: array<Vector3>;
    public var elevatorAccess: array<String>; // Required access levels
    public var emergencyExits: array<Vector3>;
}

public struct PatrolRoute {
    public var patrolId: String;
    public var guardType: String;
    public var waypoints: array<Vector3>;
    public var patrolDuration: Float;
    public var alertLevel: Int32; // How quickly they respond to threats
}

public struct SecuritySystem {
    public var systemId: String;
    public var systemType: String; // "cameras", "motion", "biometric", "ice"
    public var coverage: array<Vector3>;
    public var hackDifficulty: Int32;
    public var detectability: Float;
    public var responseTime: Float;
}

public struct KeyArea {
    public var areaId: String;
    public var areaName: String;
    public var position: Vector3;
    public var accessRequirement: String;
    public var containsObjectives: array<String>;
    public var securityRating: Int32;
}

public class CorporateRaids {
    private static var isInitialized: Bool = false;
    private static var activeRaids: array<CorporateRaid>;
    private static var availableRaids: array<CorporateRaid>;
    private static var facilityLayouts: array<FacilityLayout>;
    private static var raidUI: ref<CorporateRaidsUI>;
    private static var planningUI: ref<RaidPlanningUI>;
    
    // Network callbacks
    private static cb func OnRaidCreated(data: String) -> Void;
    private static cb func OnPlayerJoinRaid(data: String) -> Void;
    private static cb func OnRaidPhaseChange(data: String) -> Void;
    private static cb func OnObjectiveUpdate(data: String) -> Void;
    private static cb func OnPlanUpdate(data: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_RAIDS", "Initializing corporate raids system...");
        
        // Initialize facility layouts
        CorporateRaids.InitializeFacilities();
        
        // Generate available raids
        CorporateRaids.GenerateAvailableRaids();
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("raid_created", CorporateRaids.OnRaidCreated);
        NetworkingSystem.RegisterCallback("raid_player_join", CorporateRaids.OnPlayerJoinRaid);
        NetworkingSystem.RegisterCallback("raid_phase_change", CorporateRaids.OnRaidPhaseChange);
        NetworkingSystem.RegisterCallback("raid_objective_update", CorporateRaids.OnObjectiveUpdate);
        NetworkingSystem.RegisterCallback("raid_plan_update", CorporateRaids.OnPlanUpdate);
        
        isInitialized = true;
        LogChannel(n"COOP_RAIDS", "Corporate raids system initialized with " + ToString(ArraySize(availableRaids)) + " available raids");
    }
    
    private static func InitializeFacilities() -> Void {
        ArrayClear(facilityLayouts);
        
        // Arasaka Tower
        CorporateRaids.CreateFacility("arasaka_tower", "Arasaka Tower", CorporateTarget.Arasaka, 10, 
            ["R&D Labs", "Executive Floors", "Data Center", "Security Command", "Server Farm"]);
        
        // Militech Compound
        CorporateRaids.CreateFacility("militech_compound", "Militech Research Facility", CorporateTarget.Militech, 8,
            ["Weapons Testing", "Cybernetics Lab", "Command Center", "Armory", "Vehicle Bay"]);
        
        // KangTao Manufacturing
        CorporateRaids.CreateFacility("kangtao_factory", "KangTao Manufacturing Plant", CorporateTarget.KangTao, 7,
            ["Production Floor", "Quality Control", "Shipping", "Administrative", "Security"]);
        
        // Biotechnica Labs
        CorporateRaids.CreateFacility("biotechnica_labs", "Biotechnica Research Labs", CorporateTarget.Biotechnica, 9,
            ["Bio Labs", "Containment", "Research Archives", "Director's Office", "Chemical Storage"]);
        
        // Zetatech Offices
        CorporateRaids.CreateFacility("zetatech_hq", "Zetatech Headquarters", CorporateTarget.Zetatech, 6,
            ["Executive Suites", "Legal Department", "IT Infrastructure", "Accounting", "HR"]);
        
        LogChannel(n"COOP_RAIDS", "Initialized " + ToString(ArraySize(facilityLayouts)) + " corporate facilities");
    }
    
    private static func CreateFacility(id: String, name: String, corp: CorporateTarget, security: Int32, areas: array<String>) -> Void {
        let facility: FacilityLayout;
        facility.facilityId = id;
        facility.facilityName = name;
        facility.corporation = corp;
        facility.securityLevel = security;
        
        // Generate floors
        let floorCount = security / 2 + 3; // 3-8 floors based on security
        for i in Range(floorCount) {
            let floor: FacilityFloor;
            floor.floorNumber = i;
            floor.areas = areas;
            // Add security checkpoints on higher security floors
            if i > 2 && security >= 7 {
                ArrayPush(floor.securityCheckpoints, new Vector3(0.0, 0.0, Cast<Float>(i * 4)));
            }
            ArrayPush(facility.floors, floor);
        }
        
        // Generate security systems
        CorporateRaids.GenerateSecuritySystems(facility);
        
        ArrayPush(facilityLayouts, facility);
    }
    
    private static func GenerateSecuritySystems(facility: ref<FacilityLayout>) -> Void {
        // Camera system
        let cameras: SecuritySystem;
        cameras.systemId = facility.facilityId + "_cameras";
        cameras.systemType = "cameras";
        cameras.hackDifficulty = facility.securityLevel;
        cameras.detectability = 0.8;
        cameras.responseTime = 10.0;
        ArrayPush(facility.securitySystems, cameras);
        
        // Motion sensors for high security
        if facility.securityLevel >= 7 {
            let motion: SecuritySystem;
            motion.systemId = facility.facilityId + "_motion";
            motion.systemType = "motion";
            motion.hackDifficulty = facility.securityLevel + 1;
            motion.detectability = 0.9;
            motion.responseTime = 5.0;
            ArrayPush(facility.securitySystems, motion);
        }
        
        // ICE system for tech corps
        if facility.corporation == CorporateTarget.Arasaka || facility.corporation == CorporateTarget.Zetatech {
            let ice: SecuritySystem;
            ice.systemId = facility.facilityId + "_ice";
            ice.systemType = "ice";
            ice.hackDifficulty = facility.securityLevel + 2;
            ice.detectability = 0.95;
            ice.responseTime = 3.0;
            ArrayPush(facility.securitySystems, ice);
        }
    }
    
    private static func GenerateAvailableRaids() -> Void {
        ArrayClear(availableRaids);
        
        // Generate raids for each facility
        for facility in facilityLayouts {
            // Data theft raids
            CorporateRaids.CreateRaid(facility.facilityId + "_data", RaidType.DataTheft, facility.corporation, 
                facility.facilityId, facility.securityLevel);
            
            // Sabotage raids for manufacturing
            if StrContains(facility.facilityName, "Manufacturing") || StrContains(facility.facilityName, "Factory") {
                CorporateRaids.CreateRaid(facility.facilityId + "_sabotage", RaidType.Sabotage, facility.corporation,
                    facility.facilityId, facility.securityLevel + 1);
            }
            
            // Tech heist for R&D facilities
            if StrContains(facility.facilityName, "Research") || StrContains(facility.facilityName, "Labs") {
                CorporateRaids.CreateRaid(facility.facilityId + "_tech", RaidType.TechHeist, facility.corporation,
                    facility.facilityId, facility.securityLevel + 2);
            }
        }
        
        LogChannel(n"COOP_RAIDS", "Generated " + ToString(ArraySize(availableRaids)) + " available raids");
    }
    
    private static func CreateRaid(id: String, type: RaidType, corp: CorporateTarget, facility: String, difficulty: Int32) -> Void {
        let raid: CorporateRaid;
        raid.raidId = id;
        raid.raidType = type;
        raid.targetCorporation = corp;
        raid.targetFacility = facility;
        raid.currentPhase = RaidPhase.Planning;
        raid.difficulty = difficulty;
        raid.heatLevel = 0;
        raid.isActive = false;
        raid.maxDuration = 3600.0; // 1 hour max
        
        // Generate objectives
        raid.objectives = CorporateRaids.GenerateRaidObjectives(type, facility, difficulty);
        
        // Calculate rewards
        raid.rewards = CorporateRaids.CalculateRaidRewards(type, corp, difficulty);
        
        ArrayPush(availableRaids, raid);
    }
    
    public static func StartRaidPlanning(raidId: String, leaderId: String) -> String {
        // Find available raid
        let raidIndex = -1;
        for i in Range(ArraySize(availableRaids)) {
            if Equals(availableRaids[i].raidId, raidId) {
                raidIndex = i;
                break;
            }
        }
        
        if raidIndex == -1 {
            LogChannel(n"COOP_RAIDS", "Raid not found: " + raidId);
            return "";
        }
        
        let raid = availableRaids[raidIndex];
        
        // Create new active raid
        let sessionId = raidId + "_" + leaderId + "_" + ToString(GetGameTime());
        raid.raidId = sessionId;
        raid.currentPhase = RaidPhase.Planning;
        raid.planningStartTime = GetGameTime();
        raid.isActive = true;
        
        // Add leader as participant
        let leader: RaidParticipant;
        leader.playerId = leaderId;
        leader.assignedRole = RaidRole.TeamLeader;
        leader.isReady = false;
        leader.isAlive = true;
        leader.status = "planning";
        ArrayPush(raid.participants, leader);
        
        // Initialize plan
        raid.plan = CorporateRaids.CreateBasePlan(raid);
        
        ArrayPush(activeRaids, raid);
        
        LogChannel(n"COOP_RAIDS", "Started raid planning: " + sessionId + " led by " + leaderId);
        
        // Broadcast raid creation
        let raidData = CorporateRaids.SerializeRaid(raid);
        NetworkingSystem.BroadcastMessage("raid_created", raidData);
        
        // Show planning UI
        CorporateRaids.ShowPlanningUI(raid);
        
        return sessionId;
    }
    
    public static func JoinRaid(raidId: String, playerId: String, preferredRole: RaidRole) -> Bool {
        let raidIndex = CorporateRaids.FindActiveRaidIndex(raidId);
        if raidIndex == -1 {
            LogChannel(n"COOP_RAIDS", "Active raid not found: " + raidId);
            return false;
        }
        
        let raid = activeRaids[raidIndex];
        
        if raid.currentPhase != RaidPhase.Planning {
            LogChannel(n"COOP_RAIDS", "Cannot join raid - not in planning phase");
            return false;
        }
        
        // Check if player already in raid
        for participant in raid.participants {
            if Equals(participant.playerId, playerId) {
                LogChannel(n"COOP_RAIDS", "Player already in raid");
                return false;
            }
        }
        
        // Check role availability
        if !CorporateRaids.IsRoleAvailable(raid, preferredRole)) {
            LogChannel(n"COOP_RAIDS", "Preferred role not available, assigning alternative");
            preferredRole = CorporateRaids.GetAvailableRole(raid);
        }
        
        if preferredRole == RaidRole.TeamLeader {
            LogChannel(n"COOP_RAIDS", "Team leader role already taken");
            return false;
        }
        
        // Add player to raid
        let participant: RaidParticipant;
        participant.playerId = playerId;
        participant.assignedRole = preferredRole;
        participant.isReady = false;
        participant.isAlive = true;
        participant.status = "planning";
        ArrayPush(raid.participants, participant);
        
        activeRaids[raidIndex] = raid;
        
        LogChannel(n"COOP_RAIDS", "Player " + playerId + " joined raid as " + ToString(Cast<Int32>(preferredRole)));
        
        // Broadcast player join
        let joinData = raidId + "|" + playerId + "|" + ToString(Cast<Int32>(preferredRole));
        NetworkingSystem.BroadcastMessage("raid_player_join", joinData);
        
        return true;
    }
    
    public static func UpdateRaidPlan(raidId: String, playerId: String, planData: RaidPlan) -> Bool {
        let raidIndex = CorporateRaids.FindActiveRaidIndex(raidId);
        if raidIndex == -1 {
            return false;
        }
        
        let raid = activeRaids[raidIndex];
        
        // Verify player is team leader or has planning permissions
        let hasPermission = false;
        for participant in raid.participants {
            if Equals(participant.playerId, playerId) && participant.assignedRole == RaidRole.TeamLeader {
                hasPermission = true;
                break;
            }
        }
        
        if !hasPermission {
            LogChannel(n"COOP_RAIDS", "Player lacks permission to update plan");
            return false;
        }
        
        // Update plan
        raid.plan = planData;
        activeRaids[raidIndex] = raid;
        
        LogChannel(n"COOP_RAIDS", "Raid plan updated by " + playerId);
        
        // Broadcast plan update
        let updateData = CorporateRaids.SerializePlan(raidId, planData);
        NetworkingSystem.BroadcastMessage("raid_plan_update", updateData);
        
        return true;
    }
    
    public static func SetPlayerReady(raidId: String, playerId: String, isReady: Bool) -> Void {
        let raidIndex = CorporateRaids.FindActiveRaidIndex(raidId);
        if raidIndex == -1 {
            return;
        }
        
        let raid = activeRaids[raidIndex];
        
        // Update player ready status
        for i in Range(ArraySize(raid.participants)) {
            if Equals(raid.participants[i].playerId, playerId) {
                raid.participants[i].isReady = isReady;
                break;
            }
        }
        
        activeRaids[raidIndex] = raid;
        
        // Check if all players are ready
        if CorporateRaids.AllPlayersReady(raid) && ArraySize(raid.participants) >= 2 {
            CorporateRaids.AdvanceToPreparation(raidIndex);
        }
    }
    
    private static func AdvanceToPreparation(raidIndex: Int32) -> Void {
        let raid = activeRaids[raidIndex];
        raid.currentPhase = RaidPhase.Preparation;
        
        // Set preparation time (5 minutes)
        let prepTime = 300.0;
        raid.raidStartTime = GetGameTime() + prepTime;
        
        activeRaids[raidIndex] = raid;
        
        LogChannel(n"COOP_RAIDS", "Raid " + raid.raidId + " advanced to preparation phase");
        
        // Broadcast phase change
        let phaseData = raid.raidId + "|" + ToString(Cast<Int32>(RaidPhase.Preparation)) + "|" + ToString(prepTime);
        NetworkingSystem.BroadcastMessage("raid_phase_change", phaseData);
        
        // Update UI
        CorporateRaids.ShowPreparationUI(raid);
        
        // Auto-advance to infiltration after prep time
        DelaySystem.DelayCallback(CorporateRaids.StartInfiltration, prepTime, raidIndex);
    }
    
    private static func StartInfiltration(raidIndex: Int32) -> Void {
        if raidIndex >= ArraySize(activeRaids) {
            return; // Raid may have been cancelled
        }
        
        let raid = activeRaids[raidIndex];
        raid.currentPhase = RaidPhase.Infiltration;
        raid.raidStartTime = GetGameTime();
        
        // Position players at entry point
        CorporateRaids.PositionPlayersForInfiltration(raid);
        
        activeRaids[raidIndex] = raid;
        
        LogChannel(n"COOP_RAIDS", "Raid " + raid.raidId + " infiltration phase started");
        
        // Broadcast phase change
        let phaseData = raid.raidId + "|" + ToString(Cast<Int32>(RaidPhase.Infiltration));
        NetworkingSystem.BroadcastMessage("raid_phase_change", phaseData);
        
        // Show raid execution UI
        CorporateRaids.ShowExecutionUI(raid);
        
        // Start monitoring objectives
        CorporateRaids.StartObjectiveMonitoring(raidIndex);
    }
    
    public static func UpdateObjectiveProgress(raidId: String, objectiveId: String, playerId: String, progress: Float) -> Void {
        let raidIndex = CorporateRaids.FindActiveRaidIndex(raidId);
        if raidIndex == -1 {
            return;
        }
        
        let raid = activeRaids[raidIndex];
        
        // Update objective progress
        for i in Range(ArraySize(raid.objectives)) {
            let objective = raid.objectives[i];
            if Equals(objective.objectiveId, objectiveId) {
                objective.progressPercent = MinF(100.0, objective.progressPercent + progress);
                
                if objective.progressPercent >= 100.0 {
                    objective.isCompleted = true;
                    LogChannel(n"COOP_RAIDS", "Objective completed: " + objectiveId + " by " + playerId);
                }
                
                raid.objectives[i] = objective;
                break;
            }
        }
        
        activeRaids[raidIndex] = raid;
        
        // Check raid completion
        CorporateRaids.CheckRaidCompletion(raidIndex);
        
        // Broadcast objective update
        let updateData = raidId + "|" + objectiveId + "|" + playerId + "|" + ToString(progress);
        NetworkingSystem.BroadcastMessage("raid_objective_update", updateData);
    }
    
    private static func CheckRaidCompletion(raidIndex: Int32) -> Void {
        let raid = activeRaids[raidIndex];
        
        // Check if all primary objectives are complete
        let primaryCompleted = true;
        for objective in raid.objectives {
            if Equals(objective.type, "primary") && !objective.isCompleted {
                primaryCompleted = false;
                break;
            }
        }
        
        // Check if time limit exceeded
        let timeElapsed = GetGameTime() - raid.raidStartTime;
        let timeExpired = timeElapsed >= raid.maxDuration;
        
        if primaryCompleted {
            CorporateRaids.CompleteRaid(raidIndex, true);
        } else if timeExpired || raid.heatLevel >= 10 {
            CorporateRaids.CompleteRaid(raidIndex, false);
        }
    }
    
    private static func CompleteRaid(raidIndex: Int32, success: Bool) -> Void {
        let raid = activeRaids[raidIndex];
        raid.currentPhase = success ? RaidPhase.Completed : RaidPhase.Failed;
        raid.isActive = false;
        
        LogChannel(n"COOP_RAIDS", "Raid " + raid.raidId + " completed. Success: " + ToString(success));
        
        // Distribute rewards
        if success {
            CorporateRaids.DistributeRaidRewards(raid);
        } else {
            CorporateRaids.ApplyFailureConsequences(raid);
        }
        
        // Broadcast completion
        let completionData = raid.raidId + "|" + ToString(success);
        NetworkingSystem.BroadcastMessage("raid_completed", completionData);
        
        // Clean up
        ArrayErase(activeRaids, raidIndex);
        
        // Hide UI
        if IsDefined(raidUI) {
            raidUI.Hide();
        }
    }
    
    private static func DistributeRaidRewards(raid: CorporateRaid) -> Void {
        let baseRewards = raid.rewards;
        
        for participant in raid.participants {
            if !participant.isAlive {
                continue; // Dead players get reduced rewards
            }
            
            let performanceMultiplier = participant.contribution;
            let rewards: RaidRewards;
            rewards.baseXP = Cast<Int32>(Cast<Float>(baseRewards.baseXP) * performanceMultiplier);
            rewards.eddiesReward = Cast<Int32>(Cast<Float>(baseRewards.eddiesReward) * performanceMultiplier);
            rewards.streetCredGain = baseRewards.streetCredGain;
            
            // Apply rewards to player
            CorporateRaids.ApplyPlayerRewards(participant.playerId, rewards);
            
            LogChannel(n"COOP_RAIDS", "Distributed rewards to " + participant.playerId + ": " + ToString(rewards.baseXP) + " XP, " + ToString(rewards.eddiesReward) + " eddies");
        }
    }
    
    // Utility functions
    private static func FindActiveRaidIndex(raidId: String) -> Int32 {
        for i in Range(ArraySize(activeRaids)) {
            if Equals(activeRaids[i].raidId, raidId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func IsRoleAvailable(raid: CorporateRaid, role: RaidRole) -> Bool {
        for participant in raid.participants {
            if participant.assignedRole == role {
                return false;
            }
        }
        return true;
    }
    
    private static func GetAvailableRole(raid: CorporateRaid) -> RaidRole {
        let allRoles: array<RaidRole> = [
            RaidRole.Netrunner, RaidRole.TechSpecialist, RaidRole.Soldier,
            RaidRole.Infiltrator, RaidRole.Medic, RaidRole.Driver, RaidRole.FaceDancer
        ];
        
        for role in allRoles {
            if CorporateRaids.IsRoleAvailable(raid, role) {
                return role;
            }
        }
        
        return RaidRole.Soldier; // Default fallback
    }
    
    private static func AllPlayersReady(raid: CorporateRaid) -> Bool {
        for participant in raid.participants {
            if !participant.isReady {
                return false;
            }
        }
        return true;
    }
    
    // Additional utility functions would be implemented here...
    
    // Public API
    public static func GetAvailableRaids() -> array<CorporateRaid> {
        return availableRaids;
    }
    
    public static func GetActiveRaids() -> array<CorporateRaid> {
        return activeRaids;
    }
    
    public static func GetPlayerActiveRaid(playerId: String) -> CorporateRaid {
        for raid in activeRaids {
            for participant in raid.participants {
                if Equals(participant.playerId, playerId) {
                    return raid;
                }
            }
        }
        
        let emptyRaid: CorporateRaid;
        return emptyRaid;
    }
    
    public static func GetFacilityLayout(facilityId: String) -> FacilityLayout {
        for facility in facilityLayouts {
            if Equals(facility.facilityId, facilityId) {
                return facility;
            }
        }
        
        let emptyFacility: FacilityLayout;
        return emptyFacility;
    }
}