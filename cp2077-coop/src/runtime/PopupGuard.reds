// Suppresses certain pop-ups that break multiplayer flow.
public class PopupGuard {
    // IDs of pop-ups disabled in multiplayer sessions.
    private static let s_blocked: array<CName> = [n"popup_sleep", n"popup_braindance"];

    // Returns true if the pop-up should be suppressed for co-op.
    public static func IsPopupBlocked(id: CName) -> Bool {
        for item in s_blocked {
            if item == id {
                return true;
            }
        }
        return false;
    }

    // Substitute message when a pop-up is blocked.
    public static func ReplaceWithCoopNotice(id: CName) -> Void {
        CoopNotice.Show(NameToString(id));
    }

    // Hook call used by sleep/braindance pop-ups.
    public static func HandlePopup(id: CName) -> Bool {
        if IsPopupBlocked(id) {
            ReplaceWithCoopNotice(id);
            return true; // caller should skip original UI
        }
        return false;
    }
}

// --- Hooks ---------------------------------------------------------------
// TODO: Verify these controller class names exist in the game

/*
@wrapMethod(SleepPopupGameController)
protected final func ShowPopup(id: CName) -> Void {
    if PopupGuard.HandlePopup(id) {
        return; // skip original
    };
    wrappedMethod(id);
}

@wrapMethod(BraindancePopupGameController)
protected final func ShowPopup(id: CName) -> Void {
    if PopupGuard.HandlePopup(id) {
        return;
    };
    wrappedMethod(id);
}
*/
