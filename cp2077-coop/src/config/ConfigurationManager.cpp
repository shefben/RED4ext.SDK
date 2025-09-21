#include "ConfigurationManager.hpp"
#include "../core/ErrorManager.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <iomanip>
#include <spdlog/spdlog.h>

// Use COOP_LOG macros from ErrorManager.hpp (already included)

// JSON library for configuration parsing
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace CoopNet {

ConfigurationManager& ConfigurationManager::Instance() {
    static ConfigurationManager instance;
    return instance;
}

bool ConfigurationManager::Initialize(const std::string& configDirectory) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    if (m_initialized) {
        return true;
    }

    COOP_LOG_INFO("ConfigurationManager", "Initializing configuration management system");

    m_configDirectory = configDirectory;

    // Create config directory if it doesn't exist
    if (!CreateDirectoryIfNotExists(m_configDirectory)) {
        COOP_LOG_ERROR("ConfigurationManager", "Failed to create config directory: " + m_configDirectory);
        return false;
    }

    // Initialize default configuration
    CreateDefaultConfiguration();

    // Load main configuration file
    std::string mainConfigFile = m_configDirectory + "config.json";
    if (std::filesystem::exists(mainConfigFile)) {
        LoadConfiguration(mainConfigFile, ConfigFormat::JSON, ConfigScope::Global);
    } else {
        // Create default config file
        SaveConfiguration(mainConfigFile, ConfigFormat::JSON, ConfigScope::Global);
    }

    // Load user configuration if it exists
    std::string userConfigFile = m_configDirectory + "user.json";
    if (std::filesystem::exists(userConfigFile)) {
        LoadConfiguration(userConfigFile, ConfigFormat::JSON, ConfigScope::User);
    }

    // Load environment variables
    if (m_environmentOverrides) {
        LoadFromEnvironment();
    }

    m_initialized = true;
    COOP_LOG_INFO("ConfigurationManager", "Configuration management system initialized");

    return true;
}

void ConfigurationManager::Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    if (!m_initialized) {
        return;
    }

    COOP_LOG_INFO("ConfigurationManager", "Shutting down configuration management system");

    // Stop file watching
    if (m_fileWatchingEnabled) {
        EnableFileWatching(false);
    }

    // Save current configuration
    SaveConfiguration(m_configDirectory + "config.json", ConfigFormat::JSON, ConfigScope::Global);
    SaveConfiguration(m_configDirectory + "user.json", ConfigFormat::JSON, ConfigScope::User);

    // Clear all data
    m_configurations.clear();
    m_profiles.clear();
    m_callbacks.clear();
    m_validations.clear();
    m_configFiles.clear();

    m_initialized = false;
}

void ConfigurationManager::CreateDefaultConfiguration() {
    // Create default configuration structure
    ConfigBuilder("network")
        .SetString("host", "127.0.0.1", "Server host address")
        .SetInt("port", 7777, "Server port")
        .SetInt("max_players", 8, "Maximum number of players")
        .SetBool("enable_encryption", true, "Enable network encryption")
        .SetRange("max_players", ConfigValue(1), ConfigValue(32))
        .Apply(ConfigScope::Global);

    ConfigBuilder("audio")
        .SetFloat("master_volume", 1.0f, "Master audio volume")
        .SetFloat("voice_volume", 1.0f, "Voice chat volume")
        .SetBool("enable_voice_chat", true, "Enable voice communication")
        .SetEnum("voice_quality", {ConfigValue(std::string("low")), ConfigValue(std::string("medium")), ConfigValue(std::string("high"))})
        .SetRange("master_volume", ConfigValue(0.0f), ConfigValue(1.0f))
        .SetRange("voice_volume", ConfigValue(0.0f), ConfigValue(1.0f))
        .Apply(ConfigScope::Global);

    ConfigBuilder("performance")
        .SetBool("enable_monitoring", true, "Enable performance monitoring")
        .SetInt("target_fps", 60, "Target frame rate")
        .SetFloat("cpu_threshold", 80.0f, "CPU usage warning threshold")
        .SetFloat("memory_threshold", 85.0f, "Memory usage warning threshold")
        .SetRange("target_fps", ConfigValue(30), ConfigValue(144))
        .Apply(ConfigScope::Global);

    ConfigBuilder("ui")
        .SetBool("show_player_list", true, "Show connected players list")
        .SetBool("show_network_stats", false, "Show network statistics")
        .SetFloat("ui_scale", 1.0f, "UI scaling factor")
        .SetString("theme", "street", "UI theme")
        .SetEnum("theme", {ConfigValue(std::string("street")), ConfigValue(std::string("corpo")), ConfigValue(std::string("nomad"))})
        .SetRange("ui_scale", ConfigValue(0.5f), ConfigValue(2.0f))
        .Apply(ConfigScope::Global);

    ConfigBuilder("security")
        .SetString("api_key", "", "API key for server authentication")
        .SetBool("enable_anti_cheat", true, "Enable anti-cheat protection")
        .SetBool("validate_signatures", true, "Validate packet signatures")
        .SetSecret("api_key", true)
        .Apply(ConfigScope::Global);

    COOP_LOG_INFO("ConfigurationManager", "Default configuration created");
}

template<typename T>
std::optional<T> ConfigurationManager::GetValue(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    const ConfigEntry* entry = FindEntry(sectionPath, key, scope);
    if (!entry) {
        return std::nullopt;
    }

    try {
        return std::get<T>(entry->value);
    } catch (const std::bad_variant_access&) {
        COOP_LOG_WARNING("ConfigurationManager", "Type mismatch for key: " + sectionPath + "." + key);
        return std::nullopt;
    }
}

