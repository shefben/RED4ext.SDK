@wrapMethod(RipperdocSystem)
public final func BeginInstall(slot: Int32) -> Void {
    if Net_IsAuthoritative() {
        wrappedMethod(slot);
        CoopNet.Net_BroadcastCineStart(CoopNet.Fnv1a32("ripper_chair"), 0u, Net_GetPeerId(), true);
    } else {
        CoopNet.Net_SendRipperInstallRequest(Cast<Uint8>(slot));
    };
}
