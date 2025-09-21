// Complete animation synchronization system for multiplayer
// Handles animation state synchronization, emotes, combat animations, and movement

public struct AnimationState {
    public var entityId: Uint64;
    public var animationId: Uint64;
    public var animationType: AnimationType;
    public var startTime: Float;
    public var duration: Float;
    public var playbackSpeed: Float;
    public var isLooping: Bool;
    public var blendInTime: Float;
    public var blendOutTime: Float;
    public var priority: Uint32;
    public var parameters: array<AnimationParameter>;
}

public struct AnimationParameter {
    public var name: CName;
    public var valueType: AnimationParameterType;
    public var floatValue: Float;
    public var intValue: Int32;
    public var boolValue: Bool;
    public var vectorValue: Vector4;
}

public struct AnimationEvent {
    public var eventType: AnimationEventType;
    public var entityId: Uint64;
    public var timestamp: Float;
    public var eventData: String; // JSON serialized data
}

public struct EmoteRequest {
    public var playerId: Uint32;
    public var emoteId: Uint32;
    public var targetEntityId: Uint64; // For targeted emotes
    public var duration: Float;
    public var canCancel: Bool;
}

public enum AnimationType {
    Movement = 0,
    Combat = 1,
    Emote = 2,
    Interaction = 3,
    Vehicle = 4,
    Cinematic = 5,
    Idle = 6,
    Death = 7,
    Special = 8
}

public enum AnimationParameterType {
    Float = 0,
    Int = 1,
    Bool = 2,
    Vector = 3
}

public enum AnimationEventType {
    Start = 0,
    Stop = 1,
    Pause = 2,
    Resume = 3,
    Loop = 4,
    Marker = 5,
    Sync = 6
}

public class AnimationSync {
    // Animation state tracking
    private static var activeAnimations: array<AnimationState>;
    private static var pendingEmotes: array<EmoteRequest>;
    private static var animationEvents: array<AnimationEvent>;

    // System configuration
    private static var isInitialized: Bool = false;
    private static var maxConcurrentAnimations: Int32 = 50;
    private static var animationSyncRate: Float = 30.0; // FPS for animation sync
    private static var lastSyncTime: Float = 0.0;

    // Animation priority system
    private static let PRIORITY_DEATH: Uint32 = 1000u;
    private static let PRIORITY_COMBAT: Uint32 = 800u;
    private static let PRIORITY_CINEMATIC: Uint32 = 600u;
    private static let PRIORITY_EMOTE: Uint32 = 400u;
    private static let PRIORITY_MOVEMENT: Uint32 = 200u;
    private static let PRIORITY_IDLE: Uint32 = 100u;

    // === System Initialization ===

    public static func InitializeAnimationSync() -> Bool {
        if isInitialized {
            LogChannel(n"ANIMATION_SYNC", "Animation sync already initialized");
            return true;
        }

        LogChannel(n"ANIMATION_SYNC", "Initializing animation synchronization system");

        // Clear existing data
        ArrayClear(activeAnimations);
        ArrayClear(pendingEmotes);
        ArrayClear(animationEvents);

        lastSyncTime = EngineTime.ToFloat(GameInstance.GetSimTime());

        // Initialize animation tracking
        if !AnimationSync_Initialize() {
            LogChannel(n"ERROR", "Failed to initialize native animation sync backend");
            return false;
        }

        isInitialized = true;
        LogChannel(n"ANIMATION_SYNC", "Animation synchronization system initialized");
        return true;
    }

    // === Animation State Management ===

