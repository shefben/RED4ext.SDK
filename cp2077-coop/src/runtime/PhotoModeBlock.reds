// Disables photo mode during multiplayer sessions.
// TODO: Verify PhotoModeSystem class name and Enter method signature

/*
@wrapMethod(PhotoModeSystem)
protected func Enter() -> Void {
    if GameModeManager.current != GameMode.Coop {
        wrappedMethod();
        return;
    };
    PopupGuard.ReplaceWithCoopNotice(n"PhotoMode");
    LogChannel(n"DEBUG", "[PhotoMode] blocked");
}
*/

// Alternative implementation using global function
public static func PhotoModeBlock_CheckAndBlock() -> Bool {
    if GameModeManager.current == GameMode.Coop {
        PopupGuard.ReplaceWithCoopNotice(n"PhotoMode");
        LogChannel(n"DEBUG", "[PhotoMode] blocked");
        return true;
    };
    return false;
}
