public class ServerBrowser extends inkHUDLayer {
    private static let s_instance: ref<ServerBrowser>;
    private static var selectedId: Uint32;
    private static let listCtrl: wref<inkVerticalPanel>;
    private static let scrollCtrl: wref<inkScrollController>;
    private static let joinBtn: wref<inkButton>;
    private static let hostBtn: wref<inkButton>;

    public struct ServerInfo {
        var id: Uint32;
        var name: String;
        var ip: String;
        var cur: Uint16;
        var max: Uint16;
        var password: Bool;
        var mode: String;
    }

    public static func Show() -> Void {
        if IsDefined(s_instance) { return; };
        let layer = new ServerBrowser();
        s_instance = layer;
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.AddLayer(layer);
        let root = new inkVerticalPanel();
        layer.AddChild(root);
        listCtrl = new inkVerticalPanel();
        root.AddChild(listCtrl);
        scrollCtrl = new inkScrollController();
        scrollCtrl.direction = inkEScrollDirection.Vertical;
        scrollCtrl.useGlobalInput = true;
        joinBtn = new inkButton();
        joinBtn.SetText("CONNECT");
        joinBtn.RegisterToCallback(n"OnRelease", layer, n"OnJoinClick");
        joinBtn.SetEnabled(false);
        root.AddChild(joinBtn);

        hostBtn = new inkButton();
        hostBtn.SetText("HOST");
        hostBtn.RegisterToCallback(n"OnRelease", layer, n"OnHostClick");
        root.AddChild(hostBtn);
        RefreshLive();
    }

    protected cb func OnJoinClick(e: ref<inkPointerEvent>) -> Bool {
        Join();
        return true;
    }

    protected cb func OnHostClick(e: ref<inkPointerEvent>) -> Bool {
        Host();
        return true;
    }

    public static func Hide() -> Void {
        if !IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.RemoveLayer(s_instance);
        s_instance = null;
    }

    private static func QueryMaster() -> array<ref<IScriptable>> {
        let req = new HttpRequest();
        req.SetUrl("https://coop-master/api/list");
        req.Send();
        if req.GetStatusCode() == 0u {
            LogChannel(n"DEBUG", "Master server unreachable");
            return [];
        };
        if req.GetStatusCode() != 200u {
            LogChannel(n"DEBUG", "Master query failed");
            return [];
        };
        let body = req.GetBody();
        return Json.Parse(body) as array<ref<IScriptable>>;
    }

    private static func QueryServer(entry: ref<IScriptable>) -> ref<IScriptable> {
        let ip: String = entry["ip"] as String;
        let port: Uint32 = entry["port"] as Int32;
        let req = new HttpRequest();
        req.SetUrl("http://" + ip + ":" + IntToString(port) + "/info");
        req.Send();
        if req.GetStatusCode() == 0u {
            LogChannel(n"DEBUG", "Server " + ip + " unreachable");
            return null;
        };
        if req.GetStatusCode() != 200u {
            LogChannel(n"DEBUG", "Info query failed " + IntToString(req.GetStatusCode()));
            return null;
        };
        let body = req.GetBody();
        let info = Json.Parse(body) as ref<IScriptable>;
        if IsDefined(info) {
            info["id"] = entry["id"];
            info["ip"] = ip;
            info["port"] = port;
        };
        return info;
    }

    public static func RefreshLive() -> Void {
        let entries = QueryMaster();
        let servers: array<ref<IScriptable>>;
        for e in entries {
            let data = QueryServer(e);
            if IsDefined(data) {
                ArrayPush(servers, data);
            };
        };
        let json = Json.Stringify(servers);
        Refresh(json);
    }

    // Populate list from JSON payload with array<ServerInfo>
    public static func Refresh(jsonList: String) -> Void {
        LogChannel(n"DEBUG", "ServerBrowser.Refresh");
        let servers = Json.Parse(jsonList) as array<ref<IScriptable>>;
        if IsDefined(listCtrl) {
            while listCtrl.GetNumChildren() > 0 {
                listCtrl.RemoveChild(listCtrl.GetChild(0));
            };
        };
        selectedId = 0;
        if IsDefined(joinBtn) {
            joinBtn.SetEnabled(false);
        };
        for server in servers {
            let id: Uint32 = server["id"] as Int32;
            let name: String = server["name"] as String;
            let cur: Uint32 = server["cur"] as Int32;
            let max: Uint32 = server["max"] as Int32;
            let mode: String = server["mode"] as String;
            let pass: Bool = server["password"] as Bool;
            let ip: String = server["ip"] as String;
            let port: Uint32 = server["port"] as Int32;
            let row = new inkHorizontalPanel();
            row.SetName(IntToString(id));
            let nameLabel = new inkText();
            nameLabel.SetText(name);
            let playersLabel = new inkText();
            playersLabel.SetText(IntToString(cur) + "/" + IntToString(max));
            let modeLabel = new inkText();
            modeLabel.SetText(mode);
            let passLabel = new inkText();
            if pass {
                passLabel.SetText("🔒");
            } else {
                passLabel.SetText(" ");
            };
            let addrLabel = new inkText();
            addrLabel.SetText(ip + ":" + IntToString(port));
            row.AddChild(nameLabel);
            row.AddChild(playersLabel);
            row.AddChild(modeLabel);
            row.AddChild(passLabel);
            row.AddChild(addrLabel);
            // Refresh is static so use s_instance for callbacks
            row.RegisterToCallback(n"OnRelease", s_instance, n"OnRowClicked");
            listCtrl.AddChild(row);
        }
        scrollCtrl.SetTarget(listCtrl);
    }

    protected cb func OnRowClicked(widget: ref<inkWidget>) -> Bool {
        selectedId = StringToUint(NameToString(widget.GetName()));
        if IsDefined(joinBtn) {
            joinBtn.SetEnabled(true);
        };
        LogChannel(n"DEBUG", "Selected " + IntToString(selectedId));
        return true;
    }

    public static func Join() -> Void {
        if selectedId == 0 {
            LogChannel(n"DEBUG", "Join aborted - no server selected");
            return;
        }
        LogChannel(n"DEBUG", "JoinRequest -> " + IntToString(selectedId));
        CoopNet.Net_SendJoinRequest(selectedId);
    }

    public static func HostServer() -> Void {
        // Launch dedicated instance and wait for local heartbeat.
        GameProcess.Launch("coop_dedicated", "--port 7777");
        let ts = GameInstance.GetTimeSystem(GetGame());
        let start = EngineTime.ToFloat(ts.GetGameTime());
        while !Net_IsConnected() && EngineTime.ToFloat(ts.GetGameTime()) - start < 5.0 {
            Net_Poll(50u);
        };
        if Net_IsConnected() {
            CoopNet.Net_SendJoinRequest(0u);
        };
    }

    public static exec func Host() -> Void {
        HostServer();
    }

    protected cb func OnDetach() -> Void {
        if IsDefined(joinBtn) {
            // Use the static instance for symmetry with registration
            joinBtn.UnregisterFromCallback(n"OnRelease", s_instance, n"OnJoinClick");
        };
        if IsDefined(hostBtn) {
            hostBtn.UnregisterFromCallback(n"OnRelease", s_instance, n"OnHostClick");
        };
        if IsDefined(listCtrl) {
            let idx: Int32 = 0;
            while idx < listCtrl.GetNumChildren() {
                let row = listCtrl.GetChild(idx);
                // FIX: prevent leaking row callbacks
                row.UnregisterFromCallback(n"OnRelease", s_instance, n"OnRowClicked");
                idx += 1;
            };
        };
    }

    public func OnUpdate(dt: Float) -> Void {
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
