/*
 * Enterprise Plugin System with Edition Gating
 * =============================================
 * Controls plugin loading and execution based on edition.
 * Community edition does NOT support plugin loading.
 * Enterprise/Hyperscaler support custom plugin marketplace.
 */

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <stdexcept>
#include "themis/edition.h"

namespace themis {
namespace plugins {

// ============================================================================
// PLUGIN INFORMATION STRUCTURES
// ============================================================================

enum class PluginType {
    DATA_PROCESSOR,      // Custom data processing
    COMPRESSION,         // Custom compression codec
    ENCRYPTION,          // Custom encryption provider
    INDEX_BACKEND,       // Custom indexing backend
    STORAGE_ENGINE,      // Custom storage engine
    ANALYTICS,           // Custom analytics functions
    REPLICATION,         // Custom replication logic
    SECURITY,            // Custom security provider
    CUSTOM              // Arbitrary user-defined plugin
};

struct PluginMetadata {
    std::string plugin_id;
    std::string plugin_name;
    std::string plugin_version;
    std::string author;
    PluginType type;
    std::string description;
    bool requires_enterprise;  // True = Enterprise+, False = All editions
    uint64_t size_bytes;
    std::string checksum_sha256;
};

struct PluginManifest {
    PluginMetadata metadata;
    std::string plugin_path;
    bool is_loaded;
    std::string load_error;
};

// ============================================================================
// PLUGIN MANAGER - EDITION-AWARE
// ============================================================================

class PluginManager {
public:
    // Singleton instance
    static PluginManager& GetInstance() {
        static PluginManager instance;
        return instance;
    }

    // Check if plugins are supported in this edition
    static constexpr bool ArePluginsSupported() {
        return edition::FEATURE_ENTERPRISE_PLUGINS;
    }

    // Attempt to load a plugin
    // Throws exception if plugins not supported in this edition
    void LoadPlugin(const PluginManifest& manifest) {
        // Edition gating: Check if plugins are available
        if (!ArePluginsSupported()) {
            std::string error = "Plugin system not available in ";
            error += std::string(edition::EDITION_STRING);
            error += " edition. ";
            
            if (edition::GetEditionType() == edition::EditionType::COMMUNITY) {
                error += "To use plugins, please upgrade to Enterprise Edition or higher.";
            }
            throw std::runtime_error(error);
        }

        // Validate plugin manifest before loading
        ValidatePluginManifest(manifest);

        // Load the plugin (simplified - actual implementation would use dlopen/LoadLibrary)
        try {
            // Plugin loading logic would go here
            // For now: just track in registry
            PluginManifest loaded = manifest;
            loaded.is_loaded = true;
            loaded.load_error = "";
            
            plugin_registry_[manifest.metadata.plugin_id] = loaded;
        } catch (const std::exception& e) {
            PluginManifest failed = manifest;
            failed.is_loaded = false;
            failed.load_error = e.what();
            plugin_registry_[manifest.metadata.plugin_id] = failed;
            throw;
        }
    }

    // Unload a plugin
    bool UnloadPlugin(const std::string& plugin_id) {
        auto it = plugin_registry_.find(plugin_id);
        if (it != plugin_registry_.end()) {
            plugin_registry_.erase(it);
            return true;
        }
        return false;
    }

    // Get list of loaded plugins
    std::vector<PluginManifest> GetLoadedPlugins() const {
        std::vector<PluginManifest> plugins;
        for (const auto& [id, manifest] : plugin_registry_) {
            if (manifest.is_loaded) {
                plugins.push_back(manifest);
            }
        }
        return plugins;
    }

    // Get plugin by ID
    const PluginManifest* GetPlugin(const std::string& plugin_id) const {
        auto it = plugin_registry_.find(plugin_id);
        if (it != plugin_registry_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Check if plugin is loaded
    bool IsPluginLoaded(const std::string& plugin_id) const {
        auto it = plugin_registry_.find(plugin_id);
        if (it != plugin_registry_.end()) {
            return it->second.is_loaded;
        }
        return false;
    }

    // Get edition compatibility information
    std::string GetPluginSystemInfo() const {
        const auto info = edition::EditionInfo::Get();
        std::string result = "Plugin System Status:\n";
        result += "Edition: ";
        result += std::string(info.name);
        result += "\nSupported: ";
        result += (info.supports_plugins ? "YES" : "NO");
        result += "\nLoaded Plugins: ";
        result += std::to_string(GetLoadedPlugins().size());
        return result;
    }

private:
    PluginManager() = default;
    ~PluginManager() = default;

    // Prevent copying
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Validate plugin manifest before loading
    void ValidatePluginManifest(const PluginManifest& manifest) {
        // Check if plugin requires Enterprise but is being loaded in Community
        if (manifest.metadata.requires_enterprise && !ArePluginsSupported()) {
            std::string error = "Plugin '";
            error += manifest.metadata.plugin_name;
            error += "' requires Enterprise edition but is running on ";
            error += std::string(edition::EDITION_STRING);
            throw std::runtime_error(error);
        }

        // Validate checksum would go here in production
        // Validate file exists, is readable, etc.
    }

    std::map<std::string, PluginManifest> plugin_registry_;
};

// ============================================================================
// PLUGIN UTILITY FUNCTIONS - EDITION-AWARE
// ============================================================================

// Get a helpful error message for Community users trying to load plugins
inline std::string GetCommunityPluginUnavailableMessage(const std::string& plugin_name) {
    return std::string("Plugin '") + plugin_name + 
           "' is not available in Community Edition. " +
           "Custom plugins require Enterprise Edition or higher. " +
           "Please upgrade at https://themisdb.io/pricing";
}

// Get marketplace information based on edition
inline std::string GetPluginMarketplaceInfo() {
    const auto info = edition::EditionInfo::Get();
    
    if (!info.supports_plugins) {
        return "Plugin Marketplace: Not available in " + 
               std::string(info.name) + " Edition";
    }

    std::string result = "Plugin Marketplace: Available\n";
    result += "Edition: " + std::string(info.name) + "\n";
    result += "Visit: https://marketplace.themisdb.io/";
    
    if (info.type == edition::EditionType::HYPERSCALER) {
        result += " (OEM custom plugins available)";
    }
    return result;
}

// Check if a specific plugin type is available
inline bool CanUsePluginType(PluginType type) {
    // All plugin types require Enterprise or higher
    return edition::FEATURE_ENTERPRISE_PLUGINS;
}

// Get installation instructions for plugins
inline std::string GetPluginInstallationInstructions() {
    if (!PluginManager::ArePluginsSupported()) {
        return "Error: Plugins are not supported in " + 
               std::string(edition::EDITION_STRING) + 
               " Edition. Please upgrade to Enterprise or Hyperscaler.";
    }

    return "To install a plugin:\n"
           "1. Download from https://marketplace.themisdb.io/\n"
           "2. Verify SHA256 checksum\n"
           "3. Place in $THEMIS_HOME/plugins/\n"
           "4. Restart themis_server\n"
           "5. Use CREATE PLUGIN command";
}

} // namespace plugins
} // namespace themis
