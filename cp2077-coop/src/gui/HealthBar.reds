public class HealthBar {
    // Draw a simple health/armor bar above the owner avatar.
    public func Draw(owner: ref<AvatarProxy>) -> Void {
        // FIXME(next ticket): replace with proper ink widget rendering.
        LogChannel(n"DEBUG", "Draw health bar for peer " + IntToString(owner.peerId));
    }
}
