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
    private static var armourTimer: Float;
    private static var cameraTimer: Float;
    private static var vulnTimer: Float;
    public static func SendHack(info: ref<HackInfo>) -> Void {
        // NetCore.BroadcastQuickhack(info); // implemented in C++
    }

    public static func ApplyHack(info: ref<HackInfo>) -> Void {
        let target = GameInstance.GetPlayerSystem(GetGame()).FindObject(info.targetId) as AvatarProxy;
        if IsDefined(target) {
            info.startHealth = target.health;
        } else {
            info.startHealth = 0u;
        }
        if info.hackId == 1u {
            StatusEffectSync.OnApply(info.targetId, 2u, info.durationMs, 1u);
        };
        activeHacks.PushBack(*info);
    }

    public static func Tick(dt: Float) -> Void {
        tickAccum += Cast<Uint32>(dt * 1000.0);
        if tickAccum < 500u {
            return;
        }
        tickAccum -= 500u;

        if armourTimer > 0.0 {
            armourTimer -= 0.5;
            if armourTimer <= 0.0 {
                HeatSync.ApplyArmorDebuff(1.0);
                LogChannel(n"DEBUG", "Daemon armour debuff expired");
            }
        };
        if cameraTimer > 0.0 {
            cameraTimer -= 0.5;
            if cameraTimer <= 0.0 {
                LogChannel(n"DEBUG", "Daemon camera disable ended");
            }
        };
        if vulnTimer > 0.0 {
            vulnTimer -= 0.5;
            if vulnTimer <= 0.0 {
                HeatSync.ApplyDamageBuff(1.0);
                LogChannel(n"DEBUG", "Daemon mass vulnerability ended");
            }
        };

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

    public static func OnBreachResult(peerId: Uint32, mask: Uint8) -> Void {
        if (mask & 1u) != 0u {
            armourTimer = 30.0;
            HeatSync.ApplyArmorDebuff(0.7);
            LogChannel(n"DEBUG", "Daemon armour debuff 30s");
        };
        if (mask & 2u) != 0u {
            cameraTimer = 90.0;
            LogChannel(n"DEBUG", "Daemon cameras disabled for 90s"); // future work
        };
        if (mask & 4u) != 0u {
            vulnTimer = 30.0;
            HeatSync.ApplyDamageBuff(1.2);
            LogChannel(n"DEBUG", "Daemon mass vulnerability 30s");
        };
    }

    public static func OnPingOutline(peerId: Uint32, ids: array<Uint32>, dur: Uint16) -> Void {
        for id in ids {
            OutlineHelper.AddPing(id, dur);
        };
    }

    public static func SendPingOutline(ids: array<Uint32>, dur: Uint16) -> Void {
        if Net_IsAuthoritative() {
            CoopNet.Net_BroadcastPingOutline(Net_GetPeerId(), dur, ids);
        };
    }
}

public static func QuickhackSync_OnPingOutline(peer: Uint32, dur: Uint16, ids: array<Uint32>) -> Void {
    QuickhackSync.OnPingOutline(peer, ids, dur);
}