template<typename T>
T ConfigurationManager::GetValueOrDefault(const std::string& sectionPath, const std::string& key, const T& defaultValue, ConfigScope scope) const {
    auto value = GetValue<T>(sectionPath, key, scope);
    return value.value_or(defaultValue);
}

// Explicit template instantiations
template std::optional<bool> ConfigurationManager::GetValue<bool>(const std::string&, const std::string&, ConfigScope) const;
template std::optional<int32_t> ConfigurationManager::GetValue<int32_t>(const std::string&, const std::string&, ConfigScope) const;
template std::optional<uint32_t> ConfigurationManager::GetValue<uint32_t>(const std::string&, const std::string&, ConfigScope) const;
template std::optional<float> ConfigurationManager::GetValue<float>(const std::string&, const std::string&, ConfigScope) const;
template std::optional<double> ConfigurationManager::GetValue<double>(const std::string&, const std::string&, ConfigScope) const;
template std::optional<std::string> ConfigurationManager::GetValue<std::string>(const std::string&, const std::string&, ConfigScope) const;

bool ConfigurationManager::GetBool(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    return GetValueOrDefault<bool>(sectionPath, key, false, scope);
}

int32_t ConfigurationManager::GetInt(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    return GetValueOrDefault<int32_t>(sectionPath, key, 0, scope);
}

float ConfigurationManager::GetFloat(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    return GetValueOrDefault<float>(sectionPath, key, 0.0f, scope);
}

std::string ConfigurationManager::GetString(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    return GetValueOrDefault<std::string>(sectionPath, key, "", scope);
}

template<typename T>
bool ConfigurationManager::SetValue(const std::string& sectionPath, const std::string& key, const T& value, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    ConfigEntry* entry = FindEntry(sectionPath, key, scope);
    if (!entry) {
        // Create new entry
        auto* section = FindSection(sectionPath, scope);
        if (!section) {
            CreateSection(sectionPath, "", scope);
            section = FindSection(sectionPath, scope);
            if (!section) {
                return false;
            }
        }

        ConfigEntry newEntry;
        newEntry.key = key;
        newEntry.value = value;
        newEntry.defaultValue = value;
        newEntry.type = DeduceType(ConfigValue(value));
        newEntry.scope = scope;
        newEntry.isReadOnly = false;
        newEntry.isSecret = false;
        newEntry.requiresRestart = false;
        newEntry.lastModified = std::chrono::steady_clock::now();

        section->entries[key] = newEntry;
        entry = &section->entries[key];
    }

    if (entry->isReadOnly) {
        COOP_LOG_WARNING("ConfigurationManager", "Attempted to modify read-only key: " + sectionPath + "." + key);
        return false;
    }

    // Validate new value
    ConfigValue newValue(value);
    if (!ValidateValue(sectionPath, key, newValue)) {
        return false;
    }

    ConfigValue oldValue = entry->value;
    entry->value = newValue;
    entry->lastModified = std::chrono::steady_clock::now();

    // Notify change
    ConfigChange change;
    change.type = ChangeNotificationType::ValueChanged;
    change.scope = scope;
    change.sectionPath = sectionPath;
    change.key = key;
    change.oldValue = oldValue;
    change.newValue = newValue;
    change.timestamp = std::chrono::steady_clock::now();
    change.source = "API";

    NotifyChange(change);

    return true;
}

// Explicit template instantiations for SetValue
template bool ConfigurationManager::SetValue<bool>(const std::string&, const std::string&, const bool&, ConfigScope);
template bool ConfigurationManager::SetValue<int32_t>(const std::string&, const std::string&, const int32_t&, ConfigScope);
template bool ConfigurationManager::SetValue<uint32_t>(const std::string&, const std::string&, const uint32_t&, ConfigScope);
template bool ConfigurationManager::SetValue<float>(const std::string&, const std::string&, const float&, ConfigScope);
template bool ConfigurationManager::SetValue<double>(const std::string&, const std::string&, const double&, ConfigScope);
template bool ConfigurationManager::SetValue<std::string>(const std::string&, const std::string&, const std::string&, ConfigScope);

// Explicit template instantiations for GetValueOrDefault
template bool ConfigurationManager::GetValueOrDefault<bool>(const std::string&, const std::string&, const bool&, ConfigScope) const;
template int32_t ConfigurationManager::GetValueOrDefault<int32_t>(const std::string&, const std::string&, const int32_t&, ConfigScope) const;
template uint32_t ConfigurationManager::GetValueOrDefault<uint32_t>(const std::string&, const std::string&, const uint32_t&, ConfigScope) const;
template float ConfigurationManager::GetValueOrDefault<float>(const std::string&, const std::string&, const float&, ConfigScope) const;
template double ConfigurationManager::GetValueOrDefault<double>(const std::string&, const std::string&, const double&, ConfigScope) const;
template std::string ConfigurationManager::GetValueOrDefault<std::string>(const std::string&, const std::string&, const std::string&, ConfigScope) const;

bool ConfigurationManager::LoadConfiguration(const std::string& filename, ConfigFormat format, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    switch (format) {
        case ConfigFormat::JSON:
            return LoadFromJSON(filename, scope);
        default:
            COOP_LOG_ERROR("ConfigurationManager", "Unsupported configuration format");
            return false;
    }
}

bool ConfigurationManager::LoadFromYAML(const std::string& filename, ConfigScope scope) {
    // YAML support would require a YAML parsing library
    // For now, return false to indicate unsupported format
    COOP_LOG_ERROR("ConfigurationManager", "YAML format not supported: " + filename);
    return false;
}

