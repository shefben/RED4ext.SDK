@wrapMethod(WorkTable)
public final func ReRollMods(itemId: Uint64) -> Void {
    if Net_IsAuthoritative() {
        wrappedMethod(itemId);
    } else {
        Inventory.RequestReRoll(itemId);
    };
}
