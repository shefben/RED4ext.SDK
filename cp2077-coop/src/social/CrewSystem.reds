// Crew and Social Organization System for Cyberpunk 2077 Multiplayer
// Persistent player organizations with shared resources and communication

module CrewSystem

enum CrewType {
    Gang = 0,
    Corporate = 1,
    Mercenary = 2,
    Netrunner = 3,
    Nomad = 4,
    Fixer = 5,
    Academic = 6,
    Media = 7,
    Criminal = 8,
    Social = 9
}

enum CrewRank {
    Recruit = 0,
    Member = 1,
    Veteran = 2,
    Lieutenant = 3,
    Captain = 4,
    Leader = 5,
    Founder = 6
}

enum CrewStatus {
    Active = 0,
    Recruiting = 1,
    Disbanded = 2,
    Suspended = 3,
    War = 4,
    Alliance = 5
}

enum MemberStatus {
    Active = 0,
    Inactive = 1,
    OnMission = 2,
    Suspended = 3,
    OnLeave = 4,
    Expelled = 5
}

enum CrewPermission {
    InviteMembers = 0,
    KickMembers = 1,
    ManageRanks = 2,
    AccessVault = 3,
    StartMissions = 4,
    ManageAlliances = 5,
    SpendFunds = 6,
    ModifyBase = 7,
    SetMOTD = 8,
    ViewLogs = 9
}

struct CrewData {
    let crewId: String;
    let crewName: String;
    let crewTag: String; // Short identifier like [CREW]
    let crewType: CrewType;
    let founderId: String;
    let leaderId: String;
    let creationDate: Float;
    let description: String;
    let motto: String;
    let status: CrewStatus;
    let isPublic: Bool; // Can anyone see and request to join
    let isRecruiting: Bool;
    let maxMembers: Int32;
    let currentMembers: Int32;
    let totalMembers: Int32; // Historical count
    let territory: String; // Controlled area
    let headquarters: Vector4;
    let treasury: Int32;
    let reputation: Float;
    let level: Int32; // Crew level based on activities
    let experience: Int32;
    let specialization: array<String>; // What the crew is known for
    let achievements: array<String>;
    let warDeclarations: array<String>; // Active wars
    let alliances: array<String>; // Allied crew IDs
    let rivals: array<String>; // Rival crew IDs
    let messageOfTheDay: String;
    let lastActivity: Float;
}

struct CrewMember {
    let memberId: String;
    let crewId: String;
    let playerName: String;
    let rank: CrewRank;
    let joinDate: Float;
    let lastSeen: Float;
    let status: MemberStatus;
    let permissions: array<CrewPermission>;
    let contribution: Int32; // Contribution points
    let specialties: array<String>; // Member specializations
    let missions: Int32; // Missions completed with crew
    let recruits: Int32; // People they've recruited
    let notes: String; // Leader/officer notes
    let loyaltyRating: Float; // 0.0-1.0
    let activityLevel: Float; // How active they are
    let warnings: Int32; // Disciplinary warnings
}

struct CrewVault {
    let crewId: String;
    let funds: Int32;
    let items: array<String>; // Shared equipment
    let vehicles: array<String>; // Crew vehicles
    let properties: array<String>; // Owned properties
    let contracts: array<String>; // Available crew missions
    let intel: array<String>; // Gathered intelligence
    let supplies: Int32;
    let ammunition: Int32;
    let medicalSupplies: Int32;
    let accessLog: array<String>; // Who accessed what when
}

struct CrewBase {
    let crewId: String;
    let baseId: String;
    let location: Vector4;
    let baseType: String; // "apartment", "warehouse", "compound", "mobile"
    let size: String; // "small", "medium", "large"
    let defenseRating: Int32;
    let facilities: array<String>; // "workshop", "medical", "armory", etc.
    let upgrades: array<String>;
    let capacity: Int32; // How many members can be present
    let isSecure: Bool;
    let maintenanceCost: Int32; // Daily upkeep
    let lastUpgrade: Float;
    let securityLevel: Int32;
}

