// Street Cred and Reputation System for Cyberpunk 2077 Multiplayer
// Multi-layered reputation tracking affecting available content and relationships

module ReputationSystem

enum ReputationType {
    StreetCred = 0,
    Corporate = 1,
    Gang = 2,
    Netrunner = 3,
    Fixer = 4,
    Nomad = 5,
    Media = 6,
    Government = 7,
    Underground = 8,
    Academic = 9
}

enum ReputationTier {
    Unknown = 0,
    Noticed = 1,
    Known = 2,
    Respected = 3,
    Influential = 4,
    Powerful = 5,
    Legendary = 6
}

enum ActionType {
    Mission = 0,
    Trade = 1,
    Combat = 2,
    Hacking = 3,
    Social = 4,
    Criminal = 5,
    Heroic = 6,
    Betrayal = 7,
    Alliance = 8,
    Discovery = 9
}

enum FactionStanding {
    Hostile = -3,
    Unfriendly = -2,
    Disliked = -1,
    Neutral = 0,
    Liked = 1,
    Friendly = 2,
    Allied = 3
}

struct ReputationEntry {
    let playerId: String;
    let reputationType: ReputationType;
    let points: Int32;
    let tier: ReputationTier;
    let maxPoints: Int32; // Points at current tier
    let nextTierThreshold: Int32;
    let decay: Float; // Daily decay rate
    let lastActivity: Float;
    let bonusMultiplier: Float; // From events/items
    let isVisible: Bool; // Whether other players can see it
    let achievements: array<String>;
    let milestones: array<String>;
}

struct FactionReputation {
    let playerId: String;
    let factionId: String;
    let factionName: String;
    let standing: FactionStanding;
    let trustLevel: Float; // 0.0-1.0
    let fearLevel: Float; // 0.0-1.0
    let respectLevel: Float; // 0.0-1.0
    let history: array<String>; // Recent actions affecting reputation
    let contracts: array<String>; // Available contracts from this faction
    let restrictions: array<String>; // What they won't allow you to do
    let benefits: array<String>; // Special benefits from high standing
    let lastInteraction: Float;
    let relationshipStatus: String; // "neutral", "probation", "favored", "blacklisted"
}

struct ReputationAction {
    let actionId: String;
    let playerId: String;
    let actionType: ActionType;
    let description: String;
    let timestamp: Float;
    let location: Vector4;
    let witnessCount: Int32;
    let publicityLevel: Int32; // How widely known this action becomes
    let reputationChanges: array<String>; // JSON of reputation type:change pairs
    let factionChanges: array<String>; // JSON of faction:change pairs
    let consequences: array<String>;
    let evidenceLevel: Float; // How provable the action is
    let moralAlignment: Int32; // -5 to +5 (evil to good)
}

struct ReputationEvent {
    let eventId: String;
    let eventName: String;
    let description: String;
    let participantId: String;
    let eventType: String; // "achievement", "scandal", "heroic_act", "betrayal"
    let timestamp: Float;
    let publicityRating: Int32; // 1-10 how widely known
    let credibilityRating: Float; // 0.0-1.0 how believable
    let impactLevel: Int32; // 1-5 severity of impact
    let witnesses: array<String>;
    let mediaReports: array<String>;
    let verificationStatus: String; // "rumor", "confirmed", "disputed"
    let expirationTime: Float; // When this event stops affecting reputation
}

struct InfluenceNetwork {
    let networkId: String;
    let networkName: String;
    let networkType: String; // "corporate", "criminal", "social", "academic"
    let members: array<String>; // Player IDs in this network
    let leaders: array<String>; // Key influencers
    let totalInfluence: Float;
    let networkRating: Int32; // Power level of this network
    let exclusivity: Float; // How hard it is to join
    let requirements: array<String>; // Reputation requirements to join
    let benefits: array<String>; // What membership provides
    let obligations: array<String>; // What membership requires
    let rivalNetworks: array<String>;
    let alliedNetworks: array<String>;
}

