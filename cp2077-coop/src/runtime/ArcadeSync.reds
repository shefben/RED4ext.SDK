public class ArcadeSync {
    public static func OnStart(pkt: ref<ArcadeStartPacket>) -> Void {
        LogChannel(n"arcade", "start cab=" + IntToString(Cast<Int32>(pkt.cabId)));
        ArcadeMiniGameStub.Start(pkt.cabId, pkt.seed);
    }
    public static func OnScore(pkt: ref<ArcadeScorePacket>) -> Void {
        LogChannel(n"arcade", "score=" + IntToString(Cast<Int32>(pkt.score)));
        ArcadeMiniGameStub.OnScore(pkt.peerId, pkt.score);
    }
    public static func OnHighScore(pkt: ref<ArcadeHighScorePacket>) -> Void {
        LogChannel(n"arcade", "high=" + IntToString(Cast<Int32>(pkt.score)));
        ArcadeMiniGameStub.OnHighScore(pkt.cabId, pkt.peerId, pkt.score);
    }
}

public static func ArcadeSync_OnStart(pkt: ref<ArcadeStartPacket>) -> Void {
    ArcadeSync.OnStart(pkt);
}

public static func ArcadeSync_OnScore(pkt: ref<ArcadeScorePacket>) -> Void {
    ArcadeSync.OnScore(pkt);
}

public static func ArcadeSync_OnHighScore(pkt: ref<ArcadeHighScorePacket>) -> Void {
    ArcadeSync.OnHighScore(pkt);
}
