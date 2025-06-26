// Audits menus that pause gameplay and redirects unsafe ones.
public class UIPauseAudit {
    public static let blockedMenus: array<CName> = [n"WorldMap", n"Journal", n"Shards"];
    private static let discovered: array<CName>;

    public static func Audit(layer: ref<inkMenuLayer>) -> Void {
        let name = layer.GetName();
        if layer.IsPausesGame() && !ArrayContains(blockedMenus, name) {
            discovered.PushBack(name);
            LogChannel(n"DEBUG", "[UIPauseAudit] found pausing menu " + NameToString(name));
        };
    }

    public static func OnHoloStart(peerId: Uint32) -> Void {
        LogChannel(n"DEBUG", "HoloCall start peer=" + IntToString(Cast<Int32>(peerId)));
        if peerId != (GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy).peerId {
            HoloBars.Show();
        };
    }

    public static func OnHoloEnd(peerId: Uint32) -> Void {
        LogChannel(n"DEBUG", "HoloCall end peer=" + IntToString(Cast<Int32>(peerId)));
        HoloBars.Hide();
    }
}

@wrapMethod(inkMenuLayer)
protected cb func OnOpen(prev: wref<inkMenuLayer>) -> Void {
    let menuName = this.GetName();
    if menuName == n"WorldMap" {
        CoopMap.Show();
        return; // skip original to avoid pause
    };
    wrappedMethod(prev);
    UIPauseAudit.Audit(this);
}

@wrapMethod(phonePhoneSystem)
protected func StartCall(arg: variant) -> Void {
    wrappedMethod(arg);
    let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy;
    Net_BroadcastHoloCallStart(player.peerId);
}

@wrapMethod(phonePhoneSystem)
protected func EndCall() -> Void {
    wrappedMethod();
    let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy;
    Net_BroadcastHoloCallEnd(player.peerId);
}

// Shard reading replaced with notice in co-op.
@wrapMethod(ShardUI)
protected func ShowShard(data: ref<IScriptable>) -> Void {
    if GameModeManager.current == GameMode.Coop {
        CoopNotice.Show("Shards");
        return;
    };
    wrappedMethod(data);
}

public static func UIPauseAudit_OnHoloStart(peerId: Uint32) -> Void {
    UIPauseAudit.OnHoloStart(peerId);
}

public static func UIPauseAudit_OnHoloEnd(peerId: Uint32) -> Void {
    UIPauseAudit.OnHoloEnd(peerId);
}
