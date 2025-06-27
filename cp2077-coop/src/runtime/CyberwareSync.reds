// Handles cyberware equip replication and slow-motion events.
public class CyberwareSync {
    public static let slowMoEnd: Uint64 = 0u;
    public static let slowMoFactor: Float = 1.0;

    public static func OnEquip(peer: Uint32, slot: Uint8, snap: ref<ItemSnap>) -> Void {
        Inventory.OnItemSnap(snap);
        if peer == Net_GetLocalPeerId() {
            // Reload meshes for local player
            AvatarProxy.ReloadCyberware(slot);
            CyberCooldownHud.SetCooldown(slot, 30.0);
        };
        QuickhackSync.RefreshSlots(peer);
    }

    public static func OnSlowMo(peer: Uint32, factor: Float, durMs: Uint16) -> Void {
        if peer != Net_GetLocalPeerId() {
            slowMoEnd = CoopNet.GameClock.GetCurrentTick() + Cast<Uint64>(durMs / CoopNet.GameClock.currentTickMs);
            slowMoFactor = factor;
        };
    }

    public static func Tick(dt: Float) -> Void {
        if slowMoEnd > 0u {
            if CoopNet.GameClock.GetCurrentTick() >= slowMoEnd {
                slowMoEnd = 0u;
                slowMoFactor = 1.0;
            };
        }
    }
}

public static func CyberwareSync_OnEquip(peer: Uint32, slot: Uint8, snap: ItemSnap) -> Void {
    let s = snap;
    CyberwareSync.OnEquip(peer, slot, &s);
}

public static func CyberwareSync_OnSlowMo(peer: Uint32, factor: Float, dur: Uint16) -> Void {
    CyberwareSync.OnSlowMo(peer, factor, dur);
}
