public class ArcadeSync {
    public static func OnStart(pkt: ref<ArcadeStartPacket>) -> Void {
        LogChannel(n"arcade", "start cab=" + IntToString(Cast<Int32>(pkt.cabId)));
    }
    public static func OnScore(pkt: ref<ArcadeScorePacket>) -> Void {
        LogChannel(n"arcade", "score=" + IntToString(Cast<Int32>(pkt.score)));
    }
}

public static func ArcadeSync_OnStart(pkt: ref<ArcadeStartPacket>) -> Void {
    ArcadeSync.OnStart(pkt);
}

public static func ArcadeSync_OnScore(pkt: ref<ArcadeScorePacket>) -> Void {
    ArcadeSync.OnScore(pkt);
}
