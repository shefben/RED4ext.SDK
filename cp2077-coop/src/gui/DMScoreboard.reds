// P7-2: implement full scoreboard UI
public class DMScoreboard extends inkHUDLayer {
    public static let s_instance: ref<DMScoreboard>;
    public var kills: Uint16;
    public var deaths: Uint16;
    private struct RowInfo {
        public let peer: Uint32;
        public let widget: ref<inkText>;
        public var k: Uint16;
        public var d: Uint16;
    }
    private var rows: array<RowInfo>;
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
            rows[i].widget.SetVisible(true);
        };
        SetVisible(true);
        LogChannel(n"DEBUG", "Scoreboard shown");
    }

    // Legacy stub removed

    public func Update(peerId: Uint32, k: Uint16, d: Uint16) -> Void {
        kills = k;
        deaths = d;
        var idx: Int32 = -1;
        var i: Int32 = 0;
        while i < rows.Size() {
            if rows[i].peer == peerId { idx = i; break; };
            i += 1;
        };
        if idx == -1 {
            let w = new inkText();
            w.SetAnchor(inkEAnchor.Center);
            AddChild(w);
            let ri: RowInfo;
            ri.peer = peerId;
            ri.widget = w;
            ri.k = k;
            ri.d = d;
            rows.PushBack(ri);
            idx = rows.Size() - 1;
        } else {
            rows[idx].k = k;
            rows[idx].d = d;
        };

        rows[idx].widget.SetText(IntToString(peerId) + "  " + IntToString(k) + " / " + IntToString(d));
        // sort by kills desc
        var swapped: Bool = true;
        while swapped {
            swapped = false;
            i = 0;
            while i < rows.Size() - 1 {
                if rows[i].k < rows[i+1].k {
                    let tmp = rows[i];
                    rows[i] = rows[i+1];
                    rows[i+1] = tmp;
                    swapped = true;
                };
                i += 1;
            };
        };
        // reposition rows
        i = 0;
        while i < rows.Size() {
            rows[i].widget.SetTranslation(new Vector2(0.0, 40.0 * Cast<Float>(i)));
            i += 1;
        };
        if IsDefined(ticker) { ticker.SetText(IntToString(k) + " / " + IntToString(d)); };
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
