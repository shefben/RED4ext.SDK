// Custom activities framework for user-generated content and events

public enum ActivityType {
    Mission = 0,         // Story-driven missions
    Challenge = 1,       // Skill-based challenges  
    Event = 2,           // Time-limited events
    Exploration = 3,     // Open world exploration
    Social = 4,          // Social gatherings
    Competition = 5,     // Competitive activities
    Roleplay = 6,        // Roleplay scenarios
    Training = 7,        // Skill training
    Creative = 8         // Creative/building activities
}

public enum ActivityDifficulty {
    Tutorial = 0,        // Teaching/learning
    Easy = 1,           // Casual players
    Normal = 2,         // Standard difficulty
    Hard = 3,           // Challenging
    Expert = 4,         // Very difficult
    Insane = 5          // Extreme challenge
}

public enum ActivityStatus {
    Draft = 0,          // Being created
    Pending = 1,        // Awaiting approval
    Active = 2,         // Available to play
    InProgress = 3,     // Currently running
    Completed = 4,      // Finished
    Archived = 5,       // No longer available
    Suspended = 6       // Temporarily disabled
}

public struct CustomActivity {
    public var activityId: String;
    public var creatorId: String;
    public var title: String;
    public var description: String;
    public var activityType: ActivityType;
    public var difficulty: ActivityDifficulty;
    public var status: ActivityStatus;
    public var maxParticipants: Int32;
    public var minParticipants: Int32;
    public var estimatedDuration: Float; // In minutes
    public var location: ActivityLocation;
    public var objectives: array<ActivityObjective>;
    public var rewards: ActivityRewards;
    public var requirements: array<String>;
    public var tags: array<String>;
    public var createdTime: Float;
    public var lastModified: Float;
    public var playCount: Int32;
    public var rating: Float; // 0-5 star rating
    public var reviews: array<ActivityReview>;
    public var isPublic: Bool;
    public var isFeatured: Bool;
    public var version: String;
}

public struct ActivityLocation {
    public var locationId: String;
    public var locationName: String;
    public var district: String;
    public var coordinates: Vector3;
    public var area: Float; // Radius or area size
    public var environment: String; // "indoor", "outdoor", "underground", "cyberspace"
    public var customMap: Bool; // Uses custom geometry
    public var landmarks: array<String>;
}

public struct ActivityObjective {
    public var objectiveId: String;
    public var title: String;
    public var description: String;
    public var objectiveType: String; // "goto", "kill", "collect", "hack", "survive", "interact"
    public var targetLocation: Vector3;
    public var targetEntity: String;
    public var targetCount: Int32;
    public var timeLimit: Float;
    public var isOptional: Bool;
    public var pointValue: Int32;
    public var triggerConditions: array<String>;
    public var completionActions: array<String>;
    public var dependencies: array<String>; // Other objectives that must be completed first
}

public struct ActivityRewards {
    public var experiencePoints: Int32;
    public var streetCredGain: Int32;
    public var eddiesReward: Int32;
    public var customCurrency: Int32; // Activity-specific currency
    public var items: array<String>;
    public var achievements: array<String>;
    public var unlocks: array<String>; // Unlocked content
    public var cosmetics: array<String>;
    public var titles: array<String>; // Player titles
}

public struct ActivityReview {
    public var reviewId: String;
    public var reviewerId: String;
    public var rating: Int32; // 1-5 stars
    public var comment: String;
    public var reviewTime: Float;
    public var isVerified: Bool; // Reviewer completed the activity
    public var helpful: Int32; // How many found this helpful
}

public struct ActivitySession {
    public var sessionId: String;
    public var activityId: String;
    public var hostId: String;
    public var participants: array<ActivityParticipant>;
    public var status: ActivityStatus;
    public var startTime: Float;
    public var currentObjective: Int32;
    public var objectiveProgress: array<ObjectiveProgress>;
    public var sessionData: ActivitySessionData;
    public var customVariables: array<CustomVariable>;
    public var chatLog: array<String>;
}

public struct ActivityParticipant {
    public var playerId: String;
    public var playerName: String;
    public var role: String; // Custom role within activity
    public var isReady: Bool;
    public var currentScore: Int32;
    public var objectivesCompleted: Int32;
    public var isActive: Bool;
    public var joinTime: Float;
    public var contributions: array<String>;
}

public struct ObjectiveProgress {
    public var objectiveId: String;
    public var currentProgress: Int32;
    public var isCompleted: Bool;
    public var completedBy: String; // Player who completed it
    public var completionTime: Float;
    public var attempts: Int32;
}

public struct ActivitySessionData {
    public var totalScore: Int32;
    public var timeElapsed: Float;
    public var deaths: Int32;
    public var revives: Int32;
    public var secretsFound: Int32;
    public var bonusObjectives: Int32;
    public var perfectRun: Bool; // No deaths, all objectives
}

