/**
 * @file plugin_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "plugins/plugins_api_contract.h"
#include "plugins/plugin_interface.h"
#include "plugins/plugin_metrics.h"
#include "plugins/plugin_dependency_resolver.h"  // Dependency resolution
#include "plugins/plugin_hot_plug_monitor.h"  // HotPlugConfig definition
#include "plugins/oci_registry_client.h"  // OCI registry pull support
#include "plugins/signed_plugin_repository.h"  // Signed edition-upgrade plugins
#include "acceleration/plugin_loader.h"  // Reuse existing loader
#include "themis/edition.h"
#include "themis/runtime_license_gate.h"
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

struct PluginManagerTestAccess;

// Forward declarations
class PluginHotPlugMonitor;
struct HotPlugConfig;
class PluginHealthMonitor;

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
    friend struct PluginManagerTestAccess;

    struct PluginEntry {
        std::string name;
        PluginType type;
        std::string path;
        PluginManifest manifest;
        void* library_handle = nullptr;
        std::unique_ptr<IThemisPlugin> instance;
        bool loaded = false;
        std::string file_hash;

        /// Capabilities snapshot captured immediately after plugin initialization.
        /// Used by checkCapabilityEscalation() to detect post-load capability expansion.
        PluginCapabilities frozen_capabilities;

        /// Set to true when checkCapabilityEscalation() detects a superset violation.
        /// A restricted plugin remains loaded but is flagged for operator review.
        bool is_restricted = false;

        /// Phase 2A: Lifecycle state tracking for plugin instance
        /// Enforces state machine transitions during load/unload/reload operations
        PluginLifecycleState state = PluginLifecycleState::UNLOADED;

        /// Mutex protecting state transitions for thread-safe lifecycle management
        mutable std::mutex state_mutex;

        PluginEntry() = default;
        PluginEntry(PluginEntry&& other) noexcept
            : name(std::move(other.name))
            , type(other.type)
            , path(std::move(other.path))
            , manifest(std::move(other.manifest))
            , library_handle(other.library_handle)
            , instance(std::move(other.instance))
            , loaded(other.loaded)
            , file_hash(std::move(other.file_hash))
            , frozen_capabilities(other.frozen_capabilities)
            , is_restricted(other.is_restricted)
            , state(other.state)
            // state_mutex is default-constructed; each entry owns its own mutex
        {}
        PluginEntry& operator=(PluginEntry&& other) noexcept {
            if (this != &other) {
                name             = std::move(other.name);
                type             = other.type;
                path             = std::move(other.path);
                manifest         = std::move(other.manifest);
                library_handle   = other.library_handle;
                instance         = std::move(other.instance);
                loaded           = other.loaded;
                file_hash        = std::move(other.file_hash);
                frozen_capabilities = other.frozen_capabilities;
                is_restricted    = other.is_restricted;
                state            = other.state;
                // state_mutex is not moved; each entry owns its own mutex
            }
            return *this;
        }
        PluginEntry(const PluginEntry&) = delete;
        PluginEntry& operator=(const PluginEntry&) = delete;
    };
    
    std::unordered_map<std::string, PluginEntry> plugins_;  // name -> entry
    std::unordered_map<PluginType, std::vector<std::string>> type_index_;  // type -> plugin names
    PluginMetrics metrics_;  // Plugin metrics tracker
    std::unique_ptr<PluginHotPlugMonitor> hot_plug_monitor_;  // Hot-plug filesystem monitor
    std::vector<PluginReloadListener> reload_listeners_;  // Reload event listeners
    PluginHealthMonitor* health_monitor_ = nullptr;  // Optional health monitor (non-owning)
    mutable std::mutex mutex_;
    
    // Reuse existing platform-specific loading from acceleration/plugin_loader.cpp
    /**
     * @brief Load a shared library from disk.
     * @param path Absolute or relative path to the plugin binary.
     * @return Native library handle on success, nullptr on failure.
     * @note Thread-safe under PluginManager external locking discipline.
     */
    void* loadLibrary(const std::string& path);

    /**
     * @brief Resolve an exported symbol from a loaded library handle.
     * @param handle Native library handle returned by loadLibrary().
     * @param symbolName Exported symbol name to resolve.
     * @return Pointer to the resolved symbol, or nullptr if not found.
     */
    void* getSymbol(void* handle, const std::string& symbolName);

    /**
     * @brief Unload a previously loaded library handle.
     * @param handle Native library handle. nullptr is ignored.
     */
    void unloadLibrary(void* handle);
    
    // Manifest loading (with QW-43 path traversal guards)
    /// @brief Load and validate a plugin manifest from JSON file.
    /// @note Includes fail-closed guards (QW-43) for path traversal in plugin names.
    /// @return nullopt if validation fails or manifest is malformed; manifest otherwise.
    std::optional<PluginManifest> loadManifest(const std::string& manifest_path);
    
    // Phase 2C: Unified validation logic
    /**
     * @brief Validate a plugin manifest and binary for load operations.
     * 
     * This function implements a 4-stage validation contract:
     * 1. Manifest schema validation (required fields, types)
     * 2. Manifest semantic validation (constraints, dependencies)
     * 3. Signature verification (detached signature checks)
     * 4. Capability validation (required capabilities available)
     * 
     * Fail-safe semantics: If any stage fails, the plugin remains in UNLOADED state
     * and no partial activation occurs.
     * 
     * @param manifest The plugin manifest to validate.
     * @param manifest_path Path to the manifest file (for signature verification).
     * @param plugin_binary_path Path to the plugin binary (for binary verification).
     * @param error_details Output error description when validation fails.
     * @return PluginsError::kSuccess on success, or appropriate error code on failure.
     * @see src/plugins/ROADMAP.md — Phase 2C implementation
     */
    PluginsError validatePluginForLoad(
        const PluginManifest& manifest,
        const std::string& manifest_path,
        const std::string& plugin_binary_path,
        std::string& error_details
    );
    
    /**
     * @brief Validate edition restrictions and license gates (Wave C Batch 2)
     * 
     * Fail-closed validation of edition allowlists and license feature gates.
     * 
     * Behavior:
     * - If manifest.allowed_editions is non-empty AND current edition not in list → FAIL
     * - If manifest.license_feature is set AND license gate returns false → FAIL
     * - Returns detailed error codes (PLUGIN_EDITION_MISMATCH, PLUGIN_LICENSE_DENIED, etc.)
     * 
     * @param manifest Plugin manifest to validate
     * @param error_details Output error message on failure
     * @return ManifestErrorCode::MANIFEST_OK on success, or error code
     */
    ManifestErrorCode validateManifestEditionRestrictions(
        const PluginManifest& manifest,
        std::string& error_details
    );
    
    /**
     * @brief Validate public/private boundary constraints (Wave C Batch 2)
     * 
     * Enforces edition and visibility boundary rules:
     * - If visibility="private" AND current edition="community" → FAIL-CLOSED
     * - If plugin_path contains "private/" BUT visibility!="private" → WARN/BLOCK
     * - If visibility="restricted" AND no scoped-checkout context → FAIL-CLOSED
     * 
     * @param manifest Plugin manifest to validate
     * @param plugin_path Filesystem path to the plugin
     * @param error_details Output error message on failure
     * @return ManifestErrorCode::MANIFEST_OK on success, or error code
     */
    ManifestErrorCode validateManifestPublicPrivateBoundary(
        const PluginManifest& manifest,
        const std::string& plugin_path,
        std::string& error_details
    );
    
    // Manifest signature verification
    /**
     * @brief Verify the detached signature for a plugin manifest.
     * @param manifest_path Path to plugin.json manifest.
     * @param error_message Output error detail when verification fails.
     * @return true if signature policy is satisfied for the current build mode.
     * @note Production builds require a valid `.sig` file; development builds warn and continue.
     */
    bool verifyManifestSignature(const std::string& manifest_path, std::string& error_message);
    
    // Security verification (reuse acceleration/plugin_security.h)
    /**
     * @brief Validate plugin binary against configured security policy.
     * @param path Path to plugin shared library.
     * @param error_message Output error detail on verification failure.
     * @return true if the plugin passes policy checks, false otherwise.
     */
    bool verifyPlugin(const std::string& path, std::string& error_message);
    
    /**
     * @brief Compute SHA-256 hash for a file.
     * @param path File path to hash.
     * @return Lower-case hex digest string, or empty string on I/O/crypto failure.
     */
    std::string calculateFileHash(const std::string& path);
    
    // Hot-reload helper methods
    /**
     * @brief Find loaded plugins that depend on @p name.
     * @param name Plugin name to resolve reverse dependencies for.
     * @return Dependent plugin names in unspecified order.
     */
    std::vector<std::string> findDependentPlugins(const std::string& name) const;

    /**
     * @brief Notify registered reload listeners for a plugin lifecycle phase.
     * @param name Plugin name associated with the event.
     * @param phase Reload phase being emitted.
     */
    void notifyPluginReload(const std::string& name, PluginReloadPhase phase);
     
    // Phase 3: Error Handling and Edge Cases
    /**
     * @brief Detect and handle concurrent load/unload attempts on the same plugin.
     * 
     * Validates that a plugin is not in a transition state (LOADING/UNLOADING) before
     * allowing new operations. Returns appropriate error codes for concurrent attempts.
     * 
     * @param plugin_entry Reference to the plugin entry to check
     * @param requested_state Target state for the operation
     * @return PluginsError::kSuccess if transition is allowed, error code otherwise
     */
    PluginsError validateConcurrentStateChange(const PluginEntry& plugin_entry, PluginLifecycleState requested_state);
     
    /**
     * @brief Recover from partial registry state after failed plugin operations.
     * 
     * When a plugin load/unload fails mid-operation, this function restores the registry
     * to a consistent state by rolling back the partial changes and logging diagnostics.
     * 
     * @param plugin_name Name of the plugin with partial state
     * @return PluginsError::kSuccess on successful recovery
     */
    PluginsError recoverPartialRegistryState(const std::string& plugin_name);
     
    /**
     * @brief Handle manifest with missing optional fields gracefully.
     * 
     * Some manifest fields like allowed_editions and license_feature are optional.
     * This function validates and applies defaults when fields are missing.
     * 
     * @param manifest Manifest to validate and correct
     * @return PluginsError::kSuccess if manifest is valid after correction
     */
    PluginsError validateManifestOptionalFields(PluginManifest& manifest);
     
    /**
     * @brief Detect ABI compatibility issues during hot-reload.
     * 
     * Checks for ABI incompatibilities when reloading a plugin that was previously loaded.
     * Detects changes in plugin interface version, symbol availability, and calling conventions.
     * 
     * @param previous_entry Previous plugin entry before reload
     * @param new_manifest Manifest of the new plugin version
     * @return PluginsError::kSuccess if ABI is compatible
     */
    PluginsError validateABICompatibility(const PluginEntry& previous_entry, const PluginManifest& new_manifest);
     
    /**
     * @brief Implement timeout handling for long-running signature verification.
     * 
     * Signature verification on large files or with slow crypto hardware can hang.
     * This function implements configurable timeout with graceful degradation.
     * 
     * @param manifest_path Path to manifest to verify
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @param error_details Output error message on timeout
     * @return true if verification succeeded within timeout
     */
    bool verifyManifestSignatureWithTimeout(
        const std::string& manifest_path,
        uint32_t timeout_ms,
        std::string& error_details
    );
     
    /**
     * @brief Get diagnostic information about a plugin's error state.
     * 
     * Produces structured diagnostic output including lifecycle state, error history,
     * last error code, and recovery options.
     * 
     * @param plugin_name Name of the plugin to diagnose
     * @return JSON object with diagnostic information
     */
    json getDiagnosticsForPlugin(const std::string& plugin_name) const;
     
    /**
     * @brief Format error messages with consistent tagging for operator logs.
     * 
     * All error messages should be tagged with [VALIDATION:*], [LIFECYCLE:*], [SECURITY:*]
     * to enable structured logging and filtering.
     * 
     * @param error_code PluginsError code
     * @param context Additional context about the error
     * @param plugin_name Name of the affected plugin
     * @return Formatted error message with appropriate tags
     */
    static std::string formatDiagnosticMessage(
        PluginsError error_code,
        const std::string& context,
        const std::string& plugin_name = ""
    );
     
