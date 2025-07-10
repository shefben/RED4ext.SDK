public class VoiceIndicator extends inkHUDLayer {
    private struct IconInfo {
        public let peer: Uint32;
        public let root: ref<inkCanvas>;
        public let icon: ref<inkRectangle>;
        public let muteBtn: ref<inkText>;
        public let volUp: ref<inkText>;
        public let volDown: ref<inkText>;
        public var volume: Float;
        public var muted: Bool;
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
            info.root = new inkCanvas();
            info.root.SetName(StringToName(IntToString(peerId)));
            info.icon = MicIcon.CreateWidget();
            info.root.AddChild(info.icon);
            info.muteBtn = new inkText();
            info.muteBtn.SetText("M");
            info.muteBtn.SetTranslation(new Vector2(36.0, 0.0));
            info.muteBtn.RegisterToCallback(n"OnRelease", this, n"OnPeerMute");
            info.root.AddChild(info.muteBtn);
            info.volUp = new inkText();
            info.volUp.SetText("+");
            info.volUp.SetTranslation(new Vector2(36.0, 16.0));
            info.volUp.RegisterToCallback(n"OnRelease", this, n"OnPeerVolUp");
            info.root.AddChild(info.volUp);
            info.volDown = new inkText();
            info.volDown.SetText("-");
            info.volDown.SetTranslation(new Vector2(36.0, 32.0));
            info.volDown.RegisterToCallback(n"OnRelease", this, n"OnPeerVolDown");
            info.root.AddChild(info.volDown);
            info.volume = 1.0;
            info.muted = false;
            AddChild(info.root);
            icons.PushBack(info);
            idx = icons.Size() - 1;
        };
        icons[idx].fade = 1.0;
        icons[idx].root.SetOpacity(1.0);
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
        volume = CoopSettings.voiceVolume;
        CoopVoice.SetVolume(volume);
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

    protected cb func OnPeerMute(widget: ref<inkWidget>) -> Bool {
        let peerId = StringToUint(NameToString(widget.GetParentWidget().GetName()));
        var i: Int32 = 0;
        while i < icons.Size() && icons[i].peer != peerId { i += 1; };
        if i < icons.Size() {
            icons[i].muted = !icons[i].muted;
            if icons[i].muted {
                icons[i].icon.SetTintColor(new HDRColor(0.5,0.5,0.5,1.0));
            } else {
                icons[i].icon.SetTintColor(new HDRColor(1.0,1.0,1.0,1.0));
            };
        };
        return true;
    }

    protected cb func OnPeerVolUp(widget: ref<inkWidget>) -> Bool {
        let peerId = StringToUint(NameToString(widget.GetParentWidget().GetName()));
        var i: Int32 = 0;
        while i < icons.Size() && icons[i].peer != peerId { i += 1; };
        if i < icons.Size() {
            icons[i].volume += 0.1;
        };
        return true;
    }

    protected cb func OnPeerVolDown(widget: ref<inkWidget>) -> Bool {
        let peerId = StringToUint(NameToString(widget.GetParentWidget().GetName()));
        var i: Int32 = 0;
        while i < icons.Size() && icons[i].peer != peerId { i += 1; };
        if i < icons.Size() {
            icons[i].volume -= 0.1;
            if icons[i].volume < 0.0 { icons[i].volume = 0.0; };
        };
        return true;
    }

    public func OnUpdate(dt: Float) -> Void {
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let players = playerSys.GetPlayers();
        var i: Int32 = 0;
        while i < icons.Size() {
            icons[i].fade -= dt*2.0;
            if icons[i].fade <= 0.0 {
                RemoveChild(icons[i].root);
                icons.Erase(i);
                continue;
            };
            let av = playerSys.FindObject(icons[i].peer) as AvatarProxy;
            if IsDefined(av) {
                let pos = av.pos + new Vector3(0.0,0.0,1.8);
                let screen = GameInstance.GetViewportManager(GetGame()).WorldToScreen(pos);
                icons[i].root.SetMargin(new inkMargin(screen.X-16.0, screen.Y-60.0,0.0,0.0));
            };
            icons[i].root.SetOpacity(icons[i].fade);
            i += 1;
        };
    }
}

public static func VoiceIndicator_OnVoice(peerId: Uint32) -> Void {
    VoiceIndicator.OnVoice(peerId);
}