public struct CustomVariable {
    public var variableName: String;
    public var variableType: String; // "int", "float", "string", "bool"
    public var value: String; // Stored as string, converted as needed
    public var description: String;
    public var isPublic: Bool; // Visible to players
}

public struct ActivityTemplate {
    public var templateId: String;
    public var templateName: String;
    public var templateType: ActivityType;
    public var description: String;
    public var defaultObjectives: array<ActivityObjective>;
    public var defaultRewards: ActivityRewards;
    public var requiredFields: array<String>;
    public var optionalFields: array<String>;
    public var examples: array<String>;
}

public struct ActivityEvent {
    public var eventId: String;
    public var eventName: String;
    public var eventType: String; // "start", "complete", "fail", "trigger"
    public var triggerId: String; // What triggered this event
    public var activityId: String;
    public var sessionId: String;
    public var playerId: String;
    public var timestamp: Float;
    public var eventData: String; // Additional event-specific data
    public var isGlobal: Bool; // Affects all players vs single player
}

public class CustomActivities {
    private static var isInitialized: Bool = false;
    private static var activities: array<CustomActivity>;
    private static var activeSessions: array<ActivitySession>;
    private static var activityTemplates: array<ActivityTemplate>;
    private static var activityCreator: ref<ActivityCreator>;
    private static var sessionManager: ref<ActivitySessionManager>;
    private static var activitiesUI: ref<CustomActivitiesUI>;
    
    // Network callbacks
    private static cb func OnActivityCreated(data: String) -> Void;
    private static cb func OnActivityStarted(data: String) -> Void;
    private static cb func OnPlayerJoinActivity(data: String) -> Void;
    private static cb func OnActivityEvent(data: String) -> Void;
    private static cb func OnActivityCompleted(data: String) -> Void;
    
    public static func Initialize() -> Void {
        if isInitialized {
            return;
        }
        
        LogChannel(n"COOP_ACTIVITIES", "Initializing custom activities framework...");
        
        // Initialize templates
        CustomActivities.InitializeTemplates();
        
        // Initialize components
        activityCreator = new ActivityCreator();
        sessionManager = new ActivitySessionManager();
        
        // Register network callbacks
        NetworkingSystem.RegisterCallback("activity_created", CustomActivities.OnActivityCreated);
        NetworkingSystem.RegisterCallback("activity_started", CustomActivities.OnActivityStarted);
        NetworkingSystem.RegisterCallback("activity_player_join", CustomActivities.OnPlayerJoinActivity);
        NetworkingSystem.RegisterCallback("activity_event", CustomActivities.OnActivityEvent);
        NetworkingSystem.RegisterCallback("activity_completed", CustomActivities.OnActivityCompleted);
        
        // Load sample activities
        CustomActivities.LoadSampleActivities();
        
        isInitialized = true;
        LogChannel(n"COOP_ACTIVITIES", "Custom activities framework initialized with " + ToString(ArraySize(activities)) + " activities");
    }
    
    private static func InitializeTemplates() -> Void {
        ArrayClear(activityTemplates);
        
        // Mission Template
        CustomActivities.CreateTemplate("mission_basic", "Basic Mission", ActivityType.Mission,
            "Standard story mission with objectives and dialogue");
        
        // Challenge Template
        CustomActivities.CreateTemplate("challenge_combat", "Combat Challenge", ActivityType.Challenge,
            "Test combat skills against waves of enemies");
        
        // Race Template
        CustomActivities.CreateTemplate("race_circuit", "Racing Circuit", ActivityType.Competition,
            "Checkpoint-based racing competition");
        
        // Exploration Template
        CustomActivities.CreateTemplate("exploration_treasure", "Treasure Hunt", ActivityType.Exploration,
            "Find hidden items scattered across Night City");
        
        // Social Template
        CustomActivities.CreateTemplate("social_gathering", "Social Event", ActivityType.Social,
            "Meet and interact with other players");
        
        // Training Template
        CustomActivities.CreateTemplate("training_skill", "Skill Training", ActivityType.Training,
            "Practice and improve specific abilities");
        
        LogChannel(n"COOP_ACTIVITIES", "Initialized " + ToString(ArraySize(activityTemplates)) + " activity templates");
    }
    
    private static func CreateTemplate(id: String, name: String, type: ActivityType, desc: String) -> Void {
        let template: ActivityTemplate;
        template.templateId = id;
        template.templateName = name;
        template.templateType = type;
        template.description = desc;
        
        // Add common required fields
        ArrayPush(template.requiredFields, "title");
        ArrayPush(template.requiredFields, "description");
        ArrayPush(template.requiredFields, "location");
        ArrayPush(template.requiredFields, "objectives");
        
        // Add optional fields
        ArrayPush(template.optionalFields, "requirements");
        ArrayPush(template.optionalFields, "tags");
        ArrayPush(template.optionalFields, "rewards");
        
        // Template-specific objectives
        CustomActivities.AddTemplateObjectives(template, type);
        
        ArrayPush(activityTemplates, template);
    }
    
