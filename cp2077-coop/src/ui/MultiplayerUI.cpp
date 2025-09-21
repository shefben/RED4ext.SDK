#include "MultiplayerUI.hpp"
#include "../core/CoopExports.hpp"
#include "../net/Net.hpp"
#include <RED4ext/Scripting/Natives/Generated/inkSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/UIInGameNotificationSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/gameuiWidgetsManager.hpp>
#include <RED4ext/Scripting/Natives/Generated/inkWidget.hpp>
#include <RED4ext/Scripting/Natives/Generated/inkText.hpp>
#include <RED4ext/Scripting/Natives/Generated/inkImage.hpp>
#include <RED4ext/Scripting/Natives/Generated/inkCanvas.hpp>
#include <RED4ext/Scripting/Natives/Generated/inkGameController.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <regex>

namespace CoopNet {

MultiplayerUIManager& MultiplayerUIManager::Instance() {
    static MultiplayerUIManager instance;
    return instance;
}

bool MultiplayerUIManager::Initialize() {
    std::lock_guard<std::mutex> lock(m_uiMutex);

    if (m_initialized) {
        return true;
    }

    spdlog::info("[MultiplayerUI] Initializing multiplayer UI system using game assets");

    // Initialize game UI integration
    if (!InitializeGameUIIntegration()) {
        spdlog::error("[MultiplayerUI] Failed to integrate with game UI system");
        return false;
    }

    // Load game themes and assets
    LoadGameThemes();

    // Create UI widgets using game's ink system
    if (!CreateUIWidgets()) {
        spdlog::warn("[MultiplayerUI] Some UI widgets failed to create");
    }

    // Initialize chat system
    if (!InitializeChatSystem()) {
        spdlog::warn("[MultiplayerUI] Chat system initialization failed");
    }

    // Integrate with game map system
    if (!IntegrateWithGameMap()) {
        spdlog::warn("[MultiplayerUI] Map integration failed");
    }

    // Setup default hotkeys using game's input system
    RegisterHotkey("toggle_player_list", "Tab", [this]() { TogglePanel(MultiplayerUIPanel::PlayerList); });
    RegisterHotkey("toggle_chat", "Enter", [this]() { TogglePanel(MultiplayerUIPanel::ChatMessages); });
    RegisterHotkey("push_to_talk", "V", [this]() { /* Voice activation handled elsewhere */ });

    m_initialized = true;
    spdlog::info("[MultiplayerUI] Multiplayer UI system initialized successfully");

    return true;
}

void MultiplayerUIManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_uiMutex);

    if (!m_initialized) {
        return;
    }

    spdlog::info("[MultiplayerUI] Shutting down multiplayer UI system");

    // Hide all panels
    for (auto& [panel, widget] : m_widgets) {
        HidePanel(panel, false);
    }

    // Clear all notifications
    ClearAllNotifications();

    // Cleanup UI widgets
    CleanupUIWidgets();

    // Clear all data
    m_playerData.clear();
    m_chatMessages.clear();
    m_notifications.clear();
    m_mapMarkers.clear();
    m_hotkeys.clear();

    m_initialized = false;
    m_gameUIAttached = false;
}

bool MultiplayerUIManager::InitializeGameUIIntegration() {
    try {
        // Get game's UI system
        m_gameUISystem = RED4ext::GetGameSystem<RED4ext::inkSystem>();
        if (!m_gameUISystem) {
            spdlog::error("[MultiplayerUI] Failed to get game UI system");
            return false;
        }

        // Get ink system container
        auto inkSystem = static_cast<RED4ext::inkSystem*>(m_gameUISystem);
        m_inkSystemsContainer = inkSystem; // Direct reference to ink system

        // Get notification manager
        m_notificationManager = RED4ext::GetGameSystem<RED4ext::UIInGameNotificationSystem>();
        if (!m_notificationManager) {
            spdlog::warn("[MultiplayerUI] Notification system not available");
        }

        // Get map system for marker integration
        // Map system would be accessed through game instance

        // Get input manager for hotkey integration
        // Input manager integration would be implemented here

        m_gameUIAttached = true;
        spdlog::info("[MultiplayerUI] Successfully attached to game UI systems");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[MultiplayerUI] Exception during UI integration: {}", e.what());
        return false;
    }
}