bool ConfigurationManager::SaveToYAML(const std::string& filename, ConfigScope scope) {
    // YAML support would require a YAML parsing library
    // For now, return false to indicate unsupported format
    COOP_LOG_ERROR("ConfigurationManager", "YAML format not supported: " + filename);
    return false;
}

bool ConfigurationManager::LoadFromINI(const std::string& filename, ConfigScope scope) {
    // INI support would require an INI parsing library or custom parser
    // For now, return false to indicate unsupported format
    COOP_LOG_ERROR("ConfigurationManager", "INI format not supported: " + filename);
    return false;
}

bool ConfigurationManager::SaveToINI(const std::string& filename, ConfigScope scope) {
    // INI support would require an INI parsing library or custom parser
    // For now, return false to indicate unsupported format
    COOP_LOG_ERROR("ConfigurationManager", "INI format not supported: " + filename);
    return false;
}

bool ConfigurationManager::LoadFromJSON(const std::string& filename, ConfigScope scope) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            COOP_LOG_ERROR("ConfigurationManager", "Failed to open config file: " + filename);
            return false;
        }

        json configJson;
        file >> configJson;

        // Parse JSON into configuration structure
        for (auto& [sectionName, sectionData] : configJson.items()) {
            if (!sectionData.is_object()) continue;

            for (auto& [keyName, keyValue] : sectionData.items()) {
                ConfigValue value;

                if (keyValue.is_boolean()) {
                    value = keyValue.get<bool>();
                } else if (keyValue.is_number_integer()) {
                    value = keyValue.get<int32_t>();
                } else if (keyValue.is_number_float()) {
                    value = keyValue.get<float>();
                } else if (keyValue.is_string()) {
                    value = keyValue.get<std::string>();
                }

                SetValue(sectionName, keyName, value, scope);
            }
        }

        m_configFiles[scope] = filename;
        COOP_LOG_INFO("ConfigurationManager", "Loaded configuration from: " + filename);
        return true;

    } catch (const std::exception& e) {
        COOP_LOG_ERROR("ConfigurationManager", "Failed to load JSON config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationManager::SaveConfiguration(const std::string& filename, ConfigFormat format, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    switch (format) {
        case ConfigFormat::JSON:
            return SaveToJSON(filename, scope);
        default:
            COOP_LOG_ERROR("ConfigurationManager", "Unsupported configuration format");
            return false;
    }
}

bool ConfigurationManager::SaveToJSON(const std::string& filename, ConfigScope scope) {
    try {
        json configJson;

        auto configIt = m_configurations.find(scope);
        if (configIt == m_configurations.end()) {
            return false;
        }

        const auto& profile = configIt->second;

        for (const auto& [sectionName, section] : profile.sections) {
            json sectionJson;

            for (const auto& [keyName, entry] : section.entries) {
                if (entry.isSecret && m_secretsEncrypted) {
                    continue; // Skip encrypted secrets
                }

                json keyValue;
                std::visit([&keyValue](const auto& value) {
                    keyValue = value;
                }, entry.value);

                sectionJson[keyName] = keyValue;
            }

            configJson[sectionName] = sectionJson;
        }

        std::ofstream file(filename);
        if (!file.is_open()) {
            COOP_LOG_ERROR("ConfigurationManager", "Failed to open config file for writing: " + filename);
            return false;
        }

        file << configJson.dump(2);
        file.close();

        COOP_LOG_INFO("ConfigurationManager", "Saved configuration to: " + filename);
        return true;

    } catch (const std::exception& e) {
        COOP_LOG_ERROR("ConfigurationManager", "Failed to save JSON config: " + std::string(e.what()));
        return false;
    }
}

const ConfigEntry* ConfigurationManager::FindEntry(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    auto configIt = m_configurations.find(scope);
    if (configIt == m_configurations.end()) {
        return nullptr;
    }

    const auto* section = FindSection(sectionPath, scope);
    if (!section) {
        return nullptr;
    }

    auto entryIt = section->entries.find(key);
    if (entryIt == section->entries.end()) {
        return nullptr;
    }

    return &entryIt->second;
}

ConfigEntry* ConfigurationManager::FindEntry(const std::string& sectionPath, const std::string& key, ConfigScope scope) {
    return const_cast<ConfigEntry*>(static_cast<const ConfigurationManager*>(this)->FindEntry(sectionPath, key, scope));
}

const ConfigSection* ConfigurationManager::FindSection(const std::string& sectionPath, ConfigScope scope) const {
    auto configIt = m_configurations.find(scope);
    if (configIt == m_configurations.end()) {
        return nullptr;
    }

    const auto& profile = configIt->second;
    auto sectionIt = profile.sections.find(sectionPath);
    if (sectionIt == profile.sections.end()) {
        return nullptr;
    }

    return &sectionIt->second;
}

ConfigSection* ConfigurationManager::FindSection(const std::string& sectionPath, ConfigScope scope) {
    return const_cast<ConfigSection*>(static_cast<const ConfigurationManager*>(this)->FindSection(sectionPath, scope));
}

bool ConfigurationManager::CreateSection(const std::string& sectionPath, const std::string& description, ConfigScope scope) {
    auto& config = m_configurations[scope];

    ConfigSection section;
    section.name = sectionPath;
    section.description = description;
    section.isReadOnly = false;
    section.lastModified = std::chrono::steady_clock::now();

    config.sections[sectionPath] = section;
    return true;
}

ConfigType ConfigurationManager::DeduceType(const ConfigValue& value) const {
    return static_cast<ConfigType>(value.index());
}

bool ConfigurationManager::ValidateValue(const std::string& sectionPath, const std::string& key, const ConfigValue& value) const {
    auto sectionIt = m_validations.find(sectionPath);
    if (sectionIt == m_validations.end()) {
        return true;
    }

    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return true;
    }

    return ValidateEntry(ConfigEntry{}, value); // Simplified validation
}

