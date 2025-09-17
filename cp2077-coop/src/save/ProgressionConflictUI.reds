// UI for handling progression conflicts and save compatibility

import Codeware.UI

public enum ConflictResolutionChoice {
    UseHost = 0,
    UseClient = 1, 
    Merge = 2,
    Cancel = 3
}

public class ProgressionConflictUI extends inkGameController {
    private var m_rootContainer: wref<inkWidget>;
    private var m_backgroundPanel: wref<inkRectangle>;
    private var m_titleText: wref<inkText>;
    private var m_summaryText: wref<inkText>;
    private var m_conflictsList: wref<inkVerticalPanel>;
    private var m_resolutionButtons: wref<inkHorizontalPanel>;
    private var m_detailsPanel: wref<inkVerticalPanel>;
    
    private var m_compatibilityInfo: SaveCompatibilityInfo;
    private var m_hostSnapshot: ProgressionSnapshot;
    private var m_clientSnapshot: ProgressionSnapshot;
    private var m_isVisible: Bool;
    private var m_selectedResolution: ConflictResolutionChoice;
    
    public func Initialize() -> Void {
        this.CreateUI();
        this.m_isVisible = false;
        this.m_selectedResolution = ConflictResolutionChoice.UseHost;
        
        LogChannel(n"COOP_SAVE", "Progression conflict UI initialized");
    }
    