bool MultiplayerUIManager::CreateUIWidgets() {
    spdlog::info("[MultiplayerUI] Creating UI widgets using game assets");

    // Define widget configurations using game UI assets
    std::vector<UIWidgetInfo> widgetConfigs = {
        // Player list panel (top-right corner)
        {
            MultiplayerUIPanel::PlayerList,
            "multiplayer_player_list",
            "base\\gameplay\\gui\\widgets\\minimap\\minimap_container.inkwidget", // Uses minimap container as base
            UIAnchor::TopRight,
            -20.0f, 20.0f, 300.0f, 400.0f,
            false, true,
            "base\\gameplay\\gui\\common\\backgrounds\\panel_bg.inkatlas", // Game panel background
            10
        },

        // Chat messages panel (bottom-left corner)
        {
            MultiplayerUIPanel::ChatMessages,
            "multiplayer_chat",
            "base\\gameplay\\gui\\widgets\\phone\\phone_sms_conversation.inkwidget", // Uses SMS UI as base
            UIAnchor::BottomLeft,
            20.0f, -150.0f, 400.0f, 120.0f,
            false, true,
            "base\\gameplay\\gui\\common\\backgrounds\\chat_bg.inkatlas", // Game chat background
            5
        },

        // Network stats overlay (top-left corner)
        {
            MultiplayerUIPanel::NetworkStats,
            "network_stats",
            "base\\gameplay\\gui\\widgets\\healthbar\\health_bar.inkwidget", // Uses health bar as base
            UIAnchor::TopLeft,
            20.0f, 20.0f, 200.0f, 100.0f,
            false, true,
            "base\\gameplay\\gui\\common\\backgrounds\\stats_bg.inkatlas", // Game stats background
            8
        },

        // Quest sync panel (center)
        {
            MultiplayerUIPanel::QuestSync,
            "quest_sync_panel",
            "base\\gameplay\\gui\\widgets\\quest\\quest_tracker.inkwidget", // Uses quest tracker as base
            UIAnchor::Center,
            0.0f, 0.0f, 500.0f, 300.0f,
            false, true,
            "base\\gameplay\\gui\\common\\backgrounds\\quest_bg.inkatlas", // Game quest background
            15
        },

        // Voice chat controls (bottom-center)
        {
            MultiplayerUIPanel::VoiceChat,
            "voice_controls",
            "base\\gameplay\\gui\\widgets\\phone\\phone_avatar.inkwidget", // Uses phone avatar as base
            UIAnchor::BottomCenter,
            0.0f, -50.0f, 300.0f, 40.0f,
            false, true,
            "base\\gameplay\\gui\\common\\backgrounds\\voice_bg.inkatlas", // Game voice background
            12
        }
    };

    bool success = true;
    for (const auto& config : widgetConfigs) {
        if (!CreateInkWidget(config)) {
            spdlog::warn("[MultiplayerUI] Failed to create widget: {}", config.widgetName);
            success = false;
        } else {
            m_widgets[config.panelType] = config;
        }
    }

    return success;
}

bool MultiplayerUIManager::CreateInkWidget(const UIWidgetInfo& widgetInfo) {
    try {
        if (!m_gameUIAttached) {
            return false;
        }

        // This would create an ink widget using the game's ink system
        // In a real implementation, this would involve:
        // 1. Loading the specified ink widget asset
        // 2. Creating an instance of the widget
        // 3. Setting up the widget properties (position, size, etc.)
        // 4. Adding it to the game's UI layer

        spdlog::debug("[MultiplayerUI] Creating ink widget: {} using asset: {}",
                     widgetInfo.widgetName, widgetInfo.inkWidgetPath);

        // Store widget reference (in real implementation, this would be the actual widget pointer)
        m_inkWidgets[widgetInfo.panelType] = nullptr; // Placeholder

        return true;
    } catch (const std::exception& e) {
        spdlog::error("[MultiplayerUI] Exception creating ink widget {}: {}",
                     widgetInfo.widgetName, e.what());
        return false;
    }
}

