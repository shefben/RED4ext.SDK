public class HoloTableSync {
    public static func OnOpen(sceneId: Uint32) -> Void {
        LogChannel(n"htable", "open scene=" + IntToString(Cast<Int32>(sceneId)));
    }
    public static func OnScrub(timeMs: Uint32) -> Void {
        LogChannel(n"htable", "scrub=" + IntToString(Cast<Int32>(timeMs)));
    }
}

public static func HoloTableSync_OnOpen(sceneId: Uint32) -> Void {
    HoloTableSync.OnOpen(sceneId);
}

public static func HoloTableSync_OnScrub(timeMs: Uint32) -> Void {
    HoloTableSync.OnScrub(timeMs);
}