    private func CreateUI() -> Void {
        let rootCompound = this.GetRootCompoundWidget();
        
        // Main container
        this.m_rootContainer = new inkCanvas();
        this.m_rootContainer.SetName(n"ProgressionConflictRoot");
        this.m_rootContainer.SetSize(new Vector2(1920.0, 1080.0));
        this.m_rootContainer.SetAnchor(inkEAnchor.Fill);
        this.m_rootContainer.SetVisible(false);
        rootCompound.AddChild(this.m_rootContainer);
        
        // Background overlay
        this.m_backgroundPanel = new inkRectangle();
        this.m_backgroundPanel.SetName(n"ConflictBackground");
        this.m_backgroundPanel.SetSize(new Vector2(1000.0, 700.0));
        this.m_backgroundPanel.SetAnchor(inkEAnchor.CenterCenter);
        this.m_backgroundPanel.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 0.9));
        this.m_backgroundPanel.SetOpacity(0.95);
        this.m_rootContainer.AddChild(this.m_backgroundPanel);
        
        // Content container
        let contentPanel = new inkVerticalPanel();
        contentPanel.SetName(n"ConflictContent");
        contentPanel.SetSize(new Vector2(980.0, 680.0));
        contentPanel.SetAnchor(inkEAnchor.CenterCenter);
        contentPanel.SetMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        this.m_rootContainer.AddChild(contentPanel);
        
        // Title
        this.m_titleText = new inkText();
        this.m_titleText.SetName(n"ConflictTitle");
        this.m_titleText.SetText("⚠️ SAVE GAME COMPATIBILITY ISSUES");
        this.m_titleText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        this.m_titleText.SetFontSize(28);
        this.m_titleText.SetTintColor(new HDRColor(1.0, 0.647, 0.0, 1.0)); // Orange
        this.m_titleText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        this.m_titleText.SetMargin(new inkMargin(0.0, 0.0, 0.0, 20.0));
        contentPanel.AddChild(this.m_titleText);
        
        // Summary text
        this.m_summaryText = new inkText();
        this.m_summaryText.SetName(n"ConflictSummary");
        this.m_summaryText.SetText("Your save file differs from the host's save. Choose how to resolve these conflicts:");
        this.m_summaryText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        this.m_summaryText.SetFontSize(16);
        this.m_summaryText.SetTintColor(new HDRColor(0.9, 0.9, 0.9, 1.0));
        this.m_summaryText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        this.m_summaryText.SetWrapping(true);
        this.m_summaryText.SetMargin(new inkMargin(20.0, 0.0, 20.0, 25.0));
        contentPanel.AddChild(this.m_summaryText);
        
        // Conflicts list
        this.m_conflictsList = new inkVerticalPanel();
        this.m_conflictsList.SetName(n"ConflictsList");
        this.m_conflictsList.SetSize(new Vector2(960.0, 300.0));
        this.m_conflictsList.SetMargin(new inkMargin(10.0, 0.0, 10.0, 20.0));
        contentPanel.AddChild(this.m_conflictsList);
        
        // Details panel
        this.m_detailsPanel = new inkVerticalPanel();
        this.m_detailsPanel.SetName(n"DetailsPanel");
        this.m_detailsPanel.SetSize(new Vector2(960.0, 150.0));
        this.m_detailsPanel.SetMargin(new inkMargin(10.0, 0.0, 10.0, 20.0));
        contentPanel.AddChild(this.m_detailsPanel);
        
        // Resolution buttons
        this.m_resolutionButtons = new inkHorizontalPanel();
        this.m_resolutionButtons.SetName(n"ResolutionButtons");
        this.m_resolutionButtons.SetSize(new Vector2(960.0, 60.0));
        this.m_resolutionButtons.SetMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        this.m_resolutionButtons.SetChildAlignment(inkEChildAlignment.Center);
        contentPanel.AddChild(this.m_resolutionButtons);
        
        this.CreateResolutionButtons();
    }
    
    private func CreateResolutionButtons() -> Void {
        // Use Host button
        let useHostBtn = new inkButton();
        useHostBtn.SetName(n"UseHostBtn");
        useHostBtn.SetText("🏠 USE HOST SAVE");
        useHostBtn.SetStyle(n"BaseButtonLarge");
        useHostBtn.SetSize(new Vector2(220.0, 50.0));
        useHostBtn.SetMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        useHostBtn.RegisterToCallback(n"OnRelease", this, n"OnUseHostSelected");
        this.m_resolutionButtons.AddChild(useHostBtn);
        
        // Use Client button
        let useClientBtn = new inkButton();
        useClientBtn.SetName(n"UseClientBtn");
        useClientBtn.SetText("👤 USE MY SAVE");
        useClientBtn.SetStyle(n"BaseButtonLarge");
        useClientBtn.SetSize(new Vector2(220.0, 50.0));
        useClientBtn.SetMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        useClientBtn.RegisterToCallback(n"OnRelease", this, n"OnUseClientSelected");
        this.m_resolutionButtons.AddChild(useClientBtn);
        
        // Merge button
        let mergeBtn = new inkButton();
        mergeBtn.SetName(n"MergeBtn");
        mergeBtn.SetText("🔀 SMART MERGE");
        mergeBtn.SetStyle(n"BaseButtonLarge");
        mergeBtn.SetSize(new Vector2(220.0, 50.0));
        mergeBtn.SetMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        mergeBtn.RegisterToCallback(n"OnRelease", this, n"OnMergeSelected");
        this.m_resolutionButtons.AddChild(mergeBtn);
        
        // Cancel button
        let cancelBtn = new inkButton();
        cancelBtn.SetName(n"CancelBtn");
        cancelBtn.SetText("❌ CANCEL");
        cancelBtn.SetStyle(n"BaseButtonLarge");
        cancelBtn.SetSize(new Vector2(180.0, 50.0));
        cancelBtn.SetMargin(new inkMargin(10.0, 5.0, 10.0, 5.0));
        cancelBtn.SetTintColor(new HDRColor(1.0, 0.3, 0.3, 1.0)); // Red
        cancelBtn.RegisterToCallback(n"OnRelease", this, n"OnCancelSelected");
        this.m_resolutionButtons.AddChild(cancelBtn);
    }
    
    public func ShowConflicts(compatibilityInfo: SaveCompatibilityInfo, hostSnapshot: ProgressionSnapshot, clientSnapshot: ProgressionSnapshot) -> Void {
        this.m_compatibilityInfo = compatibilityInfo;
        this.m_hostSnapshot = hostSnapshot;
        this.m_clientSnapshot = clientSnapshot;
        this.m_isVisible = true;
        
        this.PopulateConflictsList();
        this.PopulateDetailsPanel();
        this.UpdateSummaryText();
        this.HighlightRecommendedOption();
        
        this.m_rootContainer.SetVisible(true);
        
        LogChannel(n"COOP_SAVE", "Showing progression conflicts UI with " + ToString(ArraySize(compatibilityInfo.conflicts)) + " conflicts");
    }
    
    public func Hide() -> Void {
        this.m_isVisible = false;
        this.m_rootContainer.SetVisible(false);
        this.ClearConflictsList();
        
        LogChannel(n"COOP_SAVE", "Progression conflicts UI hidden");
    }
    
    private func PopulateConflictsList() -> Void {
        this.ClearConflictsList();
        
        for conflict in this.m_compatibilityInfo.conflicts {
            this.CreateConflictRow(conflict);
        }
    }
    
    private func CreateConflictRow(conflict: ProgressionConflict) -> Void {
        let conflictRow = new inkHorizontalPanel();
        conflictRow.SetName(StringToName("Conflict_" + ToString(Cast<Int32>(conflict.conflictType))));
        conflictRow.SetSize(new Vector2(940.0, 50.0));
        conflictRow.SetMargin(new inkMargin(10.0, 2.0, 10.0, 2.0));
        
        // Severity indicator
        let severityIcon = new inkText();
        severityIcon.SetName(n"SeverityIcon");
        severityIcon.SetSize(new Vector2(40.0, 50.0));
        severityIcon.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        severityIcon.SetFontSize(20);
        severityIcon.SetVerticalAlignment(textVerticalAlignment.Center);
        
        switch conflict.severity {
            case 1:
                severityIcon.SetText("ℹ️");
                severityIcon.SetTintColor(new HDRColor(0.0, 0.8, 1.0, 1.0)); // Blue
                break;
            case 2:
                severityIcon.SetText("⚠️");
                severityIcon.SetTintColor(new HDRColor(1.0, 0.8, 0.0, 1.0)); // Yellow
                break;
            case 3:
                severityIcon.SetText("🚨");
                severityIcon.SetTintColor(new HDRColor(1.0, 0.3, 0.0, 1.0)); // Red
                break;
        }
        conflictRow.AddChild(severityIcon);
        
        // Conflict description
        let descText = new inkText();
        descText.SetName(n"ConflictDesc");
        descText.SetText(conflict.description);
        descText.SetSize(new Vector2(400.0, 50.0));
        descText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        descText.SetFontSize(14);
        descText.SetTintColor(new HDRColor(0.9, 0.9, 0.9, 1.0));
        descText.SetVerticalAlignment(textVerticalAlignment.Center);
        descText.SetWrapping(true);
        descText.SetOverflowPolicy(textOverflowPolicy.DotsEnd);
        conflictRow.AddChild(descText);
        
        // Host value
        let hostValueText = new inkText();
        hostValueText.SetName(n"HostValue");
        hostValueText.SetText("Host: " + conflict.hostValue);
        hostValueText.SetSize(new Vector2(200.0, 50.0));
        hostValueText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        hostValueText.SetFontSize(12);
        hostValueText.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0)); // Cyan
        hostValueText.SetVerticalAlignment(textVerticalAlignment.Center);
        hostValueText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        conflictRow.AddChild(hostValueText);
        
        // Client value
        let clientValueText = new inkText();
        clientValueText.SetName(n"ClientValue");
        clientValueText.SetText("You: " + conflict.clientValue);
        clientValueText.SetSize(new Vector2(200.0, 50.0));
        clientValueText.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        clientValueText.SetFontSize(12);
        clientValueText.SetTintColor(new HDRColor(0.0, 1.0, 0.5, 1.0)); // Green
        clientValueText.SetVerticalAlignment(textVerticalAlignment.Center);
        clientValueText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        conflictRow.AddChild(clientValueText);
        
        // Background for alternating rows
        if (ArraySize(this.m_conflictsList.GetAllChildren()) % 2 == 0) {
            let rowBg = new inkRectangle();
            rowBg.SetName(n"RowBackground");
            rowBg.SetSize(new Vector2(940.0, 50.0));
            rowBg.SetTintColor(new HDRColor(0.1, 0.1, 0.1, 0.3));
            conflictRow.AddChildAtIndex(rowBg, 0);
        }
        
        this.m_conflictsList.AddChild(conflictRow);
    }
    
    private func PopulateDetailsPanel() -> Void {
        this.m_detailsPanel.RemoveAllChildren();
        
        // Comparison details
        let detailsTitle = new inkText();
        detailsTitle.SetName(n"DetailsTitle");
        detailsTitle.SetText("📊 SAVE GAME COMPARISON");
        detailsTitle.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        detailsTitle.SetFontSize(18);
        detailsTitle.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        detailsTitle.SetMargin(new inkMargin(0.0, 0.0, 0.0, 10.0));
        this.m_detailsPanel.AddChild(detailsTitle);
        
        // Host details
        let hostDetails = new inkText();
        hostDetails.SetName(n"HostDetails");
        hostDetails.SetText(this.FormatSnapshotSummary("HOST", this.m_hostSnapshot));
        hostDetails.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        hostDetails.SetFontSize(14);
        hostDetails.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
        hostDetails.SetMargin(new inkMargin(20.0, 5.0, 20.0, 5.0));
        this.m_detailsPanel.AddChild(hostDetails);
        
        // Client details
        let clientDetails = new inkText();
        clientDetails.SetName(n"ClientDetails");
        clientDetails.SetText(this.FormatSnapshotSummary("YOUR SAVE", this.m_clientSnapshot));
        clientDetails.SetFontFamily(n"base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
        clientDetails.SetFontSize(14);
        clientDetails.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
        clientDetails.SetMargin(new inkMargin(20.0, 5.0, 20.0, 5.0));
        this.m_detailsPanel.AddChild(clientDetails);
    }
    
    private func FormatSnapshotSummary(label: String, snapshot: ProgressionSnapshot) -> String {
        let summary = label + ": Level " + ToString(snapshot.characterLevel);
        summary += ", " + ToString(snapshot.experience) + " XP";
        summary += ", Street Cred " + ToString(snapshot.streetCred);
        summary += ", " + ToString(ArraySize(snapshot.questProgress)) + " quests";
        summary += ", Last played: " + snapshot.lastSaveTimestamp;
        return summary;
    }
    
    private func UpdateSummaryText() -> Void {
        let conflictCount = ArraySize(this.m_compatibilityInfo.conflicts);
        let summaryText = "Found " + ToString(conflictCount) + " difference";
        
        if conflictCount != 1 {
            summaryText += "s";
        }
        
        summaryText += " between save files. ";
        
        if this.m_compatibilityInfo.isCompatible {
            summaryText += "These can be resolved automatically.";
        } else {
            summaryText += "Manual resolution required.";
        }
        
        this.m_summaryText.SetText(summaryText);
    }
    
    private func HighlightRecommendedOption() -> Void {
        // Highlight the recommended resolution button
        let recommendedChoice: ConflictResolutionChoice;
        
        switch this.m_compatibilityInfo.recommendedMode {
            case ProgressionSyncMode.HostProgression:
                recommendedChoice = ConflictResolutionChoice.UseHost;
                break;
            case ProgressionSyncMode.ClientProgression:
                recommendedChoice = ConflictResolutionChoice.UseClient;
                break;
            case ProgressionSyncMode.MergedProgression:
                recommendedChoice = ConflictResolutionChoice.Merge;
                break;
            default:
                recommendedChoice = ConflictResolutionChoice.UseHost;
                break;
        }
        
        this.HighlightButton(recommendedChoice);
    }
    
    private func HighlightButton(choice: ConflictResolutionChoice) -> Void {
        // Reset all button colors
        this.ResetButtonColors();
        
        let buttonName: CName;
        switch choice {
            case ConflictResolutionChoice.UseHost:
                buttonName = n"UseHostBtn";
                break;
            case ConflictResolutionChoice.UseClient:
                buttonName = n"UseClientBtn";
                break;
            case ConflictResolutionChoice.Merge:
                buttonName = n"MergeBtn";
                break;
            default:
                return;
        }
        
        let button = this.m_resolutionButtons.GetWidget(buttonName) as inkButton;
        if IsDefined(button) {
            button.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0)); // Cyan highlight
        }
    }
    
    private func ResetButtonColors() -> Void {
        let buttons: array<CName> = [n"UseHostBtn", n"UseClientBtn", n"MergeBtn"];
        for buttonName in buttons {
            let button = this.m_resolutionButtons.GetWidget(buttonName) as inkButton;
            if IsDefined(button) {
                button.SetTintColor(new HDRColor(1.0, 1.0, 1.0, 1.0)); // White
            }
        }
    }
    
    private func ClearConflictsList() -> Void {
        this.m_conflictsList.RemoveAllChildren();
    }
    
    // Button event handlers
    protected cb func OnUseHostSelected(e: ref<inkPointerEvent>) -> Void {
        this.m_selectedResolution = ConflictResolutionChoice.UseHost;
        this.HighlightButton(ConflictResolutionChoice.UseHost);
        this.ProcessResolution();
    }
    
    protected cb func OnUseClientSelected(e: ref<inkPointerEvent>) -> Void {
        this.m_selectedResolution = ConflictResolutionChoice.UseClient;
        this.HighlightButton(ConflictResolutionChoice.UseClient);
        this.ProcessResolution();
    }
    
    protected cb func OnMergeSelected(e: ref<inkPointerEvent>) -> Void {
        this.m_selectedResolution = ConflictResolutionChoice.Merge;
        this.HighlightButton(ConflictResolutionChoice.Merge);
        this.ProcessResolution();
    }
    
    protected cb func OnCancelSelected(e: ref<inkPointerEvent>) -> Void {
        this.m_selectedResolution = ConflictResolutionChoice.Cancel;
        this.ProcessResolution();
    }
    
    private func ProcessResolution() -> Void {
        // Play button sound
        let audioSystem = GameInstance.GetAudioSystem(GetGame());
        if IsDefined(audioSystem) {
            let audioEvent = new AudioEvent();
            audioEvent.eventName = n"ui_menu_onpress";
            audioSystem.Play(audioEvent);
        }
        
        LogChannel(n"COOP_SAVE", "User selected resolution: " + ToString(Cast<Int32>(this.m_selectedResolution)));
        
        if this.m_selectedResolution == ConflictResolutionChoice.Cancel {
            // Cancel connection
            NetworkingSystem.Disconnect();
            this.Hide();
            return;
        }
        
        // Convert choice to sync mode
        let syncMode: ProgressionSyncMode;
        switch this.m_selectedResolution {
            case ConflictResolutionChoice.UseHost:
                syncMode = ProgressionSyncMode.HostProgression;
                break;
            case ConflictResolutionChoice.UseClient:
                syncMode = ProgressionSyncMode.ClientProgression;
                break;
            case ConflictResolutionChoice.Merge:
                syncMode = ProgressionSyncMode.MergedProgression;
                break;
            default:
                syncMode = ProgressionSyncMode.HostProgression;
                break;
        }
        
        // Apply the resolution
        let success = ProgressionSync.SynchronizeProgression(this.m_hostSnapshot, this.m_clientSnapshot, syncMode);
        
        if success {
            this.ShowResolutionSuccess();
            DelaySystem.DelayCallback(this.Hide, 2.0);
        } else {
            this.ShowResolutionError();
        }
    }
    
    private func ShowResolutionSuccess() -> Void {
        this.m_titleText.SetText("✅ PROGRESSION SYNCHRONIZED");
        this.m_titleText.SetTintColor(new HDRColor(0.0, 1.0, 0.0, 1.0)); // Green
        this.m_summaryText.SetText("Save files have been successfully synchronized. Starting multiplayer session...");
        
        // Hide conflicts and buttons
        this.m_conflictsList.SetVisible(false);
        this.m_resolutionButtons.SetVisible(false);
        this.m_detailsPanel.SetVisible(false);
    }
    
    private func ShowResolutionError() -> Void {
        this.m_titleText.SetText("❌ SYNCHRONIZATION FAILED");
        this.m_titleText.SetTintColor(new HDRColor(1.0, 0.0, 0.0, 1.0)); // Red
        this.m_summaryText.SetText("Failed to synchronize save files. Connection will be terminated.");
        
        // Hide conflicts and buttons
        this.m_conflictsList.SetVisible(false);
        this.m_resolutionButtons.SetVisible(false);
        this.m_detailsPanel.SetVisible(false);
        
        DelaySystem.DelayCallback(NetworkingSystem.Disconnect, 3.0);
        DelaySystem.DelayCallback(this.Hide, 3.0);
    }
    
    public func IsVisible() -> Bool {
        return this.m_isVisible;
    }
    
    public func GetSelectedResolution() -> ConflictResolutionChoice {
        return this.m_selectedResolution;
    }
}

// Global function to present compatibility dialog
public static func PresentCompatibilityDialog(compatibilityInfo: SaveCompatibilityInfo) -> Void {
    LogChannel(n"COOP_SAVE", "Presenting progression compatibility dialog");
    
    // Create and show the conflict UI
    let conflictUI = new ProgressionConflictUI();
    conflictUI.Initialize();
    
    // This would need proper integration with the UI system
    // For now, just log the conflicts
    for conflict in compatibilityInfo.conflicts {
        LogChannel(n"COOP_SAVE", "Conflict: " + conflict.description + " (Host: " + conflict.hostValue + ", Client: " + conflict.clientValue + ")");
    }
}