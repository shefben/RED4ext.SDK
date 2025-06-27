public class VendorSync {
    public static let stock : ref<inkHashMap> = new inkHashMap(); // key vendorId<<32|phaseId

    public static func OnStock(pkt: ref<VendorStockPacket>) -> Void {
        let key: Uint64 = ShiftLeft(Cast<Uint64>(pkt.vendorId), 32u) | Cast<Uint64>(pkt.phaseId);
        stock.Insert(key, pkt);
    }

    public static func OnStockUpdate(pkt: ref<VendorStockUpdatePacket>) -> Void {
        let key: Uint64 = ShiftLeft(Cast<Uint64>(pkt.vendorId), 32u) | Cast<Uint64>(pkt.phaseId);
        let entry = stock.Get(key) as VendorStockPacket;
        if IsDefined(entry) {
            let count: Int32 = Cast<Int32>(entry.count);
            var i: Int32 = 0;
            while i < count {
                if entry.items[i].itemId == pkt.itemId {
                    entry.items[i].qty = pkt.qty;
                    break;
                };
                i += 1;
            };
        };
    }
}
