public struct ContainerInfo {
    public var id: Uint32;
    public var items: array<Uint64>;
}

public class LootAuthority {
    private static var ownershipIds: array<Uint64>;
    private static var owners: array<Uint32>;
    private static var containers: array<ContainerInfo>;

    public static func MarkOwnership(itemId: Uint64, ownerId: Uint32) -> Void {
        var idx: Int32 = ownershipIds.Find(itemId);
        if idx == -1 {
            ownershipIds.PushBack(itemId);
            owners.PushBack(ownerId);
        } else {
            owners[idx] = ownerId;
        };
        if ownerId != 0u && ownerId != Net_GetPeerId() {
            LootMarkers.Push("Item " + ToString(itemId) + " claimed by " + IntToString(Cast<Int32>(ownerId)));
        };
        LogChannel(n"DEBUG", "Ownership " + ToString(itemId) + " -> " + IntToString(ownerId));
    }

    public static func CanPickup(itemId: Uint64, peerId: Uint32) -> Bool {
        let idx: Int32 = ownershipIds.Find(itemId);
        if idx == -1 { return true; };
        let owner = owners[idx];
        return owner == 0u || owner == peerId;
    }

    public static func OnLootRoll(containerId: Uint32, seed: Uint32, items: array<Uint64>) -> Void {
        let info: ContainerInfo;
        info.id = containerId;
        info.items = items;
        var i: Int32 = 0;
        while i < ArraySize(containers) && containers[i].id != containerId { i += 1; };
        if i == ArraySize(containers) {
            containers.PushBack(info);
        } else {
            containers[i] = info;
        };
        LogChannel(n"loot", "roll " + IntToString(containerId) + " items=" + IntToString(ArraySize(items)));
    }
}

public static func LootAuthority_OnLootRoll(id: Uint32, seed: Uint32, items: array<Uint64>) -> Void {
    LootAuthority.OnLootRoll(id, seed, items);
}
