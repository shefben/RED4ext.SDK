public class ChatOverlay extends inkHUDLayer {
    public static let s_instance: ref<ChatOverlay>;
    public var lines: array<String>;
    private var visible: Bool;

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
        // FIXME(next ticket): render text lines using ink widgets.
        LogChannel(n"DEBUG", "Chat: " + txt);
    }

    public func Toggle() -> Void {
        visible = !visible;
        LogChannel(n"DEBUG", "ChatOverlay toggled " + BoolToString(visible));
    }

    public func OnUpdate(dt: Float) -> Void {
        // Toggle visibility when the player presses Enter.
        // FIXME(next ticket): replace with proper input listener registration.
        if GameInstance.GetInputSystem(GetGame()).IsJustPressed(EInputKey.IK_Enter) {
            Toggle();
        }
    }
}