bool ConfigurationManager::ValidateEntry(const ConfigEntry& entry, const ConfigValue& value) const {
    // Simplified validation implementation
    return true;
}

void ConfigurationManager::NotifyChange(const ConfigChange& change) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    for (const auto& [id, callbackInfo] : m_callbacks) {
        bool shouldNotify = false;

        if (callbackInfo.isGlobal) {
            shouldNotify = true;
        } else if (callbackInfo.isSectionLevel && callbackInfo.sectionPath == change.sectionPath) {
            shouldNotify = true;
        } else if (callbackInfo.sectionPath == change.sectionPath && callbackInfo.key == change.key) {
            shouldNotify = true;
        }

        if (shouldNotify) {
            try {
                callbackInfo.callback(change);
            } catch (const std::exception& e) {
                COOP_LOG_ERROR("ConfigurationManager", "Exception in config change callback: " + std::string(e.what()));
            }
        }
    }
}

uint64_t ConfigurationManager::RegisterChangeCallback(const std::string& sectionPath, const std::string& key, ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    CallbackInfo info;
    info.id = GenerateCallbackId();
    info.sectionPath = sectionPath;
    info.key = key;
    info.callback = callback;
    info.isGlobal = false;
    info.isSectionLevel = false;

    m_callbacks[info.id] = info;
    return info.id;
}

uint64_t ConfigurationManager::GenerateCallbackId() {
    return m_nextCallbackId.fetch_add(1);
}

bool ConfigurationManager::CreateDirectoryIfNotExists(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::exception&) {
        return false;
    }
}

bool ConfigurationManager::LoadFromEnvironment(const std::string& prefix) {
    // Environment variable loading would be implemented here
    // This is a simplified placeholder
    COOP_LOG_INFO("ConfigurationManager", "Loading environment variables with prefix: " + prefix);
    return true;
}

// ConfigBuilder implementation
ConfigBuilder::ConfigBuilder(const std::string& sectionPath) : m_sectionPath(sectionPath) {}

ConfigBuilder& ConfigBuilder::SetBool(const std::string& key, bool value, const std::string& description) {
    ConfigEntry entry;
    entry.key = key;
    entry.value = value;
    entry.defaultValue = value;
    entry.type = ConfigType::Boolean;
    entry.description = description;
    entry.isReadOnly = false;
    entry.isSecret = false;

    m_entries[key] = entry;
    return *this;
}

ConfigBuilder& ConfigBuilder::SetInt(const std::string& key, int32_t value, const std::string& description) {
    ConfigEntry entry;
    entry.key = key;
    entry.value = value;
    entry.defaultValue = value;
    entry.type = ConfigType::Integer;
    entry.description = description;
    entry.isReadOnly = false;
    entry.isSecret = false;

    m_entries[key] = entry;
    return *this;
}

ConfigBuilder& ConfigBuilder::SetFloat(const std::string& key, float value, const std::string& description) {
    ConfigEntry entry;
    entry.key = key;
    entry.value = value;
    entry.defaultValue = value;
    entry.type = ConfigType::Float;
    entry.description = description;
    entry.isReadOnly = false;
    entry.isSecret = false;

    m_entries[key] = entry;
    return *this;
}

ConfigBuilder& ConfigBuilder::SetString(const std::string& key, const std::string& value, const std::string& description) {
    ConfigEntry entry;
    entry.key = key;
    entry.value = value;
    entry.defaultValue = value;
    entry.type = ConfigType::String;
    entry.description = description;
    entry.isReadOnly = false;
    entry.isSecret = false;

    m_entries[key] = entry;
    return *this;
}

ConfigBuilder& ConfigBuilder::SetRange(const std::string& key, const ConfigValue& min, const ConfigValue& max) {
    ConfigValidation validation;
    validation.type = ValidationType::Range;
    validation.minValue = min;
    validation.maxValue = max;

    m_validations[key] = validation;
    return *this;
}

ConfigBuilder& ConfigBuilder::SetEnum(const std::string& key, const std::vector<ConfigValue>& allowedValues) {
    ConfigValidation validation;
    validation.type = ValidationType::Enum;
    validation.allowedValues = allowedValues;

    m_validations[key] = validation;
    return *this;
}

ConfigBuilder& ConfigBuilder::SetSecret(const std::string& key, bool secret) {
    auto it = m_entries.find(key);
    if (it != m_entries.end()) {
        it->second.isSecret = secret;
    }
    return *this;
}

bool ConfigBuilder::Apply(ConfigScope scope) {
    auto& configManager = ConfigurationManager::Instance();

    // Create section if it doesn't exist
    configManager.CreateSection(m_sectionPath, "", scope);

    // Apply all entries
    for (const auto& [key, entry] : m_entries) {
        std::visit([&](const auto& value) {
            configManager.SetValue(m_sectionPath, key, value, scope);
        }, entry.value);

        // Set metadata
        configManager.SetDescription(m_sectionPath, key, entry.description, scope);
        configManager.SetSecret(m_sectionPath, key, entry.isSecret, scope);
        configManager.SetReadOnly(m_sectionPath, key, entry.isReadOnly, scope);
    }

    // Register validations
    for (const auto& [key, validation] : m_validations) {
        configManager.RegisterValidation(m_sectionPath, key, validation);
    }

    return true;
}

// Utility function implementations
namespace ConfigUtils {
    std::string GetScopeName(ConfigScope scope) {
        switch (scope) {
            case ConfigScope::Global: return "Global";
            case ConfigScope::User: return "User";
            case ConfigScope::Session: return "Session";
            case ConfigScope::Server: return "Server";
            case ConfigScope::Temporary: return "Temporary";
            default: return "Unknown";
        }
    }

