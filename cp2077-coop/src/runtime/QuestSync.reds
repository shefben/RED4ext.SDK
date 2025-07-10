// Quest progress must remain synced or players can become soft-locked.
// If one client advances a stage while another does not,
// subsequent objectives may never trigger for that player.
// These helpers send stage and scene updates between peers.
public class QuestSync {
    public static var freezeQuests: Bool = false;
    public static var localPhase: Uint32 = 0u; // PX-2
    public static let stageMap: ref<inkHashMap> = new inkHashMap();
    public static let nameMap: ref<inkHashMap> = new inkHashMap();
    // Called after the game advances a quest stage on the server.
    // Sends a network update so all clients stay in sync.
    public static func OnAdvanceStage(questName: CName) -> Void {
        if freezeQuests { return; };
        LogChannel(n"DEBUG", "[QuestSync] " + NameToString(questName) + " stage advanced");
        SendQuestStageMsg(questName);
    }

    public static func SendQuestStageMsg(questName: CName) -> Void {
        let qs = GameInstance.GetQuestsSystem(GetGame());
        let stage: Uint16 = IsDefined(qs) ? Cast<Uint16>(qs.GetCurrentStage(questName)) : 0u;
        let hash: Uint32 = CoopNet.Fnv1a32(NameToString(questName));
        nameMap.Insert(hash, questName);
        stageMap.Insert(hash, stage);
        CoopNet.Net_BroadcastQuestStageP2P(localPhase, hash, stage);
        LogChannel(n"DEBUG", "SendQuestStageMsg " + NameToString(questName) + " stage=" + ToString(stage));
    }

    public static func ApplyQuestStage(questName: CName, stage: Uint16) -> Void {
        let qs = GameInstance.GetQuestsSystem(GetGame());
        if !IsDefined(qs) { return; };
        let current: Uint16 = Cast<Uint16>(qs.GetCurrentStage(questName));
        if current < stage {
            qs.SetStage(questName, stage);
        } else {
            if current > stage + 1u {
                LogChannel(n"ERROR", "[QuestSync] Divergence detected");
                CoopNet.Net_SendQuestResyncRequest();
            };
        };
        let hash: Uint32 = CoopNet.Fnv1a32(NameToString(questName));
        stageMap.Insert(hash, stage);
        nameMap.Insert(hash, questName);
        LogChannel(n"DEBUG", "ApplyQuestStage " + NameToString(questName) + " stage=" + ToString(stage));
    }

    public static func ApplyQuestStageByHash(hash: Uint32, stage: Uint16) -> Void {
        if !nameMap.Contains(hash) {
            LogChannel(n"WARNING", "[QuestSync] Unknown quest hash " + IntToString(Cast<Int32>(hash)));
            return;
        };
        let name: CName = nameMap.Get(hash) as CName;
        ApplyQuestStage(name, stage);
    }

    public static func ApplyFullSync(pkt: ref<QuestFullSyncPacket>) -> Void {
        LogChannel(n"quest", "[Watchdog] forced sync");
        let qCount: Uint16 = pkt.questCount;
        let i: Uint16 = 0u;
        while i < qCount {
            let entry = pkt.quests[i];
            nameMap.Insert(entry.nameHash, StringToName(IntToString(entry.nameHash)));
            ApplyQuestStageByHash(entry.nameHash, entry.stage);
            i += 1u;
        };

        let nCount: Uint16 = pkt.npcCount;
        let j: Uint16 = 0u;
        while j < nCount {
            let snap = pkt.npcs[j];
            NpcController.ClientApplySnap(&snap);
            j += 1u;
        };

        let eCount: Uint8 = pkt.eventCount;
        let k: Uint8 = 0u;
        while k < eCount {
            let ev = pkt.events[k];
            GlobalEventManager.OnEvent(ev.eventId, ev.phase, ev.seed, ev.active == 1u);
            k += 1u;
        };
    }

    // Mirror cutscene and scene triggers between peers.
    public static func OnSceneStart(id: TweakDBID) -> Void {
        let hash: Uint32 = CoopNet.Fnv1a32(TDBID.ToStringDEBUG(id));
        Net_BroadcastSceneTrigger(localPhase, hash, true);
        LogChannel(n"DEBUG", "Send SceneTrigger start " + TDBID.ToStringDEBUG(id));
    }

    public static func OnSceneEnd(id: TweakDBID) -> Void {
        let hash: Uint32 = CoopNet.Fnv1a32(TDBID.ToStringDEBUG(id));
        Net_BroadcastSceneTrigger(localPhase, hash, false);
        LogChannel(n"DEBUG", "Send SceneTrigger end " + TDBID.ToStringDEBUG(id));
    }

    public static func ApplySceneTrigger(id: TweakDBID, isStart: Bool) -> Void {
        LogChannel(n"DEBUG", "Apply SceneTrigger " + TDBID.ToStringDEBUG(id) + " start=" + BoolToString(isStart));
    }

    public static func OnDialogChoice(peerId: Uint32, idx: Uint8) -> Void {
        LogChannel(n"DEBUG", "DialogChoice " + IntToString(Cast<Int32>(idx)));
        if !Net_IsAuthoritative() {
            Net_SendDialogChoice(idx);
        } else {
            if CutsceneSync.ApplyDialogChoice(idx) {
                Net_BroadcastDialogChoice(peerId, idx);
            };
        };
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

@hook(questSceneManager.StartScene)
protected func questSceneManager_StartScene(original: func(ref<questSceneManager>, TweakDBID), self: ref<questSceneManager>, id: TweakDBID) -> Void {
    original(self, id);
    QuestSync.OnSceneStart(id);
}

@hook(questSceneManager.EndScene)
protected func questSceneManager_EndScene(original: func(ref<questSceneManager>, TweakDBID), self: ref<questSceneManager>, id: TweakDBID) -> Void {
    original(self, id);
    QuestSync.OnSceneEnd(id);
}

@hook(DialogChoiceHubController.OnOptionSelected)
protected func DialogChoiceHubController_OnOptionSelected(original: func(ref<DialogChoiceHubController>, Int32), self: ref<DialogChoiceHubController>, idx: Int32) -> Void {
    original(self, idx);
    QuestSync.OnDialogChoice(CoopNet.Net_GetPeerId(), Cast<Uint8>(idx));
}
