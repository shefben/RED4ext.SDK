// cp2077-coop entry point.
// RED4ext invokes OnReload() when ReloadAllMods is executed.
public class CoopMain extends ScriptableSystem {
    public func OnAttach() -> Void {
        LogChannel(n"DEBUG", "cp2077-coop loaded");
        MainMenuInjection.Inject();
    }

    public func OnReload() -> Void {
        LogChannel(n"DEBUG", "cp2077-coop reloaded");
    }
}
