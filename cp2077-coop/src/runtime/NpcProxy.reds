public class NpcProxy extends gameObject {
    public var npcId: Uint32;
    public var templateId: Uint16;
    public var pos: Vector3;
    public var rot: Quaternion;
    public var state: NpcState;
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
        health = snap.health;
        LogChannel(n"DEBUG", "NpcProxy.Spawn " + IntToString(npcId) + " tpl=" + IntToString(templateId));
        // Placeholder mesh spawn
        LogChannel(n"DEBUG", "Spawn mesh base\\characters\\crowd_man_01.mesh");
    }

    public func ApplySnap(snap: ref<NpcSnap>) -> Void {
        pos = snap.pos;
        rot = snap.rot;
        state = snap.state;
        health = snap.health;
        sectorHash = snap.sectorHash;
        switch state {
            case NpcState.Idle:
                LogChannel(n"DEBUG", "Anim idle");
                break;
            case NpcState.Wander:
                LogChannel(n"DEBUG", "Anim walk");
                break;
            case NpcState.Combat:
                LogChannel(n"DEBUG", "Anim combat");
                break;
        };
        LogChannel(n"DEBUG", "NpcProxy.ApplySnap " + IntToString(npcId));
    }

    public func Despawn() -> Void {
        LogChannel(n"DEBUG", "NpcProxy.Despawn " + IntToString(npcId));
    }
}
