// Honor-Based Dueling System for Cyberpunk 2077 Multiplayer
// Formal one-on-one combat with spectator modes and honor rankings

module DuelSystem

enum DuelType {
    Traditional = 0,      // Pistols at dawn style
    CyberEnhanced = 1,    // Full cyberware allowed
    Melee = 2,           // Melee weapons only
    Netrunning = 3,      // Hacking duel
    Vehicle = 4,         // Vehicular combat
    Mixed = 5,           // Any weapons
    Survival = 6,        // Last person standing
    FirstBlood = 7,      // First hit wins
    Elimination = 8      // Fight to incapacitation
}

enum DuelRules {
    NoRules = 0,
    NoLethal = 1,
    NoImplants = 2,
    StandardLoadout = 3,
    CustomLoadout = 4,
    Vintage = 5,         // Old-school weapons
    Corporate = 6,       // Corporate rules
    Street = 7,          // Street rules
    Formal = 8           // Traditional formal duel
}

enum DuelStatus {
    Challenge = 0,
    Accepted = 1,
    Preparing = 2,
    CountDown = 3,
    Active = 4,
    Paused = 5,
    Completed = 6,
    Forfeited = 7,
    Canceled = 8,
    Disputed = 9
}

enum HonorRank {
    Disgraced = 0,
    Novice = 1,
    Respected = 2,
    Honored = 3,
    Renowned = 4,
    Legendary = 5,
    Mythical = 6
}

struct DuelChallenge {
    let challengeId: String;
    let challengerId: String;
    let challengedId: String;
    let duelType: DuelType;
    let rules: DuelRules;
    let stakes: Int32;
    let wager: String; // "money", "reputation", "item", "custom"
    let location: Vector4;
    let proposedTime: Float;
    let message: String;
    let timeout: Float;
    let isPublic: Bool;
    let witnessesRequired: Int32;
    let customConditions: array<String>;
    let allowedWeapons: array<String>;
    let forbiddenItems: array<String>;
    let responseTime: Float;
}

struct DuelMatch {
    let duelId: String;
    let challengeId: String;
    let duelist1: String;
    let duelist2: String;
    let duelType: DuelType;
    let rules: DuelRules;
    let status: DuelStatus;
    let location: Vector4;
    let startTime: Float;
    let endTime: Float;
    let duration: Float;
    let winner: String;
    let loser: String;
    let outcome: String; // "victory", "draw", "forfeit", "no_contest"
    let spectators: array<String>;
    let witnesses: array<String>;
    let referee: String;
    let stakes: Int32;
    let wagerType: String;
    let damageDealt: array<Float>; // [duelist1, duelist2]
    let shotsLanded: array<Int32>;
    let accuracy: array<Float>;
    let honorGained: array<Int32>;
    let honorLost: array<Int32>;
    let finalScore: String;
    let recordings: array<String>; // Video evidence
}

struct DuelistProfile {
    let playerId: String;
    let duelistName: String;
    let honorRank: HonorRank;
    let honorPoints: Int32;
    let reputation: Float;
    let totalDuels: Int32;
    let victories: Int32;
    let defeats: Int32;
    let draws: Int32;
    let forfeits: Int32;
    let winRate: Float;
    let favoriteWeapon: String;
    let specialties: array<DuelType>;
    let averageDuelTime: Float;
    let longestStreak: Int32;
    let currentStreak: Int32;
    let totalHonorEarned: Int32;
    let totalHonorLost: Int32;
    let isAcceptingChallenges: Bool;
    let preferredRules: DuelRules;
    let blacklistedPlayers: array<String>;
    let endorsements: array<String>; // Other players who vouch for them
    let lastDuelTime: Float;
    let tournamentWins: Int32;
}

struct DuelArena {
    let arenaId: String;
    let name: String;
    let description: String;
    let location: Vector4;
    let arenaType: String; // "rooftop", "warehouse", "street", "corporate", "wasteland"
    let suitableDuelTypes: array<DuelType>;
    let maxSpectators: Int32;
    let hasReferee: Bool;
    let securityLevel: Int32;
    let ambiance: String;
    let specialFeatures: array<String>;
    let isReserved: Bool;
    let reservedBy: String;
    let reservationEnd: Float;
    let usageCount: Int32;
    let rating: Float;
}

