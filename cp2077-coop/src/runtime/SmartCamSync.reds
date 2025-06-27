public class SmartCamSync {
    public static func OnStart(id: Uint32) -> Void {
        LogChannel(n"smartcam", "start=" + IntToString(Cast<Int32>(id)));
        RED4ext.ExecuteFunction("ProjectileCamera", "Start", null, id);
    }
    public static func OnEnd(id: Uint32) -> Void {
        LogChannel(n"smartcam", "end=" + IntToString(Cast<Int32>(id)));
        RED4ext.ExecuteFunction("ProjectileCamera", "Stop", null, id);
    }
}

public static func SmartCamSync_OnStart(id: Uint32) -> Void {
    SmartCamSync.OnStart(id);
}

public static func SmartCamSync_OnEnd(id: Uint32) -> Void {
    SmartCamSync.OnEnd(id);
}