    public static func StartAnimation(entityId: Uint64, animationId: Uint64, animType: AnimationType,
                                    duration: Float, playbackSpeed: Float, priority: Uint32) -> Bool {
        if !isInitialized {
            LogChannel(n"ERROR", "Animation sync not initialized");
            return false;
        }

        // Check if entity already has an animation of this type
        let existingIndex = FindAnimationIndex(entityId, animType);

        let animState: AnimationState;
        animState.entityId = entityId;
        animState.animationId = animationId;
        animState.animationType = animType;
        animState.startTime = EngineTime.ToFloat(GameInstance.GetSimTime());
        animState.duration = duration;
        animState.playbackSpeed = playbackSpeed;
        animState.isLooping = false;
        animState.blendInTime = 0.2;
        animState.blendOutTime = 0.2;
        animState.priority = priority;

        if existingIndex >= 0 {
            // Check priority - only replace if new animation has higher priority
            if priority >= activeAnimations[existingIndex].priority {
                activeAnimations[existingIndex] = animState;
                LogChannel(n"ANIMATION_SYNC", "Replaced animation for entity " + Uint64ToString(entityId));
            } else {
                LogChannel(n"WARNING", "Animation priority too low, keeping existing animation");
                return false;
            }
        } else {
            // Add new animation
            ArrayPush(activeAnimations, animState);
        }

        // Broadcast to all players
        Net_SendAnimationStart(animState);

        // Create animation event
        let animEvent: AnimationEvent;
        animEvent.eventType = AnimationEventType.Start;
        animEvent.entityId = entityId;
        animEvent.timestamp = animState.startTime;
        animEvent.eventData = SerializeAnimationState(animState);
        ArrayPush(animationEvents, animEvent);

        LogChannel(n"ANIMATION_SYNC", "Started animation " + Uint64ToString(animationId) + " for entity " + Uint64ToString(entityId));
        return true;
    }

    public static func StopAnimation(entityId: Uint64, animationType: AnimationType) -> Bool {
        let animIndex = FindAnimationIndex(entityId, animationType);
        if animIndex < 0 {
            return false;
        }

        let animState = activeAnimations[animIndex];
        ArrayRemove(activeAnimations, animState);

        // Broadcast stop to all players
        Net_SendAnimationStop(entityId, animationType);

        // Create stop event
        let animEvent: AnimationEvent;
        animEvent.eventType = AnimationEventType.Stop;
        animEvent.entityId = entityId;
        animEvent.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime());
        ArrayPush(animationEvents, animEvent);