struct CrewMission {
    let missionId: String;
    let crewId: String;
    let missionName: String;
    let description: String;
    let missionType: String; // "heist", "territory", "contract", "rivalry"
    let difficulty: Int32;
    let minMembers: Int32;
    let maxMembers: Int32;
    let participants: array<String>;
    let leader: String;
    let startTime: Float;
    let estimatedDuration: Float;
    let status: String; // "planned", "active", "completed", "failed"
    let objectives: array<String>;
    let rewards: array<String>;
    let risks: array<String>;
    let location: Vector4;
    let isPublic: Bool; // Other crews can see it
}

struct CrewEvent {
    let eventId: String;
    let crewId: String;
    let eventType: String; // "member_joined", "mission_completed", "war_declared", etc.
    let timestamp: Float;
    let description: String;
    let involvedMembers: array<String>;
    let impact: String; // "positive", "negative", "neutral"
    let visibility: String; // "public", "crew_only", "officers_only"
}

struct CrewAlliance {
    let allianceId: String;
    let allianceName: String;
    let memberCrews: array<String>;
    let leaderCrew: String;
    let purpose: String;
    let formationDate: Float;
    let duration: Float; // 0 = permanent
    let sharedResources: Bool;
    let mutualDefense: Bool;
    let jointOperations: Bool;
    let terms: array<String>;
    let isActive: Bool;
}

class CrewSystem {
    private static let instance: ref<CrewSystem>;
    private static let registeredCrews: array<CrewData>;
    private static let crewMembers: array<CrewMember>;
    private static let crewVaults: array<CrewVault>;
    private static let crewBases: array<CrewBase>;
    private static let crewMissions: array<CrewMission>;
    private static let crewEvents: array<CrewEvent>;
    private static let crewAlliances: array<CrewAlliance>;
    