    private static func AddTemplateObjectives(template: ref<ActivityTemplate>, type: ActivityType) -> Void {
        switch type {
            case ActivityType.Mission:
                CustomActivities.AddObjectiveToTemplate(template, "goto", "Go to location", "Navigate to the specified coordinates");
                CustomActivities.AddObjectiveToTemplate(template, "interact", "Interact with target", "Interact with the specified object or NPC");
                break;
                
            case ActivityType.Challenge:
                CustomActivities.AddObjectiveToTemplate(template, "kill", "Eliminate targets", "Defeat the specified number of enemies");
                CustomActivities.AddObjectiveToTemplate(template, "survive", "Survive duration", "Stay alive for the specified time");
                break;
                
            case ActivityType.Competition:
                CustomActivities.AddObjectiveToTemplate(template, "race", "Complete race", "Finish the race course in time");
                CustomActivities.AddObjectiveToTemplate(template, "checkpoint", "Hit checkpoints", "Pass through all race checkpoints");
                break;
                
            case ActivityType.Exploration:
                CustomActivities.AddObjectiveToTemplate(template, "collect", "Collect items", "Find and collect the specified items");
                CustomActivities.AddObjectiveToTemplate(template, "discover", "Discover locations", "Find hidden or secret locations");
                break;
        }
    }
    
    private static func AddObjectiveToTemplate(template: ref<ActivityTemplate>, type: String, title: String, desc: String) -> Void {
        let objective: ActivityObjective;
        objective.objectiveId = template.templateId + "_" + type;
        objective.title = title;
        objective.description = desc;
        objective.objectiveType = type;
        objective.targetCount = 1;
        objective.isOptional = false;
        objective.pointValue = 100;
        
        ArrayPush(template.defaultObjectives, objective);
    }
    
    private static func LoadSampleActivities() -> Void {
        // Create some sample activities for testing
        
        // Exploration Activity
        CustomActivities.CreateSampleActivity("explore_watson", "Watson District Explorer", ActivityType.Exploration,
            "Explore the hidden corners of Watson district and uncover its secrets",
            "watson_northside", ActivityDifficulty.Easy, 4, 30.0);
        
        // Combat Challenge
        CustomActivities.CreateSampleActivity("combat_arena", "Night City Combat Arena", ActivityType.Challenge,
            "Test your combat skills in increasingly difficult rounds",
            "pacifica_arena", ActivityDifficulty.Hard, 8, 45.0);
        
        // Social Event
        CustomActivities.CreateSampleActivity("afterlife_meetup", "Afterlife Bar Gathering", ActivityType.Social,
            "Meet fellow edgerunners at the famous Afterlife bar",
            "heywood_afterlife", ActivityDifficulty.Tutorial, 20, 60.0);
        
        // Training Mission
        CustomActivities.CreateSampleActivity("netrunning_101", "Netrunning Basics", ActivityType.Training,
            "Learn the fundamentals of netrunning in a safe environment", 
            "cyberspace_training", ActivityDifficulty.Tutorial, 6, 20.0);
    }
    
    private static func CreateSampleActivity(id: String, title: String, type: ActivityType, desc: String, 
                                           locationId: String, difficulty: ActivityDifficulty, maxPlayers: Int32, duration: Float) -> Void {
        let activity: CustomActivity;
        activity.activityId = id;
        activity.creatorId = "system";
        activity.title = title;
        activity.description = desc;
        activity.activityType = type;
        activity.difficulty = difficulty;
        activity.status = ActivityStatus.Active;
        activity.maxParticipants = maxPlayers;
        activity.minParticipants = 1;
        activity.estimatedDuration = duration;
        activity.createdTime = GetGameTime();
        activity.lastModified = GetGameTime();
        activity.isPublic = true;
        activity.isFeatured = true;
        activity.version = "1.0";
        activity.rating = 4.5; // High rating for featured content
        
        // Set location
        activity.location = CustomActivities.CreateSampleLocation(locationId);
        
        // Add sample objectives
        CustomActivities.AddSampleObjectives(activity, type);
        
        // Set rewards
        activity.rewards = CustomActivities.CreateSampleRewards(difficulty, duration);
        
        // Add tags
        CustomActivities.AddSampleTags(activity, type);
        
        ArrayPush(activities, activity);
    }
    
