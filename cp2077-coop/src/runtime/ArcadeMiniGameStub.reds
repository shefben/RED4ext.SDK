public class ArcadeMiniGameStub {
    public static func Start(cabId: Uint32, seed: Uint32) -> Void {
        LogChannel(n"arcade", "MiniGameStub start cab=" + IntToString(Cast<Int32>(cabId)));
    }
    public static func OnScore(peer: Uint32, score: Uint32) -> Void {
        LogChannel(n"arcade", "MiniGameStub score=" + IntToString(Cast<Int32>(score)));
    }
    public static func OnHighScore(cabId: Uint32, peer: Uint32, score: Uint32) -> Void {
        LogChannel(n"arcade", "MiniGameStub highscore=" + IntToString(Cast<Int32>(score)));
    }
}
