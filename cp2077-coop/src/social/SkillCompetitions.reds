// Skill Competitions and Leaderboards System for Cyberpunk 2077 Multiplayer
// Specialized challenges for different character builds with global rankings

module SkillCompetitions

enum CompetitionType {
    Marksmanship = 0,
    Hacking = 1,
    Athletics = 2,
    Stealth = 3,
    Technical = 4,
    Cool = 5,
    Intelligence = 6,
    Reflex = 7,
    Body = 8,
    Mixed = 9
}

enum CompetitionFormat {
    TimeAttack = 0,
    Accuracy = 1,
    Survival = 2,
    Puzzle = 3,
    Endurance = 4,
    Score = 5,
    Elimination = 6,
    TeamBased = 7,
    Creative = 8,
    Speedrun = 9
}

enum LeaderboardType {
    Daily = 0,
    Weekly = 1,
    Monthly = 2,
    Seasonal = 3,
    AllTime = 4,
    Personal = 5
}

struct SkillChallenge {
    let challengeId: String;
    let name: String;
    let description: String;
    let competitionType: CompetitionType;
    let format: CompetitionFormat;
    let difficulty: Int32; // 1-10
    let requiredLevel: Int32;
    let requiredSkills: array<String>;
    let location: Vector4;
    let objectives: array<String>;
    let timeLimit: Float;
    let maxParticipants: Int32;
    let rewards: array<String>;
    let recordHolder: String;
    let bestScore: Float;
    let bestTime: Float;
    let totalAttempts: Int32;
    let successRate: Float;
    let isActive: Bool;
    let createdBy: String; // "system" or player ID
    let creationTime: Float;
    let tags: array<String>;
}

struct CompetitionSession {
    let sessionId: String;
    let challengeId: String;
    let participantId: String;
    let startTime: Float;
    let endTime: Float;
    let duration: Float;
    let score: Float;
    let accuracy: Float;
    let completed: Bool;
    let rank: Int32;
    let personalBest: Bool;
    let worldRecord: Bool;
    let statistics: array<String>; // JSON formatted stats
    let evidence: array<String>; // Screenshots/videos
    let witnesses: array<String>;
    let equipment: array<String>; // Gear used
    let modifiers: array<String>; // Buffs/debuffs active
}

struct LeaderboardEntry {
    let entryId: String;
    let challengeId: String;
    let playerId: String;
    let playerName: String;
    let score: Float;
    let time: Float;
    let accuracy: Float;
    let rank: Int32;
    let previousRank: Int32;
    let recordDate: Float;
    let sessionId: String;
    let isVerified: Bool;
    let videoEvidence: String;
    let competitionType: CompetitionType;
    let leaderboardType: LeaderboardType;
    let seasonId: String;
}

struct CompetitionSeason {
    let seasonId: String;
    let seasonName: String;
    let startDate: Float;
    let endDate: Float;
    let isActive: Bool;
    let featuredChallenges: array<String>;
    let specialRewards: array<String>;
    let participantCount: Int32;
    let totalPrizePool: Int32;
    let champions: array<String>; // Per competition type
    let theme: String;
    let bonusMultiplier: Float;
}

struct SkillTitle {
    let titleId: String;
    let titleName: String;
    let description: String;
    let requirements: array<String>;
    let competitionType: CompetitionType;
    let rarity: String; // "common", "rare", "epic", "legendary"
    let isActive: Bool;
    let holders: array<String>;
    let maxHolders: Int32; // -1 for unlimited
    let expirationTime: Float; // 0 for permanent
}

class SkillCompetitions {
    private static let instance: ref<SkillCompetitions>;
    private static let availableChallenges: array<SkillChallenge>;
    private static let activeSessions: array<CompetitionSession>;
    private static let leaderboards: array<LeaderboardEntry>;
    private static let competitionSeasons: array<CompetitionSeason>;
    private static let skillTitles: array<SkillTitle>;
    private static let sessionHistory: array<CompetitionSession>;
    
    // System configuration
    private static let maxDailyChallenges: Int32 = 20;
    private static let leaderboardSize: Int32 = 100;
    private static let recordVerificationTime: Float = 3600.0; // 1 hour
    private static let seasonDuration: Float = 2592000.0; // 30 days
    
    public static func Initialize() -> Void {
        if IsDefined(instance) {
            return;
        }
        instance = new SkillCompetitions();
        InitializeSystemChallenges();
        InitializeSkillTitles();
        StartNewSeason();
        LogChannel(n"SkillCompetitions", "Skill competitions and leaderboards system initialized");
    }
    