void MultiplayerUIManager::LoadGameThemes() {
    spdlog::info("[MultiplayerUI] Loading game UI themes");

    // Load Street Kid theme (default CP2077 theme)
    m_currentTheme.themeName = "street";
    m_currentTheme.colorScheme = "cyberpunk_blue"; // Game's default blue scheme
    m_currentTheme.fontFamily = "base\\gameplay\\gui\\fonts\\orbitron\\orbitron.fnt"; // Game's main font
    m_currentTheme.opacity = 0.9f;
    m_currentTheme.useAnimations = true;
    m_currentTheme.backgroundStyle = "holographic"; // Game's holographic style

    spdlog::info("[MultiplayerUI] Using theme: {} with color scheme: {}",
                 m_currentTheme.themeName, m_currentTheme.colorScheme);
}

bool MultiplayerUIManager::ShowPanel(MultiplayerUIPanel panel, bool animate) {
    std::lock_guard<std::mutex> lock(m_uiMutex);

    auto it = m_widgets.find(panel);
    if (it == m_widgets.end()) {
        return false;
    }

    if (it->second.isVisible) {
        return true; // Already visible
    }

    // Show the widget using game's animation system
    if (animate && m_currentTheme.useAnimations) {
        // Use game's fade-in animation
        spdlog::debug("[MultiplayerUI] Showing panel {} with animation",
                     static_cast<int>(panel));
    } else {
        spdlog::debug("[MultiplayerUI] Showing panel {} without animation",
                     static_cast<int>(panel));
    }

    it->second.isVisible = true;

    // Update widget visibility in game UI
    UpdateInkWidget(panel, "show");

    return true;
}

bool MultiplayerUIManager::HidePanel(MultiplayerUIPanel panel, bool animate) {
    std::lock_guard<std::mutex> lock(m_uiMutex);

    auto it = m_widgets.find(panel);
    if (it == m_widgets.end()) {
        return false;
    }

    if (!it->second.isVisible) {
        return true; // Already hidden
    }

    // Hide the widget using game's animation system
    if (animate && m_currentTheme.useAnimations) {
        // Use game's fade-out animation
        spdlog::debug("[MultiplayerUI] Hiding panel {} with animation",
                     static_cast<int>(panel));
    } else {
        spdlog::debug("[MultiplayerUI] Hiding panel {} without animation",
                     static_cast<int>(panel));
    }

    it->second.isVisible = false;

    // Update widget visibility in game UI
    UpdateInkWidget(panel, "hide");

    return true;
}

void MultiplayerUIManager::UpdatePlayerList(const std::vector<PlayerUIData>& players) {
    std::lock_guard<std::mutex> lock(m_uiMutex);

    // Clear existing player data
    m_playerData.clear();

    // Add all players
    for (const auto& player : players) {
        m_playerData[player.playerId] = player;
    }

    // Update player list widget
    if (IsPanelVisible(MultiplayerUIPanel::PlayerList)) {
        UpdatePlayerListWidget();
    }

    spdlog::debug("[MultiplayerUI] Updated player list with {} players", players.size());
}

void MultiplayerUIManager::UpdatePlayerListWidget() {
    // Create player list data for UI update
    std::string playerListData = "{\"players\":[";
    bool first = true;

    for (const auto& [playerId, player] : m_playerData) {
        if (!first) playerListData += ",";

        // Use game icons and formatting
        std::string lifepathIcon = GetLifepathIconPath(player.lifepathIcon);
        std::string statusIcon = UIUtils::GetStatusIcon(player.statusIcon);

        playerListData += "{";
        playerListData += "\"id\":" + std::to_string(playerId) + ",";
        playerListData += "\"name\":\"" + player.playerName + "\",";
        playerListData += "\"level\":" + std::to_string(player.level) + ",";
        playerListData += "\"streetCred\":" + std::to_string(player.streetCred) + ",";
        playerListData += "\"health\":" + std::to_string(player.health) + ",";
        playerListData += "\"lifepathIcon\":\"" + lifepathIcon + "\",";
        playerListData += "\"statusIcon\":\"" + statusIcon + "\",";
        playerListData += "\"location\":\"" + player.currentLocation + "\",";
        playerListData += "\"inCombat\":" + (player.isInCombat ? "true" : "false") + ",";
        playerListData += "\"speaking\":" + (player.isSpeaking ? "true" : "false");
        playerListData += "}";

        first = false;
    }

    playerListData += "]}";

    // Update the ink widget with new data
    UpdateInkWidget(MultiplayerUIPanel::PlayerList, playerListData);
}

