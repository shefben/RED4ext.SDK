public class EmoteSync {
    public static func Play(peerId: Uint32, emoteId: Uint8) -> Void {
        let playerSys = GameInstance.GetPlayerSystem(GetGame());
        let avatar = playerSys.FindObject(peerId) as AvatarProxy;
        if IsDefined(avatar) {
            if HasMethod(avatar, n"PlayEmote") {
                avatar.PlayEmote(emoteId);
            };
            VoiceOverQueue.Play(1000u + Cast<Uint32>(emoteId));
        };
    }
}

public static func EmoteSync_Play(peerId: Uint32, emoteId: Uint8) -> Void {
    EmoteSync.Play(peerId, emoteId);
}