    private static func InitializeSystemChallenges() -> Void {
        // Marksmanship Challenges
        let headshots: SkillChallenge;
        headshots.challengeId = "marksmanship_headshots";
        headshots.name = "Precision Shooter";
        headshots.description = "Score as many headshots as possible within the time limit";
        headshots.competitionType = CompetitionType.Marksmanship;
        headshots.format = CompetitionFormat.Score;
        headshots.difficulty = 5;
        headshots.requiredLevel = 10;
        ArrayPush(headshots.requiredSkills, "Handguns");
        headshots.location = Vector4.Create(100.0, 200.0, 25.0, 1.0);
        ArrayPush(headshots.objectives, "Score 50+ headshots");
        ArrayPush(headshots.objectives, "Maintain 80%+ accuracy");
        headshots.timeLimit = 300.0; // 5 minutes
        headshots.maxParticipants = 1;
        ArrayPush(headshots.rewards, "Marksmanship XP");
        ArrayPush(headshots.rewards, "Weapon Mod");
        headshots.bestScore = 0.0;
        headshots.bestTime = 0.0;
        headshots.totalAttempts = 0;
        headshots.successRate = 0.0;
        headshots.isActive = true;
        headshots.createdBy = "system";
        headshots.creationTime = GetGameTime();
        ArrayPush(headshots.tags, "combat");
        ArrayPush(headshots.tags, "precision");
        ArrayPush(availableChallenges, headshots);
        
        // Hacking Challenge
        let quickhack: SkillChallenge;
        quickhack.challengeId = "hacking_breach";
        quickhack.name = "Breach Protocol Master";
        quickhack.description = "Complete complex breach protocol sequences as quickly as possible";
        quickhack.competitionType = CompetitionType.Hacking;
        quickhack.format = CompetitionFormat.TimeAttack;
        quickhack.difficulty = 7;
        quickhack.requiredLevel = 15;
        ArrayPush(quickhack.requiredSkills, "Breach Protocol");
        quickhack.location = Vector4.Create(0.0, 0.0, 100.0, 1.0); // Virtual space
        ArrayPush(quickhack.objectives, "Complete 10 breach sequences");
        ArrayPush(quickhack.objectives, "No failed attempts allowed");
        quickhack.timeLimit = 600.0; // 10 minutes
        quickhack.maxParticipants = 1;
        ArrayPush(quickhack.rewards, "Intelligence XP");
        ArrayPush(quickhack.rewards, "Quickhack");
        quickhack.bestScore = 0.0;
        quickhack.bestTime = 999999.0;
        quickhack.totalAttempts = 0;
        quickhack.successRate = 0.0;
        quickhack.isActive = true;
        quickhack.createdBy = "system";
        quickhack.creationTime = GetGameTime();
        ArrayPush(quickhack.tags, "netrunning");
        ArrayPush(quickhack.tags, "puzzle");
        ArrayPush(availableChallenges, quickhack);
        
        // Athletics Challenge
        let parkour: SkillChallenge;
        parkour.challengeId = "athletics_parkour";
        parkour.name = "Night City Parkour";
        parkour.description = "Navigate through a challenging parkour course as fast as possible";
        parkour.competitionType = CompetitionType.Athletics;
        parkour.format = CompetitionFormat.TimeAttack;
        parkour.difficulty = 6;
        parkour.requiredLevel = 8;
        ArrayPush(parkour.requiredSkills, "Athletics");
        parkour.location = Vector4.Create(50.0, 150.0, 30.0, 1.0);
        ArrayPush(parkour.objectives, "Complete the course");
        ArrayPush(parkour.objectives, "Don't fall more than 3 times");
        parkour.timeLimit = 600.0;
        parkour.maxParticipants = 1;
        ArrayPush(parkour.rewards, "Body XP");
        ArrayPush(parkour.rewards, "Movement Boost");
        parkour.bestScore = 0.0;
        parkour.bestTime = 999999.0;
        parkour.totalAttempts = 0;
        parkour.successRate = 0.0;
        parkour.isActive = true;
        parkour.createdBy = "system";
        parkour.creationTime = GetGameTime();
        ArrayPush(parkour.tags, "movement");
        ArrayPush(parkour.tags, "agility");
        ArrayPush(availableChallenges, parkour);
        
        // Stealth Challenge
        let infiltration: SkillChallenge;
        infiltration.challengeId = "stealth_infiltration";
        infiltration.name = "Ghost Protocol";
        infiltration.description = "Infiltrate a secured facility without being detected";
        infiltration.competitionType = CompetitionType.Stealth;
        infiltration.format = CompetitionFormat.Stealth;
        infiltration.difficulty = 8;
        infiltration.requiredLevel = 12;
        ArrayPush(infiltration.requiredSkills, "Stealth");
        infiltration.location = Vector4.Create(-100.0, 100.0, 20.0, 1.0);
        ArrayPush(infiltration.objectives, "Reach the target undetected");
        ArrayPush(infiltration.objectives, "Disable 5 security systems");
        ArrayPush(infiltration.objectives, "Extract without raising alarm");
        infiltration.timeLimit = 900.0; // 15 minutes
        infiltration.maxParticipants = 1;
        ArrayPush(infiltration.rewards, "Cool XP");
        ArrayPush(infiltration.rewards, "Stealth Gear");
        infiltration.bestScore = 0.0;
        infiltration.bestTime = 999999.0;
        infiltration.totalAttempts = 0;
        infiltration.successRate = 0.0;
        infiltration.isActive = true;
        infiltration.createdBy = "system";
        infiltration.creationTime = GetGameTime();
        ArrayPush(infiltration.tags, "stealth");
        ArrayPush(infiltration.tags, "infiltration");
        ArrayPush(availableChallenges, infiltration);
        
        // Technical Challenge
        let crafting: SkillChallenge;
        crafting.challengeId = "technical_crafting";
        crafting.name = "Master Craftsman";
        crafting.description = "Craft high-quality items using limited materials";
        crafting.competitionType = CompetitionType.Technical;
        crafting.format = CompetitionFormat.Score;
        crafting.difficulty = 6;
        crafting.requiredLevel = 10;
        ArrayPush(crafting.requiredSkills, "Crafting");
        crafting.location = Vector4.Create(200.0, 50.0, 15.0, 1.0);
        ArrayPush(crafting.objectives, "Craft 10 items");
        ArrayPush(crafting.objectives, "Achieve 90%+ quality rating");
        crafting.timeLimit = 1200.0; // 20 minutes
        crafting.maxParticipants = 1;
        ArrayPush(crafting.rewards, "Technical Ability XP");
        ArrayPush(crafting.rewards, "Crafting Materials");
        crafting.bestScore = 0.0;
        crafting.bestTime = 0.0;
        crafting.totalAttempts = 0;
        crafting.successRate = 0.0;
        crafting.isActive = true;
        crafting.createdBy = "system";
        crafting.creationTime = GetGameTime();
        ArrayPush(crafting.tags, "crafting");
        ArrayPush(crafting.tags, "technical");
        ArrayPush(availableChallenges, crafting);
    }
    