        LogChannel(n"ANIMATION_SYNC", "Stopped animation for entity " + Uint64ToString(entityId));
        return true;
    }

    // === Emote System ===

    public static func RequestEmote(playerId: Uint32, emoteId: Uint32, targetEntityId: Uint64) -> Bool {
        if !isInitialized {
            return false;
        }

        // Validate emote ID
        if !IsValidEmoteId(emoteId) {
            LogChannel(n"ERROR", "Invalid emote ID: " + IntToString(emoteId));
            return false;
        }

        // Check if player can perform emote
        if !CanPlayerPerformEmote(playerId) {
            LogChannel(n"WARNING", "Player " + IntToString(playerId) + " cannot perform emote");
            return false;
        }

        let emoteRequest: EmoteRequest;
        emoteRequest.playerId = playerId;
        emoteRequest.emoteId = emoteId;
        emoteRequest.targetEntityId = targetEntityId;
        emoteRequest.duration = GetEmoteDuration(emoteId);
        emoteRequest.canCancel = true;

        ArrayPush(pendingEmotes, emoteRequest);

        // Process emote immediately
        return ProcessEmoteRequest(emoteRequest);
    }

    private static func ProcessEmoteRequest(emoteRequest: EmoteRequest) -> Bool {
        // Get emote animation data
        let emoteData = GetEmoteAnimationData(emoteRequest.emoteId);
        if !IsDefined(emoteData) {
            LogChannel(n"ERROR", "Failed to get emote animation data");
            return false;
        }

        // Get player entity ID
        let playerEntityId = GetPlayerEntityId(emoteRequest.playerId);
        if playerEntityId == 0ul {
            LogChannel(n"ERROR", "Failed to get player entity ID");
            return false;
        }

        // Start emote animation with high priority
        return StartAnimation(playerEntityId, emoteData.animationId, AnimationType.Emote,
                            emoteRequest.duration, 1.0, PRIORITY_EMOTE);
    }

    // === Animation Synchronization ===

    public static func UpdateAnimationSync() -> Void {
        if !isInitialized {
            return;
        }

        let currentTime = EngineTime.ToFloat(GameInstance.GetSimTime());
        let deltaTime = currentTime - lastSyncTime;

        // Only sync at specified rate
        if deltaTime < (1.0 / animationSyncRate) {
            return;
        }

        lastSyncTime = currentTime;

        // Update active animations
        CleanupExpiredAnimations(currentTime);

        // Process pending emotes
        ProcessPendingEmotes();

        // Cleanup old events
        CleanupOldEvents(currentTime);

        // Sync animations to server
        if ArraySize(activeAnimations) > 0 {
            Net_SyncAnimationStates(activeAnimations);
        }
    }

    private static func CleanupExpiredAnimations(currentTime: Float) -> Void {
        let toRemove: array<AnimationState>;

        for animState in activeAnimations {
            if !animState.isLooping && (currentTime - animState.startTime) > animState.duration {
                ArrayPush(toRemove, animState);
            }
        }

        for expiredAnim in toRemove {
            ArrayRemove(activeAnimations, expiredAnim);
            LogChannel(n"DEBUG", "Cleaned up expired animation for entity " + Uint64ToString(expiredAnim.entityId));
        }
    }

    private static func ProcessPendingEmotes() -> Void {
        let processedEmotes: array<EmoteRequest>;

        for emoteRequest in pendingEmotes {
            if ProcessEmoteRequest(emoteRequest) {
                ArrayPush(processedEmotes, emoteRequest);
            }
        }

        // Remove processed emotes
        for processed in processedEmotes {
            ArrayRemove(pendingEmotes, processed);
        }
    }

    private static func CleanupOldEvents(currentTime: Float) -> Void {
        let maxEventAge: Float = 30.0; // Keep events for 30 seconds
        let toRemove: array<AnimationEvent>;

        for event in animationEvents {
            if (currentTime - event.timestamp) > maxEventAge {
                ArrayPush(toRemove, event);
            }
        }

        for oldEvent in toRemove {
            ArrayRemove(animationEvents, oldEvent);
        }
    }

    // === Network Message Handlers ===

    public static func OnAnimationReceived(animState: AnimationState) -> Void {
        if !isInitialized {
            return;
        }

        // Apply animation to entity
        ApplyAnimationToEntity(animState);

        // Update local animation state
        let existingIndex = FindAnimationIndex(animState.entityId, animState.animationType);
        if existingIndex >= 0 {
            activeAnimations[existingIndex] = animState;
        } else {
            ArrayPush(activeAnimations, animState);
        }

        LogChannel(n"ANIMATION_SYNC", "Received and applied animation for entity " + Uint64ToString(animState.entityId));
    }

    public static func OnAnimationStop(entityId: Uint64, animationType: AnimationType) -> Void {
        let animIndex = FindAnimationIndex(entityId, animationType);
        if animIndex >= 0 {
            let animState = activeAnimations[animIndex];
            ArrayRemove(activeAnimations, animState);

            // Stop animation on entity
            StopAnimationOnEntity(entityId, animationType);

            LogChannel(n"ANIMATION_SYNC", "Stopped remote animation for entity " + Uint64ToString(entityId));
        }
    }

    // === Utility Functions ===

    private static func FindAnimationIndex(entityId: Uint64, animationType: AnimationType) -> Int32 {
        let count = ArraySize(activeAnimations);
        var i = 0;
        while i < count {
            if activeAnimations[i].entityId == entityId && activeAnimations[i].animationType == animationType {
                return i;
            }
            i += 1;
        }
        return -1;
    }

    private static func IsValidEmoteId(emoteId: Uint32) -> Bool {
        // Check against available emote IDs
        return emoteId > 0u && emoteId <= 50u; // Assuming 50 emotes available
    }

    private static func CanPlayerPerformEmote(playerId: Uint32) -> Bool {
        // Check if player is in a state where emotes are allowed
        let player = GetPlayerFromId(playerId);
        if !IsDefined(player) {
            return false;
        }

        // Check if player is in combat
        if IsPlayerInCombat(player) {
            return false;
        }

        // Check if player is already performing an emote
        let playerEntityId = GetPlayerEntityId(playerId);
        return FindAnimationIndex(playerEntityId, AnimationType.Emote) < 0;
    }

    private static func GetEmoteDuration(emoteId: Uint32) -> Float {
        // Return appropriate duration based on emote type
        switch emoteId {
            case 1u: return 2.0; // Wave
            case 2u: return 3.0; // Dance
            case 3u: return 1.5; // Thumbs up
            case 4u: return 4.0; // Sit
            case 5u: return 2.5; // Clap
            default: return 2.0;
        }
    }

    private static func SerializeAnimationState(animState: AnimationState) -> String {
        // Simple JSON serialization of animation state
        return "{\"animationId\":" + Uint64ToString(animState.animationId) +
               ",\"type\":" + IntToString(EnumInt(animState.animationType)) +
               ",\"startTime\":" + FloatToString(animState.startTime) +
               ",\"duration\":" + FloatToString(animState.duration) + "}";
    }

    public static func GetActiveAnimationCount() -> Int32 {
        return ArraySize(activeAnimations);
    }

    public static func GetPendingEmoteCount() -> Int32 {
        return ArraySize(pendingEmotes);
    }

    public static func ClearAllAnimations() -> Void {
        ArrayClear(activeAnimations);
        ArrayClear(pendingEmotes);
        ArrayClear(animationEvents);
        LogChannel(n"ANIMATION_SYNC", "Cleared all animation data");
    }
}

