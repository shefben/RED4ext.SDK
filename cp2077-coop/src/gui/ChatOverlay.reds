public class ChatOverlay extends inkHUDLayer {
    public static let s_instance: ref<ChatOverlay>;
    public var lines: array<String>;
    private var visible: Bool;
    private var root: ref<inkVerticalPanel>;
    private var talking: Bool;
    private var seq: Uint16;

    public static func Instance() -> ref<ChatOverlay> {
        if !IsDefined(s_instance) {
            s_instance = new ChatOverlay();
        }
        return s_instance;
    }

    public static func PushGlobal(txt: String) -> Void {
        Instance().PushLine(txt);
    }

    public func PushLine(txt: String) -> Void {
        lines.PushBack(txt);
        if lines.Size() > 50 {
            lines.Erase(0);
            if IsDefined(root) && root.GetNumChildren() > 0 {
                root.RemoveChild(root.GetChild(0));
            }
        }
        if !IsDefined(root) {
            root = new inkVerticalPanel();
            root.SetName(n"chatRoot");
            root.SetAnchor(inkEAnchor.BottomLeft);
            AddChild(root);
        }
        let line = new inkText();
        line.SetText(txt);
        root.AddChild(line);
        LogChannel(n"DEBUG", "Chat: " + txt);
    }

    public func Toggle() -> Void {
        visible = !visible;
        LogChannel(n"DEBUG", "ChatOverlay toggled " + BoolToString(visible));
    }

    public func OnUpdate(dt: Float) -> Void {
        // Toggle visibility when the player presses Enter.
        // Input listener would be cleaner but simple poll works
        if GameInstance.GetInputSystem(GetGame()).IsJustPressed(EInputKey.IK_Enter) {
            Toggle();
        }

        if IsDefined(root) {
            root.SetVisible(visible);
        }

        let input = GameInstance.GetInputSystem(GetGame());
        if input.IsPressed(CoopSettings.pushToTalk) {
            if !talking {
                if CoopVoice.StartCapture("default", CoopSettings.voiceSampleRate, CoopSettings.voiceBitrate) {
                    talking = true;
                    LogChannel(n"DEBUG", "PTT start");
                    MicIcon.Show();
                } else {
                    CoopNotice.Show("Microphone init failed");
                };
            };
            let pcm: array<Int16>;
            pcm.Resize(Cast<Int32>(CoopSettings.voiceSampleRate / 50u));
            let buf: array<Uint8>;
            buf.Resize(256);
            let written = CoopVoice.EncodeFrame(pcm[0], buf[0]);
            if written > 0 {
                Net_SendVoice(buf[0], Cast<Uint16>(written), Cast<Uint16>(seq));
                seq += 1u;
            };
        } else {
            if talking {
                CoopVoice.StopCapture();
                talking = false;
                LogChannel(n"DEBUG", "PTT stop");
                MicIcon.Hide();
            };
        };

        EmoteWheel.Tick();
    }
}
