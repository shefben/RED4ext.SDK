// Extended game modes system for CP2077 multiplayer
// Includes specialized game modes with unique mechanics and objectives

public enum ExtendedGameMode {
    // Cooperative modes
    StoryCoOp = 0,           // Campaign with friends
    OpenWorldCoop = 1,       // Free roam cooperation
    HeistMode = 2,           // Coordinated heist missions

    // Competitive modes
    Deathmatch = 10,         // Free-for-all combat
    TeamDeathmatch = 11,     // Team-based combat
    Capture = 12,            // Territory control
    Racing = 13,             // Vehicle racing
    Braindance = 14,         // Memory investigation races

    // Objective-based modes
    Extraction = 20,         // VIP escort missions
    Sabotage = 21,          // Corporate warfare
    DataTheft = 22,         // Netrunning challenges
    Survival = 23,          // Wave-based survival

    // Role-playing modes
    Freeform = 30,          // Open-ended roleplay
    Detective = 31,         // Investigation scenarios
    Corporate = 32,         // Business simulation
    Street = 33,            // Gang warfare

    // Special event modes
    BlackOut = 40,          // City-wide blackout events
    Riot = 41,             // Civil unrest scenarios
    CyberPsycho = 42,      // Hunt dangerous criminals
    Tournament = 43         // Competitive tournaments
}

public struct GameModeConfig {
    public var modeId: ExtendedGameMode;
    public var name: String;
    public var description: String;
    public var minPlayers: Int32;
    public var maxPlayers: Int32;
    public var duration: Float; // In minutes
    public var friendlyFire: Bool;
    public var respawn: Bool;
    public var sharedInventory: Bool;
    public var allowedWeapons: array<String>;
    public var allowedCyberware: array<String>;
    public var objectives: array<GameModeObjective>;
    public var scoring: GameModeScoring;
    public var mapSettings: GameModeMap;
    public var customRules: array<String>;
}

public struct GameModeObjective {
    public var objectiveType: String;
    public var description: String;
    public var points: Int32;
    public var timeLimit: Float;
    public var isRequired: Bool;
    public var completionCondition: String;
}

public struct GameModeScoring {
    public var scoringType: String; // "kills", "objectives", "time", "points"
    public var killPoints: Int32;
    public var deathPenalty: Int32;
    public var objectivePoints: Int32;
    public var teamScoring: Bool;
    public var victoryCondition: String;
}

public struct GameModeMap {
    public var allowedAreas: array<String>;
    public var restrictedAreas: array<String>;
    public var spawnPoints: array<Vector3>;
    public var objectiveLocations: array<Vector3>;
    public var boundaryRadius: Float;
    public var boundaryCenter: Vector3;
}

public struct GameModeSession {
    public var sessionId: String;
    public var modeConfig: GameModeConfig;
    public var participants: array<GameModePlayer>;
    public var teams: array<GameModeTeam>;
    public var currentPhase: String;
    public var startTime: Float;
    public var endTime: Float;
    public var scores: array<PlayerScore>;
    public var events: array<GameModeEvent>;
    public var isActive: Bool;
    public var isPaused: Bool;
}

public struct GameModePlayer {
    public var playerId: String;
    public var playerName: String;
    public var teamId: String;
    public var role: String;
    public var isReady: Bool;
    public var isAlive: Bool;
    public var currentScore: Int32;
    public var kills: Int32;
    public var deaths: Int32;
    public var assists: Int32;
    public var objectivesCompleted: Int32;
    public var joinTime: Float;
}

public struct GameModeTeam {
    public var teamId: String;
    public var teamName: String;
    public var teamColor: String;
    public var players: array<String>;
    public var teamScore: Int32;
    public var isActive: Bool;
}

public struct PlayerScore {
    public var playerId: String;
    public var score: Int32;
    public var rank: Int32;
    public var details: array<String>;
}

public struct GameModeEvent {
    public var eventId: String;
    public var eventType: String;
    public var playerId: String;
    public var teamId: String;
    public var timestamp: Float;
    public var description: String;
    public var points: Int32;
}

