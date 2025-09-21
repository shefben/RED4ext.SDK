#pragma once

#include <RED4ext/RED4ext.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <variant>
#include <optional>

namespace CoopNet {

// Configuration value types
using ConfigValue = std::variant<bool, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string>;

// Configuration scopes
enum class ConfigScope : uint8_t {
    Global = 0,         // Global application settings
    User = 1,           // User-specific settings
    Session = 2,        // Current session settings
    Server = 3,         // Server-specific settings
    Temporary = 4       // Temporary runtime settings
};

// Configuration data types
enum class ConfigType : uint8_t {
    Boolean = 0,
    Integer = 1,
    UnsignedInteger = 2,
    LongInteger = 3,
    UnsignedLongInteger = 4,
    Float = 5,
    Double = 6,
    String = 7,
    Array = 8,
    Object = 9
};

// Configuration validation types
enum class ValidationType : uint8_t {
    None = 0,
    Range = 1,          // Numeric range validation
    Enum = 2,           // Enumerated values
    Regex = 3,          // String pattern matching
    Custom = 4          // Custom validation function
};

// Configuration change notification types
enum class ChangeNotificationType : uint8_t {
    ValueChanged = 0,
    KeyAdded = 1,
    KeyRemoved = 2,
    SectionAdded = 3,
    SectionRemoved = 4,
    FileReloaded = 5
};

// Forward declarations
struct ConfigEntry;
struct ConfigSection;
struct ConfigChange;
struct ConfigProfile;

// Configuration validation rules
struct ConfigValidation {
    ValidationType type;
    ConfigValue minValue;
    ConfigValue maxValue;
    std::vector<ConfigValue> allowedValues;
    std::string regexPattern;
    std::function<bool(const ConfigValue&)> customValidator;
    std::string errorMessage;
};

// Configuration entry
struct ConfigEntry {
    std::string key;
    ConfigValue value;
    ConfigValue defaultValue;
    ConfigType type;
    ConfigScope scope;
    std::string description;
    ConfigValidation validation;
    bool isReadOnly;
    bool isSecret;
    bool requiresRestart;
    std::chrono::steady_clock::time_point lastModified;
    std::string modifiedBy;
    std::vector<std::string> tags;
};

// Configuration section
struct ConfigSection {
    std::string name;
    std::string description;
    std::unordered_map<std::string, ConfigEntry> entries;
    std::unordered_map<std::string, ConfigSection> subsections;
    bool isReadOnly;
    std::chrono::steady_clock::time_point lastModified;
};

// Configuration change notification
struct ConfigChange {
    ChangeNotificationType type;
    ConfigScope scope;
    std::string sectionPath;
    std::string key;
    ConfigValue oldValue;
    ConfigValue newValue;
    std::chrono::steady_clock::time_point timestamp;
    std::string source;
};

// Configuration profile
struct ConfigProfile {
    std::string profileName;
    std::string description;
    std::string author;
    std::string version;
    std::chrono::steady_clock::time_point created;
    std::chrono::steady_clock::time_point lastModified;
    std::unordered_map<std::string, ConfigSection> sections;
    std::vector<std::string> dependencies;
    bool isActive;
};

// Configuration file format
enum class ConfigFormat : uint8_t {
    JSON = 0,
    YAML = 1,
    INI = 2,
    XML = 3,
    Binary = 4
};

// Main configuration management system
class ConfigurationManager {
public:
    static ConfigurationManager& Instance();

    // System lifecycle
    bool Initialize(const std::string& configDirectory = "config/");
    void Shutdown();

    // Configuration loading and saving
    bool LoadConfiguration(const std::string& filename, ConfigFormat format = ConfigFormat::JSON, ConfigScope scope = ConfigScope::Global);
    bool SaveConfiguration(const std::string& filename, ConfigFormat format = ConfigFormat::JSON, ConfigScope scope = ConfigScope::Global);
    bool ReloadConfiguration(ConfigScope scope = ConfigScope::Global);

    // Profile management
    bool LoadProfile(const std::string& profileName);
    bool SaveProfile(const std::string& profileName, const std::string& description = "");
    bool DeleteProfile(const std::string& profileName);
    std::vector<std::string> GetAvailableProfiles() const;
    std::string GetActiveProfile() const;
    bool SetActiveProfile(const std::string& profileName);

