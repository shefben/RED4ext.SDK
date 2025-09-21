#pragma once

#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <chrono>
#include <queue>

namespace CoopNet {

// UI panel types using existing game UI systems
enum class MultiplayerUIPanel : uint8_t {
    PlayerList = 0,         // Shows connected players
    QuestSync = 1,          // Quest coordination interface
    VoiceChat = 2,          // Voice communication controls
    SessionInfo = 3,        // Current session information
    NetworkStats = 4,       // Connection and performance stats
    InventorySync = 5,      // Shared inventory interface
    MapMarkers = 6,         // Multiplayer map markers
    ChatMessages = 7,       // Text chat system
    GameModeSelect = 8,     // Game mode selection
    ServerBrowser = 9       // Session browser
};

// UI notification types using game's notification system
enum class UINotificationType : uint8_t {
    PlayerJoined = 0,
    PlayerLeft = 1,
    QuestUpdate = 2,
    InventorySync = 3,
    VoiceActivity = 4,
    NetworkIssue = 5,
    GameModeChange = 6,
    SystemMessage = 7
};

// UI widget positioning using game's layout system
enum class UIAnchor : uint8_t {
    TopLeft = 0,
    TopRight = 1,
    BottomLeft = 2,
    BottomRight = 3,
    Center = 4,
    TopCenter = 5,
    BottomCenter = 6
};

// Forward declarations
struct UIWidgetInfo;
struct PlayerUIData;
struct UINotification;
struct UIThemeData;

// Player information for UI display
struct PlayerUIData {
    uint32_t playerId;
    std::string playerName;
    std::string lifepathIcon;      // Uses game's lifepath icons
    uint32_t level;
    uint32_t streetCred;
    float health;
    float stamina;
    bool isInCombat;
    bool isInVehicle;
    bool isSpeaking;               // Voice chat indicator
    std::string currentLocation;
    std::string statusIcon;        // Uses game's status icons
    uint64_t lastUpdate;
};

// UI widget configuration using game assets
struct UIWidgetInfo {
    MultiplayerUIPanel panelType;
    std::string widgetName;
    std::string inkWidgetPath;     // Path to game's ink widget
    UIAnchor anchor;
    float posX, posY;
    float width, height;
    bool isVisible;
    bool useGameTheme;             // Use current game UI theme
    std::string backgroundAsset;   // Game UI background asset
    uint32_t zOrder;
};

// Notification data using game's notification system
struct UINotification {
    uint64_t notificationId;
    UINotificationType type;
    std::string title;
    std::string message;
    std::string iconPath;          // Game icon asset path
    float duration;                // Display duration
    bool playSound;               // Use game notification sound
    std::string soundEvent;       // Game audio event
    uint64_t timestamp;
};

// UI theme data leveraging game's theming
struct UIThemeData {
    std::string themeName;         // Game UI theme (corpo, nomad, street)
    std::string colorScheme;       // Uses game's color schemes
    std::string fontFamily;        // Game font assets
    float opacity;
    bool useAnimations;           // Game UI animations
    std::string backgroundStyle;  // Game UI background styles
};

// Main multiplayer UI management system
class MultiplayerUIManager {
public:
    static MultiplayerUIManager& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Tick(float deltaTime);

    // UI panel management using game systems
    bool ShowPanel(MultiplayerUIPanel panel, bool animate = true);
    bool HidePanel(MultiplayerUIPanel panel, bool animate = true);
    bool TogglePanel(MultiplayerUIPanel panel);
    bool IsPanelVisible(MultiplayerUIPanel panel) const;

    // Player list interface
    void UpdatePlayerList(const std::vector<PlayerUIData>& players);
    void AddPlayer(const PlayerUIData& player);
    void RemovePlayer(uint32_t playerId);
    void UpdatePlayerStatus(uint32_t playerId, const PlayerUIData& newData);

