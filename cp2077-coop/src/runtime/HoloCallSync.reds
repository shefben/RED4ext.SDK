public class HoloCallSync {
    public static func OnStart(pkt: ref<HolocallStartPacket>) -> Void {
        // show overlay for each peer
        LogChannel(n"holocall", "start call=" + IntToString(Cast<Int32>(pkt.callId)));
    }
    public static func OnEnd(callId: Uint32) -> Void {
        LogChannel(n"holocall", "end call=" + IntToString(Cast<Int32>(callId)));
    }
}

public static func HoloCallSync_OnStart(pkt: ref<HolocallStartPacket>) -> Void {
    HoloCallSync.OnStart(pkt);
}

public static func HoloCallSync_OnEnd(callId: Uint32) -> Void {
    HoloCallSync.OnEnd(callId);
}