    std::string ValueToString(const ConfigValue& value) {
        return std::visit([](const auto& v) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                return v ? "true" : "false";
            } else {
                return std::to_string(v);
            }
        }, value);
    }
}

// Missing function implementations

uint32_t ConfigurationManager::GetUInt(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    return GetValueOrDefault<uint32_t>(sectionPath, key, 0, scope);
}

double ConfigurationManager::GetDouble(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    return GetValueOrDefault<double>(sectionPath, key, 0.0, scope);
}

bool ConfigurationManager::SetBool(const std::string& sectionPath, const std::string& key, bool value, ConfigScope scope) {
    return SetValue(sectionPath, key, value, scope);
}

bool ConfigurationManager::SetInt(const std::string& sectionPath, const std::string& key, int32_t value, ConfigScope scope) {
    return SetValue(sectionPath, key, value, scope);
}

bool ConfigurationManager::SetUInt(const std::string& sectionPath, const std::string& key, uint32_t value, ConfigScope scope) {
    return SetValue(sectionPath, key, value, scope);
}

bool ConfigurationManager::SetFloat(const std::string& sectionPath, const std::string& key, float value, ConfigScope scope) {
    return SetValue(sectionPath, key, value, scope);
}

bool ConfigurationManager::SetDouble(const std::string& sectionPath, const std::string& key, double value, ConfigScope scope) {
    return SetValue(sectionPath, key, value, scope);
}

bool ConfigurationManager::SetString(const std::string& sectionPath, const std::string& key, const std::string& value, ConfigScope scope) {
    return SetValue(sectionPath, key, value, scope);
}

bool ConfigurationManager::HasKey(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    return FindEntry(sectionPath, key, scope) != nullptr;
}

bool ConfigurationManager::HasSection(const std::string& sectionPath, ConfigScope scope) const {
    return FindSection(sectionPath, scope) != nullptr;
}

bool ConfigurationManager::RemoveKey(const std::string& sectionPath, const std::string& key, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto* section = FindSection(sectionPath, scope);
    if (!section) return false;

    auto it = section->entries.find(key);
    if (it == section->entries.end()) return false;

    section->entries.erase(it);

    // Notify change
    ConfigChange change;
    change.type = ChangeNotificationType::KeyRemoved;
    change.scope = scope;
    change.sectionPath = sectionPath;
    change.key = key;
    change.timestamp = std::chrono::steady_clock::now();
    change.source = "API";

    NotifyChange(change);
    return true;
}

bool ConfigurationManager::RemoveSection(const std::string& sectionPath, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto configIt = m_configurations.find(scope);
    if (configIt == m_configurations.end()) return false;

    auto& profile = configIt->second;
    auto it = profile.sections.find(sectionPath);
    if (it == profile.sections.end()) return false;

    profile.sections.erase(it);

    // Notify change
    ConfigChange change;
    change.type = ChangeNotificationType::SectionRemoved;
    change.scope = scope;
    change.sectionPath = sectionPath;
    change.timestamp = std::chrono::steady_clock::now();
    change.source = "API";

    NotifyChange(change);
    return true;
}

std::vector<std::string> ConfigurationManager::GetSectionNames(const std::string& parentPath, ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto configIt = m_configurations.find(scope);
    if (configIt == m_configurations.end()) return {};

    const auto& profile = configIt->second;
    std::vector<std::string> sectionNames;

    for (const auto& [sectionName, section] : profile.sections) {
        if (parentPath.empty() || sectionName.find(parentPath) == 0) {
            sectionNames.push_back(sectionName);
        }
    }

    return sectionNames;
}

std::vector<std::string> ConfigurationManager::GetKeyNames(const std::string& sectionPath, ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    const auto* section = FindSection(sectionPath, scope);
    if (!section) return {};

    std::vector<std::string> keyNames;
    for (const auto& [keyName, entry] : section->entries) {
        keyNames.push_back(keyName);
    }

    return keyNames;
}

bool ConfigurationManager::ReloadConfiguration(ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto fileIt = m_configFiles.find(scope);
    if (fileIt == m_configFiles.end()) return false;

    return LoadConfiguration(fileIt->second, ConfigFormat::JSON, scope);
}

bool ConfigurationManager::LoadProfile(const std::string& profileName) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto it = m_profiles.find(profileName);
    if (it == m_profiles.end()) {
        // Try to load from file
        std::string filename = m_configDirectory + "profiles/" + profileName + ".json";
        if (!std::filesystem::exists(filename)) {
            return false;
        }

        // Load profile from file (simplified implementation)
        return LoadConfiguration(filename, ConfigFormat::JSON, ConfigScope::Global);
    }

    m_activeProfile = profileName;
    return true;
}

bool ConfigurationManager::SaveProfile(const std::string& profileName, const std::string& description) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    ConfigProfile profile;
    profile.profileName = profileName;
    profile.description = description;
    profile.author = "System";
    profile.version = "1.0.0";
    profile.created = std::chrono::steady_clock::now();
    profile.lastModified = profile.created;
    profile.isActive = false;

    // Copy current configuration
    auto configIt = m_configurations.find(ConfigScope::Global);
    if (configIt != m_configurations.end()) {
        profile.sections = configIt->second.sections;
    }

    m_profiles[profileName] = profile;

    // Save to file
    std::string profileDir = m_configDirectory + "profiles/";
    CreateDirectoryIfNotExists(profileDir);
    std::string filename = profileDir + profileName + ".json";

    return SaveConfiguration(filename, ConfigFormat::JSON, ConfigScope::Global);
}