    // System configuration
    private static let maxCrewsPerPlayer: Int32 = 3;
    private static let defaultMaxMembers: Int32 = 50;
    private static let crewCreationCost: Int32 = 100000;
    private static let dailyUpkeepBase: Int32 = 1000;
    private static let loyaltyDecayRate: Float = 0.01; // Per day inactive
    private static let maxInactiveWarnings: Int32 = 5;
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new CrewSystem();
        LogChannel(n"CrewSystem", "Crew and social organization system initialized");
    }
    
    // Crew creation and management
    public static func CreateCrew(founderId: String, crewName: String, crewTag: String, crewType: CrewType, description: String, motto: String) -> String {
        if !CanCreateCrew(founderId) {
            return "";
        }
        
        if !IsCrewNameAvailable(crewName) || !IsCrewTagAvailable(crewTag) {
            return "";
        }
        
        if !EconomySystem.HasFunds(founderId, crewCreationCost) {
            return "";
        }
        
        let crewId = "crew_" + founderId + "_" + ToString(GetGameTime());
        
        let crew: CrewData;
        crew.crewId = crewId;
        crew.crewName = crewName;
        crew.crewTag = crewTag;
        crew.crewType = crewType;
        crew.founderId = founderId;
        crew.leaderId = founderId;
        crew.creationDate = GetGameTime();
        crew.description = description;
        crew.motto = motto;
        crew.status = CrewStatus.Active;
        crew.isPublic = true;
        crew.isRecruiting = true;
        crew.maxMembers = defaultMaxMembers;
        crew.currentMembers = 1;
        crew.totalMembers = 1;
        crew.territory = "";
        crew.headquarters = PlayerSystem.GetPlayerLocation(founderId);
        crew.treasury = 10000; // Starting funds
        crew.reputation = 100.0;
        crew.level = 1;
        crew.experience = 0;
        crew.messageOfTheDay = "Welcome to " + crewName + "!";
        crew.lastActivity = GetGameTime();
        
        ArrayPush(registeredCrews, crew);
        
        // Create founder membership
        CreateFounderMembership(founderId, crewId);
        
        // Create crew vault
        CreateCrewVault(crewId);
        
        // Charge creation fee
        EconomySystem.ChargeFunds(founderId, crewCreationCost);
        
        let crewData = JsonStringify(crew);
        NetworkingSystem.SendToPlayer(founderId, "crew_created", crewData);
        NetworkingSystem.BroadcastMessage("crew_announced", crewData);
        
        LogChannel(n"CrewSystem", StrCat("Created crew: ", crewId));
        return crewId;
    }
    
    public static func JoinCrew(playerId: String, crewId: String) -> Bool {
        let crewIndex = GetCrewIndex(crewId);
        if crewIndex == -1 {
            return false;
        }
        
        let crew = registeredCrews[crewIndex];
        
        if !CanJoinCrew(playerId, crew) {
            return false;
        }
        
        // Create membership
        let member: CrewMember;
        member.memberId = playerId;
        member.crewId = crewId;
        member.playerName = PlayerSystem.GetPlayerName(playerId);
        member.rank = CrewRank.Recruit;
        member.joinDate = GetGameTime();
        member.lastSeen = GetGameTime();
        member.status = MemberStatus.Active;
        member.contribution = 0;
        member.missions = 0;
        member.recruits = 0;
        member.loyaltyRating = 0.5; // Neutral starting loyalty
        member.activityLevel = 1.0;
        member.warnings = 0;
        
        // Basic permissions for recruits
        ArrayPush(member.permissions, CrewPermission.ViewLogs);
        
        ArrayPush(crewMembers, member);
        
        // Update crew member count
        crew.currentMembers += 1;
        crew.totalMembers += 1;
        crew.lastActivity = GetGameTime();
        registeredCrews[crewIndex] = crew;
        
        // Record event
        RecordCrewEvent(crewId, "member_joined", playerId + " joined the crew", [playerId], "positive", "crew_only");
        
        // Notify crew
        let joinData = "player:" + member.playerName + ",rank:" + ToString(Cast<Int32>(member.rank));
        BroadcastToCrew(crewId, "member_joined", joinData);
        
        LogChannel(n"CrewSystem", StrCat("Player joined crew: ", playerId, " -> ", crewId));
        return true;
    }
    
    public static func LeaveCrew(playerId: String, crewId: String) -> Bool {
        let memberIndex = GetCrewMemberIndex(playerId, crewId);
        if memberIndex == -1 {
            return false;
        }
        
        let member = crewMembers[memberIndex];
        
        // Can't leave if you're the only leader
        if member.rank == CrewRank.Leader || member.rank == CrewRank.Founder {
            let leaderCount = GetLeaderCount(crewId);
            if leaderCount <= 1 {
                return false; // Must transfer leadership first
            }
        }
        
        // Remove member
        ArrayRemove(crewMembers, member);
        
        // Update crew member count
        let crewIndex = GetCrewIndex(crewId);
        if crewIndex != -1 {
            registeredCrews[crewIndex].currentMembers -= 1;
            registeredCrews[crewIndex].lastActivity = GetGameTime();
        }
        
        // Record event
        RecordCrewEvent(crewId, "member_left", member.playerName + " left the crew", [playerId], "negative", "crew_only");
        
        // Notify crew
        let leaveData = "player:" + member.playerName;
        BroadcastToCrew(crewId, "member_left", leaveData);
        
        LogChannel(n"CrewSystem", StrCat("Player left crew: ", playerId, " -> ", crewId));
        return true;
    }
    
    public static func PromoteMember(promoterId: String, targetId: String, crewId: String, newRank: CrewRank) -> Bool {
        if !CanPromoteMember(promoterId, targetId, crewId, newRank) {
            return false;
        }
        
        let memberIndex = GetCrewMemberIndex(targetId, crewId);
        if memberIndex == -1 {
            return false;
        }
        
        let member = crewMembers[memberIndex];
        let oldRank = member.rank;
        member.rank = newRank;
        
        // Update permissions based on rank
        UpdateMemberPermissions(member);
        
        crewMembers[memberIndex] = member;
        
        // Record event
        let promoterName = PlayerSystem.GetPlayerName(promoterId);
        RecordCrewEvent(crewId, "member_promoted", member.playerName + " promoted by " + promoterName, [promoterId, targetId], "positive", "crew_only");
        
        // Notify crew
        let promoteData = "player:" + member.playerName + ",old_rank:" + ToString(Cast<Int32>(oldRank)) + ",new_rank:" + ToString(Cast<Int32>(newRank));
        BroadcastToCrew(crewId, "member_promoted", promoteData);
        
        return true;
    }
    
    // Communication system
    public static func SendCrewMessage(senderId: String, crewId: String, message: String, channel: String) -> Void {
        let member = GetCrewMember(senderId, crewId);
        if !IsDefined(member) {
            return;
        }
        
        let timestamp = GetGameTime();
        let senderName = member.playerName;
        let rankTag = GetRankTag(member.rank);
        
        let messageData = "sender:" + senderName + ",rank:" + rankTag + ",channel:" + channel + ",message:" + message + ",time:" + ToString(timestamp);
        
        // Determine recipients based on channel
        let recipients: array<String>;
        if Equals(channel, "general") {
            recipients = GetAllCrewMembers(crewId);
        } else if Equals(channel, "officers") {
            recipients = GetOfficers(crewId);
        } else if Equals(channel, "leadership") {
            recipients = GetLeadership(crewId);
        }
        
        // Send to all recipients
        for recipientId in recipients {
            if !Equals(recipientId, senderId) { // Don't send to self
                NetworkingSystem.SendToPlayer(recipientId, "crew_message", messageData);
            }
        }
        
        // Log message for history
        RecordCrewEvent(crewId, "message_sent", "Message in " + channel + " channel", [senderId], "neutral", "crew_only");
    }
    
    public static func SetMessageOfTheDay(setterId: String, crewId: String, motd: String) -> Bool {
        if !HasPermission(setterId, crewId, CrewPermission.SetMOTD) {
            return false;
        }
        
        let crewIndex = GetCrewIndex(crewId);
        if crewIndex == -1 {
            return false;
        }
        
        registeredCrews[crewIndex].messageOfTheDay = motd;
        registeredCrews[crewIndex].lastActivity = GetGameTime();
        
        // Notify all crew members
        BroadcastToCrew(crewId, "motd_updated", motd);
        
        return true;
    }
    
    // Crew missions and activities
    public static func CreateCrewMission(creatorId: String, crewId: String, missionName: String, description: String, missionType: String, difficulty: Int32, minMembers: Int32, maxMembers: Int32) -> String {
        if !HasPermission(creatorId, crewId, CrewPermission.StartMissions) {
            return "";
        }
        
        let missionId = crewId + "_mission_" + ToString(GetGameTime());
        
        let mission: CrewMission;
        mission.missionId = missionId;
        mission.crewId = crewId;
        mission.missionName = missionName;
        mission.description = description;
        mission.missionType = missionType;
        mission.difficulty = difficulty;
        mission.minMembers = minMembers;
        mission.maxMembers = maxMembers;
        mission.leader = creatorId;
        mission.startTime = 0.0; // Will be set when mission starts
        mission.estimatedDuration = CalculateMissionDuration(missionType, difficulty);
        mission.status = "planned";
        mission.location = GetMissionLocation(missionType);
        mission.isPublic = false;
        
        // Generate objectives and rewards based on type
        mission.objectives = GenerateMissionObjectives(missionType, difficulty);
        mission.rewards = GenerateMissionRewards(missionType, difficulty);
        mission.risks = GenerateMissionRisks(missionType, difficulty);
        
        ArrayPush(crewMissions, mission);
        
        // Notify crew
        let missionData = JsonStringify(mission);
        BroadcastToCrew(crewId, "crew_mission_created", missionData);
        
        LogChannel(n"CrewSystem", StrCat("Created crew mission: ", missionId));
        return missionId;
    }
    
    public static func JoinCrewMission(playerId: String, missionId: String) -> Bool {
        let missionIndex = GetCrewMissionIndex(missionId);
        if missionIndex == -1 {
            return false;
        }
        
        let mission = crewMissions[missionIndex];
        
        if !CanJoinCrewMission(playerId, mission) {
            return false;
        }
        
        ArrayPush(mission.participants, playerId);
        crewMissions[missionIndex] = mission;
        
        // Update member status
        SetMemberStatus(playerId, mission.crewId, MemberStatus.OnMission);
        
        let joinData = "mission:" + mission.missionName + ",player:" + PlayerSystem.GetPlayerName(playerId);
        BroadcastToCrew(mission.crewId, "mission_member_joined", joinData);
        
        return true;
    }
    
    // Crew vault and resource management
    public static func DepositToVault(playerId: String, crewId: String, amount: Int32) -> Bool {
        if !HasPermission(playerId, crewId, CrewPermission.AccessVault) {
            return false;
        }
        
        if !EconomySystem.HasFunds(playerId, amount) {
            return false;
        }
        
        let vaultIndex = GetCrewVaultIndex(crewId);
        if vaultIndex == -1 {
            return false;
        }
        
        // Transfer funds
        EconomySystem.ChargeFunds(playerId, amount);
        crewVaults[vaultIndex].funds += amount;
        
        // Log transaction
        let logEntry = ToString(GetGameTime()) + ":" + PlayerSystem.GetPlayerName(playerId) + ":deposit:" + ToString(amount);
        ArrayPush(crewVaults[vaultIndex].accessLog, logEntry);
        
        // Notify crew
        let depositData = "player:" + PlayerSystem.GetPlayerName(playerId) + ",amount:" + ToString(amount);
        BroadcastToCrew(crewId, "vault_deposit", depositData);
        
        // Award contribution points
        AwardContribution(playerId, crewId, amount / 100);
        
        return true;
    }
    
    public static func WithdrawFromVault(playerId: String, crewId: String, amount: Int32, reason: String) -> Bool {
        if !HasPermission(playerId, crewId, CrewPermission.SpendFunds) {
            return false;
        }
        
        let vaultIndex = GetCrewVaultIndex(crewId);
        if vaultIndex == -1 {
            return false;
        }
        
        if crewVaults[vaultIndex].funds < amount {
            return false;
        }
        
        // Transfer funds
        crewVaults[vaultIndex].funds -= amount;
        EconomySystem.AddFunds(playerId, amount);
        
        // Log transaction
        let logEntry = ToString(GetGameTime()) + ":" + PlayerSystem.GetPlayerName(playerId) + ":withdraw:" + ToString(amount) + ":" + reason;
        ArrayPush(crewVaults[vaultIndex].accessLog, logEntry);
        
        // Notify crew officers
        let withdrawData = "player:" + PlayerSystem.GetPlayerName(playerId) + ",amount:" + ToString(amount) + ",reason:" + reason;
        NotifyOfficers(crewId, "vault_withdrawal", withdrawData);
        
        return true;
    }
    
    // Crew alliances and rivalries
    public static func ProposeAlliance(proposerId: String, proposingCrewId: String, targetCrewId: String, terms: array<String>) -> String {
        if !HasPermission(proposerId, proposingCrewId, CrewPermission.ManageAlliances) {
            return "";
        }
        
        if AreCrewsAllied(proposingCrewId, targetCrewId) {
            return "";
        }
        
        let allianceId = "alliance_" + proposingCrewId + "_" + targetCrewId + "_" + ToString(GetGameTime());
        
        // Create alliance proposal (will be pending until accepted)
        let proposal = "alliance_proposal:" + allianceId + ",terms:" + ToString(ArraySize(terms));
        
        // Notify target crew leadership
        NotifyLeadership(targetCrewId, "alliance_proposed", proposal);
        
        LogChannel(n"CrewSystem", StrCat("Alliance proposed: ", proposingCrewId, " -> ", targetCrewId));
        return allianceId;
    }
    
    public static func DeclareWar(declarerId: String, declaringCrewId: String, targetCrewId: String, reason: String) -> Bool {
        if !HasPermission(declarerId, declaringCrewId, CrewPermission.ManageAlliances) {
            return false;
        }
        
        if AreCrewsAtWar(declaringCrewId, targetCrewId) {
            return false;
        }
        
        // Add to war declarations
        let declaringIndex = GetCrewIndex(declaringCrewId);
        let targetIndex = GetCrewIndex(targetCrewId);
        
        if declaringIndex != -1 {
            ArrayPush(registeredCrews[declaringIndex].warDeclarations, targetCrewId);
            registeredCrews[declaringIndex].status = CrewStatus.War;
        }
        
        if targetIndex != -1 {
            ArrayPush(registeredCrews[targetIndex].warDeclarations, declaringCrewId);
            registeredCrews[targetIndex].status = CrewStatus.War;
        }
        
        // Broadcast war declaration
        let warData = "declaring_crew:" + GetCrewName(declaringCrewId) + ",target_crew:" + GetCrewName(targetCrewId) + ",reason:" + reason;
        NetworkingSystem.BroadcastMessage("crew_war_declared", warData);
        
        // Record events
        RecordCrewEvent(declaringCrewId, "war_declared", "War declared against " + GetCrewName(targetCrewId), [declarerId], "negative", "public");
        RecordCrewEvent(targetCrewId, "war_declared_against", "War declared by " + GetCrewName(declaringCrewId), [], "negative", "crew_only");
        
        LogChannel(n"CrewSystem", StrCat("War declared: ", declaringCrewId, " -> ", targetCrewId));
        return true;
    }
    
    // Utility functions
    private static func GetCrewIndex(crewId: String) -> Int32 {
        for i in Range(ArraySize(registeredCrews)) {
            if Equals(registeredCrews[i].crewId, crewId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetCrewMemberIndex(playerId: String, crewId: String) -> Int32 {
        for i in Range(ArraySize(crewMembers)) {
            if Equals(crewMembers[i].memberId, playerId) && Equals(crewMembers[i].crewId, crewId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func CanCreateCrew(playerId: String) -> Bool {
        // Check how many crews player owns
        let ownedCrews = 0;
        for crew in registeredCrews {
            if Equals(crew.founderId, playerId) {
                ownedCrews += 1;
            }
        }
        
        return ownedCrews < maxCrewsPerPlayer;
    }
    
    private static func CanJoinCrew(playerId: String, crew: CrewData) -> Bool {
        // Check if already a member
        if IsCrewMember(playerId, crew.crewId) {
            return false;
        }
        
        // Check if crew is recruiting
        if !crew.isRecruiting {
            return false;
        }
        
        // Check member limit
        if crew.currentMembers >= crew.maxMembers {
            return false;
        }
        
        // Check crew status
        if crew.status == CrewStatus.Disbanded || crew.status == CrewStatus.Suspended {
            return false;
        }
        
        return true;
    }
    
    private static func HasPermission(playerId: String, crewId: String, permission: CrewPermission) -> Bool {
        let member = GetCrewMember(playerId, crewId);
        if !IsDefined(member) {
            return false;
        }
        
        return ArrayContains(member.permissions, permission);
    }
    
    private static func GetCrewMember(playerId: String, crewId: String) -> CrewMember {
        for member in crewMembers {
            if Equals(member.memberId, playerId) && Equals(member.crewId, crewId) {
                return member;
            }
        }
        
        let emptyMember: CrewMember;
        return emptyMember;
    }
    
    public static func GetPlayerCrews(playerId: String) -> array<CrewData> {
        let playerCrews: array<CrewData>;
        
        // Find crews where player is a member
        for member in crewMembers {
            if Equals(member.memberId, playerId) && member.status == MemberStatus.Active {
                for crew in registeredCrews {
                    if Equals(crew.crewId, member.crewId) {
                        ArrayPush(playerCrews, crew);
                        break;
                    }
                }
            }
        }
        
        return playerCrews;
    }
    
    public static func GetCrewMembers(crewId: String) -> array<CrewMember> {
        let members: array<CrewMember>;
        
        for member in crewMembers {
            if Equals(member.crewId, crewId) && member.status != MemberStatus.Expelled {
                ArrayPush(members, member);
            }
        }
        
        return members;
    }
    
    public static func GetCrewData(crewId: String) -> CrewData {
        for crew in registeredCrews {
            if Equals(crew.crewId, crewId) {
                return crew;
            }
        }
        
        let emptyCrew: CrewData;
        return emptyCrew;
    }
    
    public static func GetPublicCrews() -> array<CrewData> {
        let publicCrews: array<CrewData>;
        
        for crew in registeredCrews {
            if crew.isPublic && crew.status == CrewStatus.Active {
                ArrayPush(publicCrews, crew);
            }
        }
        
        return publicCrews;
    }
}