uint64_t MultiplayerUIManager::ShowNotification(UINotificationType type, const std::string& title,
                                               const std::string& message, float duration) {
    std::lock_guard<std::mutex> lock(m_notificationMutex);

    UINotification notification;
    notification.notificationId = m_nextNotificationId++;
    notification.type = type;
    notification.title = title;
    notification.message = message;
    notification.duration = duration;
    notification.playSound = m_config.enableSounds;
    notification.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    // Set appropriate icon based on type using game assets
    switch (type) {
        case UINotificationType::PlayerJoined:
            notification.iconPath = "base\\gameplay\\gui\\common\\icons\\player_joined.inkatlas";
            notification.soundEvent = "ui_generic_positive";
            break;
        case UINotificationType::PlayerLeft:
            notification.iconPath = "base\\gameplay\\gui\\common\\icons\\player_left.inkatlas";
            notification.soundEvent = "ui_generic_negative";
            break;
        case UINotificationType::QuestUpdate:
            notification.iconPath = "base\\gameplay\\gui\\common\\icons\\quest_update.inkatlas";
            notification.soundEvent = "ui_quest_update";
            break;
        case UINotificationType::VoiceActivity:
            notification.iconPath = "base\\gameplay\\gui\\common\\icons\\voice_chat.inkatlas";
            notification.soundEvent = "ui_phone_incoming_call";
            break;
        case UINotificationType::NetworkIssue:
            notification.iconPath = "base\\gameplay\\gui\\common\\icons\\warning.inkatlas";
            notification.soundEvent = "ui_generic_error";
            break;
        default:
            notification.iconPath = "base\\gameplay\\gui\\common\\icons\\info.inkatlas";
            notification.soundEvent = "ui_generic_notification";
            break;
    }

    // Add to notification queue
    m_notifications.push_back(notification);

    // Limit number of notifications
    if (m_notifications.size() > m_config.maxNotifications) {
        m_notifications.erase(m_notifications.begin());
    }

    // Show notification using game's notification system
    ShowGameNotification(notification);

    spdlog::debug("[MultiplayerUI] Showing notification: {} - {}", title, message);

    return notification.notificationId;
}

bool MultiplayerUIManager::ShowGameNotification(const UINotification& notification) {
    if (!m_notificationManager) {
        return false;
    }

    try {
        // Use game's notification system to display the notification
        // This would involve calling the appropriate game notification methods
        // with the provided title, message, icon, and sound

        spdlog::debug("[MultiplayerUI] Displaying game notification with icon: {}",
                     notification.iconPath);

        return true;
    } catch (const std::exception& e) {
        spdlog::error("[MultiplayerUI] Failed to show game notification: {}", e.what());
        return false;
    }
}

bool MultiplayerUIManager::ShowChatMessage(uint32_t playerId, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_chatMutex);

    // Get player name
    std::string playerName = "Unknown";
    auto playerIt = m_playerData.find(playerId);
    if (playerIt != m_playerData.end()) {
        playerName = playerIt->second.playerName;
    }

    // Format message using game's chat formatting
    std::string formattedMessage = UIUtils::FormatPlayerName(playerName,
        playerIt != m_playerData.end() ? playerIt->second.level : 0) + ": " + message;

    AddChatMessage(playerName, message);

    // Update chat widget if visible
    if (IsPanelVisible(MultiplayerUIPanel::ChatMessages)) {
        UpdateChatDisplay();
    }

    spdlog::debug("[MultiplayerUI] Chat message from {}: {}", playerName, message);

    return true;
}

