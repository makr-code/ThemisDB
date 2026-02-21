/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_manager.h                                   ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     347                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "plugins/plugin_metrics.h"
#include "plugins/plugin_dependency_resolver.h"  // Dependency resolution
#include "plugins/plugin_hot_plug_monitor.h"  // HotPlugConfig definition
#include "acceleration/plugin_loader.h"  // Reuse existing loader
#include "utils/expected.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace plugins {

using json = nlohmann::json;

// Forward declarations
class PluginHotPlugMonitor;
struct HotPlugConfig;

/**
 * @brief Plugin Reload Phase
 * 
 * Used for event notifications during hot-reload operations
 */
enum class PluginReloadPhase {
    BEFORE_UNLOAD,  ///< About to unload plugin
    AFTER_UNLOAD,   ///< Plugin unloaded successfully
    AFTER_LOAD      ///< Plugin reloaded successfully
};

/**
 * @brief Plugin Reload Event Listener
 * 
 * Callback type for reload event notifications.
 * Listeners are notified during different phases of plugin reload.
 * 
 * @param plugin_name Name of the plugin being reloaded
 * @param phase Current reload phase
 */
using PluginReloadListener = std::function<void(const std::string& plugin_name, PluginReloadPhase phase)>;

/**
 * @brief Unified Plugin Manager
 * 
 * Consolidates existing plugin systems:
 * 1. acceleration::PluginLoader (compute backends)
 * 2. HSM PKCS#11 dynamic loading
 * 3. ZLUDA dynamic loading
 * 
 * New capabilities:
 * - Plugin manifest (plugin.json) support
 * - Type-based plugin registry
 * - Auto-discovery from plugin directory
 * - Dependency resolution
 * - Hot-reload support
 * 
 * Thread-Safety: All methods are thread-safe
 */
class PluginManager {
private:
    struct PluginEntry {
        std::string name;
        PluginType type;
        std::string path;
        PluginManifest manifest;
        void* library_handle = nullptr;
        std::unique_ptr<IThemisPlugin> instance;
        bool loaded = false;
        std::string file_hash;
    };
    
    std::unordered_map<std::string, PluginEntry> plugins_;  // name -> entry
    std::unordered_map<PluginType, std::vector<std::string>> type_index_;  // type -> plugin names
    PluginMetrics metrics_;  // Plugin metrics tracker
    std::unique_ptr<PluginHotPlugMonitor> hot_plug_monitor_;  // Hot-plug filesystem monitor
    std::vector<PluginReloadListener> reload_listeners_;  // Reload event listeners
    mutable std::mutex mutex_;
    
    // Reuse existing platform-specific loading from acceleration/plugin_loader.cpp
    void* loadLibrary(const std::string& path);
    void* getSymbol(void* handle, const std::string& symbolName);
    void unloadLibrary(void* handle);
    
    // Manifest loading
    std::optional<PluginManifest> loadManifest(const std::string& manifest_path);
    
    // Manifest signature verification
    bool verifyManifestSignature(const std::string& manifest_path, std::string& error_message);
    
    // Security verification (reuse acceleration/plugin_security.h)
    bool verifyPlugin(const std::string& path, std::string& error_message);
    
    std::string calculateFileHash(const std::string& path);
    
    // Hot-reload helper methods
    std::vector<std::string> findDependentPlugins(const std::string& name) const;
    void notifyPluginReload(const std::string& name, PluginReloadPhase phase);
    
public:
    PluginManager() = default;
    ~PluginManager();
    
    // Prevent copying
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    /**
     * @brief Scan plugin directory for manifests
     * @param directory Path to plugin directory
     * @return Result<size_t> - Number of plugins discovered or error
     */
    Result<size_t> scanPluginDirectory(const std::string& directory);
    
    /**
     * @brief Load a plugin by name
     * @param name Plugin name (from manifest)
     * @return Result<IThemisPlugin*> with loaded plugin instance or error
     */
    Result<IThemisPlugin*> loadPlugin(const std::string& name);
    
    /**
     * @brief Load a plugin from explicit path
     * @param path Path to plugin DLL/SO
     * @param config Optional configuration JSON
     * @return Result<IThemisPlugin*> with loaded plugin instance or error
     */
    Result<IThemisPlugin*> loadPluginFromPath(
        const std::string& path,
        const std::string& config = "{}"
    );
    
    /**
     * @brief Unload a plugin
     * @param name Plugin name
     * @return Result<void> - success or error
     */
    Result<void> unloadPlugin(const std::string& name);
    
    /**
     * @brief Unload all plugins
     * @return Result<void> - success or error
     */
    Result<void> unloadAllPlugins();
    
