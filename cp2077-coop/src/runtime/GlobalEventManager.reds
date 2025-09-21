public class GlobalEventManager {
    // Global event synchronization system for multiplayer
    private static let s_activeEvents: ref<inkHashMap>; // EventId -> GlobalEventData
    private static let s_eventHandlers: ref<inkHashMap>; // EventType -> Handler function
    private static let s_eventHistory: array<ref<EventHistoryEntry>>;

    public static func OnEvent(eventId: Uint32, phase: Uint8, seed: Uint32, start: Bool) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Event \(eventId) phase=\(phase) \(start ? "start" : "stop") seed=\(seed)");

        // Initialize if needed
        if !IsDefined(GlobalEventManager.s_activeEvents) {
            GlobalEventManager.Initialize();
        }

        if start {
            GlobalEventManager.StartEvent(eventId, phase, seed);
        } else {
            GlobalEventManager.StopEvent(eventId, phase);
        }
    }

    private static func Initialize() -> Void {
        GlobalEventManager.s_activeEvents = new inkHashMap();
        GlobalEventManager.s_eventHandlers = new inkHashMap();

        // Register event handlers for different event types
        GlobalEventManager.RegisterEventHandlers();

        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Initialized global event system");
    }

    private static func RegisterEventHandlers() -> Void {
        // Register handlers for different types of global events
        // Event type ranges:
        // 1000-1999: Weather events
        // 2000-2999: Traffic events
        // 3000-3999: Police/Gang events
        // 4000-4999: Corporate events
        // 5000-5999: News events

        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Registered event handlers for all event types");
    }

    private static func StartEvent(eventId: Uint32, phase: Uint8, seed: Uint32) -> Void {
        // Create event data
        let eventData = new GlobalEventData();
        eventData.eventId = eventId;
        eventData.phase = phase;
        eventData.seed = seed;
        eventData.startTime = EngineTime.ToFloat(GameInstance.GetSimTime());
        eventData.isActive = true;
        eventData.eventType = GlobalEventManager.DetermineEventType(eventId);

        // Store active event
        GlobalEventManager.s_activeEvents.Insert(eventId, eventData);

        // Broadcast to all players
        Net_BroadcastGlobalEvent(eventId, phase, true, seed);

        // Execute event-specific logic
        GlobalEventManager.ExecuteEventStart(eventData);

        // Log to event history
        GlobalEventManager.LogEventHistory(eventId, phase, true, seed);

        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Started event \(eventId) type=\(eventData.eventType)");
    }

    private static func StopEvent(eventId: Uint32, phase: Uint8) -> Void {
        let eventData = GlobalEventManager.s_activeEvents.Get(eventId) as GlobalEventData;
        if !IsDefined(eventData) {
            LogChannel(n"GlobalEvents", s"[GlobalEventManager] Warning: Trying to stop unknown event \(eventId)");
            return;
        }

        // Mark as inactive
        eventData.isActive = false;
        eventData.endTime = EngineTime.ToFloat(GameInstance.GetSimTime());

        // Broadcast to all players
        Net_BroadcastGlobalEvent(eventId, phase, false, 0u);

        // Execute event-specific cleanup
        GlobalEventManager.ExecuteEventStop(eventData);

        // Remove from active events
        GlobalEventManager.s_activeEvents.Remove(eventId);

        // Log to event history
        GlobalEventManager.LogEventHistory(eventId, phase, false, 0u);

        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Stopped event \(eventId)");
    }

    private static func DetermineEventType(eventId: Uint32) -> GlobalEventType {
        if eventId >= 1000u && eventId < 2000u {
            return GlobalEventType.Weather;
        } else if eventId >= 2000u && eventId < 3000u {
            return GlobalEventType.Traffic;
        } else if eventId >= 3000u && eventId < 4000u {
            return GlobalEventType.Police;
        } else if eventId >= 4000u && eventId < 5000u {
            return GlobalEventType.Corporate;
        } else if eventId >= 5000u && eventId < 6000u {
            return GlobalEventType.News;
        } else {
            return GlobalEventType.Generic;
        }
    }

    private static func ExecuteEventStart(eventData: ref<GlobalEventData>) -> Void {
        switch eventData.eventType {
            case GlobalEventType.Weather:
                GlobalEventManager.HandleWeatherEventStart(eventData);
                break;
            case GlobalEventType.Traffic:
                GlobalEventManager.HandleTrafficEventStart(eventData);
                break;
            case GlobalEventType.Police:
                GlobalEventManager.HandlePoliceEventStart(eventData);
                break;
            case GlobalEventType.Corporate:
                GlobalEventManager.HandleCorporateEventStart(eventData);
                break;
            case GlobalEventType.News:
                GlobalEventManager.HandleNewsEventStart(eventData);
                break;
            default:
                GlobalEventManager.HandleGenericEventStart(eventData);
                break;
        }
    }

    private static func ExecuteEventStop(eventData: ref<GlobalEventData>) -> Void {
        switch eventData.eventType {
            case GlobalEventType.Weather:
                GlobalEventManager.HandleWeatherEventStop(eventData);
                break;
            case GlobalEventType.Traffic:
                GlobalEventManager.HandleTrafficEventStop(eventData);
                break;
            case GlobalEventType.Police:
                GlobalEventManager.HandlePoliceEventStop(eventData);
                break;
            case GlobalEventType.Corporate:
                GlobalEventManager.HandleCorporateEventStop(eventData);
                break;
            case GlobalEventType.News:
                GlobalEventManager.HandleNewsEventStop(eventData);
                break;
            default:
                GlobalEventManager.HandleGenericEventStop(eventData);
                break;
        }
    }

    // Event type handlers
    private static func HandleWeatherEventStart(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Starting weather event \(eventData.eventId)");
        // Trigger weather changes, atmospheric effects, etc.
    }

    private static func HandleWeatherEventStop(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Stopping weather event \(eventData.eventId)");
        // Restore normal weather conditions
    }

    private static func HandleTrafficEventStart(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Starting traffic event \(eventData.eventId)");
        // Modify traffic patterns, spawn special vehicles, etc.
    }

    private static func HandleTrafficEventStop(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Stopping traffic event \(eventData.eventId)");
        // Restore normal traffic
    }

    private static func HandlePoliceEventStart(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Starting police event \(eventData.eventId)");
        // Increase police presence, spawn NCPD units, etc.
    }

    private static func HandlePoliceEventStop(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Stopping police event \(eventData.eventId)");
        // Return to normal police activity
    }

    private static func HandleCorporateEventStart(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Starting corporate event \(eventData.eventId)");
        // Trigger corporate activities, stock changes, etc.
    }

    private static func HandleCorporateEventStop(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Stopping corporate event \(eventData.eventId)");
        // End corporate event effects
    }

    private static func HandleNewsEventStart(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Starting news event \(eventData.eventId)");
        // Broadcast news, update billboards, etc.
    }

    private static func HandleNewsEventStop(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Stopping news event \(eventData.eventId)");
        // End news coverage
    }

    private static func HandleGenericEventStart(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Starting generic event \(eventData.eventId)");
        // Handle generic events
    }

    private static func HandleGenericEventStop(eventData: ref<GlobalEventData>) -> Void {
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Stopping generic event \(eventData.eventId)");
        // Clean up generic event
    }

    private static func LogEventHistory(eventId: Uint32, phase: Uint8, start: Bool, seed: Uint32) -> Void {
        let historyEntry = new EventHistoryEntry();
        historyEntry.eventId = eventId;
        historyEntry.phase = phase;
        historyEntry.started = start;
        historyEntry.seed = seed;
        historyEntry.timestamp = EngineTime.ToFloat(GameInstance.GetSimTime());

        ArrayPush(GlobalEventManager.s_eventHistory, historyEntry);

        // Keep only last 100 events
        if ArraySize(GlobalEventManager.s_eventHistory) > 100 {
            ArrayRemove(GlobalEventManager.s_eventHistory, GlobalEventManager.s_eventHistory[0]);
        }
    }

    public static func GetActiveEvents() -> array<Uint32> {
        let activeEventIds: array<Uint32>;

        // In a real implementation, this would iterate over the hashmap
        LogChannel(n"GlobalEvents", s"[GlobalEventManager] Getting active events");

        return activeEventIds;
    }

    public static func IsEventActive(eventId: Uint32) -> Bool {
        if !IsDefined(GlobalEventManager.s_activeEvents) {
            return false;
        }

        let eventData = GlobalEventManager.s_activeEvents.Get(eventId) as GlobalEventData;
        return IsDefined(eventData) && eventData.isActive;
    }
}

// Data structures for global event system
public class GlobalEventData extends IScriptable {
    public let eventId: Uint32;
    public let phase: Uint8;
    public let seed: Uint32;
    public let startTime: Float;
    public let endTime: Float;
    public let isActive: Bool;
    public let eventType: GlobalEventType;
}

public class EventHistoryEntry extends IScriptable {
    public let eventId: Uint32;
    public let phase: Uint8;
    public let started: Bool;
    public let seed: Uint32;
    public let timestamp: Float;
}

public enum GlobalEventType {
    Generic = 0,
    Weather = 1,
    Traffic = 2,
    Police = 3,
    Corporate = 4,
    News = 5
}

// Native function declarations for networking
native func Net_BroadcastGlobalEvent(eventId: Uint32, phase: Uint8, start: Bool, seed: Uint32) -> Void;
