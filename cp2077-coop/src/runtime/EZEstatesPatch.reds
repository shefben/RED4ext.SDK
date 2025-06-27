public static exec func CoopPurchaseApt(id: Int32) -> Void {
    Apartments.Purchase(Cast<Uint32>(id));
}

public class EZEstatesOverlay extends inkHUDLayer {
    private let view: ref<inkWebView>;
    protected cb func OnCreate() -> Bool {
        view = new inkWebView();
        view.SetUrl("file://ezestates.html");
        AddChild(view);
        return true;
    }
}

public static exec func ShowEzEstates() -> Void {
    let hud = GameInstance.GetHUDManager(GetGame());
    hud.AddLayer(new EZEstatesOverlay());
}
