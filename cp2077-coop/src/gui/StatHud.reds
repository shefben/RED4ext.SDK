public struct NetStats {
    public var ping: Uint32;
    public var loss: Float;
    public var vKbps: Uint16;
    public var sKbps: Uint16;
    public var dropPkts: Uint16;
}

public class StatHud extends inkHUDLayer {
    public static let s_instance: ref<StatHud>;
    private let table: ref<inkCanvas>;
    private let peerIds: array<Uint32>;
    private let rows: array<ref<inkText>>;
    private var visible: Bool;

    public static func Instance() -> ref<StatHud> {
        if !IsDefined(s_instance) {
            s_instance = new StatHud();
            GameInstance.GetHUDManager(GetGame()).AddLayer(s_instance);
        };
        return s_instance;
    }

    public static func Toggle() -> Void {
        let inst = Instance();
        inst.visible = !inst.visible;
        if IsDefined(inst.table) { inst.table.SetVisible(inst.visible); };
    }

    public static func OnNetStats(peerId: Uint32, s: NetStats) -> Void {
        Instance().UpdateRow(peerId, s);
    }

    private func UpdateRow(id: Uint32, s: NetStats) -> Void {
        if !IsDefined(table) {
            table = new inkCanvas();
            table.SetAnchor(inkEAnchor.TopLeft);
            table.SetMargin(new inkMargin(20.0,20.0,0.0,0.0));
            AddChild(table);
        };
        var idx: Int32 = peerIds.Find(id);
        if idx == -1 {
            let row = new inkText();
            row.SetStyle(n"Medium 32px Bold");
            row.SetMargin(new inkMargin(0.0, 20.0 * Cast<Float>(peerIds.Size()),0.0,0.0));
            table.AddChild(row);
            peerIds.Push(id);
            rows.Push(row);
            idx = rows.Size() - 1;
        };
        let color: HDRColor;
        if s.dropPkts > 5u {
            color = new HDRColor(1.0,0.2,0.2,1.0);
        } else if s.ping <= 80u {
            color = new HDRColor(0.2,1.0,0.2,1.0);
        } else if s.ping <= 150u {
            color = new HDRColor(1.0,0.85,0.2,1.0);
        } else {
            color = new HDRColor(1.0,0.2,0.2,1.0);
        };
        rows[idx].SetTintColor(color);
        rows[idx].SetText(IntToString(id) + "  " + IntToString(Cast<Int32>(s.ping)) + "ms  " +
                          IntToString(Cast<Int32>(RoundF(s.loss*100.0))) + "%  " +
                          IntToString(Cast<Int32>(s.vKbps)) + "  " +
                          IntToString(Cast<Int32>(s.sKbps)) + "  " +
                          IntToString(Cast<Int32>(s.dropPkts)) + "%  " +
                          IntToString(Cast<Int32>(1000u / GameClock.currentTickMs)));
    }

    public func OnUpdate(dt: Float) -> Void {
        let input = GameInstance.GetInputSystem(GetGame());
        if input.IsActionJustPressed(EInputKey.IK_F1) {
            Toggle();
        };
    }
}