    // Notification system using game notifications
    uint64_t ShowNotification(UINotificationType type, const std::string& title,
                             const std::string& message, float duration = 5.0f);
    bool HideNotification(uint64_t notificationId);
    void ClearAllNotifications();

    // Chat system integration
    bool ShowChatMessage(uint32_t playerId, const std::string& message);
    bool SendChatMessage(const std::string& message);
    void SetChatVisible(bool visible);

    // Map integration using game's map system
    bool AddMapMarker(uint32_t playerId, float x, float y, float z,
                     const std::string& markerType = "player");
    bool RemoveMapMarker(uint32_t playerId);
    void UpdatePlayerMapPosition(uint32_t playerId, float x, float y, float z);

    // Quest UI integration
    void ShowQuestSyncPanel(uint32_t questHash, const std::string& questName);
    void UpdateQuestProgress(uint32_t questHash, float progress);
    void ShowQuestVoting(uint32_t questHash, const std::string& description);

    // Network status UI
    void UpdateNetworkStats(float ping, float packetLoss, uint32_t bandwidth);
    void ShowConnectionIssue(const std::string& issue, bool critical = false);
    void HideConnectionIssue();

    // Voice chat UI
    void UpdateVoiceActivity(uint32_t playerId, bool isSpeaking, float volume);
    void ShowVoiceControls(bool show);
    void SetPushToTalkKey(const std::string& key);

    // Inventory sync UI
    void ShowInventorySyncProgress(float progress);
    void ShowItemTransferRequest(uint32_t fromPlayer, uint32_t toPlayer,
                               const std::string& itemName);

    // Game mode UI
    void ShowGameModeSelector(const std::vector<std::string>& availableModes);
    void SetCurrentGameMode(const std::string& modeName);

    // Session browser UI
    void ShowSessionBrowser(const std::vector<std::string>& sessions);
    void UpdateSessionInfo(const std::string& sessionName, uint32_t playerCount,
                          const std::string& gameMode);

    // UI theming using game themes
    void SetUITheme(const std::string& themeName);
    void ApplyPlayerLifepathTheme(uint32_t playerId, const std::string& lifepath);
    UIThemeData GetCurrentTheme() const;

    // Hotkey integration using game's input system
    bool RegisterHotkey(const std::string& action, const std::string& key,
                       std::function<void()> callback);
    bool UnregisterHotkey(const std::string& action);
    void SetHotkeysEnabled(bool enabled);

    // Accessibility features using game's accessibility system
    void SetTextScale(float scale);
    void SetHighContrast(bool enabled);
    void SetColorBlindMode(const std::string& mode);

    // Integration with game's pause system
    void OnGamePaused(bool paused);
    void OnMenuOpened(const std::string& menuType);
    void OnMenuClosed(const std::string& menuType);

    // Event callbacks
    using UIEventCallback = std::function<void(const std::string& event, const std::string& data)>;
    void RegisterUICallback(const std::string& eventType, UIEventCallback callback);
    void UnregisterUICallback(const std::string& eventType);

private:
    MultiplayerUIManager() = default;
    ~MultiplayerUIManager() = default;
    MultiplayerUIManager(const MultiplayerUIManager&) = delete;
    MultiplayerUIManager& operator=(const MultiplayerUIManager&) = delete;

    // Core UI management
    bool InitializeGameUIIntegration();
    bool CreateUIWidgets();
    void UpdateUIWidgets();
    void CleanupUIWidgets();

    // Game UI system integration
    bool AttachToGameUI();
    bool CreateInkWidget(const UIWidgetInfo& widgetInfo);
    bool UpdateInkWidget(MultiplayerUIPanel panel, const std::string& data);
    void DestroyInkWidget(MultiplayerUIPanel panel);

    // Notification system integration
    bool ShowGameNotification(const UINotification& notification);
    void ProcessNotificationQueue();
    void ExpireOldNotifications();

    // Chat system implementation
    bool InitializeChatSystem();
    void UpdateChatDisplay();
    void AddChatMessage(const std::string& playerName, const std::string& message);

