public class CrowdCfgSync {
    private static var locked: Bool = false;
    private static var saved: Uint8 = 0u;

    public static func OnApply(density: Uint8) -> Void {
        if !locked {
            saved = density; // placeholder for ini read
            locked = true;
        };
        LogChannel(n"crowd", "Density forced " + IntToString(Cast<Int32>(density)));
        ChatOverlay.PushGlobal("Density locked for multiplayer stability");
    }

    public static func OnRestore() -> Void {
        if locked {
            LogChannel(n"crowd", "Density restored");
            locked = false;
        };
    }
}
