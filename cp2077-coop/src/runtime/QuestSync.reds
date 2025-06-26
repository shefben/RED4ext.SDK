// Quest progress must remain synced or players can become soft-locked.
// If one client advances a stage while another does not,
// subsequent objectives may never trigger for that player.
// These helpers send stage and scene updates between peers.
public class QuestSync {
    public static var freezeQuests: Bool = false;
    // Called after the game advances a quest stage on the server.
    // Sends a network update so all clients stay in sync.
    public static func OnAdvanceStage(questName: CName) -> Void {
        if freezeQuests { return; };
        LogChannel(n"DEBUG", "[QuestSync] " + NameToString(questName) + " stage advanced");
        SendQuestStageMsg(questName);
    }

    public static func SendQuestStageMsg(questName: CName) -> Void {
        // NetCore.BroadcastQuestStage(questName);
        LogChannel(n"DEBUG", "SendQuestStageMsg " + NameToString(questName));
    }

    public static func ApplyQuestStage(questName: CName) -> Void {
        // Client applies quest stage locally via quest system API.
        let qs = GameInstance.GetQuestsSystem(GetGame());
        if IsDefined(qs) {
            qs.AdvanceStage(questName);
        };
        LogChannel(n"DEBUG", "ApplyQuestStage " + NameToString(questName));
    }

    // Mirror cutscene and scene triggers between peers.
    public static func OnSceneStart(id: TweakDBID) -> Void {
        // NetCore.BroadcastSceneTrigger(id, true);
        LogChannel(n"DEBUG", "Send SceneTrigger start " + TDBID.ToStringDEBUG(id));
    }

    public static func OnSceneEnd(id: TweakDBID) -> Void {
        // NetCore.BroadcastSceneTrigger(id, false);
        LogChannel(n"DEBUG", "Send SceneTrigger end " + TDBID.ToStringDEBUG(id));
    }

    public static func ApplySceneTrigger(id: TweakDBID, isStart: Bool) -> Void {
        LogChannel(n"DEBUG", "Apply SceneTrigger " + TDBID.ToStringDEBUG(id) + " start=" + BoolToString(isStart));
    }

    public static func SetFreeze(f: Bool) -> Void {
        freezeQuests = f;
        LogChannel(n"DEBUG", "freezeQuests=" + BoolToString(f));
    }
}

// --- Hooks ---------------------------------------------------------------

// Detours QuestSystem::AdvanceStage so the server can broadcast quest updates.
// Using the `hook` attribute lets us call the original function then run our
// own logic after it executes.
@hook(QuestSystem.AdvanceStage)
protected func QuestSystem_AdvanceStage(original: func(ref<QuestSystem>, CName),
                                        self: ref<QuestSystem>, questName: CName) -> Void {
    // Server processes the quest update, then notify peers.
    original(self, questName);
    QuestSync.OnAdvanceStage(questName);
}