bool ConfigurationManager::DeleteProfile(const std::string& profileName) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto it = m_profiles.find(profileName);
    if (it == m_profiles.end()) return false;

    m_profiles.erase(it);

    // Delete file
    std::string filename = m_configDirectory + "profiles/" + profileName + ".json";
    try {
        return std::filesystem::remove(filename);
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<std::string> ConfigurationManager::GetAvailableProfiles() const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    std::vector<std::string> profiles;
    for (const auto& [profileName, profile] : m_profiles) {
        profiles.push_back(profileName);
    }

    return profiles;
}

std::string ConfigurationManager::GetActiveProfile() const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);
    return m_activeProfile;
}

bool ConfigurationManager::SetActiveProfile(const std::string& profileName) {
    return LoadProfile(profileName);
}

bool ConfigurationManager::RegisterValidation(const std::string& sectionPath, const std::string& key, const ConfigValidation& validation) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    m_validations[sectionPath][key] = validation;
    return true;
}

bool ConfigurationManager::ValidateConfiguration(ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto configIt = m_configurations.find(scope);
    if (configIt == m_configurations.end()) return true;

    const auto& profile = configIt->second;

    for (const auto& [sectionName, section] : profile.sections) {
        for (const auto& [keyName, entry] : section.entries) {
            if (!ValidateValue(sectionName, keyName, entry.value)) {
                return false;
            }
        }
    }

    return true;
}

bool ConfigurationManager::SetDefault(const std::string& sectionPath, const std::string& key, const ConfigValue& defaultValue, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto* entry = FindEntry(sectionPath, key, scope);
    if (!entry) return false;

    const_cast<ConfigEntry*>(entry)->defaultValue = defaultValue;
    return true;
}

bool ConfigurationManager::ResetToDefault(const std::string& sectionPath, const std::string& key, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto* entry = FindEntry(sectionPath, key, scope);
    if (!entry) return false;

    ConfigValue oldValue = entry->value;
    const_cast<ConfigEntry*>(entry)->value = entry->defaultValue;
    const_cast<ConfigEntry*>(entry)->lastModified = std::chrono::steady_clock::now();

    // Notify change
    ConfigChange change;
    change.type = ChangeNotificationType::ValueChanged;
    change.scope = scope;
    change.sectionPath = sectionPath;
    change.key = key;
    change.oldValue = oldValue;
    change.newValue = entry->value;
    change.timestamp = std::chrono::steady_clock::now();
    change.source = "Reset";

    NotifyChange(change);
    return true;
}

bool ConfigurationManager::ResetSectionToDefaults(const std::string& sectionPath, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto* section = FindSection(sectionPath, scope);
    if (!section) return false;

    for (auto& [keyName, entry] : section->entries) {
        entry.value = entry.defaultValue;
        entry.lastModified = std::chrono::steady_clock::now();
    }

    return true;
}

bool ConfigurationManager::ResetAllToDefaults(ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto configIt = m_configurations.find(scope);
    if (configIt == m_configurations.end()) return false;

    auto& profile = configIt->second;

    for (auto& [sectionName, section] : profile.sections) {
        for (auto& [keyName, entry] : section.entries) {
            entry.value = entry.defaultValue;
            entry.lastModified = std::chrono::steady_clock::now();
        }
    }

    return true;
}

uint64_t ConfigurationManager::RegisterSectionCallback(const std::string& sectionPath, ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    CallbackInfo info;
    info.id = GenerateCallbackId();
    info.sectionPath = sectionPath;
    info.callback = callback;
    info.isGlobal = false;
    info.isSectionLevel = true;

    m_callbacks[info.id] = info;
    return info.id;
}

uint64_t ConfigurationManager::RegisterGlobalCallback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    CallbackInfo info;
    info.id = GenerateCallbackId();
    info.callback = callback;
    info.isGlobal = true;
    info.isSectionLevel = false;

    m_callbacks[info.id] = info;
    return info.id;
}

bool ConfigurationManager::UnregisterCallback(uint64_t callbackId) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = m_callbacks.find(callbackId);
    if (it == m_callbacks.end()) return false;

    m_callbacks.erase(it);
    return true;
}

bool ConfigurationManager::SetDescription(const std::string& sectionPath, const std::string& key, const std::string& description, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto* entry = FindEntry(sectionPath, key, scope);
    if (!entry) return false;

    const_cast<ConfigEntry*>(entry)->description = description;
    return true;
}

std::string ConfigurationManager::GetDescription(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    const auto* entry = FindEntry(sectionPath, key, scope);
    return entry ? entry->description : "";
}

bool ConfigurationManager::SetReadOnly(const std::string& sectionPath, const std::string& key, bool readOnly, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto* entry = FindEntry(sectionPath, key, scope);
    if (!entry) return false;

    const_cast<ConfigEntry*>(entry)->isReadOnly = readOnly;
    return true;
}

bool ConfigurationManager::IsReadOnly(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    const auto* entry = FindEntry(sectionPath, key, scope);
    return entry ? entry->isReadOnly : false;
}

bool ConfigurationManager::SetSecret(const std::string& sectionPath, const std::string& key, bool secret, ConfigScope scope) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    auto* entry = FindEntry(sectionPath, key, scope);
    if (!entry) return false;

    const_cast<ConfigEntry*>(entry)->isSecret = secret;
    return true;
}

bool ConfigurationManager::IsSecret(const std::string& sectionPath, const std::string& key, ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    const auto* entry = FindEntry(sectionPath, key, scope);
    return entry ? entry->isSecret : false;
}

bool ConfigurationManager::EncryptSecrets(const std::string& password) {
    // Simplified encryption implementation
    m_encryptionKey = password;
    m_secretsEncrypted = true;
    return true;
}

