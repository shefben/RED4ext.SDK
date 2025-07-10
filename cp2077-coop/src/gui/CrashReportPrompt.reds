public class CrashReportPrompt extends inkHUDLayer {
    private static let s_instance: ref<CrashReportPrompt>;
    private let label: ref<inkText>;
    private let sendBtn: ref<inkButton>;
    private let dismissBtn: ref<inkButton>;
    private let path: String;

    public static func Show(zipPath: String) -> Void {
        if IsDefined(s_instance) { return; };
        let hud = GameInstance.GetHUDManager(GetGame());
        let o = new CrashReportPrompt();
        o.path = zipPath;
        o.label = new inkText();
        o.label.SetText("Crash report ready. Send?");
        o.sendBtn = new inkButton();
        o.sendBtn.SetText("SEND");
        o.sendBtn.RegisterToCallback(n"OnRelease", o, n"OnSend");
        o.dismissBtn = new inkButton();
        o.dismissBtn.SetText("DISMISS");
        o.dismissBtn.RegisterToCallback(n"OnRelease", o, n"OnDismiss");
        let panel = new inkVerticalPanel();
        panel.AddChild(o.label);
        panel.AddChild(o.sendBtn);
        panel.AddChild(o.dismissBtn);
        o.AddChild(panel);
        hud.AddLayer(o);
        s_instance = o;
        LogChannel(n"DEBUG", "CrashReportPrompt shown");
    }

    protected cb func OnSend(e: ref<inkPointerEvent>) -> Bool {
        UploadReport(path);
        Hide();
        return true;
    }

    protected cb func OnDismiss(e: ref<inkPointerEvent>) -> Bool {
        Hide();
        return true;
    }

    private native func UploadReport(path: String) -> Void

    private func Hide() -> Void {
        let hud = GameInstance.GetHUDManager(GetGame());
        hud.RemoveLayer(this);
        s_instance = null;
    }
}
