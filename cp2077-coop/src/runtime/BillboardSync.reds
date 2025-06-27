public class BillboardSync {
    public static func OnSeed(hash: Uint64, seed: Uint64) -> Void {
    }
    public static func OnNextAd(hash: Uint64, ad: Uint32) -> Void {
    }
}

public static func BillboardSync_OnSeed(hash: Uint64, seed: Uint64) -> Void {
    BillboardSync.OnSeed(hash, seed);
}

public static func BillboardSync_OnNextAd(hash: Uint64, ad: Uint32) -> Void {
    BillboardSync.OnNextAd(hash, ad);
}