    // Map system integration
    bool IntegrateWithGameMap();
    void UpdateMapMarkers();
    std::string GetPlayerMarkerIcon(const PlayerUIData& player) const;

    // UI layout and positioning
    void UpdateUILayout();
    void HandleScreenResolutionChange();
    void AdjustUIForAspectRatio();

    // Theme management
    void LoadGameThemes();
    void ApplyThemeToWidget(MultiplayerUIPanel panel, const UIThemeData& theme);
    std::string GetLifepathIconPath(const std::string& lifepath) const;

    // Input handling
    void ProcessUIInput();
    void HandleHotkeyPress(const std::string& action);

    // Performance optimization
    void OptimizeUIPerformance();
    void UpdateVisibleWidgetsOnly();
    void BatchUIUpdates();

    // Data storage
    std::unordered_map<MultiplayerUIPanel, UIWidgetInfo> m_widgets;
    std::unordered_map<uint32_t, PlayerUIData> m_playerData;
    std::vector<UINotification> m_notifications;
    std::vector<std::string> m_chatMessages;
    std::unordered_map<uint32_t, std::string> m_mapMarkers;

    // Game UI integration
    void* m_gameUISystem = nullptr;          // Game's UI system
    void* m_inkSystemsContainer = nullptr;   // Ink widget container
    void* m_notificationManager = nullptr;   // Game notification manager
    void* m_mapSystem = nullptr;             // Game map system
    void* m_inputManager = nullptr;          // Game input manager

    // Widget instances
    std::unordered_map<MultiplayerUIPanel, void*> m_inkWidgets;

    // State management
    bool m_initialized = false;
    bool m_gameUIAttached = false;
    UIThemeData m_currentTheme;
    bool m_hotkeysEnabled = true;
    bool m_gamePaused = false;

    // Synchronization
    mutable std::mutex m_uiMutex;
    mutable std::mutex m_notificationMutex;
    mutable std::mutex m_chatMutex;

    // Configuration
    struct UIConfig {
        float notificationDuration = 5.0f;
        uint32_t maxChatMessages = 100;
        uint32_t maxNotifications = 10;
        bool enableAnimations = true;
        bool enableSounds = true;
        float uiScale = 1.0f;
        std::string defaultTheme = "street";
        bool autoHideInCombat = true;
        bool showNetworkStats = true;
        float voiceActivityThreshold = 0.1f;
    } m_config;

    // Event system
    std::unordered_map<std::string, std::vector<UIEventCallback>> m_callbacks;
    std::mutex m_callbackMutex;

    // Update timers
    float m_updateTimer = 0.0f;
    float m_notificationTimer = 0.0f;
    float m_chatTimer = 0.0f;
    float m_mapTimer = 0.0f;

    // Notification management
    uint64_t m_nextNotificationId = 1;
    std::queue<UINotification> m_notificationQueue;

    // Hotkey system
    std::unordered_map<std::string, std::function<void()>> m_hotkeys;
    std::unordered_map<std::string, std::string> m_hotkeyBindings;

    // UI performance tracking
    struct UIPerformanceStats {
        uint32_t widgetUpdates = 0;
        uint32_t notificationsShown = 0;
        uint32_t chatMessagesDisplayed = 0;
        float averageFrameTime = 0.0f;
        uint64_t totalUIUpdates = 0;
    } m_performanceStats;
};

// Utility functions for UI integration
namespace UIUtils {
    std::string GetGameIconPath(const std::string& iconName);
    std::string GetLifepathIcon(const std::string& lifepath);
    std::string GetStatusIcon(const std::string& status);
    std::string GetGameFontPath(const std::string& fontName);
    std::string GetUIThemeColor(const std::string& themeName, const std::string& element);
    bool IsGameUIVisible();
    float GetUIScale();
    std::string FormatPlayerName(const std::string& name, uint32_t level);
    std::string FormatNetworkStats(float ping, float packetLoss);
}

} // namespace CoopNet