    private static func InitializeSkillTitles() -> Void {
        // Marksmanship Titles
        let deadeye: SkillTitle;
        deadeye.titleId = "deadeye_champion";
        deadeye.titleName = "Deadeye Champion";
        deadeye.description = "Achieved #1 rank in marksmanship competitions";
        ArrayPush(deadeye.requirements, "Rank #1 in any marksmanship challenge");
        deadeye.competitionType = CompetitionType.Marksmanship;
        deadeye.rarity = "legendary";
        deadeye.isActive = true;
        deadeye.maxHolders = 1;
        deadeye.expirationTime = 0.0; // Until dethroned
        ArrayPush(skillTitles, deadeye);
        
        // Hacking Titles
        let ghostInShell: SkillTitle;
        ghostInShell.titleId = "ghost_in_shell";
        ghostInShell.titleName = "Ghost in the Shell";
        ghostInShell.description = "Master of netrunning and digital infiltration";
        ArrayPush(ghostInShell.requirements, "Top 10 in hacking competitions");
        ArrayPush(ghostInShell.requirements, "Complete 50 hacking challenges");
        ghostInShell.competitionType = CompetitionType.Hacking;
        ghostInShell.rarity = "epic";
        ghostInShell.isActive = true;
        ghostInShell.maxHolders = 10;
        ghostInShell.expirationTime = 0.0;
        ArrayPush(skillTitles, ghostInShell);
        
        // Athletics Titles
        let cityRunner: SkillTitle;
        cityRunner.titleId = "city_runner";
        cityRunner.titleName = "City Runner";
        cityRunner.description = "Parkour master of Night City";
        ArrayPush(cityRunner.requirements, "Complete 20 athletics challenges");
        ArrayPush(cityRunner.requirements, "Sub 2-minute parkour record");
        cityRunner.competitionType = CompetitionType.Athletics;
        cityRunner.rarity = "rare";
        cityRunner.isActive = true;
        cityRunner.maxHolders = 50;
        cityRunner.expirationTime = 0.0;
        ArrayPush(skillTitles, cityRunner);
    }
    
