public class PropSync {
    public static func OnBreak(id: Uint32, seed: Uint32) -> Void {
        LogChannel(n"prop", "Break " + IntToString(Cast<Int32>(id)) + " seed=" + IntToString(Cast<Int32>(seed)));
    }

    public static func OnIgnite(id: Uint32, delay: Uint16) -> Void {
        LogChannel(n"prop", "Ignite " + IntToString(Cast<Int32>(id)) + " delay=" + IntToString(Cast<Int32>(delay)));
    }

    public static func ScheduleChain(prop: ref<physicsProp>, seed: Uint32) -> Void {
        let query = GameInstance.GetWorldQuerySystem(GetGame());
        if !IsDefined(query) { return; };
        let pos = prop.GetWorldPosition();
        let actors: array<ref<GameObject>>;
        query.GetActorsInSphere(pos, 8.0, actors);
        for obj in actors {
            let barrel = obj as physicsProp;
            if IsDefined(barrel) && barrel.IsExplosive() && barrel != prop {
                let delay: Uint16 = Cast<Uint16>(100u + (seed & 0xFFu) % 200u);
                CoopNet.Net_BroadcastPropIgnite(Cast<Uint32>(EntityID.GetHash(barrel.GetEntityID())), delay);
            };
        };
    }

    public static func HandleBreak(prop: ref<physicsProp>) -> Void {
        if Net_IsAuthoritative() {
            let id: Uint32 = Cast<Uint32>(EntityID.GetHash(prop.GetEntityID()));
            let seed: Uint32 = CoopNet.Fnv1a32(IntToString(Cast<Int32>(id)) + IntToString(Cast<Int32>(GameInstance.GetSimTime(GetGame()))));
            CoopNet.Net_BroadcastPropBreak(id, seed);
            if prop.IsExplosive() { ScheduleChain(prop, seed); };
        };
    }
}

@hook(physicsProp.OnBreak)
protected func physicsProp_OnBreak(original: func(ref<physicsProp>), self: ref<physicsProp>) -> Void {
    original(self);
    PropSync.HandleBreak(self);
}
