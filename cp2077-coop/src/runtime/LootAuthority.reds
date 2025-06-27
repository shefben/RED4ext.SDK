public class LootAuthority {
    public static func MarkOwnership(itemId: Uint64, ownerId: Uint32) -> Void {
        LogChannel(n"DEBUG", "Ownership " + Uint64ToString(itemId) + " -> " + IntToString(ownerId));
    }

    public static func CanPickup(itemId: Uint64, peerId: Uint32) -> Bool {
        LogChannel(n"DEBUG", "CanPickup " + Uint64ToString(itemId) + " by " + IntToString(peerId));
        return true;
    }

    public static func OnLootRoll(containerId: Uint32, seed: Uint32) -> Void {
        LogChannel(n"loot", "roll " + IntToString(containerId) + " seed " + IntToString(seed));
    }
}

public static func LootAuthority_OnLootRoll(id: Uint32, seed: Uint32) -> Void {
    LootAuthority.OnLootRoll(id, seed);
}