    // Challenge participation
    public static func StartChallenge(playerId: String, challengeId: String) -> String {
        let challenge = GetChallengeById(challengeId);
        if !IsDefined(challenge) {
            return "";
        }
        
        if !CanParticipateInChallenge(playerId, challenge) {
            return "";
        }
        
        let sessionId = playerId + "_" + challengeId + "_" + ToString(GetGameTime());
        
        let session: CompetitionSession;
        session.sessionId = sessionId;
        session.challengeId = challengeId;
        session.participantId = playerId;
        session.startTime = GetGameTime();
        session.endTime = 0.0;
        session.duration = 0.0;
        session.score = 0.0;
        session.accuracy = 0.0;
        session.completed = false;
        session.rank = 0;
        session.personalBest = false;
        session.worldRecord = false;
        
        // Record player equipment and stats
        session.equipment = GetPlayerEquipment(playerId);
        session.modifiers = GetActiveModifiers(playerId);
        
        ArrayPush(activeSessions, session);
        
        // Teleport player to challenge location
        TeleportPlayerToChallenge(playerId, challenge.location, challengeId);
        
        // Initialize challenge-specific setup
        InitializeChallengeEnvironment(playerId, challengeId);
        
        let sessionData = JsonStringify(session);
        NetworkingSystem.SendToPlayer(playerId, "challenge_started", sessionData);
        
        LogChannel(n"SkillCompetitions", StrCat("Started challenge session: ", sessionId));
        return sessionId;
    }
    
    public static func UpdateChallengeProgress(sessionId: String, score: Float, accuracy: Float, objectives: array<String>) -> Void {
        let sessionIndex = GetSessionIndex(sessionId);
        if sessionIndex == -1 {
            return;
        }
        
        let session = activeSessions[sessionIndex];
        session.score = score;
        session.accuracy = accuracy;
        
        // Update objectives
        let progressData = "objectives:" + ToString(ArraySize(objectives)) + ",score:" + ToString(score) + ",accuracy:" + ToString(accuracy);
        ArrayPush(session.statistics, progressData);
        
        activeSessions[sessionIndex] = session;
        
        // Send progress update
        let updateData = "score:" + ToString(score) + ",accuracy:" + ToString(accuracy);
        NetworkingSystem.SendToPlayer(session.participantId, "challenge_progress", updateData);
    }
    
    public static func CompleteChallenge(sessionId: String, finalScore: Float, finalAccuracy: Float, completed: Bool) -> Void {
        let sessionIndex = GetSessionIndex(sessionId);
        if sessionIndex == -1 {
            return;
        }
        
        let session = activeSessions[sessionIndex];
        session.endTime = GetGameTime();
        session.duration = session.endTime - session.startTime;
        session.score = finalScore;
        session.accuracy = finalAccuracy;
        session.completed = completed;
        
        // Calculate rank and check for records
        session.rank = CalculatePlayerRank(session.challengeId, finalScore, session.duration);
        session.personalBest = IsPersonalBest(session.participantId, session.challengeId, finalScore, session.duration);
        session.worldRecord = IsWorldRecord(session.challengeId, finalScore, session.duration);
        
        // Update challenge statistics
        UpdateChallengeStatistics(session.challengeId, completed);
        
        // Add to leaderboard if qualifying score
        if ShouldAddToLeaderboard(session) {
            AddToLeaderboard(session);
        }
        
        // Check for title unlocks
        CheckTitleUnlocks(session.participantId, session);
        
        // Move to history and remove from active
        ArrayPush(sessionHistory, session);
        ArrayRemove(activeSessions, session);
        
        // Send completion notification
        let completionData = "score:" + ToString(finalScore) + ",rank:" + ToString(session.rank) + ",personal_best:" + ToString(session.personalBest) + ",world_record:" + ToString(session.worldRecord);
        NetworkingSystem.SendToPlayer(session.participantId, "challenge_completed", completionData);
        
        // Distribute rewards
        DistributeChallengeRewards(session);
        
        LogChannel(n"SkillCompetitions", StrCat("Completed challenge session: ", sessionId));
    }
    
