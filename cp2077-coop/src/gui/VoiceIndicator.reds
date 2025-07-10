public class VoiceIndicator extends inkHUDLayer {
    private struct IconInfo {
        public let peer: Uint32;
        public let widget: ref<inkRectangle>;
        public var fade: Float;
    }
    private static let s_instance: ref<VoiceIndicator>;
    private let icons: array<IconInfo>;
    private let muteBtn: ref<inkText>;
    private let volUp: ref<inkText>;
    private let volDown: ref<inkText>;
    private var volume: Float = 1.0;
    private var muted: Bool;

    public static func Instance() -> ref<VoiceIndicator> {
        if !IsDefined(s_instance) {
            s_instance = new VoiceIndicator();
            GameInstance.GetHUDManager(GetGame()).AddLayer(s_instance);
        };
        return s_instance;
    }

    public static func OnVoice(peerId: Uint32) -> Void {
        Instance().MarkSpeaking(peerId);
    }

    private func MarkSpeaking(peerId: Uint32) -> Void {
        var idx: Int32 = 0;
        while idx < icons.Size() && icons[idx].peer != peerId { idx += 1; };
        if idx == icons.Size() {
            let info: IconInfo;
            info.peer = peerId;
            info.widget = new inkRectangle();
            info.widget.SetSize(32.0,32.0);
            info.widget.SetTintColor(new HDRColor(1.0,1.0,1.0,1.0));
            AddChild(info.widget);
            icons.PushBack(info);
            idx = icons.Size() - 1;
        };
        icons[idx].fade = 1.0;
        icons[idx].widget.SetOpacity(1.0);
    }

    protected func OnCreate() -> Void {
        muteBtn = new inkText();
        muteBtn.SetText("Mute");
        muteBtn.SetAnchor(inkEAnchor.TopRight);
        muteBtn.SetMargin(new inkMargin(0.0,20.0,120.0,0.0));
        AddChild(muteBtn);
        muteBtn.RegisterToCallback(n"OnRelease", this, n"OnMuteClick");

        volUp = new inkText();
        volUp.SetText("+");
        volUp.SetAnchor(inkEAnchor.TopRight);
        volUp.SetMargin(new inkMargin(0.0,20.0,80.0,0.0));
        AddChild(volUp);
        volUp.RegisterToCallback(n"OnRelease", this, n"OnVolUp");

        volDown = new inkText();
        volDown.SetText("-");
        volDown.SetAnchor(inkEAnchor.TopRight);
        volDown.SetMargin(new inkMargin(0.0,20.0,100.0,0.0));
        AddChild(volDown);
        volDown.RegisterToCallback(n"OnRelease", this, n"OnVolDown");
    }

    protected cb func OnMuteClick(widget: ref<inkWidget>) -> Bool {
        muted = !muted;
        if muted {
            CoopVoice.SetVolume(0.0);
        } else {
            CoopVoice.SetVolume(volume);
        };
        return true;
    }

    protected cb func OnVolUp(widget: ref<inkWidget>) -> Bool {
        volume += 0.1;
        CoopVoice.SetVolume(volume);
        return true;
    }

    protected cb func OnVolDown(widget: ref<inkWidget>) -> Bool {
        volume -= 0.1;
        if volume < 0.0 { volume = 0.0; };
        CoopVoice.SetVolume(volume);
        return true;
    }

    public func OnUpdate(dt: Float) -> Void {
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let players = playerSys.GetPlayers();
        var i: Int32 = 0;
        while i < icons.Size() {
            icons[i].fade -= dt*2.0;
            if icons[i].fade <= 0.0 {
                RemoveChild(icons[i].widget);
                icons.Erase(i);
                continue;
            };
            let av = playerSys.FindObject(icons[i].peer) as AvatarProxy;
            if IsDefined(av) {
                let pos = av.pos + new Vector3(0.0,0.0,1.8);
                let screen = GameInstance.GetViewportManager(GetGame()).WorldToScreen(pos);
                icons[i].widget.SetMargin(new inkMargin(screen.X-16.0, screen.Y-60.0,0.0,0.0));
            };
            icons[i].widget.SetOpacity(icons[i].fade);
            i += 1;
        };
    }
}

public static func VoiceIndicator_OnVoice(peerId: Uint32) -> Void {
    VoiceIndicator.OnVoice(peerId);
}
