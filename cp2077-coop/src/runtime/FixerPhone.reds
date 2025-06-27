@wrapMethod(PhoneSystem)
protected func StartCall(fixerId: CName) -> Void {
    if GameModeManager.current != GameMode.Coop {
        wrappedMethod(fixerId);
        return;
    };
    let idHash: Uint32 = CoopNet.Fnv1a32(NameToString(fixerId));
    if Net_IsAuthoritative() {
        CoopNet.Net_BroadcastFixerCallStart(idHash);
    };
    FixerCallSync.OnStart(idHash);
}

@wrapMethod(PhoneSystem)
protected func EndCall() -> Void {
    if GameModeManager.current != GameMode.Coop {
        wrappedMethod();
        return;
    };
    if Net_IsAuthoritative() {
        CoopNet.Net_BroadcastFixerCallEnd(FixerCallSync.currentId);
    };
    FixerCallSync.OnEnd(FixerCallSync.currentId);
}
