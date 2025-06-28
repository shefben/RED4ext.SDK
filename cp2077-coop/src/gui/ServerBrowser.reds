public class ServerBrowser {
    private static var selectedId: Uint32;
    private static let listCtrl: wref<inkListController>;
    private static let scrollCtrl: wref<inkScrollController>;
    private static let joinBtn: wref<inkButton>;

    // jsonList should come from tests/static/serverlist.json
    public static func Refresh(jsonList: String) -> Void {
        LogChannel(n"DEBUG", "ServerBrowser.Refresh");
        let servers = Json.Parse(jsonList) as array<ref<IScriptable>>;
        if IsDefined(listCtrl) {
            listCtrl.Clear();
        };
        selectedId = 0;
        if IsDefined(joinBtn) {
            joinBtn.SetEnabled(false);
        };
        for server in servers {
            let id: Uint32 = server["id"] as Int32;
            let name: String = server["name"] as String;
            let players: String = server["players"] as String;
            let row = new inkHorizontalPanel();
            row.SetName(IntToString(id));
            let nameLabel = new inkText();
            nameLabel.SetText(name);
            let playersLabel = new inkText();
            playersLabel.SetText(players);
            row.AddChild(nameLabel);
            row.AddChild(playersLabel);
            row.RegisterToCallback(n"OnRelease", this, n"OnRowClicked");
            listCtrl.AddChild(row);
        }
        scrollCtrl.SetTarget(listCtrl);
    }

    public static func OnRowClicked(widget: ref<inkWidget>) -> Void {
        selectedId = StringToUint(widget.GetName());
        if IsDefined(joinBtn) {
            joinBtn.SetEnabled(true);
        };
        listCtrl.SetSelected(widget);
        LogChannel(n"DEBUG", "Selected " + IntToString(selectedId));
    }

    public static func Join() -> Void {
        if selectedId == 0 {
            LogChannel(n"DEBUG", "Join aborted - no server selected");
            return;
        }
        LogChannel(n"DEBUG", "JoinRequest -> " + IntToString(selectedId));
        CoopNet.Net_SendJoinRequest(selectedId);
    }

    public static func Host() -> Void {
        // Pseudo-code: spawn the dedicated server then join localhost.
        // SystemCommand("coop_dedicated --port 7777");
        LogChannel(n"DEBUG", "Host -> coop_dedicated --port 7777");
        selectedId = 0;
        // NetCore.Connect("127.0.0.1");
        LogChannel(n"DEBUG", "JoinRequest -> 127.0.0.1");
    }

    public static func OnUpdate() -> Void {
        if IsDefined(scrollCtrl) {
            let input = GameInstance.GetInputSystem(GetGame());
            if input.IsActionJustPressed(n"IK_Up") {
                scrollCtrl.ScrollBy(-1.0);
            } else if input.IsActionJustPressed(n"IK_Down") {
                scrollCtrl.ScrollBy(1.0);
            } else if input.GetMouseWheel() != 0 {
                scrollCtrl.ScrollBy(input.GetMouseWheel());
            }
        }
    }
}
