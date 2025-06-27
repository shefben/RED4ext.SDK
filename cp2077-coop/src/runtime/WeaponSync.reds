public class WeaponSync {
    public static func OnInspect(peer: Uint32, anim: Uint16) -> Void {
        LogChannel(n"weapon", "Inspect peer=" + IntToString(Cast<Int32>(peer)));
        let avatar = GameInstance.GetPlayerSystem(GetGame()).FindObject(peer) as AvatarProxy;
        if IsDefined(avatar) {
            if HasMethod(avatar, n"PlayInspect") {
                avatar.PlayInspect(anim);
            };
        };
    }

    public static func OnFinisherStart(actor: Uint32, victim: Uint32, ft: Uint8) -> Void {
        LogChannel(n"weapon", "Finisher start actor=" + IntToString(Cast<Int32>(actor)));
    }

    public static func OnFinisherEnd(actor: Uint32) -> Void {
        LogChannel(n"weapon", "Finisher end actor=" + IntToString(Cast<Int32>(actor)));
    }
}

public static func WeaponSync_OnInspect(peer: Uint32, anim: Uint16) -> Void {
    WeaponSync.OnInspect(peer, anim);
}

public static func WeaponSync_OnFinisherStart(actor: Uint32, victim: Uint32, ft: Uint8) -> Void {
    WeaponSync.OnFinisherStart(actor, victim, ft);
}

public static func WeaponSync_OnFinisherEnd(actor: Uint32) -> Void {
    WeaponSync.OnFinisherEnd(actor);
}
