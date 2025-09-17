@wrapMethod(BDSystem)
public final func EnterEditor() -> Void {
    if !Net_IsAuthoritative() {
        let timeSys = GameInstance.GetTimeSystem(GetGame());
        timeSys.SetLocalTimeDilation(0.0);
        Net_BroadcastCineStart(Fnv1a32("bd_editor"), 0u, Net_GetPeerId(), true);
    } else {
        wrappedMethod();
    };
}