    private static func CreateSampleLocation(locationId: String) -> ActivityLocation {
        let location: ActivityLocation;
        location.locationId = locationId;
        
        if Equals(locationId, "watson_northside") {
            location.locationName = "Watson Northside";
            location.district = "Watson";
            location.coordinates = new Vector3(-1200.0, 1300.0, 50.0);
            location.area = 500.0;
            location.environment = "outdoor";
            ArrayPush(location.landmarks, "Industrial Complex");
            ArrayPush(location.landmarks, "Abandoned Factory");
        } else if Equals(locationId, "pacifica_arena") {
            location.locationName = "Combat Arena";
            location.district = "Pacifica";
            location.coordinates = new Vector3(-1800.0, -1300.0, 15.0);
            location.area = 200.0;
            location.environment = "indoor";
            ArrayPush(location.landmarks, "Central Arena");
            ArrayPush(location.landmarks, "Spectator Stands");
        } else if Equals(locationId, "heywood_afterlife") {
            location.locationName = "Afterlife Bar";
            location.district = "Heywood";
            location.coordinates = new Vector3(800.0, -800.0, 25.0);
            location.area = 100.0;
            location.environment = "indoor";
            ArrayPush(location.landmarks, "Main Bar");
            ArrayPush(location.landmarks, "VIP Booth");
        } else if Equals(locationId, "cyberspace_training") {
            location.locationName = "Training Cyberspace";
            location.district = "Virtual";
            location.coordinates = new Vector3(0.0, 0.0, 100.0);
            location.area = 1000.0;
            location.environment = "cyberspace";
            ArrayPush(location.landmarks, "Training Network");
            ArrayPush(location.landmarks, "Practice ICE");
        }
        
        return location;
    }
    
    private static func AddSampleObjectives(activity: ref<CustomActivity>, type: ActivityType) -> Void {
        switch type {
            case ActivityType.Exploration:
                CustomActivities.AddObjective(activity, "collect", "Find Data Shards", "Locate 5 hidden data shards", 5, false, 200);
                CustomActivities.AddObjective(activity, "discover", "Discover Locations", "Find 3 secret locations", 3, false, 300);
                CustomActivities.AddObjective(activity, "interact", "Scan Graffiti", "Scan unique graffiti art", 10, true, 100);
                break;
                
            case ActivityType.Challenge:
                CustomActivities.AddObjective(activity, "survive", "Survive Round 1", "Survive for 2 minutes", 120, false, 500);
                CustomActivities.AddObjective(activity, "kill", "Eliminate Wave 2", "Defeat 15 enemies", 15, false, 750);
                CustomActivities.AddObjective(activity, "survive", "Final Round", "Survive boss encounter", 300, false, 1000);
                break;
                
            case ActivityType.Social:
                CustomActivities.AddObjective(activity, "interact", "Meet Players", "Interact with 5 different players", 5, false, 100);
                CustomActivities.AddObjective(activity, "emote", "Social Activities", "Use 10 different emotes", 10, true, 50);
                CustomActivities.AddObjective(activity, "chat", "Join Conversation", "Participate in group chat", 1, true, 75);
                break;
                
            case ActivityType.Training:
                CustomActivities.AddObjective(activity, "hack", "Basic Hacking", "Successfully hack 3 devices", 3, false, 300);
                CustomActivities.AddObjective(activity, "stealth", "Stealth Movement", "Move undetected for 1 minute", 60, false, 400);
                CustomActivities.AddObjective(activity, "complete", "Pass Final Test", "Complete training scenario", 1, false, 500);
                break;
        }
    }
    
    private static func AddObjective(activity: ref<CustomActivity>, type: String, title: String, desc: String, count: Int32, optional: Bool, points: Int32) -> Void {
        let objective: ActivityObjective;
        objective.objectiveId = activity.activityId + "_obj_" + ToString(ArraySize(activity.objectives));
        objective.title = title;
        objective.description = desc;
        objective.objectiveType = type;
        objective.targetCount = count;
        objective.isOptional = optional;
        objective.pointValue = points;
        objective.timeLimit = 0.0; // No time limit by default
        
        ArrayPush(activity.objectives, objective);
    }
    
    private static func CreateSampleRewards(difficulty: ActivityDifficulty, duration: Float) -> ActivityRewards {
        let rewards: ActivityRewards;
        
        let difficultyMultiplier = Cast<Float>(Cast<Int32>(difficulty) + 1);
        let durationMultiplier = duration / 30.0; // Base on 30 minute activity
        
        rewards.experiencePoints = Cast<Int32>(500.0 * difficultyMultiplier * durationMultiplier);
        rewards.streetCredGain = Cast<Int32>(50.0 * difficultyMultiplier);
        rewards.eddiesReward = Cast<Int32>(1000.0 * difficultyMultiplier * durationMultiplier);
        rewards.customCurrency = Cast<Int32>(100.0 * difficultyMultiplier);
        
        // Add sample items based on difficulty
        if difficulty >= ActivityDifficulty.Normal {
            ArrayPush(rewards.items, "crafting_components");
        }
        if difficulty >= ActivityDifficulty.Hard {
            ArrayPush(rewards.items, "rare_weapon_mod");
        }
        if difficulty >= ActivityDifficulty.Expert {
            ArrayPush(rewards.items, "legendary_cyberware");
        }
        
        return rewards;
    }
    
