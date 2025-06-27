public class QuestTracker {
    public static func ShowObjective(phaseId: Uint32, text: String) -> Void {
        if phaseId != QuestSync.localPhase { return; };
        CoopNotice.Show(text);
    }
}

public static func QuestTracker_ShowObjective(phaseId: Uint32, text: String) -> Void {
    QuestTracker.ShowObjective(phaseId, text);
}