    // Value retrieval (with type safety)
    template<typename T>
    std::optional<T> GetValue(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;

    template<typename T>
    T GetValueOrDefault(const std::string& sectionPath, const std::string& key, const T& defaultValue, ConfigScope scope = ConfigScope::Global) const;

    // Specialized getters
    bool GetBool(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    int32_t GetInt(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    uint32_t GetUInt(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    float GetFloat(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    double GetDouble(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    std::string GetString(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;

    // Value setting
    template<typename T>
    bool SetValue(const std::string& sectionPath, const std::string& key, const T& value, ConfigScope scope = ConfigScope::Global);

    bool SetBool(const std::string& sectionPath, const std::string& key, bool value, ConfigScope scope = ConfigScope::Global);
    bool SetInt(const std::string& sectionPath, const std::string& key, int32_t value, ConfigScope scope = ConfigScope::Global);
    bool SetUInt(const std::string& sectionPath, const std::string& key, uint32_t value, ConfigScope scope = ConfigScope::Global);
    bool SetFloat(const std::string& sectionPath, const std::string& key, float value, ConfigScope scope = ConfigScope::Global);
    bool SetDouble(const std::string& sectionPath, const std::string& key, double value, ConfigScope scope = ConfigScope::Global);
    bool SetString(const std::string& sectionPath, const std::string& key, const std::string& value, ConfigScope scope = ConfigScope::Global);

    // Key and section management
    bool HasKey(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    bool HasSection(const std::string& sectionPath, ConfigScope scope = ConfigScope::Global) const;
    bool RemoveKey(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global);
    bool RemoveSection(const std::string& sectionPath, ConfigScope scope = ConfigScope::Global);

    // Section operations
    std::vector<std::string> GetSectionNames(const std::string& parentPath = "", ConfigScope scope = ConfigScope::Global) const;
    std::vector<std::string> GetKeyNames(const std::string& sectionPath, ConfigScope scope = ConfigScope::Global) const;
    bool CreateSection(const std::string& sectionPath, const std::string& description = "", ConfigScope scope = ConfigScope::Global);

    // Validation
    bool RegisterValidation(const std::string& sectionPath, const std::string& key, const ConfigValidation& validation);
    bool ValidateValue(const std::string& sectionPath, const std::string& key, const ConfigValue& value) const;
    bool ValidateConfiguration(ConfigScope scope = ConfigScope::Global) const;

    // Default values
    bool SetDefault(const std::string& sectionPath, const std::string& key, const ConfigValue& defaultValue, ConfigScope scope = ConfigScope::Global);
    bool ResetToDefault(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global);
    bool ResetSectionToDefaults(const std::string& sectionPath, ConfigScope scope = ConfigScope::Global);
    bool ResetAllToDefaults(ConfigScope scope = ConfigScope::Global);

    // Change notifications
    using ConfigChangeCallback = std::function<void(const ConfigChange&)>;
    uint64_t RegisterChangeCallback(const std::string& sectionPath, const std::string& key, ConfigChangeCallback callback);
    uint64_t RegisterSectionCallback(const std::string& sectionPath, ConfigChangeCallback callback);
    uint64_t RegisterGlobalCallback(ConfigChangeCallback callback);
    bool UnregisterCallback(uint64_t callbackId);

    // Configuration metadata
    bool SetDescription(const std::string& sectionPath, const std::string& key, const std::string& description, ConfigScope scope = ConfigScope::Global);
    std::string GetDescription(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    bool SetReadOnly(const std::string& sectionPath, const std::string& key, bool readOnly, ConfigScope scope = ConfigScope::Global);
    bool IsReadOnly(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;

    // Security features
    bool SetSecret(const std::string& sectionPath, const std::string& key, bool secret, ConfigScope scope = ConfigScope::Global);
    bool IsSecret(const std::string& sectionPath, const std::string& key, ConfigScope scope = ConfigScope::Global) const;
    bool EncryptSecrets(const std::string& password);
    bool DecryptSecrets(const std::string& password);

    // Import/Export
    bool ImportConfiguration(const std::string& filename, ConfigFormat format, ConfigScope targetScope = ConfigScope::Global);
    bool ExportConfiguration(const std::string& filename, ConfigFormat format, ConfigScope sourceScope = ConfigScope::Global);
    bool MergeConfiguration(const std::string& filename, ConfigFormat format, ConfigScope targetScope = ConfigScope::Global);

    // Configuration watching
    bool EnableFileWatching(bool enabled);
    bool IsFileWatchingEnabled() const;
    void ForceReload();

    // Environment variables
    bool LoadFromEnvironment(const std::string& prefix = "COOPNET_");
    bool SetEnvironmentOverrides(bool enabled);

    // Command line arguments
    bool ParseCommandLine(int argc, char* argv[]);
    void RegisterCommandLineOption(const std::string& option, const std::string& sectionPath, const std::string& key, const std::string& description);

    // Configuration backup and restore
    bool CreateBackup(const std::string& backupName = "");
    bool RestoreBackup(const std::string& backupName);
    std::vector<std::string> GetAvailableBackups() const;
    bool DeleteBackup(const std::string& backupName);

    // Advanced features
    bool EnableConfiguration(const std::string& sectionPath, bool enabled, ConfigScope scope = ConfigScope::Global);
    bool IsConfigurationEnabled(const std::string& sectionPath, ConfigScope scope = ConfigScope::Global) const;
    std::string GenerateConfigurationReport() const;
    std::unordered_map<std::string, std::string> GetChangedValues(ConfigScope scope = ConfigScope::Global) const;

    // Thread safety
    void Lock() const;
    void Unlock() const;
    bool TryLock() const;

private:
    ConfigurationManager() = default;
    ~ConfigurationManager() = default;
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

    // Core operations
    ConfigEntry* FindEntry(const std::string& sectionPath, const std::string& key, ConfigScope scope);
    const ConfigEntry* FindEntry(const std::string& sectionPath, const std::string& key, ConfigScope scope) const;
    ConfigSection* FindSection(const std::string& sectionPath, ConfigScope scope);
    const ConfigSection* FindSection(const std::string& sectionPath, ConfigScope scope) const;

    // File operations
    bool LoadFromJSON(const std::string& filename, ConfigScope scope);
    bool SaveToJSON(const std::string& filename, ConfigScope scope);
    bool LoadFromYAML(const std::string& filename, ConfigScope scope);
    bool SaveToYAML(const std::string& filename, ConfigScope scope);
    bool LoadFromINI(const std::string& filename, ConfigScope scope);
    bool SaveToINI(const std::string& filename, ConfigScope scope);

    // Value conversion and validation
    bool ConvertValue(const ConfigValue& source, ConfigValue& target, ConfigType targetType) const;
    bool ValidateEntry(const ConfigEntry& entry, const ConfigValue& value) const;
    ConfigType DeduceType(const ConfigValue& value) const;

    // Path parsing
    std::vector<std::string> ParseSectionPath(const std::string& path) const;
    std::string JoinSectionPath(const std::vector<std::string>& parts) const;

    // Change notification
    void NotifyChange(const ConfigChange& change);
    uint64_t GenerateCallbackId();

    // File watching
    void FileWatchingLoop();
    bool CheckFileModification(const std::string& filename, ConfigScope scope);

    // Utility methods
    std::string GetScopeString(ConfigScope scope) const;
    std::string GetCurrentTimestamp() const;
    std::string GenerateBackupName() const;
    bool CreateDirectoryIfNotExists(const std::string& path);

    // Data storage
    std::unordered_map<ConfigScope, ConfigProfile> m_configurations;
    std::unordered_map<std::string, ConfigProfile> m_profiles;
    std::string m_activeProfile;
    std::string m_configDirectory;

    // Change callbacks
    struct CallbackInfo {
        uint64_t id;
        std::string sectionPath;
        std::string key;
        ConfigChangeCallback callback;
        bool isGlobal;
        bool isSectionLevel;
    };
    std::unordered_map<uint64_t, CallbackInfo> m_callbacks;

    // Validation rules
    std::unordered_map<std::string, std::unordered_map<std::string, ConfigValidation>> m_validations;

    // File tracking
    std::unordered_map<ConfigScope, std::string> m_configFiles;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;

    // Security
    std::string m_encryptionKey;
    bool m_secretsEncrypted = false;

    // Synchronization
    mutable std::recursive_mutex m_configMutex;
    mutable std::mutex m_callbackMutex;

    // System state
    bool m_initialized = false;
    bool m_fileWatchingEnabled = false;
    bool m_environmentOverrides = true;

    // Threading
    std::thread m_fileWatchingThread;
    std::atomic<bool> m_shouldStopWatching{false};

    // ID generation
    std::atomic<uint64_t> m_nextCallbackId{1};

    // Default configuration structure
    void InitializeDefaults();
    void CreateDefaultConfiguration();
};

// Configuration builder helper class
class ConfigBuilder {
public:
    ConfigBuilder(const std::string& sectionPath);

    ConfigBuilder& SetBool(const std::string& key, bool value, const std::string& description = "");
    ConfigBuilder& SetInt(const std::string& key, int32_t value, const std::string& description = "");
    ConfigBuilder& SetFloat(const std::string& key, float value, const std::string& description = "");
    ConfigBuilder& SetString(const std::string& key, const std::string& value, const std::string& description = "");

    ConfigBuilder& SetRange(const std::string& key, const ConfigValue& min, const ConfigValue& max);
    ConfigBuilder& SetEnum(const std::string& key, const std::vector<ConfigValue>& allowedValues);
    ConfigBuilder& SetReadOnly(const std::string& key, bool readOnly = true);
    ConfigBuilder& SetSecret(const std::string& key, bool secret = true);

    bool Apply(ConfigScope scope = ConfigScope::Global);

private:
    std::string m_sectionPath;
    std::unordered_map<std::string, ConfigEntry> m_entries;
    std::unordered_map<std::string, ConfigValidation> m_validations;
};

// Utility functions for configuration management
namespace ConfigUtils {
    std::string GetScopeName(ConfigScope scope);
    std::string GetTypeName(ConfigType type);
    std::string GetValidationTypeName(ValidationType type);

    // Value conversion utilities
    template<typename T>
    std::optional<T> ConvertValue(const ConfigValue& value);

    std::string ValueToString(const ConfigValue& value);
    ConfigValue StringToValue(const std::string& str, ConfigType type);

    // Path utilities
    bool IsValidKeyName(const std::string& key);
    bool IsValidSectionPath(const std::string& path);
    std::string SanitizeKeyName(const std::string& key);

    // Validation helpers
    bool ValidateRange(const ConfigValue& value, const ConfigValue& min, const ConfigValue& max);
    bool ValidateEnum(const ConfigValue& value, const std::vector<ConfigValue>& allowedValues);
    bool ValidateRegex(const std::string& value, const std::string& pattern);
}

} // namespace CoopNet