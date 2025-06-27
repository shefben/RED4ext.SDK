public struct PanicEventPacket {
    public let pos: Vector3;
    public let seed: Uint32;
}

public class PanicSync {
    public static func OnEvent(pos: Vector3, seed: Uint32) -> Void {
        LogChannel(n"panic", "seed=" + IntToString(Cast<Int32>(seed)));
        // FIXME(next ticket): spawn flee behavior
    }

    public static func SendEvent(pos: Vector3) -> Void {
        if Net_IsAuthoritative() {
            let s: Uint32 = CoopNet.Fnv1a32(FloatToString(pos.X) + FloatToString(pos.Y) + FloatToString(pos.Z) + IntToString(Cast<Int32>(CoopNet.GameClock.GetCurrentTick())));
            CoopNet.Net_BroadcastPanicEvent(pos, s);
        };
    }
}

public static func PanicSync_OnEvent(pkt: ref<PanicEventPacket>) -> Void {
    PanicSync.OnEvent(pkt.pos, pkt.seed);
}
