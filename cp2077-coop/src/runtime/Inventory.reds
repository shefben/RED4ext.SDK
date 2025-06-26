// Temporary runtime table; persistent storage handled in a later ticket.
public struct ItemSnap {
    public var itemId: Uint64;
    public var ownerId: Uint32;
    public var tpl: Uint16;
    public var level: Uint16;
    public var quality: Uint16;
    public var rolls: array<Uint32>;
    public var slotMask: Uint8;
    public var attachmentIds: array<Uint64>;
}

public class Inventory {
    public static var items: array<ItemSnap>;

    public static func OnItemSnap(snap: ref<ItemSnap>) -> Void {
        let count: Int32 = ArraySize(items);
        var i: Int32 = 0;
        while i < count {
            if items[i].itemId == snap.itemId {
                items[i] = *snap;
                return;
            };
            i += 1;
        };
        items.PushBack(*snap);
        LogChannel(n"DEBUG", "OnItemSnap " + Uint64ToString(snap.itemId));
    }

    public static func OnCraftResult(snap: ref<ItemSnap>) -> Void {
        items.PushBack(*snap);
        LogChannel(n"DEBUG", "Crafted item " + Uint64ToString(snap.itemId));
    }

    public static func OnAttachResult(success: Bool, snap: ref<ItemSnap>) -> Void {
        if success {
            LogChannel(n"DEBUG", "Mod attached " + Uint64ToString(snap.itemId));
        } else {
            LogChannel(n"DEBUG", "Attach failed " + Uint64ToString(snap.itemId));
        };
    }

    public static func RequestCraft(recipeId: Uint32) -> Void {
        Net_SendCraftRequest(recipeId);
    }

    public static func RequestAttach(itemId: Uint64, slotIdx: Uint8, attachId: Uint64) -> Void {
        Net_SendAttachRequest(itemId, slotIdx, attachId);
    }
}
