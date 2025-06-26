public class GlobalEventManager {
    public static func OnEvent(eventId: Uint32, phase: Uint8, seed: Uint32, start: Bool) -> Void {
        LogChannel(n"DEBUG", "Event " + IntToString(eventId) + " phase=" + IntToString(phase) + (start ? " start" : " stop"));
    }
}
