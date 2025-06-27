// Disables photo mode during multiplayer sessions.

@wrapMethod(PhotoModeSystem)
protected func Enter() -> Void {
    if GameModeManager.current != GameMode.Coop {
        wrappedMethod();
        return;
    };
    PopupGuard.ReplaceWithCoopNotice(n"PhotoMode");
    LogChannel(n"DEBUG", "[PhotoMode] blocked");
}