    private static func AddSampleTags(activity: ref<CustomActivity>, type: ActivityType) -> Void {
        // Add type-specific tags
        switch type {
            case ActivityType.Exploration:
                ArrayPush(activity.tags, "exploration");
                ArrayPush(activity.tags, "discovery");
                ArrayPush(activity.tags, "secrets");
                break;
                
            case ActivityType.Challenge:
                ArrayPush(activity.tags, "combat");
                ArrayPush(activity.tags, "survival");
                ArrayPush(activity.tags, "challenging");
                break;
                
            case ActivityType.Social:
                ArrayPush(activity.tags, "social");
                ArrayPush(activity.tags, "meetup");
                ArrayPush(activity.tags, "community");
                break;
                
            case ActivityType.Training:
                ArrayPush(activity.tags, "tutorial");
                ArrayPush(activity.tags, "learning");
                ArrayPush(activity.tags, "practice");
                break;
        }
        
        // Add general tags
        ArrayPush(activity.tags, "multiplayer");
        ArrayPush(activity.tags, "featured");
    }
    
    public static func CreateActivity(creatorId: String, title: String, description: String, activityType: ActivityType,
                                     difficulty: ActivityDifficulty, maxParticipants: Int32, estimatedDuration: Float) -> String {
        let activityId = "custom_" + creatorId + "_" + ToString(GetGameTime());
        
        let activity: CustomActivity;
        activity.activityId = activityId;
        activity.creatorId = creatorId;
        activity.title = title;
        activity.description = description;
        activity.activityType = activityType;
        activity.difficulty = difficulty;
        activity.status = ActivityStatus.Draft;
        activity.maxParticipants = maxParticipants;
        activity.minParticipants = 1;
        activity.estimatedDuration = estimatedDuration;
        activity.createdTime = GetGameTime();
        activity.lastModified = GetGameTime();
        activity.playCount = 0;
        activity.rating = 0.0;
        activity.isPublic = false;
        activity.isFeatured = false;
        activity.version = "1.0";
        
        ArrayPush(activities, activity);
        
        LogChannel(n"COOP_ACTIVITIES", "Created custom activity: " + activityId + " by " + creatorId);
        
        // Broadcast activity creation
        let activityData = CustomActivities.SerializeActivity(activity);
        NetworkingSystem.BroadcastMessage("activity_created", activityData);
        
        return activityId;
    }
    
    public static func StartActivity(activityId: String, hostId: String) -> String {
        let activity = CustomActivities.GetActivity(activityId);
        if Equals(activity.activityId, "") {
            LogChannel(n"COOP_ACTIVITIES", "Activity not found: " + activityId);
            return "";
        }
        
        if activity.status != ActivityStatus.Active {
            LogChannel(n"COOP_ACTIVITIES", "Activity not available: " + activityId);
            return "";
        }
        
        // Create new session
        let sessionId = activityId + "_" + hostId + "_" + ToString(GetGameTime());
        
        let session: ActivitySession;
        session.sessionId = sessionId;
        session.activityId = activityId;
        session.hostId = hostId;
        session.status = ActivityStatus.Active;
        session.startTime = GetGameTime();
        session.currentObjective = 0;
        
        // Initialize objective progress
        for objective in activity.objectives {
            let progress: ObjectiveProgress;
            progress.objectiveId = objective.objectiveId;
            progress.currentProgress = 0;
            progress.isCompleted = false;
            progress.attempts = 0;
            ArrayPush(session.objectiveProgress, progress);
        }
        
        // Add host as participant
        let hostParticipant: ActivityParticipant;
        hostParticipant.playerId = hostId;
        hostParticipant.playerName = NetworkingSystem.GetPlayerName(hostId);
        hostParticipant.role = "host";
        hostParticipant.isReady = true;
        hostParticipant.currentScore = 0;
        hostParticipant.objectivesCompleted = 0;
        hostParticipant.isActive = true;
        hostParticipant.joinTime = GetGameTime();
        ArrayPush(session.participants, hostParticipant);
        
        ArrayPush(activeSessions, session);
        
        LogChannel(n"COOP_ACTIVITIES", "Started activity session: " + sessionId);
        
        // Broadcast session start
        let sessionData = CustomActivities.SerializeSession(session);
        NetworkingSystem.BroadcastMessage("activity_started", sessionData);
        
        return sessionId;
    }
    