public class ExtendedGameModes {
    private static var isInitialized: Bool = false;
    private static var gameModeConfigs: array<GameModeConfig>;
    private static var activeSessions: array<GameModeSession>;
    private static var currentSession: ref<GameModeSession>;

    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }

        LogChannel(n"EXTENDED_MODES", "Initializing extended game modes system...");

        // Initialize all game mode configurations
        ExtendedGameModes.InitializeGameModeConfigs();

        isInitialized = true;
        LogChannel(n"EXTENDED_MODES", "Extended game modes system initialized with " + ToString(ArraySize(gameModeConfigs)) + " modes");
    }

    private static func InitializeGameModeConfigs() -> Void {
        ArrayClear(gameModeConfigs);

        // === COOPERATIVE MODES ===
        ExtendedGameModes.CreateCooperativeMode(ExtendedGameMode.StoryCoOp, "Story Co-op", "Play the main campaign with friends", 1, 4, 180.0);
        ExtendedGameModes.CreateCooperativeMode(ExtendedGameMode.OpenWorldCoop, "Open World Co-op", "Explore Night City together", 1, 8, 0.0);
        ExtendedGameModes.CreateCooperativeMode(ExtendedGameMode.HeistMode, "Heist Mode", "Plan and execute coordinated heists", 2, 6, 60.0);

        // === COMPETITIVE MODES ===
        ExtendedGameModes.CreateCompetitiveMode(ExtendedGameMode.Deathmatch, "Deathmatch", "Every merc for themselves", 2, 16, 15.0);
        ExtendedGameModes.CreateCompetitiveMode(ExtendedGameMode.TeamDeathmatch, "Team Deathmatch", "Team vs team combat", 4, 16, 20.0);
        ExtendedGameModes.CreateCompetitiveMode(ExtendedGameMode.Capture, "Capture Mode", "Control key locations in Night City", 4, 12, 25.0);
        ExtendedGameModes.CreateCompetitiveMode(ExtendedGameMode.Racing, "Street Racing", "High-speed races through Night City", 2, 8, 10.0);
        ExtendedGameModes.CreateCompetitiveMode(ExtendedGameMode.Braindance, "Braindance Detective", "Solve cases faster than opponents", 2, 6, 30.0);

        // === OBJECTIVE-BASED MODES ===
        ExtendedGameModes.CreateObjectiveMode(ExtendedGameMode.Extraction, "Extraction", "Escort VIPs to safety", 3, 8, 20.0);
        ExtendedGameModes.CreateObjectiveMode(ExtendedGameMode.Sabotage, "Corporate Sabotage", "Infiltrate and sabotage corporate facilities", 2, 6, 30.0);
        ExtendedGameModes.CreateObjectiveMode(ExtendedGameMode.DataTheft, "Data Theft", "Netrunning competition for valuable data", 2, 8, 25.0);
        ExtendedGameModes.CreateObjectiveMode(ExtendedGameMode.Survival, "Survival Mode", "Survive waves of increasingly difficult enemies", 1, 4, 45.0);

        // === ROLE-PLAYING MODES ===
        ExtendedGameModes.CreateRoleplayMode(ExtendedGameMode.Freeform, "Freeform RP", "Open-ended roleplay scenarios", 2, 20, 0.0);
        ExtendedGameModes.CreateRoleplayMode(ExtendedGameMode.Detective, "Detective Mode", "Investigate crimes and solve mysteries", 1, 6, 60.0);
        ExtendedGameModes.CreateRoleplayMode(ExtendedGameMode.Corporate, "Corporate Life", "Navigate corporate politics and business", 2, 12, 90.0);
        ExtendedGameModes.CreateRoleplayMode(ExtendedGameMode.Street, "Street Life", "Gang warfare and street survival", 2, 16, 120.0);

        // === SPECIAL EVENT MODES ===
        ExtendedGameModes.CreateEventMode(ExtendedGameMode.BlackOut, "City BlackOut", "Navigate Night City during a power outage", 2, 12, 40.0);
        ExtendedGameModes.CreateEventMode(ExtendedGameMode.Riot, "Night City Riots", "Survive civil unrest across the city", 2, 16, 50.0);
        ExtendedGameModes.CreateEventMode(ExtendedGameMode.CyberPsycho, "Cyber Psycho Hunt", "Track and neutralize dangerous cyber psychos", 1, 6, 35.0);
        ExtendedGameModes.CreateEventMode(ExtendedGameMode.Tournament, "Tournament Mode", "Structured competitive tournaments", 8, 32, 120.0);
    }

    private static func CreateCooperativeMode(mode: ExtendedGameMode, name: String, desc: String, minP: Int32, maxP: Int32, duration: Float) -> Void {
        let config: GameModeConfig;
        config.modeId = mode;
        config.name = name;
        config.description = desc;
        config.minPlayers = minP;
        config.maxPlayers = maxP;
        config.duration = duration;
        config.friendlyFire = false;
        config.respawn = true;
        config.sharedInventory = true;

        // Cooperative scoring
        config.scoring.scoringType = "objectives";
        config.scoring.teamScoring = true;
        config.scoring.victoryCondition = "complete_all_objectives";

        // Add cooperative objectives
        ExtendedGameModes.AddCooperativeObjectives(config);

        ArrayPush(gameModeConfigs, config);
    }

    private static func CreateCompetitiveMode(mode: ExtendedGameMode, name: String, desc: String, minP: Int32, maxP: Int32, duration: Float) -> Void {
        let config: GameModeConfig;
        config.modeId = mode;
        config.name = name;
        config.description = desc;
        config.minPlayers = minP;
        config.maxPlayers = maxP;
        config.duration = duration;
        config.friendlyFire = true;
        config.respawn = true;
        config.sharedInventory = false;

        // Competitive scoring
        config.scoring.scoringType = "kills";
        config.scoring.killPoints = 100;
        config.scoring.deathPenalty = -25;
        config.scoring.teamScoring = (mode == ExtendedGameMode.TeamDeathmatch || mode == ExtendedGameMode.Capture);
        config.scoring.victoryCondition = "highest_score";

        // Add competitive objectives
        ExtendedGameModes.AddCompetitiveObjectives(config, mode);

        ArrayPush(gameModeConfigs, config);
    }

    private static func CreateObjectiveMode(mode: ExtendedGameMode, name: String, desc: String, minP: Int32, maxP: Int32, duration: Float) -> Void {
        let config: GameModeConfig;
        config.modeId = mode;
        config.name = name;
        config.description = desc;
        config.minPlayers = minP;
        config.maxPlayers = maxP;
        config.duration = duration;
        config.friendlyFire = false;
        config.respawn = (mode != ExtendedGameMode.Survival);
        config.sharedInventory = false;

        // Objective-based scoring
        config.scoring.scoringType = "objectives";
        config.scoring.objectivePoints = 500;
        config.scoring.teamScoring = true;
        config.scoring.victoryCondition = "complete_objectives";

        // Add objective-specific objectives
        ExtendedGameModes.AddObjectiveModeObjectives(config, mode);

        ArrayPush(gameModeConfigs, config);
    }

    private static func CreateRoleplayMode(mode: ExtendedGameMode, name: String, desc: String, minP: Int32, maxP: Int32, duration: Float) -> Void {
        let config: GameModeConfig;
        config.modeId = mode;
        config.name = name;
        config.description = desc;
        config.minPlayers = minP;
        config.maxPlayers = maxP;
        config.duration = duration;
        config.friendlyFire = false;
        config.respawn = true;
        config.sharedInventory = false;

        // Roleplay scoring
        config.scoring.scoringType = "roleplay";
        config.scoring.teamScoring = false;
        config.scoring.victoryCondition = "participation";

        // Add roleplay objectives
        ExtendedGameModes.AddRoleplayObjectives(config, mode);

        ArrayPush(gameModeConfigs, config);
    }

    private static func CreateEventMode(mode: ExtendedGameMode, name: String, desc: String, minP: Int32, maxP: Int32, duration: Float) -> Void {
        let config: GameModeConfig;
        config.modeId = mode;
        config.name = name;
        config.description = desc;
        config.minPlayers = minP;
        config.maxPlayers = maxP;
        config.duration = duration;
        config.friendlyFire = (mode == ExtendedGameMode.Tournament);
        config.respawn = (mode != ExtendedGameMode.CyberPsycho);
        config.sharedInventory = false;

        // Event-specific scoring
        config.scoring.scoringType = "survival";
        config.scoring.teamScoring = (mode != ExtendedGameMode.Tournament);
        config.scoring.victoryCondition = "survive_event";

        // Add event objectives
        ExtendedGameModes.AddEventObjectives(config, mode);

        ArrayPush(gameModeConfigs, config);
    }

    private static func AddCooperativeObjectives(config: ref<GameModeConfig>) -> Void {
        let obj1: GameModeObjective;
        obj1.objectiveType = "teamwork";
        obj1.description = "Complete objectives together";
        obj1.points = 1000;
        obj1.isRequired = true;
        ArrayPush(config.objectives, obj1);

        let obj2: GameModeObjective;
        obj2.objectiveType = "survival";
        obj2.description = "Keep all team members alive";
        obj2.points = 500;
        obj2.isRequired = false;
        ArrayPush(config.objectives, obj2);
    }

    private static func AddCompetitiveObjectives(config: ref<GameModeConfig>, mode: ExtendedGameMode) -> Void {
        switch mode {
            case ExtendedGameMode.Deathmatch:
                ExtendedGameModes.AddObjective(config, "elimination", "Eliminate other players", 100, true);
                break;
            case ExtendedGameMode.TeamDeathmatch:
                ExtendedGameModes.AddObjective(config, "team_elimination", "Eliminate enemy team members", 100, true);
                break;
            case ExtendedGameMode.Capture:
                ExtendedGameModes.AddObjective(config, "control_points", "Capture and hold strategic locations", 200, true);
                break;
            case ExtendedGameMode.Racing:
                ExtendedGameModes.AddObjective(config, "finish_race", "Complete the race course", 1000, true);
                ExtendedGameModes.AddObjective(config, "checkpoints", "Pass through all checkpoints", 50, true);
                break;
            case ExtendedGameMode.Braindance:
                ExtendedGameModes.AddObjective(config, "solve_case", "Solve the investigation first", 500, true);
                ExtendedGameModes.AddObjective(config, "find_clues", "Discover evidence faster than others", 100, false);
                break;
        }
    }

    private static func AddObjectiveModeObjectives(config: ref<GameModeConfig>, mode: ExtendedGameMode) -> Void {
        switch mode {
            case ExtendedGameMode.Extraction:
                ExtendedGameModes.AddObjective(config, "protect_vip", "Escort the VIP to safety", 1000, true);
                ExtendedGameModes.AddObjective(config, "eliminate_hostiles", "Clear the extraction route", 200, false);
                break;
            case ExtendedGameMode.Sabotage:
                ExtendedGameModes.AddObjective(config, "infiltrate", "Infiltrate the corporate facility", 300, true);
                ExtendedGameModes.AddObjective(config, "sabotage_systems", "Sabotage critical systems", 500, true);
                ExtendedGameModes.AddObjective(config, "extract", "Escape without detection", 400, true);
                break;
            case ExtendedGameMode.DataTheft:
                ExtendedGameModes.AddObjective(config, "hack_systems", "Break through corporate ICE", 400, true);
                ExtendedGameModes.AddObjective(config, "steal_data", "Extract valuable corporate data", 600, true);
                ExtendedGameModes.AddObjective(config, "avoid_trace", "Complete without being traced", 300, false);
                break;
            case ExtendedGameMode.Survival:
                ExtendedGameModes.AddObjective(config, "survive_waves", "Survive increasingly difficult enemy waves", 500, true);
                ExtendedGameModes.AddObjective(config, "boss_defeat", "Defeat the final boss", 1000, true);
                break;
        }
    }

    private static func AddRoleplayObjectives(config: ref<GameModeConfig>, mode: ExtendedGameMode) -> Void {
        switch mode {
            case ExtendedGameMode.Detective:
                ExtendedGameModes.AddObjective(config, "investigate", "Investigate the crime scene", 300, true);
                ExtendedGameModes.AddObjective(config, "interview", "Interview witnesses and suspects", 200, true);
                ExtendedGameModes.AddObjective(config, "solve_case", "Identify the perpetrator", 500, true);
                break;
            case ExtendedGameMode.Corporate:
                ExtendedGameModes.AddObjective(config, "meetings", "Attend corporate meetings", 100, false);
                ExtendedGameModes.AddObjective(config, "negotiations", "Complete business negotiations", 300, true);
                ExtendedGameModes.AddObjective(config, "climb_ladder", "Advance your corporate position", 500, false);
                break;
            case ExtendedGameMode.Street:
                ExtendedGameModes.AddObjective(config, "territory", "Establish gang territory", 400, false);
                ExtendedGameModes.AddObjective(config, "reputation", "Build street reputation", 200, true);
                ExtendedGameModes.AddObjective(config, "survival", "Survive gang conflicts", 300, true);
                break;
        }
    }

    private static func AddEventObjectives(config: ref<GameModeConfig>, mode: ExtendedGameMode) -> Void {
        switch mode {
            case ExtendedGameMode.BlackOut:
                ExtendedGameModes.AddObjective(config, "navigate", "Navigate the darkened city", 300, true);
                ExtendedGameModes.AddObjective(config, "restore_power", "Help restore power to districts", 500, false);
                break;
            case ExtendedGameMode.Riot:
                ExtendedGameModes.AddObjective(config, "survive_chaos", "Survive the civil unrest", 400, true);
                ExtendedGameModes.AddObjective(config, "help_civilians", "Assist civilians in danger", 300, false);
                break;
            case ExtendedGameMode.CyberPsycho:
                ExtendedGameModes.AddObjective(config, "track_psycho", "Track the cyber psycho", 300, true);
                ExtendedGameModes.AddObjective(config, "neutralize", "Neutralize the threat", 700, true);
                break;
            case ExtendedGameMode.Tournament:
                ExtendedGameModes.AddObjective(config, "advance", "Advance through tournament brackets", 500, true);
                ExtendedGameModes.AddObjective(config, "victory", "Win the tournament", 2000, true);
                break;
        }
    }

    private static func AddObjective(config: ref<GameModeConfig>, type: String, desc: String, points: Int32, required: Bool) -> Void {
        let objective: GameModeObjective;
        objective.objectiveType = type;
        objective.description = desc;
        objective.points = points;
        objective.isRequired = required;
        ArrayPush(config.objectives, objective);
    }

    // === SESSION MANAGEMENT ===

    public static func CreateSession(modeId: ExtendedGameMode, hostId: String) -> String {
        let config = ExtendedGameModes.GetGameModeConfig(modeId);
        if Equals(config.name, "") {
            LogChannel(n"EXTENDED_MODES", "Invalid game mode: " + IntToString(EnumInt(modeId)));
            return "";
        }

        let sessionId = "session_" + hostId + "_" + ToString(GetGameTime());

        let session: GameModeSession;
        session.sessionId = sessionId;
        session.modeConfig = config;
        session.currentPhase = "waiting";
        session.startTime = GetGameTime();
        session.isActive = true;
        session.isPaused = false;

        // Add host as first player
        ExtendedGameModes.AddPlayerToSession(session, hostId, "Host", "");

        ArrayPush(activeSessions, session);

        LogChannel(n"EXTENDED_MODES", "Created " + config.name + " session: " + sessionId);
        return sessionId;
    }

    public static func JoinSession(sessionId: String, playerId: String, playerName: String) -> Bool {
        let sessionIndex = ExtendedGameModes.FindSessionIndex(sessionId);
        if sessionIndex == -1 {
            return false;
        }

        let session = activeSessions[sessionIndex];

        // Check if session is full
        if ArraySize(session.participants) >= session.modeConfig.maxPlayers {
            LogChannel(n"EXTENDED_MODES", "Session is full");
            return false;
        }

        // Check if player already in session
        if ExtendedGameModes.IsPlayerInSession(session, playerId) {
            LogChannel(n"EXTENDED_MODES", "Player already in session");
            return false;
        }

        // Add player to session
        ExtendedGameModes.AddPlayerToSession(session, playerId, playerName, "");
        activeSessions[sessionIndex] = session;

        LogChannel(n"EXTENDED_MODES", "Player " + playerName + " joined " + session.modeConfig.name + " session");
        return true;
    }

    public static func StartSession(sessionId: String) -> Bool {
        let sessionIndex = ExtendedGameModes.FindSessionIndex(sessionId);
        if sessionIndex == -1 {
            return false;
        }

        let session = activeSessions[sessionIndex];

        // Check minimum players
        if ArraySize(session.participants) < session.modeConfig.minPlayers {
            LogChannel(n"EXTENDED_MODES", "Not enough players to start session");
            return false;
        }

        // Check if all players are ready
        for player in session.participants {
            if !player.isReady {
                LogChannel(n"EXTENDED_MODES", "Not all players are ready");
                return false;
            }
        }

        session.currentPhase = "active";
        session.startTime = GetGameTime();
        activeSessions[sessionIndex] = session;

        LogChannel(n"EXTENDED_MODES", "Started " + session.modeConfig.name + " session with " + ToString(ArraySize(session.participants)) + " players");

        // Initialize game mode specific logic
        ExtendedGameModes.InitializeGameModeLogic(session);

        return true;
    }

    private static func AddPlayerToSession(session: ref<GameModeSession>, playerId: String, playerName: String, teamId: String) -> Void {
        let player: GameModePlayer;
        player.playerId = playerId;
        player.playerName = playerName;
        player.teamId = teamId;
        player.isReady = false;
        player.isAlive = true;
        player.currentScore = 0;
        player.kills = 0;
        player.deaths = 0;
        player.assists = 0;
        player.objectivesCompleted = 0;
        player.joinTime = GetGameTime();

        ArrayPush(session.participants, player);
    }

    private static func InitializeGameModeLogic(session: GameModeSession) -> Void {
        // Initialize mode-specific logic based on game mode
        switch session.modeConfig.modeId {
            case ExtendedGameMode.TeamDeathmatch:
            case ExtendedGameMode.Capture:
                ExtendedGameModes.CreateTeams(session);
                break;
            case ExtendedGameMode.Racing:
                ExtendedGameModes.SetupRaceTrack(session);
                break;
            case ExtendedGameMode.Survival:
                ExtendedGameModes.InitializeSurvivalWaves(session);
                break;
        }
    }

    private static func CreateTeams(session: GameModeSession) -> Void {
        // Create two balanced teams
        let team1: GameModeTeam;
        team1.teamId = "team_1";
        team1.teamName = "Team Alpha";
        team1.teamColor = "blue";
        team1.isActive = true;

        let team2: GameModeTeam;
        team2.teamId = "team_2";
        team2.teamName = "Team Bravo";
        team2.teamColor = "red";
        team2.isActive = true;

        ArrayPush(session.teams, team1);
        ArrayPush(session.teams, team2);

        // Assign players to teams
        let currentTeam = 0;
        for i in Range(ArraySize(session.participants)) {
            session.participants[i].teamId = (currentTeam == 0) ? "team_1" : "team_2";
            currentTeam = (currentTeam + 1) % 2;
        }
    }

    // === UTILITY FUNCTIONS ===

    public static func GetGameModeConfig(modeId: ExtendedGameMode) -> GameModeConfig {
        for config in gameModeConfigs {
            if config.modeId == modeId {
                return config;
            }
        }

        let emptyConfig: GameModeConfig;
        return emptyConfig;
    }

    public static func GetAvailableGameModes() -> array<GameModeConfig> {
        return gameModeConfigs;
    }

    public static func GetActiveSessions() -> array<GameModeSession> {
        let activeOnly: array<GameModeSession>;
        for session in activeSessions {
            if session.isActive {
                ArrayPush(activeOnly, session);
            }
        }
        return activeOnly;
    }

    private static func FindSessionIndex(sessionId: String) -> Int32 {
        for i in Range(ArraySize(activeSessions)) {
            if Equals(activeSessions[i].sessionId, sessionId) {
                return i;
            }
        }
        return -1;
    }

    private static func IsPlayerInSession(session: GameModeSession, playerId: String) -> Bool {
        for player in session.participants {
            if Equals(player.playerId, playerId) {
                return true;
            }
        }
        return false;
    }

    // Placeholder functions for mode-specific initialization
    private static func SetupRaceTrack(session: GameModeSession) -> Void {
        LogChannel(n"EXTENDED_MODES", "Setting up race track for session");
    }

    private static func InitializeSurvivalWaves(session: GameModeSession) -> Void {
        LogChannel(n"EXTENDED_MODES", "Initializing survival waves for session");
    }

    public static func GetSessionById(sessionId: String) -> GameModeSession {
        let index = ExtendedGameModes.FindSessionIndex(sessionId);
        if index >= 0 {
            return activeSessions[index];
        }

        let emptySession: GameModeSession;
        return emptySession;
    }
}