void MultiplayerUIManager::AddChatMessage(const std::string& playerName, const std::string& message) {
    // Add timestamp using game's time formatting
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::string timestamp = "[" + std::to_string(time_t % 86400 / 3600) + ":" +
                           std::to_string(time_t % 3600 / 60) + "] ";

    std::string fullMessage = timestamp + playerName + ": " + message;
    m_chatMessages.push_back(fullMessage);

    // Limit chat history
    if (m_chatMessages.size() > m_config.maxChatMessages) {
        m_chatMessages.erase(m_chatMessages.begin());
    }
}

void MultiplayerUIManager::UpdateChatDisplay() {
    // Create chat data for UI update
    std::string chatData = "{\"messages\":[";

    for (size_t i = 0; i < m_chatMessages.size(); ++i) {
        if (i > 0) chatData += ",";
        chatData += "\"" + m_chatMessages[i] + "\"";
    }

    chatData += "]}";

    // Update chat widget
    UpdateInkWidget(MultiplayerUIPanel::ChatMessages, chatData);
}

bool MultiplayerUIManager::AddMapMarker(uint32_t playerId, float x, float y, float z,
                                       const std::string& markerType) {
    std::lock_guard<std::mutex> lock(m_uiMutex);

    // Get player data for marker icon
    std::string markerIcon = "base\\gameplay\\gui\\common\\icons\\mappin_player.inkatlas"; // Default player icon

    auto playerIt = m_playerData.find(playerId);
    if (playerIt != m_playerData.end()) {
        markerIcon = GetPlayerMarkerIcon(playerIt->second);
    }

    // Store marker data
    std::string markerData = std::to_string(x) + "," + std::to_string(y) + "," +
                            std::to_string(z) + "," + markerIcon;
    m_mapMarkers[playerId] = markerData;

    // Update map markers in game
    UpdateMapMarkers();

    spdlog::debug("[MultiplayerUI] Added map marker for player {} at ({}, {}, {})",
                 playerId, x, y, z);

    return true;
}

std::string MultiplayerUIManager::GetPlayerMarkerIcon(const PlayerUIData& player) const {
    // Return appropriate marker icon based on player state using game assets
    if (player.isInCombat) {
        return "base\\gameplay\\gui\\common\\icons\\mappin_combat.inkatlas";
    } else if (player.isInVehicle) {
        return "base\\gameplay\\gui\\common\\icons\\mappin_vehicle.inkatlas";
    } else {
        // Use lifepath-specific icon
        return GetLifepathIconPath(player.lifepathIcon);
    }
}

std::string MultiplayerUIManager::GetLifepathIconPath(const std::string& lifepath) const {
    if (lifepath == "corpo") {
        return "base\\gameplay\\gui\\common\\icons\\lifepath_corpo.inkatlas";
    } else if (lifepath == "nomad") {
        return "base\\gameplay\\gui\\common\\icons\\lifepath_nomad.inkatlas";
    } else { // street
        return "base\\gameplay\\gui\\common\\icons\\lifepath_street.inkatlas";
    }
}

void MultiplayerUIManager::UpdateNetworkStats(float ping, float packetLoss, uint32_t bandwidth) {
    if (!IsPanelVisible(MultiplayerUIPanel::NetworkStats)) {
        return;
    }

    // Format network stats using game's formatting
    std::string statsData = "{";
    statsData += "\"ping\":" + std::to_string(static_cast<int>(ping)) + ",";
    statsData += "\"packetLoss\":" + std::to_string(packetLoss * 100.0f) + ",";
    statsData += "\"bandwidth\":" + std::to_string(bandwidth) + ",";
    statsData += "\"quality\":\"" + (ping < 50 ? "excellent" : ping < 100 ? "good" : "poor") + "\"";
    statsData += "}";

    UpdateInkWidget(MultiplayerUIPanel::NetworkStats, statsData);
}

