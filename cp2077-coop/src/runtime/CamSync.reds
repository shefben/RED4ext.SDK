public class CamSync {
    public static func OnHijack(pkt: ref<CamHijackPacket>) -> Void {
        LogChannel(n"cam", "hijack cam=" + IntToString(Cast<Int32>(pkt.camId)));
    }
    public static func OnFrame(camId: Uint32) -> Void {
        LogChannel(n"cam", "frame cam=" + IntToString(Cast<Int32>(camId)));
    }
}

public static func CamSync_OnHijack(pkt: ref<CamHijackPacket>) -> Void {
    CamSync.OnHijack(pkt);
}

public static func CamSync_OnFrame(camId: Uint32) -> Void {
    CamSync.OnFrame(camId);
}
