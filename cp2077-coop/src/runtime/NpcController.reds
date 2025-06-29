// Server-authoritative NPC controller.
public enum NpcState {
    Idle,
    Wander,
    Combat
}

public class NpcController {
    public static let proxies: array<ref<NpcProxy>>;
    public static let crowdSeeds: ref<inkHashMap> = new inkHashMap();
    private static let reinforceTimer: Float = 0.0;
    private static let waveCount: Uint32 = 0u;
    private static let desired: Uint32 = 6u;

    public static func ServerTick(dt: Float) -> Void {
        CoopNet.NpcController_ServerTick(dt);
        if Net_IsAuthoritative() {
            ReinforcementTick(dt);
        };
    }

    private static func FindProxy(id: Uint32) -> ref<NpcProxy> {
        for p in proxies {
            if p.npcId == id { return p; };
        };
        return null;
    }

    public static func ClientApplySnap(snap: ref<NpcSnap>) -> Void {
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
        let dist = VectorDistance(player.GetWorldPosition(), snap.pos);
        if dist > 120.0 {
            let existing = FindProxy(snap.npcId);
            if IsDefined(existing) {
                existing.Despawn();
                proxies.Erase(existing);
            };
            return;
        };

        let npc = FindProxy(snap.npcId);
        if !IsDefined(npc) {
            npc = new NpcProxy();
            npc.Spawn(snap);
            proxies.PushBack(npc);
        } else {
            npc.ApplySnap(snap);
        };
    }

    public static func DespawnNpc(id: Uint32) -> Void {
        let npc = FindProxy(id);
        if IsDefined(npc) {
            npc.Despawn();
            proxies.Erase(npc);
        };
    }

    public static func ApplyCrowdSeed(hash: Uint64, seed: Uint32) -> Void {
        crowdSeeds.Insert(hash, seed);
    }

    public static func GetCrowdSeed(hash: Uint64) -> Uint32 {
        let val = crowdSeeds.Get(hash) as Uint32;
        return val;
    }

    private static func ReinforcementTick(dt: Float) -> Void {
        let players = Cast<Uint32>(Net_GetPeerCount());
        if players > 2u && proxies.Size() < Cast<Int32>(desired) {
            reinforceTimer += dt;
            if reinforceTimer >= 30.0 {
                reinforceTimer = 0.0;
                let need: Uint32 = desired - Cast<Uint32>(proxies.Size());
                var i: Uint32 = 0u;
                while i < need {
                    CoopNet.SpawnPhaseNpc();
                    i += 1u;
                };
                waveCount += 1u;
            };
        } else {
            reinforceTimer = 0.0;
        };
    }
}