struct HonorCode {
    let codeId: String;
    let codeName: String;
    let description: String;
    let rules: array<String>;
    let penalties: array<String>;
    let rewards: array<String>;
    let authorId: String;
    let adoptedBy: array<String>;
    let isOfficial: Bool;
    let creationDate: Float;
    let lastModified: Float;
}

class DuelSystem {
    private static let instance: ref<DuelSystem>;
    private static let activeChallenges: array<DuelChallenge>;
    private static let activeDuels: array<DuelMatch>;
    private static let duelistProfiles: array<DuelistProfile>;
    private static let duelHistory: array<DuelMatch>;
    private static let duelArenas: array<DuelArena>;
    private static let honorCodes: array<HonorCode>;
    
    // System configuration
    private static let challengeTimeout: Float = 600.0; // 10 minutes to respond
    private static let preparationTime: Float = 60.0; // 1 minute preparation
    private static let countdownTime: Float = 10.0; // 10 second countdown
    private static let maxSpectators: Int32 = 20;
    private static let minHonorForChallenge: Int32 = 100;
    private static let duelCooldown: Float = 300.0; // 5 minutes between duels
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new DuelSystem();
        InitializeArenas();
        InitializeHonorCodes();
        LogChannel(n"DuelSystem", "Honor-based dueling system initialized");
    }
    
    private static func InitializeArenas() -> Void {
        // Corporate Rooftop Arena
        let corporateRoof: DuelArena;
        corporateRoof.arenaId = "corp_rooftop_01";
        corporateRoof.name = "Arasaka Tower Helipad";
        corporateRoof.description = "High-altitude corporate dueling ground with city views";
        corporateRoof.location = Vector4.Create(100.0, 200.0, 150.0, 1.0);
        corporateRoof.arenaType = "corporate";
        ArrayPush(corporateRoof.suitableDuelTypes, DuelType.Traditional);
        ArrayPush(corporateRoof.suitableDuelTypes, DuelType.Formal);
        ArrayPush(corporateRoof.suitableDuelTypes, DuelType.CyberEnhanced);
        corporateRoof.maxSpectators = 25;
        corporateRoof.hasReferee = true;
        corporateRoof.securityLevel = 8;
        corporateRoof.ambiance = "Professional";
        ArrayPush(corporateRoof.specialFeatures, "Holographic Recording");
        ArrayPush(corporateRoof.specialFeatures, "Medical Bay");
        corporateRoof.rating = 4.8;
        ArrayPush(duelArenas, corporateRoof);
        
        // Street Arena
        let streetArena: DuelArena;
        streetArena.arenaId = "street_arena_01";
        streetArena.name = "Night City Underpass";
        streetArena.description = "Underground street fighting arena";
        streetArena.location = Vector4.Create(-50.0, 100.0, -10.0, 1.0);
        streetArena.arenaType = "street";
        ArrayPush(streetArena.suitableDuelTypes, DuelType.Melee);
        ArrayPush(streetArena.suitableDuelTypes, DuelType.Mixed);
        ArrayPush(streetArena.suitableDuelTypes, DuelType.Survival);
        streetArena.maxSpectators = 30;
        streetArena.hasReferee = false;
        streetArena.securityLevel = 3;
        streetArena.ambiance = "Gritty";
        ArrayPush(streetArena.specialFeatures, "Betting Ring");
        ArrayPush(streetArena.specialFeatures, "Crowd Noise");
        streetArena.rating = 4.2;
        ArrayPush(duelArenas, streetArena);
        
        // Wasteland Arena
        let wasteland: DuelArena;
        wasteland.arenaId = "wasteland_01";
        wasteland.name = "Badlands Crater";
        wasteland.description = "Remote wasteland dueling ground";
        wasteland.location = Vector4.Create(500.0, 300.0, 5.0, 1.0);
        wasteland.arenaType = "wasteland";
        ArrayPush(wasteland.suitableDuelTypes, DuelType.Vehicle);
        ArrayPush(wasteland.suitableDuelTypes, DuelType.Traditional);
        ArrayPush(wasteland.suitableDuelTypes, DuelType.Elimination);
        wasteland.maxSpectators = 15;
        wasteland.hasReferee = false;
        wasteland.securityLevel = 2;
        wasteland.ambiance = "Desolate";
        ArrayPush(wasteland.specialFeatures, "Open Space");
        ArrayPush(wasteland.specialFeatures, "Vehicle Access");
        wasteland.rating = 3.9;
        ArrayPush(duelArenas, wasteland);
        
        // Netrunner Cyberspace
        let cyberspace: DuelArena;
        cyberspace.arenaId = "cyberspace_01";
        cyberspace.name = "Virtual Dojo";
        cyberspace.description = "Cyberspace arena for netrunning duels";
        cyberspace.location = Vector4.Create(0.0, 0.0, 100.0, 1.0);
        cyberspace.arenaType = "virtual";
        ArrayPush(cyberspace.suitableDuelTypes, DuelType.Netrunning);
        cyberspace.maxSpectators = 50; // Virtual space allows more
        cyberspace.hasReferee = true;
        cyberspace.securityLevel = 10;
        cyberspace.ambiance = "Digital";
        ArrayPush(cyberspace.specialFeatures, "ICE Protocols");
        ArrayPush(cyberspace.specialFeatures, "Data Visualization");
        cyberspace.rating = 4.5;
        ArrayPush(duelArenas, cyberspace);
    }
    
    private static func InitializeHonorCodes() -> Void {
        // Traditional Honor Code
        let traditional: HonorCode;
        traditional.codeId = "traditional_honor";
        traditional.codeName = "Traditional Dueling Code";
        traditional.description = "Classical honor-based dueling principles";
        ArrayPush(traditional.rules, "Accept challenges from equals in rank");
        ArrayPush(traditional.rules, "Fight with agreed weapons only");
        ArrayPush(traditional.rules, "No interference from outside parties");
        ArrayPush(traditional.rules, "Respect the outcome");
        ArrayPush(traditional.penalties, "Honor loss for rule violations");
        ArrayPush(traditional.rewards, "Honor gain for victories");
        traditional.isOfficial = true;
        traditional.creationDate = GetGameTime();
        ArrayPush(honorCodes, traditional);
        
        // Street Code
        let street: HonorCode;
        street.codeId = "street_honor";
        street.codeName = "Street Fighting Code";
        street.description = "No-holds-barred street fighting principles";
        ArrayPush(street.rules, "Anything goes except outside interference");
        ArrayPush(street.rules, "Fight until one cannot continue");
        ArrayPush(street.rules, "Respect earned through action");
        ArrayPush(street.penalties, "Loss of street cred for cowardice");
        ArrayPush(street.rewards, "Reputation boost for victories");
        street.isOfficial = true;
        street.creationDate = GetGameTime();
        ArrayPush(honorCodes, street);
    }
    
    // Challenge system
    public static func IssueDuelChallenge(challengerId: String, challengedId: String, duelType: DuelType, rules: DuelRules, stakes: Int32, message: String) -> String {
        if !CanIssueChallenge(challengerId, challengedId) {
            return "";
        }
        
        let challengeId = "challenge_" + challengerId + "_" + ToString(GetGameTime());
        
        let challenge: DuelChallenge;
        challenge.challengeId = challengeId;
        challenge.challengerId = challengerId;
        challenge.challengedId = challengedId;
        challenge.duelType = duelType;
        challenge.rules = rules;
        challenge.stakes = stakes;
        challenge.wager = DetermineWagerType(stakes);
        challenge.location = SelectSuitableArena(duelType);
        challenge.proposedTime = GetGameTime() + 300.0; // 5 minutes from now
        challenge.message = message;
        challenge.timeout = GetGameTime() + challengeTimeout;
        challenge.isPublic = ShouldMakePublic(challengerId, challengedId);
        challenge.witnessesRequired = GetRequiredWitnesses(rules);
        challenge.allowedWeapons = GetAllowedWeapons(duelType, rules);
        challenge.forbiddenItems = GetForbiddenItems(rules);
        
        ArrayPush(activeChallenges, challenge);
        
        // Notify challenged player
        let challengeData = JsonStringify(challenge);
        NetworkingSystem.SendToPlayer(challengedId, "duel_challenge_received", challengeData);
        
        // Notify spectators if public
        if challenge.isPublic {
            NetworkingSystem.BroadcastMessage("duel_challenge_issued", challengeData);
        }
        
        LogChannel(n"DuelSystem", StrCat("Duel challenge issued: ", challengeId));
        return challengeId;
    }
    
    public static func RespondToChallenge(challengeId: String, playerId: String, accept: Bool, counterConditions: array<String>) -> Bool {
        let challengeIndex = GetChallengeIndex(challengeId);
        if challengeIndex == -1 {
            return false;
        }
        
        let challenge = activeChallenges[challengeIndex];
        
        if !Equals(challenge.challengedId, playerId) {
            return false;
        }
        
        if GetGameTime() > challenge.timeout {
            // Challenge expired
            ArrayRemove(activeChallenges, challenge);
            return false;
        }
        
        challenge.responseTime = GetGameTime();
        
        if accept {
            // Accept challenge and create duel
            let duelId = CreateDuelFromChallenge(challenge);
            
            // Remove challenge from active list
            ArrayRemove(activeChallenges, challenge);
            
            let acceptData = "duel:" + duelId;
            NetworkingSystem.SendToPlayer(challenge.challengerId, "duel_challenge_accepted", acceptData);
            NetworkingSystem.SendToPlayer(challenge.challengedId, "duel_challenge_accepted", acceptData);
            
            // Notify spectators
            if challenge.isPublic {
                NetworkingSystem.BroadcastMessage("duel_accepted", acceptData);
            }
        } else {
            // Decline challenge
            ArrayRemove(activeChallenges, challenge);
            
            let declineData = "reason:declined";
            NetworkingSystem.SendToPlayer(challenge.challengerId, "duel_challenge_declined", declineData);
            
            // Minor honor impact for declining
            ApplyHonorChange(challenge.challengedId, -5);
        }
        
        return true;
    }
    
    // Duel execution
    private static func CreateDuelFromChallenge(challenge: DuelChallenge) -> String {
        let duelId = "duel_" + challenge.challengerId + "_" + challenge.challengedId + "_" + ToString(GetGameTime());
        
        let duel: DuelMatch;
        duel.duelId = duelId;
        duel.challengeId = challenge.challengeId;
        duel.duelist1 = challenge.challengerId;
        duel.duelist2 = challenge.challengedId;
        duel.duelType = challenge.duelType;
        duel.rules = challenge.rules;
        duel.status = DuelStatus.Preparing;
        duel.location = challenge.location;
        duel.startTime = 0.0;
        duel.endTime = 0.0;
        duel.stakes = challenge.stakes;
        duel.wagerType = challenge.wager;
        
        // Initialize damage tracking
        ArrayPush(duel.damageDealt, 0.0);
        ArrayPush(duel.damageDealt, 0.0);
        ArrayPush(duel.shotsLanded, 0);
        ArrayPush(duel.shotsLanded, 0);
        ArrayPush(duel.accuracy, 0.0);
        ArrayPush(duel.accuracy, 0.0);
        ArrayPush(duel.honorGained, 0);
        ArrayPush(duel.honorGained, 0);
        ArrayPush(duel.honorLost, 0);
        ArrayPush(duel.honorLost, 0);
        
        ArrayPush(activeDuels, duel);
        
        // Reserve arena
        ReserveArena(GetArenaFromLocation(challenge.location), duelId);
        
        // Start preparation phase
        StartDuelPreparation(duelId);
        
        return duelId;
    }
    
    public static func StartDuelPreparation(duelId: String) -> Void {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return;
        }
        
        let duel = activeDuels[duelIndex];
        duel.status = DuelStatus.Preparing;
        
        // Teleport duelists to arena
        TeleportToArena(duel.duelist1, duel.location, "duelist1");
        TeleportToArena(duel.duelist2, duel.location, "duelist2");
        
        // Apply loadout restrictions
        ApplyDuelLoadout(duel.duelist1, duel.duelType, duel.rules);
        ApplyDuelLoadout(duel.duelist2, duel.duelType, duel.rules);
        
        activeDuels[duelIndex] = duel;
        
        // Notify participants
        BroadcastToDuel(duelId, "duel_preparation_started", "");
        
        // Start countdown timer
        DelaySystem.DelayCallback(StartDuelCountdown, preparationTime, duelId);
        
        LogChannel(n"DuelSystem", StrCat("Started duel preparation: ", duelId));
    }
    
    public static func StartDuelCountdown(duelId: String) -> Void {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return;
        }
        
        let duel = activeDuels[duelIndex];
        duel.status = DuelStatus.CountDown;
        activeDuels[duelIndex] = duel;
        
        BroadcastToDuel(duelId, "duel_countdown_started", ToString(countdownTime));
        
        // Start actual duel after countdown
        DelaySystem.DelayCallback(StartDuel, countdownTime, duelId);
    }
    
    public static func StartDuel(duelId: String) -> Void {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return;
        }
        
        let duel = activeDuels[duelIndex];
        duel.status = DuelStatus.Active;
        duel.startTime = GetGameTime();
        activeDuels[duelIndex] = duel;
        
        // Enable combat for participants
        EnableDuelCombat(duel.duelist1, duel.duelist2);
        
        BroadcastToDuel(duelId, "duel_started", "");
        
        // Start recording if arena supports it
        StartDuelRecording(duelId);
        
        LogChannel(n"DuelSystem", StrCat("Started duel: ", duelId));
    }
    
    // Combat tracking
    public static func OnDuelDamage(duelId: String, attackerId: String, victimId: String, damage: Float, weaponUsed: String) -> Void {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return;
        }
        
        let duel = activeDuels[duelIndex];
        if duel.status != DuelStatus.Active {
            return;
        }
        
        // Track damage
        if Equals(attackerId, duel.duelist1) {
            duel.damageDealt[0] += damage;
            duel.shotsLanded[0] += 1;
        } else if Equals(attackerId, duel.duelist2) {
            duel.damageDealt[1] += damage;
            duel.shotsLanded[1] += 1;
        }
        
        activeDuels[duelIndex] = duel;
        
        // Check win conditions
        CheckDuelWinConditions(duelId);
        
        let damageData = "attacker:" + attackerId + ",damage:" + ToString(damage) + ",weapon:" + weaponUsed;
        BroadcastToDuel(duelId, "duel_damage_dealt", damageData);
    }
    
    public static func OnDuelPlayerDown(duelId: String, downedPlayerId: String, killerId: String) -> Void {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return;
        }
        
        let duel = activeDuels[duelIndex];
        if duel.status != DuelStatus.Active {
            return;
        }
        
        // Determine winner and loser
        if Equals(downedPlayerId, duel.duelist1) {
            duel.winner = duel.duelist2;
            duel.loser = duel.duelist1;
        } else {
            duel.winner = duel.duelist1;
            duel.loser = duel.duelist2;
        }
        
        duel.outcome = "victory";
        EndDuel(duelId);
    }
    
    public static func CheckDuelWinConditions(duelId: String) -> Void {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return;
        }
        
        let duel = activeDuels[duelIndex];
        
        // First blood rule
        if duel.rules == DuelRules.NoLethal && (duel.damageDealt[0] > 0.0 || duel.damageDealt[1] > 0.0) {
            if duel.damageDealt[0] > duel.damageDealt[1] {
                duel.winner = duel.duelist1;
                duel.loser = duel.duelist2;
            } else {
                duel.winner = duel.duelist2;
                duel.loser = duel.duelist1;
            }
            duel.outcome = "first_blood";
            EndDuel(duelId);
        }
        
        // Time limit (if applicable)
        if GetGameTime() - duel.startTime > GetDuelTimeLimit(duel.duelType) {
            // Determine winner by damage dealt
            if duel.damageDealt[0] > duel.damageDealt[1] {
                duel.winner = duel.duelist1;
                duel.loser = duel.duelist2;
            } else if duel.damageDealt[1] > duel.damageDealt[0] {
                duel.winner = duel.duelist2;
                duel.loser = duel.duelist1;
            } else {
                duel.outcome = "draw";
            }
            EndDuel(duelId);
        }
    }
    
    public static func EndDuel(duelId: String) -> Void {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return;
        }
        
        let duel = activeDuels[duelIndex];
        duel.status = DuelStatus.Completed;
        duel.endTime = GetGameTime();
        duel.duration = duel.endTime - duel.startTime;
        
        // Calculate final statistics
        CalculateDuelStatistics(duel);
        
        // Apply honor changes
        ApplyDuelHonorChanges(duel);
        
        // Distribute stakes
        DistributeDuelStakes(duel);
        
        // Update duelist profiles
        UpdateDuelistProfiles(duel);
        
        // Move to history
        ArrayPush(duelHistory, duel);
        ArrayRemove(activeDuels, duel);
        
        // Release arena
        ReleaseArena(GetArenaFromLocation(duel.location));
        
        // Disable combat
        DisableDuelCombat(duel.duelist1, duel.duelist2);
        
        let endData = "winner:" + duel.winner + ",outcome:" + duel.outcome + ",duration:" + ToString(duel.duration);
        BroadcastToDuel(duelId, "duel_ended", endData);
        
        LogChannel(n"DuelSystem", StrCat("Ended duel: ", duelId, " Winner: ", duel.winner));
    }
    
    // Spectator system
    public static func JoinAsSpectator(playerId: String, duelId: String) -> Bool {
        let duelIndex = GetDuelIndex(duelId);
        if duelIndex == -1 {
            return false;
        }
        
        let duel = activeDuels[duelIndex];
        
        // Can't spectate your own duel
        if Equals(playerId, duel.duelist1) || Equals(playerId, duel.duelist2) {
            return false;
        }
        
        // Check spectator limit
        let arena = GetArenaFromLocation(duel.location);
        if ArraySize(duel.spectators) >= arena.maxSpectators {
            return false;
        }
        
        ArrayPush(duel.spectators, playerId);
        activeDuels[duelIndex] = duel;
        
        // Teleport to spectator area
        TeleportToSpectatorArea(playerId, duel.location);
        
        let specData = "spectator:" + playerId;
        BroadcastToDuel(duelId, "spectator_joined", specData);
        
        return true;
    }
    
    // Honor and reputation system
    public static func RegisterDuelist(playerId: String, duelistName: String) -> Bool {
        if IsDuelistRegistered(playerId) {
            return false;
        }
        
        let profile: DuelistProfile;
        profile.playerId = playerId;
        profile.duelistName = duelistName;
        profile.honorRank = HonorRank.Novice;
        profile.honorPoints = 1000; // Starting honor
        profile.reputation = 50.0;
        profile.totalDuels = 0;
        profile.victories = 0;
        profile.defeats = 0;
        profile.draws = 0;
        profile.forfeits = 0;
        profile.winRate = 0.0;
        profile.favoriteWeapon = "";
        profile.averageDuelTime = 0.0;
        profile.longestStreak = 0;
        profile.currentStreak = 0;
        profile.totalHonorEarned = 0;
        profile.totalHonorLost = 0;
        profile.isAcceptingChallenges = true;
        profile.preferredRules = DuelRules.Formal;
        profile.lastDuelTime = 0.0;
        profile.tournamentWins = 0;
        
        ArrayPush(duelistProfiles, profile);
        
        let profileData = JsonStringify(profile);
        NetworkingSystem.SendToPlayer(playerId, "duelist_registered", profileData);
        
        LogChannel(n"DuelSystem", StrCat("Registered duelist: ", playerId));
        return true;
    }
    
    public static func ApplyHonorChange(playerId: String, honorChange: Int32) -> Void {
        let profileIndex = GetDuelistProfileIndex(playerId);
        if profileIndex == -1 {
            return;
        }
        
        let profile = duelistProfiles[profileIndex];
        profile.honorPoints += honorChange;
        profile.honorPoints = MaxI(profile.honorPoints, 0); // Can't go below 0
        
        if honorChange > 0 {
            profile.totalHonorEarned += honorChange;
        } else {
            profile.totalHonorLost += AbsI(honorChange);
        }
        
        // Update rank based on honor points
        profile.honorRank = CalculateHonorRank(profile.honorPoints);
        
        duelistProfiles[profileIndex] = profile;
        
        let honorData = "change:" + ToString(honorChange) + ",total:" + ToString(profile.honorPoints) + ",rank:" + ToString(Cast<Int32>(profile.honorRank));
        NetworkingSystem.SendToPlayer(playerId, "honor_changed", honorData);
    }
    
    // Utility functions
    private static func GetChallengeIndex(challengeId: String) -> Int32 {
        for i in Range(ArraySize(activeChallenges)) {
            if Equals(activeChallenges[i].challengeId, challengeId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetDuelIndex(duelId: String) -> Int32 {
        for i in Range(ArraySize(activeDuels)) {
            if Equals(activeDuels[i].duelId, duelId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func GetDuelistProfileIndex(playerId: String) -> Int32 {
        for i in Range(ArraySize(duelistProfiles)) {
            if Equals(duelistProfiles[i].playerId, playerId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func IsDuelistRegistered(playerId: String) -> Bool {
        return GetDuelistProfileIndex(playerId) != -1;
    }
    
    private static func CanIssueChallenge(challengerId: String, challengedId: String) -> Bool {
        // Can't challenge yourself
        if Equals(challengerId, challengedId) {
            return false;
        }
        
        // Both must be registered duelists
        if !IsDuelistRegistered(challengerId) || !IsDuelistRegistered(challengedId) {
            return false;
        }
        
        // Check if challenged player accepts challenges
        let challengedProfile = GetDuelistProfile(challengedId);
        if !challengedProfile.isAcceptingChallenges {
            return false;
        }
        
        // Check blacklist
        if ArrayContains(challengedProfile.blacklistedPlayers, challengerId) {
            return false;
        }
        
        // Check honor requirements
        let challengerProfile = GetDuelistProfile(challengerId);
        if challengerProfile.honorPoints < minHonorForChallenge {
            return false;
        }
        
        // Check cooldown
        if GetGameTime() - challengerProfile.lastDuelTime < duelCooldown {
            return false;
        }
        
        return true;
    }
    
    private static func CalculateHonorRank(honorPoints: Int32) -> HonorRank {
        if honorPoints < 100 {
            return HonorRank.Disgraced;
        } else if honorPoints < 500 {
            return HonorRank.Novice;
        } else if honorPoints < 1500 {
            return HonorRank.Respected;
        } else if honorPoints < 3000 {
            return HonorRank.Honored;
        } else if honorPoints < 5000 {
            return HonorRank.Renowned;
        } else if honorPoints < 8000 {
            return HonorRank.Legendary;
        } else {
            return HonorRank.Mythical;
        }
    }
    
    public static func GetActiveDuels() -> array<DuelMatch> {
        return activeDuels;
    }
    
    public static func GetDuelistProfile(playerId: String) -> DuelistProfile {
        for profile in duelistProfiles {
            if Equals(profile.playerId, playerId) {
                return profile;
            }
        }
        
        let emptyProfile: DuelistProfile;
        return emptyProfile;
    }
    
    public static func GetDuelHistory(playerId: String) -> array<DuelMatch> {
        let history: array<DuelMatch>;
        
        for duel in duelHistory {
            if Equals(duel.duelist1, playerId) || Equals(duel.duelist2, playerId) {
                ArrayPush(history, duel);
            }
        }
        
        return history;
    }
    
    public static func GetAvailableArenas() -> array<DuelArena> {
        return duelArenas;
    }
}