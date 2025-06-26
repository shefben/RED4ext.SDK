public class MergePrompt extends inkGameController {
    private let listPanel: ref<inkVerticalPanel>;
    private let conflicts: array<String>;

    public func Show(conflictJson: String) -> Void {
        conflicts = [];
        listPanel = new inkVerticalPanel();
        let rows = Json.Parse(conflictJson) as array<ref<IScriptable>>;
        for r in rows {
            let text = r as String;
            conflicts += text;
            let row = new inkText();
            row.SetText(text);
            listPanel.AddChild(row);
        }
        AddChild(listPanel);
        LogChannel(n"DEBUG", "MergePrompt.Show count=" + IntToString(ArraySize(conflicts)));
    }

    public func AcceptAll() -> Void {
        PersistResolution(true);
    }

    public func SkipEach() -> Void {
        PersistResolution(false);
    }

    private native func SaveMergeResolution(acceptAll: Bool) -> Void

    private func PersistResolution(acceptAll: Bool) -> Void {
        SaveMergeResolution(acceptAll);
        LogChannel(n"DEBUG", "MergePrompt.Persist acceptAll=" + BoolToString(acceptAll));
    }
}
