// Server-authoritative NPC controller.
public class NpcController {
    public static func ServerTick(dt: Float) -> Void {
        // Random walk placeholder driven by deterministic RNG.
        LogChannel(n"DEBUG", "NpcController.ServerTick dt=" + FloatToString(dt));
    }

    public static func ClientApplySnap(snap: ref<NpcSnap>) -> Void {
        LogChannel(n"DEBUG", "ApplyNpcSnap id=" + IntToString(snap.npcId));
    }
}