    // Leaderboard management
    public static func GetLeaderboard(challengeId: String, leaderboardType: LeaderboardType, count: Int32) -> array<LeaderboardEntry> {
        let entries: array<LeaderboardEntry>;
        let filteredEntries: array<LeaderboardEntry>;
        
        // Filter by challenge and leaderboard type
        for entry in leaderboards {
            if Equals(entry.challengeId, challengeId) && entry.leaderboardType == leaderboardType {
                ArrayPush(filteredEntries, entry);
            }
        }
        
        // Sort by score (descending) and time (ascending)
        SortLeaderboardEntries(filteredEntries);
        
        // Return top N entries
        let returnCount = MinI(count, ArraySize(filteredEntries));
        for i in Range(returnCount) {
            ArrayPush(entries, filteredEntries[i]);
        }
        
        return entries;
    }
    
    public static func GetPlayerRanking(playerId: String, challengeId: String, leaderboardType: LeaderboardType) -> LeaderboardEntry {
        for entry in leaderboards {
            if Equals(entry.playerId, playerId) && Equals(entry.challengeId, challengeId) && entry.leaderboardType == leaderboardType {
                return entry;
            }
        }
        
        let emptyEntry: LeaderboardEntry;
        return emptyEntry;
    }
    
    public static func GetPlayerHistory(playerId: String, competitionType: CompetitionType) -> array<CompetitionSession> {
        let history: array<CompetitionSession>;
        
        for session in sessionHistory {
            if Equals(session.participantId, playerId) {
                let challenge = GetChallengeById(session.challengeId);
                if challenge.competitionType == competitionType || competitionType == CompetitionType.Mixed {
                    ArrayPush(history, session);
                }
            }
        }
        
        return history;
    }
    
    // Season management
    public static func StartNewSeason() -> String {
        let seasonId = "season_" + ToString(GetGameTime());
        
        let season: CompetitionSeason;
        season.seasonId = seasonId;
        season.seasonName = GenerateSeasonName();
        season.startDate = GetGameTime();
        season.endDate = GetGameTime() + seasonDuration;
        season.isActive = true;
        season.participantCount = 0;
        season.totalPrizePool = 100000;
        season.theme = GenerateSeasonTheme();
        season.bonusMultiplier = 1.5;
        
        // Select featured challenges
        season.featuredChallenges = SelectFeaturedChallenges();
        
        // Deactivate previous season
        for i in Range(ArraySize(competitionSeasons)) {
            competitionSeasons[i].isActive = false;
        }
        
        ArrayPush(competitionSeasons, season);
        
        // Reset seasonal leaderboards
        ResetSeasonalLeaderboards();
        
        let seasonData = JsonStringify(season);
        NetworkingSystem.BroadcastMessage("competition_season_started", seasonData);
        
        LogChannel(n"SkillCompetitions", StrCat("Started new competition season: ", seasonId));
        return seasonId;
    }
    
    public static func EndCurrentSeason() -> Void {
        let currentSeason = GetCurrentSeason();
        if !IsDefined(currentSeason) {
            return;
        }
        
        currentSeason.isActive = false;
        currentSeason.endDate = GetGameTime();
        
        // Determine season champions
        DetermineSeasonChampions(currentSeason);
        
        // Distribute season rewards
        DistributeSeasonRewards(currentSeason);
        
        let endData = JsonStringify(currentSeason);
        NetworkingSystem.BroadcastMessage("competition_season_ended", endData);
        
        // Archive seasonal data
        ArchiveSeasonData(currentSeason);
        
        LogChannel(n"SkillCompetitions", StrCat("Ended competition season: ", currentSeason.seasonId));
    }
    