    public static func JoinActivity(sessionId: String, playerId: String) -> Bool {
        let sessionIndex = CustomActivities.FindSessionIndex(sessionId);
        if sessionIndex == -1 {
            LogChannel(n"COOP_ACTIVITIES", "Session not found: " + sessionId);
            return false;
        }
        
        let session = activeSessions[sessionIndex];
        let activity = CustomActivities.GetActivity(session.activityId);
        
        if ArraySize(session.participants) >= activity.maxParticipants {
            LogChannel(n"COOP_ACTIVITIES", "Session is full");
            return false;
        }
        
        // Check if player already in session
        for participant in session.participants {
            if Equals(participant.playerId, playerId) {
                LogChannel(n"COOP_ACTIVITIES", "Player already in session");
                return false;
            }
        }
        
        // Add participant
        let participant: ActivityParticipant;
        participant.playerId = playerId;
        participant.playerName = NetworkingSystem.GetPlayerName(playerId);
        participant.role = "participant";
        participant.isReady = false;
        participant.currentScore = 0;
        participant.objectivesCompleted = 0;
        participant.isActive = true;
        participant.joinTime = GetGameTime();
        
        ArrayPush(session.participants, participant);
        activeSessions[sessionIndex] = session;
        
        LogChannel(n"COOP_ACTIVITIES", "Player " + playerId + " joined activity session " + sessionId);
        
        // Broadcast player join
        let joinData = sessionId + "|" + playerId;
        NetworkingSystem.BroadcastMessage("activity_player_join", joinData);
        
        return true;
    }
    
    public static func UpdateObjectiveProgress(sessionId: String, objectiveId: String, playerId: String, progress: Int32) -> Void {
        let sessionIndex = CustomActivities.FindSessionIndex(sessionId);
        if sessionIndex == -1 {
            return;
        }
        
        let session = activeSessions[sessionIndex];
        
        // Update objective progress
        for i in Range(ArraySize(session.objectiveProgress)) {
            let objProgress = session.objectiveProgress[i];
            if Equals(objProgress.objectiveId, objectiveId) {
                objProgress.currentProgress += progress;
                objProgress.attempts += 1;
                
                // Check if objective is completed
                let activity = CustomActivities.GetActivity(session.activityId);
                let objective = CustomActivities.GetObjective(activity, objectiveId);
                
                if objProgress.currentProgress >= objective.targetCount {
                    objProgress.isCompleted = true;
                    objProgress.completedBy = playerId;
                    objProgress.completionTime = GetGameTime();
                    
                    // Update participant score
                    for j in Range(ArraySize(session.participants)) {
                        if Equals(session.participants[j].playerId, playerId) {
                            session.participants[j].currentScore += objective.pointValue;
                            session.participants[j].objectivesCompleted += 1;
                            break;
                        }
                    }
                    
                    LogChannel(n"COOP_ACTIVITIES", "Objective completed: " + objectiveId + " by " + playerId);
                }
                
                session.objectiveProgress[i] = objProgress;
                break;
            }
        }
        
        activeSessions[sessionIndex] = session;
        
        // Check if activity is completed
        CustomActivities.CheckActivityCompletion(sessionIndex);
        
        // Broadcast progress update
        let progressData = sessionId + "|" + objectiveId + "|" + playerId + "|" + ToString(progress);
        NetworkingSystem.BroadcastMessage("activity_progress", progressData);
    }
    
    private static func CheckActivityCompletion(sessionIndex: Int32) -> Void {
        let session = activeSessions[sessionIndex];
        let activity = CustomActivities.GetActivity(session.activityId);
        
        // Check if all required objectives are completed
        let requiredCompleted = 0;
        let requiredTotal = 0;
        
        for i in Range(ArraySize(activity.objectives)) {
            let objective = activity.objectives[i];
            if !objective.isOptional {
                requiredTotal += 1;
                if session.objectiveProgress[i].isCompleted {
                    requiredCompleted += 1;
                }
            }
        }
        
        if requiredCompleted >= requiredTotal {
            CustomActivities.CompleteActivity(sessionIndex);
        }
    }
    
    private static func CompleteActivity(sessionIndex: Int32) -> Void {
        let session = activeSessions[sessionIndex];
        session.status = ActivityStatus.Completed;
        
        let activity = CustomActivities.GetActivity(session.activityId);
        
        // Calculate completion time
        let completionTime = GetGameTime() - session.startTime;
        session.sessionData.timeElapsed = completionTime;
        
        // Determine if it was a perfect run
        session.sessionData.perfectRun = (session.sessionData.deaths == 0) && 
                                        (session.sessionData.bonusObjectives == CustomActivities.CountOptionalObjectives(activity));
        
        // Distribute rewards
        CustomActivities.DistributeActivityRewards(session, activity);
        
        // Update activity stats
        CustomActivities.UpdateActivityStats(activity);
        
        activeSessions[sessionIndex] = session;
        
        LogChannel(n"COOP_ACTIVITIES", "Activity completed: " + session.activityId + " in " + ToString(completionTime) + " seconds");
        
        // Broadcast completion
        let completionData = CustomActivities.SerializeCompletion(session);
        NetworkingSystem.BroadcastMessage("activity_completed", completionData);
    }
    