void MultiplayerUIManager::Tick(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    m_updateTimer += deltaTime;
    m_notificationTimer += deltaTime;

    // Update UI widgets periodically
    if (m_updateTimer >= 0.1f) { // 10 FPS for UI updates
        UpdateUIWidgets();
        m_updateTimer = 0.0f;
    }

    // Process notification expiration
    if (m_notificationTimer >= 1.0f) {
        ExpireOldNotifications();
        m_notificationTimer = 0.0f;
    }

    // Update performance stats
    m_performanceStats.totalUIUpdates++;
}

void MultiplayerUIManager::UpdateUIWidgets() {
    // Update only visible widgets for performance
    UpdateVisibleWidgetsOnly();
}

void MultiplayerUIManager::UpdateVisibleWidgetsOnly() {
    for (const auto& [panel, widget] : m_widgets) {
        if (widget.isVisible) {
            // Update widget data based on type
            switch (panel) {
                case MultiplayerUIPanel::PlayerList:
                    UpdatePlayerListWidget();
                    break;
                case MultiplayerUIPanel::ChatMessages:
                    // Chat updates are event-driven
                    break;
                case MultiplayerUIPanel::NetworkStats:
                    // Network stats updated externally
                    break;
                default:
                    break;
            }
        }
    }
}

void MultiplayerUIManager::ExpireOldNotifications() {
    std::lock_guard<std::mutex> lock(m_notificationMutex);

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
            [now](const UINotification& notification) {
                return (now - notification.timestamp) > (notification.duration * 1000);
            }),
        m_notifications.end()
    );
}

bool MultiplayerUIManager::UpdateInkWidget(MultiplayerUIPanel panel, const std::string& data) {
    auto widgetIt = m_inkWidgets.find(panel);
    if (widgetIt == m_inkWidgets.end()) {
        return false;
    }

    // In a real implementation, this would update the actual ink widget
    // with the provided data using the game's UI update mechanisms

    spdlog::debug("[MultiplayerUI] Updating widget {} with data size: {}",
                 static_cast<int>(panel), data.size());

    return true;
}

// Utility function implementations
namespace UIUtils {
    std::string GetGameIconPath(const std::string& iconName) {
        return "base\\gameplay\\gui\\common\\icons\\" + iconName + ".inkatlas";
    }

    std::string GetLifepathIcon(const std::string& lifepath) {
        if (lifepath == "corpo") {
            return "base\\gameplay\\gui\\common\\icons\\lifepath_corpo.inkatlas";
        } else if (lifepath == "nomad") {
            return "base\\gameplay\\gui\\common\\icons\\lifepath_nomad.inkatlas";
        } else {
            return "base\\gameplay\\gui\\common\\icons\\lifepath_street.inkatlas";
        }
    }

    std::string GetStatusIcon(const std::string& status) {
        if (status == "combat") {
            return "base\\gameplay\\gui\\common\\icons\\status_combat.inkatlas";
        } else if (status == "driving") {
            return "base\\gameplay\\gui\\common\\icons\\status_vehicle.inkatlas";
        } else if (status == "speaking") {
            return "base\\gameplay\\gui\\common\\icons\\status_voice.inkatlas";
        } else {
            return "base\\gameplay\\gui\\common\\icons\\status_normal.inkatlas";
        }
    }

    std::string GetGameFontPath(const std::string& fontName) {
        return "base\\gameplay\\gui\\fonts\\" + fontName + "\\" + fontName + ".fnt";
    }

    std::string FormatPlayerName(const std::string& name, uint32_t level) {
        return "[L" + std::to_string(level) + "] " + name;
    }

    std::string FormatNetworkStats(float ping, float packetLoss) {
        return "Ping: " + std::to_string(static_cast<int>(ping)) + "ms | " +
               "Loss: " + std::to_string(static_cast<int>(packetLoss * 100)) + "%";
    }

    bool IsGameUIVisible() {
        // Check if game UI is currently visible
        return true; // Placeholder implementation
    }

    float GetUIScale() {
        // Get current UI scale from game settings
        return 1.0f; // Placeholder implementation
    }
}

} // namespace CoopNet