bool ConfigurationManager::DecryptSecrets(const std::string& password) {
    // Simplified decryption implementation
    if (m_encryptionKey == password) {
        m_secretsEncrypted = false;
        return true;
    }
    return false;
}

bool ConfigurationManager::ImportConfiguration(const std::string& filename, ConfigFormat format, ConfigScope targetScope) {
    return LoadConfiguration(filename, format, targetScope);
}

bool ConfigurationManager::ExportConfiguration(const std::string& filename, ConfigFormat format, ConfigScope sourceScope) {
    return SaveConfiguration(filename, format, sourceScope);
}

bool ConfigurationManager::MergeConfiguration(const std::string& filename, ConfigFormat format, ConfigScope targetScope) {
    // For now, just load the configuration (merging would require more complex logic)
    return LoadConfiguration(filename, format, targetScope);
}

bool ConfigurationManager::EnableFileWatching(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    if (m_fileWatchingEnabled == enabled) return true;

    m_fileWatchingEnabled = enabled;

    if (enabled && !m_fileWatchingThread.joinable()) {
        m_shouldStopWatching = false;
        m_fileWatchingThread = std::thread(&ConfigurationManager::FileWatchingLoop, this);
    } else if (!enabled && m_fileWatchingThread.joinable()) {
        m_shouldStopWatching = true;
        m_fileWatchingThread.join();
    }

    return true;
}

bool ConfigurationManager::IsFileWatchingEnabled() const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);
    return m_fileWatchingEnabled;
}

void ConfigurationManager::ForceReload() {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    for (const auto& [scope, filename] : m_configFiles) {
        LoadConfiguration(filename, ConfigFormat::JSON, scope);
    }
}

bool ConfigurationManager::SetEnvironmentOverrides(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);
    m_environmentOverrides = enabled;
    return true;
}

bool ConfigurationManager::ParseCommandLine(int argc, char* argv[]) {
    // Simplified command line parsing implementation
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--") == 0) {
            // Handle --key=value format
            size_t equals = arg.find('=');
            if (equals != std::string::npos) {
                std::string key = arg.substr(2, equals - 2);
                std::string value = arg.substr(equals + 1);
                // This would need proper mapping to section/key pairs
                SetString("cli", key, value, ConfigScope::Temporary);
            }
        }
    }
    return true;
}

void ConfigurationManager::RegisterCommandLineOption(const std::string& option, const std::string& sectionPath, const std::string& key, const std::string& description) {
    // Store command line option mappings (simplified implementation)
    // In a full implementation, this would store the mappings for ParseCommandLine to use
}

bool ConfigurationManager::CreateBackup(const std::string& backupName) {
    std::string actualBackupName = backupName.empty() ? GenerateBackupName() : backupName;
    std::string backupDir = m_configDirectory + "backups/";
    CreateDirectoryIfNotExists(backupDir);

    std::string backupFile = backupDir + actualBackupName + ".json";
    return SaveConfiguration(backupFile, ConfigFormat::JSON, ConfigScope::Global);
}

bool ConfigurationManager::RestoreBackup(const std::string& backupName) {
    std::string backupFile = m_configDirectory + "backups/" + backupName + ".json";
    if (!std::filesystem::exists(backupFile)) return false;

    return LoadConfiguration(backupFile, ConfigFormat::JSON, ConfigScope::Global);
}

std::vector<std::string> ConfigurationManager::GetAvailableBackups() const {
    std::vector<std::string> backups;
    std::string backupDir = m_configDirectory + "backups/";

    try {
        if (std::filesystem::exists(backupDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    backups.push_back(entry.path().stem().string());
                }
            }
        }
    } catch (const std::exception&) {
        // Ignore filesystem errors
    }

    return backups;
}

bool ConfigurationManager::DeleteBackup(const std::string& backupName) {
    std::string backupFile = m_configDirectory + "backups/" + backupName + ".json";
    try {
        return std::filesystem::remove(backupFile);
    } catch (const std::exception&) {
        return false;
    }
}

bool ConfigurationManager::EnableConfiguration(const std::string& sectionPath, bool enabled, ConfigScope scope) {
    return SetBool(sectionPath, "enabled", enabled, scope);
}

bool ConfigurationManager::IsConfigurationEnabled(const std::string& sectionPath, ConfigScope scope) const {
    return GetBool(sectionPath, "enabled", scope);
}

std::string ConfigurationManager::GenerateConfigurationReport() const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    std::stringstream report;
    report << "Configuration Report\n";
    report << "==================\n\n";

    for (const auto& [scope, profile] : m_configurations) {
        report << "Scope: " << ConfigUtils::GetScopeName(scope) << "\n";
        report << "Sections: " << profile.sections.size() << "\n";

        for (const auto& [sectionName, section] : profile.sections) {
            report << "  Section: " << sectionName << " (" << section.entries.size() << " keys)\n";
        }

        report << "\n";
    }

    return report.str();
}

std::unordered_map<std::string, std::string> ConfigurationManager::GetChangedValues(ConfigScope scope) const {
    std::lock_guard<std::recursive_mutex> lock(m_configMutex);

    std::unordered_map<std::string, std::string> changedValues;

    auto configIt = m_configurations.find(scope);
    if (configIt == m_configurations.end()) return changedValues;

    const auto& profile = configIt->second;

    for (const auto& [sectionName, section] : profile.sections) {
        for (const auto& [keyName, entry] : section.entries) {
            if (entry.value != entry.defaultValue) {
                std::string fullKey = sectionName + "." + keyName;
                changedValues[fullKey] = ConfigUtils::ValueToString(entry.value);
            }
        }
    }

    return changedValues;
}

