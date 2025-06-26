// Server-authoritative NPC controller.
public enum NpcState {
    Idle,
    Wander,
    Combat
}

public class NpcController {
    public static let proxies: array<ref<NpcProxy>>;

    public static func ServerTick(dt: Float) -> Void {
        CoopNet.NpcController_ServerTick(dt);
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
}
