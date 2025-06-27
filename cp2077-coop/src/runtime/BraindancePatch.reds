@wrapMethod(BDSystem)
public final func EnterEditor() -> Void {
    if !Net_IsAuthoritative() {
        let timeSys = GameInstance.GetTimeSystem(GetGame());
        timeSys.SetLocalTimeDilation(0.0);
        CoopNet.Net_BroadcastCineStart(CoopNet.Fnv1a32("bd_editor"), 0u, Net_GetPeerId(), true);
    } else {
        wrappedMethod();
    };
}
