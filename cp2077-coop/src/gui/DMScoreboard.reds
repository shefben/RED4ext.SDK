// P7-2: implement full scoreboard UI
public class DMScoreboard extends inkHUDLayer {
    public static let s_instance: ref<DMScoreboard>;
    public var kills: Uint16;
    public var deaths: Uint16;

    public static func Instance() -> ref<DMScoreboard> {
        if !IsDefined(s_instance) {
            s_instance = new DMScoreboard();
        }
        return s_instance;
    }

    public func Show() -> Void {
        // Display scoreboard (ink widget code TBD)
        LogChannel(n"DEBUG", "Scoreboard shown");
    }

    public func Update(peerId: Uint32, k: Uint16, d: Uint16) -> Void {
        kills = k;
        deaths = d;
        LogChannel(n"DEBUG", "ScoreUpdate " + IntToString(peerId));
    }

    public static func OnScorePacket(peerId: Uint32, k: Uint16, d: Uint16) -> Void {
        Instance().Update(peerId, k, d);
    }
}

// Input hook will call Show() when Tab is pressed.