    private static func DistributeActivityRewards(session: ActivitySession, activity: CustomActivity) -> Void {
        let baseRewards = activity.rewards;
        
        for participant in session.participants {
            if !participant.isActive {
                continue; // Skip inactive participants
            }
            
            // Calculate performance multiplier
            let performanceMultiplier = CustomActivities.CalculatePerformanceMultiplier(participant, session);
            
            let rewards: ActivityRewards;
            rewards.experiencePoints = Cast<Int32>(Cast<Float>(baseRewards.experiencePoints) * performanceMultiplier);
            rewards.streetCredGain = Cast<Int32>(Cast<Float>(baseRewards.streetCredGain) * performanceMultiplier);
            rewards.eddiesReward = Cast<Int32>(Cast<Float>(baseRewards.eddiesReward) * performanceMultiplier);
            rewards.customCurrency = Cast<Int32>(Cast<Float>(baseRewards.customCurrency) * performanceMultiplier);
            
            // Perfect run bonus
            if session.sessionData.perfectRun {
                rewards.experiencePoints = Cast<Int32>(Cast<Float>(rewards.experiencePoints) * 1.5);
                rewards.streetCredGain = Cast<Int32>(Cast<Float>(rewards.streetCredGain) * 1.5);
            }
            
            // Apply rewards to player
            CustomActivities.ApplyPlayerRewards(participant.playerId, rewards);
            
            LogChannel(n"COOP_ACTIVITIES", "Distributed rewards to " + participant.playerId + ": " + ToString(rewards.experiencePoints) + " XP");
        }
    }
    
    private static func CalculatePerformanceMultiplier(participant: ActivityParticipant, session: ActivitySession) -> Float {
        // Base multiplier
        let multiplier = 1.0;
        
        // Objective completion bonus
        let objectiveRatio = Cast<Float>(participant.objectivesCompleted) / Cast<Float>(ArraySize(session.objectiveProgress));
        multiplier *= (0.5 + objectiveRatio * 0.5); // 50%-100% based on completion
        
        // Participation time bonus
        let sessionDuration = GetGameTime() - session.startTime;
        let participationTime = sessionDuration - (participant.joinTime - session.startTime);
        let participationRatio = participationTime / sessionDuration;
        multiplier *= (0.7 + participationRatio * 0.3); // 70%-100% based on participation
        
        return ClampF(multiplier, 0.25, 2.0); // Cap between 25% and 200%
    }
    
    // Utility functions
    private static func GetActivity(activityId: String) -> CustomActivity {
        for activity in activities {
            if Equals(activity.activityId, activityId) {
                return activity;
            }
        }
        
        let emptyActivity: CustomActivity;
        return emptyActivity;
    }
    
    private static func GetObjective(activity: CustomActivity, objectiveId: String) -> ActivityObjective {
        for objective in activity.objectives {
            if Equals(objective.objectiveId, objectiveId) {
                return objective;
            }
        }
        
        let emptyObjective: ActivityObjective;
        return emptyObjective;
    }
    
    private static func FindSessionIndex(sessionId: String) -> Int32 {
        for i in Range(ArraySize(activeSessions)) {
            if Equals(activeSessions[i].sessionId, sessionId) {
                return i;
            }
        }
        return -1;
    }
    
    private static func CountOptionalObjectives(activity: CustomActivity) -> Int32 {
        let count = 0;
        for objective in activity.objectives {
            if objective.isOptional {
                count += 1;
            }
        }
        return count;
    }
    
    // Network event handlers
    private static cb func OnActivityCreated(data: String) -> Void {
        LogChannel(n"COOP_ACTIVITIES", "Received activity creation: " + data);
        let activity = CustomActivities.DeserializeActivity(data);
        ArrayPush(activities, activity);
    }
    
    private static cb func OnActivityStarted(data: String) -> Void {
        LogChannel(n"COOP_ACTIVITIES", "Activity session started: " + data);
        let session = CustomActivities.DeserializeSession(data);
        ArrayPush(activeSessions, session);
    }
    
    private static cb func OnPlayerJoinActivity(data: String) -> Void {
        let parts = StrSplit(data, "|");
        if ArraySize(parts) >= 2 {
            let sessionId = parts[0];
            let playerId = parts[1];
            CustomActivities.JoinActivity(sessionId, playerId);
        }
    }
    
    private static cb func OnActivityEvent(data: String) -> Void {
        LogChannel(n"COOP_ACTIVITIES", "Activity event: " + data);
        // Handle custom activity events
    }
    
    private static cb func OnActivityCompleted(data: String) -> Void {
        LogChannel(n"COOP_ACTIVITIES", "Activity completed: " + data);
    }
    