// === Enhanced EmoteController Integration ===

@wrapMethod(EmoteController)
public static func GetAnim(emoteId: Uint32) -> String {
    // Enhanced emote controller with extended emote support
    let extendedPaths: array<String> = [
        "animations/emotes/wave.anim",        // 0
        "animations/emotes/dance.anim",       // 1
        "animations/emotes/thumbsup.anim",    // 2
        "animations/emotes/sit.anim",         // 3
        "animations/emotes/clap.anim",        // 4
        "animations/emotes/point.anim",       // 5
        "animations/emotes/salute.anim",      // 6
        "animations/emotes/laugh.anim",       // 7
        "animations/emotes/cry.anim",         // 8
        "animations/emotes/bow.anim",         // 9
        "animations/emotes/shrug.anim",       // 10
        "animations/emotes/flex.anim",        // 11
        "animations/emotes/peace.anim",       // 12
        "animations/emotes/heart.anim",       // 13
        "animations/emotes/angry.anim",       // 14
        "animations/emotes/sleep.anim"        // 15
    ];

    if emoteId < Cast<Uint32>(ArraySize(extendedPaths)) {
        return extendedPaths[emoteId];
    }

    // Fallback to original method for compatibility
    return wrappedMethod(emoteId);
}

// === Placeholder functions for game integration ===

private static func GetEmoteAnimationData(emoteId: Uint32) -> ref<EmoteAnimationData> {
    // Would return actual emote animation data
    let data = new EmoteAnimationData();
    data.animationId = Cast<Uint64>(emoteId + 1000u); // Offset for emote animations
    return data;
}

private static func GetPlayerFromId(playerId: Uint32) -> ref<PlayerPuppet> {
    // Would integrate with player management system
    return GetPlayer(GetGame());
}

private static func GetPlayerEntityId(playerId: Uint32) -> Uint64 {
    // Would get actual entity ID from player
    let player = GetPlayerFromId(playerId);
    if IsDefined(player) {
        return Cast<Uint64>(player.GetEntityID());
    }
    return 0ul;
}

private static func IsPlayerInCombat(player: ref<PlayerPuppet>) -> Bool {
    // Would check actual combat state
    return false; // Placeholder
}

private static func ApplyAnimationToEntity(animState: AnimationState) -> Void {
    // Would apply animation to actual game entity
    LogChannel(n"DEBUG", "Applying animation " + Uint64ToString(animState.animationId) + " to entity");
}

private static func StopAnimationOnEntity(entityId: Uint64, animationType: AnimationType) -> Void {
    // Would stop animation on actual game entity
    LogChannel(n"DEBUG", "Stopping animation type " + IntToString(EnumInt(animationType)) + " on entity");
}

// Support data structure
public class EmoteAnimationData extends IScriptable {
    public let animationId: Uint64;
    public let duration: Float;
    public let priority: Uint32;
}

// === Network Integration Functions ===

private static native func AnimationSync_Initialize() -> Bool;
private static native func Net_SendAnimationStart(animState: AnimationState) -> Void;
private static native func Net_SendAnimationStop(entityId: Uint64, animationType: AnimationType) -> Void;
private static native func Net_SyncAnimationStates(animations: array<AnimationState>) -> Void;