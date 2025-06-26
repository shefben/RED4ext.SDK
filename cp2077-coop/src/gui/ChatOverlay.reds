public class ChatOverlay extends inkHUDLayer {
    public static let s_instance: ref<ChatOverlay>;
    public var lines: array<String>;
    private var visible: Bool;
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
        }
        // P3-4: render text lines with ink widgets
        LogChannel(n"DEBUG", "Chat: " + txt);
    }

    public func Toggle() -> Void {
        visible = !visible;
        LogChannel(n"DEBUG", "ChatOverlay toggled " + BoolToString(visible));
    }

    public func OnUpdate(dt: Float) -> Void {
        // Toggle visibility when the player presses Enter.
        // P3-4: use input listener callback
        if GameInstance.GetInputSystem(GetGame()).IsJustPressed(EInputKey.IK_Enter) {
            Toggle();
        }

        let input = GameInstance.GetInputSystem(GetGame());
        if input.IsPressed(CoopSettings.pushToTalk) {
            if !talking {
                CoopVoice.StartCapture("default");
                talking = true;
                LogChannel(n"DEBUG", "PTT start");
                MicIcon.Show();
            };
            let pcm: array<Int16>;
            pcm.Resize(960); // VC-1: capture real PCM from mic
            let buf: array<Uint8>;
            buf.Resize(256);
            let written = CoopVoice.EncodeFrame(pcm[0], buf[0]);
            if written > 0 {
                Net_SendVoice(buf[0], Cast<Uint16>(written), Cast<Uint16>(seq));
                seq += 1u;
            };
        } else {
            if talking {
                talking = false;
                LogChannel(n"DEBUG", "PTT stop");
                MicIcon.Hide();
            };
        };
    }
}
