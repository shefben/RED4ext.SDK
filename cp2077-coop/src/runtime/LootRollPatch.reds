public static exec func CoopContainerOpened(id: Int32) -> Void {
    if !Net_IsAuthoritative() { return; };
    let tick: Uint32 = CoopNet.GameClock.GetTime();
    let pid: Uint32 = Net_GetPeerId();
    let seed: Uint32 = CoopNet.Fnv1a32(IntToString(Cast<Int32>(tick)) + IntToString(id) + IntToString(Cast<Int32>(pid)));
    CoopNet.Net_BroadcastLootRoll(Cast<Uint32>(id), seed);
}