    // Custom challenges
    public static func CreatePlayerChallenge(creatorId: String, name: String, description: String, competitionType: CompetitionType, format: CompetitionFormat, objectives: array<String>) -> String {
        let challengeId = "player_" + creatorId + "_" + ToString(GetGameTime());
        
        let challenge: SkillChallenge;
        challenge.challengeId = challengeId;
        challenge.name = name;
        challenge.description = description;
        challenge.competitionType = competitionType;
        challenge.format = format;
        challenge.difficulty = 5; // Default difficulty
        challenge.requiredLevel = 1;
        challenge.location = PlayerSystem.GetPlayerLocation(creatorId);
        challenge.objectives = objectives;
        challenge.timeLimit = 600.0; // 10 minutes default
        challenge.maxParticipants = 10;
        challenge.bestScore = 0.0;
        challenge.bestTime = 0.0;
        challenge.totalAttempts = 0;
        challenge.successRate = 0.0;
        challenge.isActive = true;
        challenge.createdBy = creatorId;
        challenge.creationTime = GetGameTime();
        ArrayPush(challenge.tags, "player_created");
        
        ArrayPush(availableChallenges, challenge);
        
        let challengeData = JsonStringify(challenge);
        NetworkingSystem.BroadcastMessage("player_challenge_created", challengeData);
        
        LogChannel(n"SkillCompetitions", StrCat("Created player challenge: ", challengeId));
        return challengeId;
    }
    
    // Title system
    public static func CheckTitleUnlocks(playerId: String, session: CompetitionSession) -> Void {
        for title in skillTitles {
            if !title.isActive {
                continue;
            }
            
            if HasTitle(playerId, title.titleId) {
                continue;
            }
            
            if MeetsTitleRequirements(playerId, title, session) {
                GrantTitle(playerId, title.titleId);
            }
        }
    }
    
    public static func GrantTitle(playerId: String, titleId: String) -> Bool {
        let titleIndex = GetTitleIndex(titleId);
        if titleIndex == -1 {
            return false;
        }
        
        let title = skillTitles[titleIndex];
        
        // Check holder limits
        if title.maxHolders > 0 && ArraySize(title.holders) >= title.maxHolders {
            // Remove oldest holder if necessary
            if title.maxHolders == 1 {
                RemoveTitle(title.holders[0], titleId);
                ArrayClear(title.holders);
            } else {
                return false; // Can't grant if at limit and not exclusive
            }
        }
        
        ArrayPush(title.holders, playerId);
        skillTitles[titleIndex] = title;
        
        let titleData = "title:" + title.titleName + ",rarity:" + title.rarity;
        NetworkingSystem.SendToPlayer(playerId, "title_granted", titleData);
        NetworkingSystem.BroadcastMessage("title_awarded", "player:" + playerId + ",title:" + title.titleName);
        
        LogChannel(n"SkillCompetitions", StrCat("Granted title: ", titleId, " to ", playerId));
        return true;
    }
    
    // Utility functions
    private static func GetChallengeById(challengeId: String) -> SkillChallenge {
        for challenge in availableChallenges {
            if Equals(challenge.challengeId, challengeId) {
                return challenge;
            }
        }
        
        let emptyChallenge: SkillChallenge;
        return emptyChallenge;
    }
    
    private static func GetSessionIndex(sessionId: String) -> Int32 {
        for i in Range(ArraySize(activeSessions)) {
            if Equals(activeSessions[i].sessionId, sessionId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func CanParticipateInChallenge(playerId: String, challenge: SkillChallenge) -> Bool {
        // Check level requirement
        let playerLevel = PlayerSystem.GetPlayerLevel(playerId);
        if playerLevel < challenge.requiredLevel {
            return false;
        }
        
        // Check skill requirements
        for requiredSkill in challenge.requiredSkills {
            if !PlayerSystem.HasSkillLevel(playerId, requiredSkill, 1) {
                return false;
            }
        }
        
        // Check if already participating
        for session in activeSessions {
            if Equals(session.participantId, playerId) && Equals(session.challengeId, challenge.challengeId) {
                return false;
            }
        }
        
        return true;
    }
    
    public static func GetAvailableChallenges() -> array<SkillChallenge> {
        let activeChallenges: array<SkillChallenge>;
        
        for challenge in availableChallenges {
            if challenge.isActive {
                ArrayPush(activeChallenges, challenge);
            }
        }
        
        return activeChallenges;
    }
    
    public static func GetCurrentSeason() -> CompetitionSeason {
        for season in competitionSeasons {
            if season.isActive {
                return season;
            }
        }
        
        let emptySeason: CompetitionSeason;
        return emptySeason;
    }
    
    public static func GetPlayerTitles(playerId: String) -> array<SkillTitle> {
        let playerTitles: array<SkillTitle>;
        
        for title in skillTitles {
            if ArrayContains(title.holders, playerId) {
                ArrayPush(playerTitles, title);
            }
        }
        
        return playerTitles;
    }
}