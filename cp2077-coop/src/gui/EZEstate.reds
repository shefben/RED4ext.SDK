// Apartment permission UI helper.
public class EZEstatePerms {
    public static func AddPeer(apt: Uint32, peer: Uint32) -> Void {
        Net_SendAptPermChange(apt, peer, true);
    }

    public static func RemovePeer(apt: Uint32, peer: Uint32) -> Void {
        Net_SendAptPermChange(apt, peer, false);
    }

    public static func SetPublic(apt: Uint32, allow: Bool) -> Void {
        Net_SendAptPermChange(apt, 0u, allow);
    }
}