struct ReputationBoost {
    let boostId: String;
    let playerId: String;
    let boostType: String; // "item", "service", "event", "quest"
    let reputationType: ReputationType;
    let multiplier: Float; // Reputation gain multiplier
    let duration: Float; // How long the boost lasts
    let startTime: Float;
    let remainingTime: Float;
    let source: String; // What granted this boost
    let stackable: Bool;
    let conditions: array<String>; // Conditions that must be met
}

class ReputationSystem {
    private static let instance: ref<ReputationSystem>;
    private static let playerReputations: array<ReputationEntry>;
    private static let factionReputations: array<FactionReputation>;
    private static let reputationActions: array<ReputationAction>;
    private static let reputationEvents: array<ReputationEvent>;
    private static let influenceNetworks: array<InfluenceNetwork>;
    private static let activeBoosts: array<ReputationBoost>;
    
    // System configuration
    private static let dailyDecayRate: Float = 0.02; // 2% daily decay
    private static let maxReputationPerType: Int32 = 10000;
    private static let witnessMultiplier: Float = 0.1; // Reputation bonus per witness
    private static let publicityMultiplier: Float = 0.2; // Media attention multiplier
    private static let timeDecayFactor: Float = 0.1; // How much time affects action impact
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new ReputationSystem();
        InitializeFactions();
        InitializeInfluenceNetworks();
        LogChannel(n"ReputationSystem", "Street cred and reputation system initialized");
    }
    
    private static func InitializeFactions() -> Void {
        // Major factions that track reputation
        RegisterFaction("arasaka", "Arasaka Corporation");
        RegisterFaction("militech", "Militech International");
        RegisterFaction("biotechnica", "Biotechnica");
        RegisterFaction("kang_tao", "Kang Tao");
        RegisterFaction("maelstrom", "Maelstrom");
        RegisterFaction("valentinos", "Valentinos");
        RegisterFaction("tyger_claws", "Tyger Claws");
        RegisterFaction("voodoo_boys", "Voodoo Boys");
        RegisterFaction("animals", "Animals");
        RegisterFaction("scavengers", "Scavengers");
        RegisterFaction("sixth_street", "6th Street Gang");
        RegisterFaction("aldecaldos", "Aldecaldos");
        RegisterFaction("wraiths", "Wraiths");
        RegisterFaction("ncpd", "Night City Police Department");
        RegisterFaction("max_tac", "MaxTac");
        RegisterFaction("fixers", "Fixer Network");
        RegisterFaction("netwatch", "NetWatch");
        RegisterFaction("media", "Media Corps");
        RegisterFaction("academics", "Academic Network");
        RegisterFaction("street_vendors", "Street Vendor Association");
    }
    
    private static func InitializeInfluenceNetworks() -> Void {
        // Corporate Elite Network
        let corporate: InfluenceNetwork;
        corporate.networkId = "corporate_elite";
        corporate.networkName = "Corporate Elite";
        corporate.networkType = "corporate";
        corporate.totalInfluence = 85.0;
        corporate.networkRating = 9;
        corporate.exclusivity = 0.9;
        ArrayPush(corporate.requirements, "Corporate:4000");
        ArrayPush(corporate.requirements, "StreetCred:2000");
        ArrayPush(corporate.benefits, "Executive Access");
        ArrayPush(corporate.benefits, "Corporate Contracts");
        ArrayPush(corporate.benefits, "Financial Services");
        ArrayPush(corporate.obligations, "Corporate Loyalty");
        ArrayPush(corporate.obligations, "No Gang Affiliations");
        ArrayPush(influenceNetworks, corporate);
        
        // Street Legend Network
        let street: InfluenceNetwork;
        street.networkId = "street_legends";
        street.networkName = "Street Legends";
        street.networkType = "criminal";
        street.totalInfluence = 75.0;
        street.networkRating = 8;
        street.exclusivity = 0.8;
        ArrayPush(street.requirements, "StreetCred:5000");
        ArrayPush(street.requirements, "Gang:3000");
        ArrayPush(street.benefits, "Underground Access");
        ArrayPush(street.benefits, "Black Market Contacts");
        ArrayPush(street.benefits, "Protection Services");
        ArrayPush(street.obligations, "Territory Respect");
        ArrayPush(street.obligations, "No Corpo Dealings");
        ArrayPush(influenceNetworks, street);
        
        // Netrunner Collective
        let netrunner: InfluenceNetwork;
        netrunner.networkId = "net_collective";
        netrunner.networkName = "Netrunner Collective";
        netrunner.networkType = "academic";
        netrunner.totalInfluence = 65.0;
        netrunner.networkRating = 7;
        netrunner.exclusivity = 0.7;
        ArrayPush(netrunner.requirements, "Netrunner:3500");
        ArrayPush(netrunner.requirements, "Academic:2000");
        ArrayPush(netrunner.benefits, "ICE Protocols");
        ArrayPush(netrunner.benefits, "Data Markets");
        ArrayPush(netrunner.benefits, "Tech Resources");
        ArrayPush(netrunner.obligations, "Information Sharing");
        ArrayPush(netrunner.obligations, "No Malicious Hacking");
        ArrayPush(influenceNetworks, netrunner);
    }
    
    // Core reputation management
    public static func InitializePlayerReputation(playerId: String) -> Void {
        if HasPlayerReputation(playerId) {
            return;
        }
        
        // Initialize all reputation types
        for i in Range(10) { // Number of reputation types
            let repType = Cast<ReputationType>(i);
            let entry: ReputationEntry;
            entry.playerId = playerId;
            entry.reputationType = repType;
            entry.points = GetStartingPoints(repType);
            entry.tier = ReputationTier.Unknown;
            entry.maxPoints = 100; // First tier cap
            entry.nextTierThreshold = 100;
            entry.decay = dailyDecayRate;
            entry.lastActivity = GetGameTime();
            entry.bonusMultiplier = 1.0;
            entry.isVisible = true;
            
            ArrayPush(playerReputations, entry);
        }
        
        LogChannel(n"ReputationSystem", StrCat("Initialized reputation for player: ", playerId));
    }
    
    public static func RecordAction(playerId: String, actionType: ActionType, description: String, location: Vector4, witnesses: array<String>, publicityLevel: Int32) -> String {
        let actionId = playerId + "_action_" + ToString(GetGameTime());
        
        let action: ReputationAction;
        action.actionId = actionId;
        action.playerId = playerId;
        action.actionType = actionType;
        action.description = description;
        action.timestamp = GetGameTime();
        action.location = location;
        action.witnessCount = ArraySize(witnesses);
        action.publicityLevel = publicityLevel;
        action.evidenceLevel = CalculateEvidenceLevel(witnesses, publicityLevel);
        action.moralAlignment = DetermineActionMorality(actionType, description);
        
        // Calculate reputation changes based on action
        action.reputationChanges = CalculateReputationChanges(action);
        action.factionChanges = CalculateFactionChanges(action);
        
        ArrayPush(reputationActions, action);
        
        // Apply the reputation changes
        ApplyActionReputationChanges(action);
        
        // Notify witnesses and create potential events
        ProcessActionWitnesses(action, witnesses);
        
        // Check for news worthiness
        if ShouldCreateEvent(action) {
            CreateReputationEvent(action);
        }
        
        LogChannel(n"ReputationSystem", StrCat("Recorded action: ", actionId));
        return actionId;
    }
    
    public static func ModifyReputation(playerId: String, reputationType: ReputationType, change: Int32, reason: String) -> Void {
        let repIndex = GetReputationIndex(playerId, reputationType);
        if repIndex == -1 {
            InitializePlayerReputation(playerId);
            repIndex = GetReputationIndex(playerId, reputationType);
        }
        
        let reputation = playerReputations[repIndex];
        
        // Apply boost multiplier
        let adjustedChange = Cast<Int32>(Cast<Float>(change) * reputation.bonusMultiplier);
        
        reputation.points += adjustedChange;
        reputation.points = ClampI(reputation.points, 0, maxReputationPerType);
        reputation.lastActivity = GetGameTime();
        
        // Check for tier advancement
        let oldTier = reputation.tier;
        reputation.tier = CalculateReputationTier(reputation.points);
        
        if Cast<Int32>(reputation.tier) > Cast<Int32>(oldTier) {
            // Tier up!
            OnReputationTierAdvanced(playerId, reputationType, reputation.tier);
        }
        
        // Update thresholds
        UpdateTierThresholds(reputation);
        
        playerReputations[repIndex] = reputation;
        
        // Notify player
        let changeData = "type:" + ToString(Cast<Int32>(reputationType)) + ",change:" + ToString(adjustedChange) + ",total:" + ToString(reputation.points) + ",tier:" + ToString(Cast<Int32>(reputation.tier)) + ",reason:" + reason;
        NetworkingSystem.SendToPlayer(playerId, "reputation_changed", changeData);
        
        // Check for unlocked content
        CheckUnlockedContent(playerId, reputationType, reputation.points);
    }
    
    public static func ModifyFactionReputation(playerId: String, factionId: String, change: Float, reason: String) -> Void {
        let factionIndex = GetFactionReputationIndex(playerId, factionId);
        if factionIndex == -1 {
            // Create new faction reputation entry
            InitializeFactionReputation(playerId, factionId);
            factionIndex = GetFactionReputationIndex(playerId, factionId);
        }
        
        let faction = factionReputations[factionIndex];
        
        // Apply change based on current standing
        let oldStanding = faction.standing;
        
        if change > 0.0 {
            faction.trustLevel += change * 0.1;
            faction.respectLevel += change * 0.05;
        } else {
            faction.trustLevel += change * 0.15; // Trust drops faster
            faction.fearLevel += AbsF(change) * 0.1;
        }
        
        // Clamp values
        faction.trustLevel = ClampF(faction.trustLevel, 0.0, 1.0);
        faction.fearLevel = ClampF(faction.fearLevel, 0.0, 1.0);
        faction.respectLevel = ClampF(faction.respectLevel, 0.0, 1.0);
        
        // Calculate new standing
        faction.standing = CalculateFactionStanding(faction.trustLevel, faction.fearLevel, faction.respectLevel);
        faction.lastInteraction = GetGameTime();
        
        // Update relationship status
        UpdateRelationshipStatus(faction);
        
        // Add to history
        let historyEntry = ToString(GetGameTime()) + ":" + reason + ":" + ToString(change);
        ArrayPush(faction.history, historyEntry);
        
        // Keep history manageable
        if ArraySize(faction.history) > 10 {
            ArrayRemove(faction.history, faction.history[0]);
        }
        
        factionReputations[factionIndex] = faction;
        
        // Check for standing changes
        if Cast<Int32>(faction.standing) != Cast<Int32>(oldStanding) {
            OnFactionStandingChanged(playerId, factionId, faction.standing);
        }
        
        let factionData = "faction:" + factionId + ",change:" + ToString(change) + ",standing:" + ToString(Cast<Int32>(faction.standing)) + ",trust:" + ToString(faction.trustLevel) + ",reason:" + reason;
        NetworkingSystem.SendToPlayer(playerId, "faction_reputation_changed", factionData);
    }
    
    // Reputation events and publicity
    public static func CreateReputationEvent(actionData: ReputationAction) -> String {
        let eventId = "event_" + actionData.playerId + "_" + ToString(GetGameTime());
        
        let event: ReputationEvent;
        event.eventId = eventId;
        event.eventName = GenerateEventName(actionData);
        event.description = GenerateEventDescription(actionData);
        event.participantId = actionData.playerId;
        event.eventType = DetermineEventType(actionData);
        event.timestamp = GetGameTime();
        event.publicityRating = actionData.publicityLevel;
        event.credibilityRating = actionData.evidenceLevel;
        event.impactLevel = CalculateImpactLevel(actionData);
        event.verificationStatus = "rumor"; // Starts as rumor
        event.expirationTime = GetGameTime() + CalculateEventLifespan(event.impactLevel);
        
        ArrayPush(reputationEvents, event);
        
        // Broadcast event to appropriate audience
        BroadcastReputationEvent(event);
        
        // Schedule verification process
        ScheduleEventVerification(eventId);
        
        LogChannel(n"ReputationSystem", StrCat("Created reputation event: ", eventId));
        return eventId;
    }
    
    public static func VerifyEvent(eventId: String, verifierId: String, isTrue: Bool, evidence: String) -> Void {
        let eventIndex = GetEventIndex(eventId);
        if eventIndex == -1 {
            return;
        }
        
        let event = reputationEvents[eventIndex];
        
        // Add verifier
        if !ArrayContains(event.witnesses, verifierId) {
            ArrayPush(event.witnesses, verifierId);
        }
        
        // Update credibility based on verification
        if isTrue {
            event.credibilityRating += 0.2;
        } else {
            event.credibilityRating -= 0.1;
        }
        
        event.credibilityRating = ClampF(event.credibilityRating, 0.0, 1.0);
        
        // Update verification status
        if event.credibilityRating >= 0.8 {
            event.verificationStatus = "confirmed";
        } else if event.credibilityRating <= 0.3 {
            event.verificationStatus = "disputed";
        }
        
        reputationEvents[eventIndex] = event;
        
        // Adjust reputation impact based on verification
        AdjustEventReputationImpact(event);
    }
    
    // Influence networks and social connections
    public static func JoinInfluenceNetwork(playerId: String, networkId: String) -> Bool {
        let networkIndex = GetNetworkIndex(networkId);
        if networkIndex == -1 {
            return false;
        }
        
        let network = influenceNetworks[networkIndex];
        
        // Check if already a member
        if ArrayContains(network.members, playerId) {
            return false;
        }
        
        // Check requirements
        if !MeetsNetworkRequirements(playerId, network.requirements) {
            return false;
        }
        
        // Check exclusivity (random chance based on exclusivity rating)
        if RandF() > (1.0 - network.exclusivity)) {
            return false; // Failed exclusivity check
        }
        
        ArrayPush(network.members, playerId);
        network.totalInfluence += GetPlayerInfluenceContribution(playerId);
        
        influenceNetworks[networkIndex] = network;
        
        // Grant network benefits
        GrantNetworkBenefits(playerId, network.benefits);
        
        // Notify player and network
        let joinData = "network:" + networkId + ",benefits:" + ToString(ArraySize(network.benefits));
        NetworkingSystem.SendToPlayer(playerId, "influence_network_joined", joinData);
        
        BroadcastToNetwork(networkId, "member_joined", playerId);
        
        LogChannel(n"ReputationSystem", StrCat("Player joined influence network: ", playerId, " -> ", networkId));
        return true;
    }
    
    public static func CreatePlayerNetwork(creatorId: String, networkName: String, networkType: String, requirements: array<String>) -> String {
        let networkId = "player_" + creatorId + "_" + ToString(GetGameTime());
        
        let network: InfluenceNetwork;
        network.networkId = networkId;
        network.networkName = networkName;
        network.networkType = networkType;
        ArrayPush(network.members, creatorId);
        ArrayPush(network.leaders, creatorId);
        network.totalInfluence = GetPlayerInfluenceContribution(creatorId);
        network.networkRating = 1; // Start small
        network.exclusivity = 0.3; // Easy to join initially
        network.requirements = requirements;
        
        ArrayPush(influenceNetworks, network);
        
        let networkData = JsonStringify(network);
        NetworkingSystem.SendToPlayer(creatorId, "influence_network_created", networkData);
        NetworkingSystem.BroadcastMessage("new_influence_network", networkData);
        
        LogChannel(n"ReputationSystem", StrCat("Created player network: ", networkId));
        return networkId;
    }
    
    // Reputation boosts and modifiers
    public static func ApplyReputationBoost(playerId: String, boostType: String, reputationType: ReputationType, multiplier: Float, duration: Float, source: String) -> String {
        let boostId = playerId + "_boost_" + ToString(GetGameTime());
        
        let boost: ReputationBoost;
        boost.boostId = boostId;
        boost.playerId = playerId;
        boost.boostType = boostType;
        boost.reputationType = reputationType;
        boost.multiplier = multiplier;
        boost.duration = duration;
        boost.startTime = GetGameTime();
        boost.remainingTime = duration;
        boost.source = source;
        boost.stackable = IsStackableBoost(boostType);
        
        ArrayPush(activeBoosts, boost);
        
        // Update player's reputation multiplier
        UpdatePlayerReputationMultiplier(playerId, reputationType);
        
        let boostData = "type:" + ToString(Cast<Int32>(reputationType)) + ",multiplier:" + ToString(multiplier) + ",duration:" + ToString(duration) + ",source:" + source;
        NetworkingSystem.SendToPlayer(playerId, "reputation_boost_applied", boostData);
        
        LogChannel(n"ReputationSystem", StrCat("Applied reputation boost: ", boostId));
        return boostId;
    }
    
    public static func ProcessDailyDecay() -> Void {
        for i in Range(ArraySize(playerReputations)) {
            let reputation = playerReputations[i];
            
            // Only decay if inactive for more than 24 hours
            if GetGameTime() - reputation.lastActivity > 86400.0 {
                let decayAmount = Cast<Int32>(Cast<Float>(reputation.points) * reputation.decay);
                reputation.points = MaxI(reputation.points - decayAmount, 0);
                
                // Check for tier demotion
                let oldTier = reputation.tier;
                reputation.tier = CalculateReputationTier(reputation.points);
                
                if Cast<Int32>(reputation.tier) < Cast<Int32>(oldTier) {
                    OnReputationTierDemoted(reputation.playerId, reputation.reputationType, reputation.tier);
                }
                
                playerReputations[i] = reputation;
            }
        }
        
        // Clean up expired boosts
        CleanupExpiredBoosts();
        
        // Clean up expired events
        CleanupExpiredEvents();
    }
    
    // Query functions
    public static func GetPlayerReputation(playerId: String, reputationType: ReputationType) -> ReputationEntry {
        let index = GetReputationIndex(playerId, reputationType);
        if index != -1 {
            return playerReputations[index];
        }
        
        let emptyRep: ReputationEntry;
        return emptyRep;
    }
    
    public static func GetFactionReputation(playerId: String, factionId: String) -> FactionReputation {
        let index = GetFactionReputationIndex(playerId, factionId);
        if index != -1 {
            return factionReputations[index];
        }
        
        let emptyFaction: FactionReputation;
        return emptyFaction;
    }
    
    public static func GetPlayerReputationSummary(playerId: String) -> array<ReputationEntry> {
        let summary: array<ReputationEntry>;
        
        for reputation in playerReputations {
            if Equals(reputation.playerId, playerId) {
                ArrayPush(summary, reputation);
            }
        }
        
        return summary;
    }
    
    public static func GetTopReputationPlayers(reputationType: ReputationType, count: Int32) -> array<ReputationEntry> {
        let topPlayers: array<ReputationEntry>;
        let filteredReps: array<ReputationEntry>;
        
        // Filter by reputation type
        for reputation in playerReputations {
            if reputation.reputationType == reputationType && reputation.isVisible {
                ArrayPush(filteredReps, reputation);
            }
        }
        
        // Sort by points (simplified bubble sort for demonstration)
        SortReputationsByPoints(filteredReps);
        
        // Return top N
        let returnCount = MinI(count, ArraySize(filteredReps));
        for i in Range(returnCount) {
            ArrayPush(topPlayers, filteredReps[i]);
        }
        
        return topPlayers;
    }
    
    // Utility functions
    private static func GetReputationIndex(playerId: String, reputationType: ReputationType) -> Int32 {
        for i in Range(ArraySize(playerReputations)) {
            if Equals(playerReputations[i].playerId, playerId) && playerReputations[i].reputationType == reputationType {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetFactionReputationIndex(playerId: String, factionId: String) -> Int32 {
        for i in Range(ArraySize(factionReputations)) {
            if Equals(factionReputations[i].playerId, playerId) && Equals(factionReputations[i].factionId, factionId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func HasPlayerReputation(playerId: String) -> Bool {
        for reputation in playerReputations {
            if Equals(reputation.playerId, playerId) {
                return true;
            }
        }
        return false;
    }
    
    private static func CalculateReputationTier(points: Int32) -> ReputationTier {
        if points < 100 {
            return ReputationTier.Unknown;
        } else if points < 500 {
            return ReputationTier.Noticed;
        } else if points < 1500 {
            return ReputationTier.Known;
        } else if points < 3500 {
            return ReputationTier.Respected;
        } else if points < 6500 {
            return ReputationTier.Influential;
        } else if points < 9000 {
            return ReputationTier.Powerful;
        } else {
            return ReputationTier.Legendary;
        }
    }
    
    private static func CalculateFactionStanding(trust: Float, fear: Float, respect: Float) -> FactionStanding {
        let combinedScore = (trust * 2.0) + respect - (fear * 0.5);
        
        if combinedScore <= -1.5 {
            return FactionStanding.Hostile;
        } else if combinedScore <= -0.5 {
            return FactionStanding.Unfriendly;
        } else if combinedScore <= 0.0 {
            return FactionStanding.Disliked;
        } else if combinedScore <= 0.5 {
            return FactionStanding.Neutral;
        } else if combinedScore <= 1.0 {
            return FactionStanding.Liked;
        } else if combinedScore <= 1.5 {
            return FactionStanding.Friendly;
        } else {
            return FactionStanding.Allied;
        }
    }
    
    private static func GetStartingPoints(reputationType: ReputationType) -> Int32 {
        switch reputationType {
            case ReputationType.StreetCred: return 50;
            case ReputationType.Corporate: return 0;
            case ReputationType.Gang: return 25;
            case ReputationType.Netrunner: return 10;
            case ReputationType.Fixer: return 0;
            case ReputationType.Nomad: return 0;
            case ReputationType.Media: return 0;
            case ReputationType.Government: return 0;
            case ReputationType.Underground: return 20;
            case ReputationType.Academic: return 5;
            default: return 0;
        }
    }
    
    public static func GetInfluenceNetworks() -> array<InfluenceNetwork> {
        return influenceNetworks;
    }
    
    public static func GetRecentEvents(playerId: String) -> array<ReputationEvent> {
        let events: array<ReputationEvent>;
        let cutoffTime = GetGameTime() - 86400.0; // Last 24 hours
        
        for event in reputationEvents {
            if Equals(event.participantId, playerId) && event.timestamp > cutoffTime {
                ArrayPush(events, event);
            }
        }
        
        return events;
    }
    
    public static func GetActiveBoosts(playerId: String) -> array<ReputationBoost> {
        let boosts: array<ReputationBoost>;
        
        for boost in activeBoosts {
            if Equals(boost.playerId, playerId) && boost.remainingTime > 0.0 {
                ArrayPush(boosts, boost);
            }
        }
        
        return boosts;
    }
}