    /**
     * @brief Get loaded plugin by name
     * @param name Plugin name
     * @return Result<IThemisPlugin*> with plugin instance or ERR_PLUGIN_NOT_FOUND if not loaded
     */
    Result<IThemisPlugin*> getPlugin(const std::string& name) const;
    
    /**
     * @brief Get all plugins of a specific type
     * @param type Plugin type
     * @return Vector of plugin instances
     */
    std::vector<IThemisPlugin*> getPluginsByType(PluginType type) const;
    
    /**
     * @brief List all discovered plugins (loaded or not)
     * @return Vector of plugin manifests
     */
    std::vector<PluginManifest> listPlugins() const;
    
    /**
     * @brief List loaded plugins
     * @return Vector of plugin names
     */
    std::vector<std::string> listLoadedPlugins() const;
    
    /**
     * @brief Check if plugin is loaded
     * @param name Plugin name
     * @return true if loaded
     */
    bool isPluginLoaded(const std::string& name) const;
    
    /**
     * @brief Reload a plugin (hot-reload)
     * @param name Plugin name
     * @return Result<void> - success or error
     */
    Result<void> reloadPlugin(const std::string& name);
    
    /**
     * @brief Auto-load plugins marked with auto_load=true
     * @return Result<size_t> - Number of plugins loaded or error
     */
    Result<size_t> autoLoadPlugins();
    
    /**
     * @brief Get plugin manifest
     * @param name Plugin name
     * @return Result<PluginManifest> - Manifest or error if not found
     */
    Result<PluginManifest> getManifest(const std::string& name) const;
    
    /**
     * @brief Get plugin metrics
     * @return Reference to plugin metrics
     */
    const PluginMetrics& getMetrics() const { return metrics_; }
    
    /**
     * @brief Get mutable plugin metrics (for testing)
     * @return Mutable reference to plugin metrics
     */
    PluginMetrics& getMetricsMutable() { return metrics_; }
    
    /**
     * @brief Enable hot-plug monitoring for a directory
     * @param directory Directory to monitor
     * @param config Hot-plug configuration
     * @return true if monitoring started successfully
     */
    bool enableHotPlug(const std::string& directory, const HotPlugConfig& config = HotPlugConfig());
    
    /**
     * @brief Disable hot-plug monitoring
     */
    void disableHotPlug();
    
    /**
     * @brief Check if hot-plug monitoring is enabled
     * @return true if monitoring is active
     */
    bool isHotPlugEnabled() const;
    
    /**
     * @brief Register a reload event listener
     * 
     * Listeners are notified during plugin reload phases:
     * - BEFORE_UNLOAD: Before unloading old plugin
     * - AFTER_UNLOAD: After unloading old plugin
     * - AFTER_LOAD: After loading new plugin
     * 
     * @param listener Callback function to be notified
     * @note Thread-safe: Can be called from any thread
     */
    void registerReloadListener(PluginReloadListener listener);
    
    /**
     * @brief Clear all reload event listeners
     * @note Thread-safe: Can be called from any thread
     */
    void clearReloadListeners();
    
    /**
     * @brief Singleton instance
     */
    static PluginManager& instance();
};

/**
 * @brief Plugin Registry
 * 
 * Global registry for type-specific plugin factories.
 * Allows third-party code to register plugin types.
 */
class PluginManagerRegistry {
public:
    using PluginFactory = std::function<std::unique_ptr<IThemisPlugin>()>;
    
    /**
     * @brief Register a plugin factory
     * @param name Plugin name
     * @param type Plugin type
     * @param factory Factory function
     */
    static void registerFactory(
        const std::string& name,
        PluginType type,
        PluginFactory factory
    );
    
    /**
     * @brief Create plugin from factory
     * @param name Plugin name
     * @return Plugin instance or nullptr
     */
    static std::unique_ptr<IThemisPlugin> createPlugin(const std::string& name);
    
    /**
     * @brief Get singleton instance
     */
    static PluginManagerRegistry& instance();
    
private:
    std::unordered_map<std::string, std::pair<PluginType, PluginFactory>> factories_;
    mutable std::mutex mutex_;
};

/**
 * @brief Helper class for automatic plugin registration
 * 
 * Usage:
 * ```cpp
 * // In plugin implementation
 * static PluginRegistrar<MyPlugin> registrar("my_plugin", PluginType::BLOB_STORAGE);
 * ```
 */
template<typename PluginClass>
class PluginManagerRegistrar {
public:
    PluginManagerRegistrar(const std::string& name, PluginType type) {
        PluginManagerRegistry::registerFactory(
            name,
            type,
            []() { return std::make_unique<PluginClass>(); }
        );
    }
};

} // namespace plugins
} // namespace themis
