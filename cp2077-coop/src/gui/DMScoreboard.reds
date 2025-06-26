// P7-2: implement full scoreboard UI
public class DMScoreboard extends inkHUDLayer {
    public static let s_instance: ref<DMScoreboard>;
    public var kills: Uint16;
    public var deaths: Uint16;
    private var rowIds: array<Uint32>;
    private var rows: array<ref<inkText>>;
    private var banner: ref<inkText>;

    public static func Instance() -> ref<DMScoreboard> {
        if !IsDefined(s_instance) {
            s_instance = new DMScoreboard();
        }
        return s_instance;
    }

    private var ticker: ref<inkText>;
    
    public func Show() -> Void {
        if !IsDefined(ticker) {
            ticker = new inkText();
            ticker.SetName(n"FragTicker");
            ticker.SetSize(400.0, 50.0);
            ticker.SetAnchor(inkEAnchor.Center);
            ticker.SetTranslation(new Vector2(0.0, 10.0));
            AddChild(ticker);
        };
        for i in 0 ..< rows.Size() {
            rows[i].SetVisible(true);
        };
        SetVisible(true);
        LogChannel(n"DEBUG", "Scoreboard shown");
    }

    // Legacy stub removed

    public func Update(peerId: Uint32, k: Uint16, d: Uint16) -> Void {
        kills = k;
        deaths = d;
        let idx = rowIds.Find(peerId);
        if idx == -1 {
            let t = new inkText();
            t.SetAnchor(inkEAnchor.Center);
            t.SetTranslation(new Vector2(0.0, 40.0 * Cast<Float>(rowIds.Size())));
            AddChild(t);
            rowIds.Push(peerId);
            rows.Push(t);
            idx = rows.Size() - 1;
        };
        rows[idx].SetText(IntToString(peerId) + "  " + IntToString(k) + " / " + IntToString(d));
        if IsDefined(ticker) {
            ticker.SetText(IntToString(k) + " / " + IntToString(d));
        };
        LogChannel(n"DEBUG", "ScoreUpdate " + IntToString(peerId));
    }

    public static func OnScorePacket(peerId: Uint32, k: Uint16, d: Uint16) -> Void {
        Instance().Update(peerId, k, d);
    }

    public static func OnMatchOver(winner: Uint32) -> Void {
        Instance().ShowWinner(winner);
    }

    public func ShowWinner(winId: Uint32) -> Void {
        if !IsDefined(banner) {
            banner = new inkText();
            banner.SetAnchor(inkEAnchor.Center);
            banner.SetTranslation(new Vector2(0.0, 100.0));
            banner.SetFontSize(48);
            AddChild(banner);
        };
        let local = (GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy).peerId;
        if winId == local {
            banner.SetText("VICTORY");
        } else {
            banner.SetText("DEFEAT");
        };
        LogChannel(n"DEBUG", "Confetti!");
    }
}

// Input hook will call Show() when Tab is pressed.
