// Quickhack replication utilities.
public struct HackInfo {
    public var targetId: Uint32;
    public var hackId: Uint32;
    public var durationMs: Uint16;
    public var startHealth: Uint16; // health when hack applied
}

public class QuickhackSync {
    public static var activeHacks: array<HackInfo>;
    private static var tickAccum: Uint32;
    public static func SendHack(info: ref<HackInfo>) -> Void {
        // NetCore.BroadcastQuickhack(info);
        LogChannel(n"DEBUG", "SendHack target=" + IntToString(info.targetId));
    }

    public static func ApplyHack(info: ref<HackInfo>) -> Void {
        LogChannel(n"DEBUG", "ApplyHack target=" + IntToString(info.targetId) +
            " hack=" + IntToString(info.hackId));
        let target = GameInstance.GetPlayerSystem(GetGame()).FindObject(info.targetId) as AvatarProxy;
        if IsDefined(target) {
            info.startHealth = target.health;
        } else {
            info.startHealth = 0u;
        }
        activeHacks.PushBack(*info);
    }

    public static func Tick(dt: Float) -> Void {
        tickAccum += Cast<Uint32>(dt * 1000.0);
        if tickAccum < 500u {
            return;
        }
        tickAccum -= 500u;

        var i: Int32 = 0;
        while i < activeHacks.Size() {
            let hack = activeHacks[i];
            let target = GameInstance.GetPlayerSystem(GetGame()).FindObject(hack.targetId) as AvatarProxy;
            if IsDefined(target) && hack.hackId == 1u {
                let totalTicks = Cast<Uint16>(hack.durationMs / 500u);
                if totalTicks == 0u { totalTicks = 1u; }
                let step = Cast<Uint16>(FloorF(Cast<Float>(hack.startHealth) * 0.25 / Cast<Float>(totalTicks)));
                if step > target.health {
                    step = target.health;
                }
                target.health -= step;
                target.OnVitalsChanged();
            }
            if hack.durationMs <= 500u {
                activeHacks.Erase(i);
                continue;
            } else {
                activeHacks[i].durationMs -= 500u;
            }
            i += 1;
        }
    }
}
