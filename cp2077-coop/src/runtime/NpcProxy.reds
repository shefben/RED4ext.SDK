public class NpcProxy extends gameObject {
    public var npcId: Uint32;
    public var templateId: Uint16;
    public var pos: Vector3;
    public var rot: Quaternion;
    public var state: NpcState;
    public var aiState: PoliceAIState;
    public var health: Uint16;
    public var appearanceSeed: Uint8;
    public var sectorHash: Uint64;

    public func Spawn(snap: ref<NpcSnap>) -> Void {
        npcId = snap.npcId;
        templateId = snap.templateId;
        appearanceSeed = snap.appearanceSeed;
        sectorHash = snap.sectorHash;
        pos = snap.pos;
        rot = snap.rot;
        state = snap.state;
        aiState = Cast<PoliceAIState>(snap.aiState);
        health = snap.health;
        LogChannel(n"DEBUG", "NpcProxy.Spawn " + IntToString(npcId) + " tpl=" + IntToString(templateId));
        RED4ext.ExecuteFunction("WorldObjectSpawner", "SpawnNPC", npcId, templateId, pos, rot);
    }

    public func ApplySnap(snap: ref<NpcSnap>) -> Void {
        pos = snap.pos;
        rot = snap.rot;
        state = snap.state;
        aiState = Cast<PoliceAIState>(snap.aiState);
        health = snap.health;
        sectorHash = snap.sectorHash;
        switch state {
           case NpcState.Idle:
                let seed = NpcController.GetCrowdSeed(sectorHash);
                let tick = RoundF(GameInstance.GetSimTime() as Float);
                let val = CoopNet.Fnv1a32(IntToString(npcId + tick)) ^ seed;
                if val % 2u == 0u {
                    SetAnimation(n"idle_wave");
                } else {
                    SetAnimation(n"idle");
                };
                break;
           case NpcState.Wander:
                let seed = NpcController.GetCrowdSeed(sectorHash);
                let tick = RoundF(GameInstance.GetSimTime() as Float);
                let val = CoopNet.Fnv1a32(IntToString(npcId + tick)) ^ seed;
                if val % 2u == 0u {
                    SetAnimation(n"walk_phone");
                } else {
                    SetAnimation(n"walk");
                };
                break;
            case NpcState.Combat:
                SetAnimation(n"combat");
                break;
        };
        LogChannel(n"DEBUG", "NpcProxy.ApplySnap " + IntToString(npcId));
    }

    public func Despawn() -> Void {
        LogChannel(n"DEBUG", "NpcProxy.Despawn " + IntToString(npcId));
    }

    public func OnAIState(stateVal: Uint8) -> Void {
        aiState = Cast<PoliceAIState>(stateVal);
        LogChannel(n"ncpd", "AIState " + IntToString(Cast<Int32>(stateVal)));
    }

    private func SetAnimation(name: CName) -> Void {
        // Would call animation controller when available
        LogChannel(n"DEBUG", "Play anim " + NameToString(name));
    }
}