private:
    /// Error state tracking for diagnostics
    struct PluginErrorState {
        PluginsError last_error = PluginsError::kSuccess;
        std::string last_error_message;
        std::chrono::system_clock::time_point last_error_time;
        int error_count = 0;
    };
     
    std::unordered_map<std::string, PluginErrorState> error_states_;  ///< Track error state per plugin
     
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
     * 
     * @note Fail-closed guards (QW-43): Validates plugin names against path traversal attacks.
     * Rejects plugin names containing directory separators (/, \), path traversal (..), 
     * absolute paths (C:\, /etc/), or special characters. Only alphanumeric, underscore (_),
     * and hyphen (-) are permitted. Manifests with invalid names are rejected (fail-closed).
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
     * @brief Pull a plugin from an OCI registry and load it.
     *
     * Parses the OCI reference, downloads the plugin binary layer into
     * @p cache_dir (reusing a cached copy when the SHA-256 digest matches),
     * verifies the binary, and delegates to loadPluginFromPath().
     *
     * OCI reference format: [registry/]name[:tag][\@digest]
     * Example: "ghcr.io/themisdb/plugins/s3_blob:1.2.0"
     *
     * @param oci_ref   OCI image reference string.
     * @param cache_dir Local directory for cached plugin binaries.
     *                  Defaults to the system temp directory when empty.
     * @param auth_token Optional Bearer token for authenticated registries.
     * @return Result<IThemisPlugin*> with loaded plugin or error.
     */
    Result<IThemisPlugin*> loadPluginFromOci(
        const std::string& oci_ref,
        const std::string& cache_dir   = "",
        const std::string& auth_token  = ""
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
     * @brief Hot-reload a plugin without server restart (atomic with rollback)
     *
     * Reload process:
     *  1. Validate plugin is loaded; fail if dependents exist
     *  2. Save IStatefulPlugin state (if applicable)
     *  3. Verify new binary (hash + signature) — old plugin still running
     *  4. Load new binary, create instance, initialize, restore state
     *  5. If any of the above fail: return error, old plugin continues unaffected
     *  6. Atomically swap old entry for new entry under mutex
     *  7. Notify BEFORE_UNLOAD / AFTER_UNLOAD listeners
     *  8. Shutdown and unload old binary (outside mutex)
     *  9. Notify AFTER_LOAD listeners
     *
     * @param name Plugin name
     * @return Result<void> - success or error; old plugin remains running on failure
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
     * @brief Negotiate capabilities between a loaded plugin and a set of requirements.
     *
     * Checks that the named plugin is loaded, then delegates to
     * PluginCapabilityNegotiator::negotiate() to verify each requirement against
     * the plugin's capability flags and version.
     *
     * @param name         Name of the loaded plugin.
     * @param requirements List of capability requirements (name + optional version range).
     * @return PluginNegotiationResult with success flag and per-requirement details.
     *         If the plugin is not found/loaded, success is false and error_message is set.
     */
    PluginNegotiationResult negotiateCapabilities(
        const std::string& name,
        const std::vector<PluginCapabilityRequirement>& requirements) const;

    /**
     * @brief Check whether a loaded plugin has escalated its capabilities beyond
     *        what was declared in the manifest at load time.
     *
     * The capabilities returned by the plugin's getCapabilities() are compared
     * against the snapshot frozen at load time.  If the current capabilities are
     * a strict superset (i.e. any flag that was false at load is now true), the
     * plugin is marked RESTRICTED and ERR_PLUGIN_CAPABILITY_ESCALATION is returned.
     *
     * This method is a no-op on the hot call path: capabilities are only checked
     * when this method is explicitly called (e.g. from a periodic security scan or
     * after an explicit re-negotiation request).
     *
     * @param name  Name of the loaded plugin.
     * @return      Ok(void) if no escalation is detected.
     *              Err(ERR_PLUGIN_CAPABILITY_ESCALATION) if escalation is detected;
     *              the plugin is also marked as RESTRICTED in the registry.
     *              Err(ERR_PLUGIN_NOT_FOUND) if the plugin is not loaded.
     */
    Result<void> checkCapabilityEscalation(const std::string& name);

    /**
     * @brief Query whether a plugin has been marked RESTRICTED due to a capability
     *        escalation attempt.
     *
     * @param name  Plugin name.
     * @return      true if the plugin exists, is loaded, and has been marked
     *              RESTRICTED; false in all other cases.
     */
    bool isPluginRestricted(const std::string& name) const;
    
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
     * @brief Attach a health monitor to observe loaded self-healing plugins.
     *
     * When set, every plugin that implements ISelfHealingPlugin is automatically
     * registered with the monitor on load and unregistered on unload.
     *
     * @param monitor Pointer to an existing PluginHealthMonitor (non-owning).
     *                Pass nullptr to detach the current monitor.
     * @note Thread-safe: Can be called from any thread
     */
    void attachHealthMonitor(PluginHealthMonitor* monitor);
    
    /**
     * @brief Singleton instance
     */
    static PluginManager& instance();

    // -----------------------------------------------------------------------
    // Edition / License gating helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Check whether the running edition supports plugins (compile-time gate).
     * @return true for Enterprise / Hyperscaler editions.
     */
    static bool isEditionSupported();

    /**
     * @brief Check whether the runtime license allows the enterprise_plugins feature.
     * @return true when the runtime license grants plugin loading.
     */
    static bool isLicensed();

    /**
     * @brief Human-readable error message for Community-edition plugin load attempts.
     */
    static std::string communityUnavailableMessage(const std::string& plugin_name);

    /**
     * @brief Returns marketplace availability info for the running edition.
     */
    static std::string marketplaceInfo();

    /**
     * @brief Returns installation instructions, gated by edition.
     */
    static std::string installationInstructions();
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