    // Serialization functions
    private static func SerializeActivity(activity: CustomActivity) -> String {
        let data = activity.activityId + "|" + activity.creatorId + "|" + activity.title;
        data += "|" + ToString(Cast<Int32>(activity.activityType)) + "|" + ToString(Cast<Int32>(activity.difficulty));
        data += "|" + ToString(activity.maxParticipants) + "|" + ToString(activity.estimatedDuration);
        return data;
    }
    
    private static func DeserializeActivity(data: String) -> CustomActivity {
        let activity: CustomActivity;
        let parts = StrSplit(data, "|");
        
        if ArraySize(parts) >= 7 {
            activity.activityId = parts[0];
            activity.creatorId = parts[1];
            activity.title = parts[2];
            activity.activityType = IntToEnum(StringToInt(parts[3]), ActivityType.Mission);
            activity.difficulty = IntToEnum(StringToInt(parts[4]), ActivityDifficulty.Normal);
            activity.maxParticipants = StringToInt(parts[5]);
            activity.estimatedDuration = StringToFloat(parts[6]);
        }
        
        return activity;
    }
    
    private static func SerializeSession(session: ActivitySession) -> String {
        let data = session.sessionId + "|" + session.activityId + "|" + session.hostId;
        data += "|" + ToString(Cast<Int32>(session.status)) + "|" + ToString(session.startTime);
        return data;
    }
    
    private static func DeserializeSession(data: String) -> ActivitySession {
        let session: ActivitySession;
        let parts = StrSplit(data, "|");
        
        if ArraySize(parts) >= 5 {
            session.sessionId = parts[0];
            session.activityId = parts[1];
            session.hostId = parts[2];
            session.status = IntToEnum(StringToInt(parts[3]), ActivityStatus.Active);
            session.startTime = StringToFloat(parts[4]);
        }
        
        return session;
    }
    
    // Public API
    public static func GetAvailableActivities(activityType: ActivityType) -> array<CustomActivity> {
        let filteredActivities: array<CustomActivity>;
        for activity in activities {
            if (activityType == ActivityType.Mission || activity.activityType == activityType) && 
               activity.status == ActivityStatus.Active && activity.isPublic {
                ArrayPush(filteredActivities, activity);
            }
        }
        return filteredActivities;
    }
    
    public static func GetActiveSessions() -> array<ActivitySession> {
        let activeSessions_filtered: array<ActivitySession>;
        for session in activeSessions {
            if session.status == ActivityStatus.Active || session.status == ActivityStatus.InProgress {
                ArrayPush(activeSessions_filtered, session);
            }
        }
        return activeSessions_filtered;
    }
    
    public static func GetPlayerActivities(playerId: String) -> array<CustomActivity> {
        let playerActivities: array<CustomActivity>;
        for activity in activities {
            if Equals(activity.creatorId, playerId) {
                ArrayPush(playerActivities, activity);
            }
        }
        return playerActivities;
    }
    
    public static func GetActivityTemplates() -> array<ActivityTemplate> {
        return activityTemplates;
    }
    
    public static func SearchActivities(searchTerm: String, tags: array<String>) -> array<CustomActivity> {
        let results: array<CustomActivity>;
        for activity in activities {
            if activity.status != ActivityStatus.Active || !activity.isPublic {
                continue;
            }
            
            // Check title and description
            let titleMatch = StrContains(StrLower(activity.title), StrLower(searchTerm));
            let descMatch = StrContains(StrLower(activity.description), StrLower(searchTerm));
            
            // Check tags
            let tagMatch = false;
            if ArraySize(tags) > 0 {
                for searchTag in tags {
                    for activityTag in activity.tags {
                        if Equals(StrLower(activityTag), StrLower(searchTag)) {
                            tagMatch = true;
                            break;
                        }
                    }
                    if tagMatch { break; }
                }
            } else {
                tagMatch = true; // No tag filter
            }
            
            if (titleMatch || descMatch) && tagMatch {
                ArrayPush(results, activity);
            }
        }
        return results;
    }
}

// Activity Creator helper class
public class ActivityCreator {
    public func CreateFromTemplate(templateId: String, creatorId: String) -> String {
        // Implementation for creating activity from template
        return "";
    }
    
    public func ValidateActivity(activity: CustomActivity) -> array<String> {
        let errors: array<String>;
        // Implementation for activity validation
        return errors;
    }
    
    public func PublishActivity(activityId: String) -> Bool {
        // Implementation for publishing activity
        return true;
    }
}

// Session Manager helper class
public class ActivitySessionManager {
    public func GetSessionStatus(sessionId: String) -> ActivityStatus {
        // Implementation for getting session status
        return ActivityStatus.Active;
    }
    
    public func PauseSession(sessionId: String) -> Bool {
        // Implementation for pausing session
        return true;
    }
    
    public func ResumeSession(sessionId: String) -> Bool {
        // Implementation for resuming session
        return true;
    }
}