void ConfigurationManager::Lock() const {
    m_configMutex.lock();
}

void ConfigurationManager::Unlock() const {
    m_configMutex.unlock();
}

bool ConfigurationManager::TryLock() const {
    return m_configMutex.try_lock();
}

void ConfigurationManager::FileWatchingLoop() {
    while (!m_shouldStopWatching) {
        try {
            for (const auto& [scope, filename] : m_configFiles) {
                CheckFileModification(filename, scope);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            COOP_LOG_ERROR("ConfigurationManager", "File watching error: " + std::string(e.what()));
        }
    }
}

bool ConfigurationManager::CheckFileModification(const std::string& filename, ConfigScope scope) {
    try {
        if (!std::filesystem::exists(filename)) return false;

        auto lastWriteTime = std::filesystem::last_write_time(filename);
        auto it = m_fileTimestamps.find(filename);

        if (it == m_fileTimestamps.end()) {
            m_fileTimestamps[filename] = lastWriteTime;
            return false;
        }

        if (lastWriteTime != it->second) {
            it->second = lastWriteTime;
            LoadConfiguration(filename, ConfigFormat::JSON, scope);

            ConfigChange change;
            change.type = ChangeNotificationType::FileReloaded;
            change.scope = scope;
            change.timestamp = std::chrono::steady_clock::now();
            change.source = "FileWatcher";

            NotifyChange(change);
            return true;
        }

        return false;
    } catch (const std::exception&) {
        return false;
    }
}

std::string ConfigurationManager::GetScopeString(ConfigScope scope) const {
    return ConfigUtils::GetScopeName(scope);
}

std::string ConfigurationManager::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string ConfigurationManager::GenerateBackupName() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "backup_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

// Additional ConfigBuilder missing implementations
ConfigBuilder& ConfigBuilder::SetReadOnly(const std::string& key, bool readOnly) {
    auto it = m_entries.find(key);
    if (it != m_entries.end()) {
        it->second.isReadOnly = readOnly;
    }
    return *this;
}

// Additional ConfigUtils missing implementations
namespace ConfigUtils {
    std::string GetTypeName(ConfigType type) {
        switch (type) {
            case ConfigType::Boolean: return "Boolean";
            case ConfigType::Integer: return "Integer";
            case ConfigType::UnsignedInteger: return "UnsignedInteger";
            case ConfigType::LongInteger: return "LongInteger";
            case ConfigType::UnsignedLongInteger: return "UnsignedLongInteger";
            case ConfigType::Float: return "Float";
            case ConfigType::Double: return "Double";
            case ConfigType::String: return "String";
            case ConfigType::Array: return "Array";
            case ConfigType::Object: return "Object";
            default: return "Unknown";
        }
    }

    std::string GetValidationTypeName(ValidationType type) {
        switch (type) {
            case ValidationType::None: return "None";
            case ValidationType::Range: return "Range";
            case ValidationType::Enum: return "Enum";
            case ValidationType::Regex: return "Regex";
            case ValidationType::Custom: return "Custom";
            default: return "Unknown";
        }
    }

    ConfigValue StringToValue(const std::string& str, ConfigType type) {
        switch (type) {
            case ConfigType::Boolean:
                return str == "true" || str == "1";
            case ConfigType::Integer:
                return std::stoi(str);
            case ConfigType::UnsignedInteger:
                return static_cast<uint32_t>(std::stoul(str));
            case ConfigType::Float:
                return std::stof(str);
            case ConfigType::Double:
                return std::stod(str);
            case ConfigType::String:
            default:
                return str;
        }
    }

    bool IsValidKeyName(const std::string& key) {
        if (key.empty()) return false;

        // Check for valid characters (alphanumeric, underscore, hyphen)
        for (char c : key) {
            if (!std::isalnum(c) && c != '_' && c != '-') {
                return false;
            }
        }

        return true;
    }

    bool IsValidSectionPath(const std::string& path) {
        if (path.empty()) return true; // Root path is valid

        // Check for valid characters and proper format
        return std::all_of(path.begin(), path.end(), [](char c) {
            return std::isalnum(c) || c == '_' || c == '-' || c == '.';
        });
    }

    std::string SanitizeKeyName(const std::string& key) {
        std::string sanitized;
        for (char c : key) {
            if (std::isalnum(c) || c == '_' || c == '-') {
                sanitized += c;
            } else {
                sanitized += '_';
            }
        }
        return sanitized;
    }

    bool ValidateRange(const ConfigValue& value, const ConfigValue& min, const ConfigValue& max) {
        // Simplified range validation
        try {
            if (std::holds_alternative<int32_t>(value)) {
                int32_t val = std::get<int32_t>(value);
                int32_t minVal = std::get<int32_t>(min);
                int32_t maxVal = std::get<int32_t>(max);
                return val >= minVal && val <= maxVal;
            } else if (std::holds_alternative<float>(value)) {
                float val = std::get<float>(value);
                float minVal = std::get<float>(min);
                float maxVal = std::get<float>(max);
                return val >= minVal && val <= maxVal;
            }
        } catch (const std::bad_variant_access&) {
            return false;
        }

        return true;
    }

    bool ValidateEnum(const ConfigValue& value, const std::vector<ConfigValue>& allowedValues) {
        return std::find(allowedValues.begin(), allowedValues.end(), value) != allowedValues.end();
    }

    bool ValidateRegex(const std::string& value, const std::string& pattern) {
        try {
            std::regex regex(pattern);
            return std::regex_match(value, regex);
        } catch (const std::regex_error&) {
            return false;
        }
    }
}

} // namespace CoopNet