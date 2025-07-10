public class PartyManager {
    public static let members: array<Uint32>;

    public static func Invite(peer: Uint32) -> Void {
        CoopNet.Net_SendPartyInvite(peer);
    }

    public static func Leave() -> Void {
        CoopNet.Net_SendPartyLeave();
    }

    public static func Kick(peer: Uint32) -> Void {
        CoopNet.Net_SendPartyKick(peer);
    }

    public static func OnPartyInfo(ids: script_ref<array<Uint32>>, count: Uint8) -> Void {
        members.Clear();
        var i: Int32 = 0;
        while i < count {
            members.PushBack(ids[i]);
            i += 1;
        };
    }

    public static func OnInvite(from: Uint32, to: Uint32) -> Void {
        if to == Net_GetLocalPeerId() {
            CoopNotice.Show("Invited by " + IntToString(Cast<Int32>(from)));
        };
    }

    public static func OnLeave(peer: Uint32) -> Void {
        CoopNotice.Show("Peer " + IntToString(Cast<Int32>(peer)) + " left party");
    }

    public static func OnKick(peer: Uint32) -> Void {
        if peer == Net_GetLocalPeerId() {
            CoopNotice.Show("Kicked from party");
        } else {
            CoopNotice.Show("Kick " + IntToString(Cast<Int32>(peer)));
        };
    }
}

public static func PartyManager_OnPartyInfo(ids: script_ref<array<Uint32>>, count: Uint8) -> Void {
    PartyManager.OnPartyInfo(ids, count);
}
public static func PartyManager_OnInvite(f: Uint32, t: Uint32) -> Void {
    PartyManager.OnInvite(f, t);
}
public static func PartyManager_OnLeave(p: Uint32) -> Void {
    PartyManager.OnLeave(p);
}
public static func PartyManager_OnKick(p: Uint32) -> Void {
    PartyManager.OnKick(p);
}
