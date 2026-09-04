/**
 * @file hot_reload_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.21
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Hot-reload manager for ThemisDB plugins and modules.
//
// Provides atomic module replacement, state preservation, and rollback
// capabilities so plugins can be updated without a database restart.
//
// See src/base/ROADMAP.md – Phase 2: Dynamic Loading & Dependency Management

#pragma once

#include "themis/base/module_loader.h"
#include "themis/base/module_sandbox.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>
#include <unordered_map>

namespace themis {
namespace modules {

// =============================================================================
// ModuleVersion – lightweight semantic version extracted from a loaded module
// =============================================================================

/**
 * @brief Version information for a loaded module.
 */
struct ModuleVersion {
    std::string version;     ///< Semantic version string (e.g. "1.2.3")
    std::string abiVersion;  ///< ABI version token
    std::string buildId;     ///< Build identifier / commit hash
    uint32_t major = 0;
    uint32_t minor = 0;
    uint32_t patch = 0;

    bool operator==(const ModuleVersion& o) const noexcept {
        return major == o.major && minor == o.minor && patch == o.patch
               && version == o.version;
    }
    bool operator!=(const ModuleVersion& o) const noexcept { return !(*this == o); }

    /// @return "major.minor.patch" or version string, whichever is available.
    std::string toString() const {
        if (!version.empty()) {
          return version;
        }
        return std::to_string(major) + "." + std::to_string(minor) + "." +
               std::to_string(patch);
    }

    /// @brief Construct from ModuleMetadata.
    static ModuleVersion fromMetadata(const ModuleMetadata& m) {
        ModuleVersion v;
        v.version    = m.version;
        v.abiVersion = m.abiVersion;
        v.buildId    = m.buildId;
        v.major      = m.themisMajor;
        v.minor      = m.themisMinor;
        v.patch      = m.themisPatch;
        return v;
    }
};

// =============================================================================
// HotReloadResult – outcome of a reload or rollback operation
// =============================================================================

/**
 * @brief Result of a hot-reload or rollback operation.
 */
struct HotReloadResult {
    bool        success          = false;
    std::string errorMessage;
    std::string previousVersion; ///< Version before the reload
    std::string newVersion;      ///< Version after the reload (empty on failure)
    bool        rollbackAvailable = false; ///< Whether a rollback is now possible
    uint64_t    reloadDurationMs  = 0;     ///< Wall-clock time for the operation
};

// =============================================================================
// HotReloadManager – atomic plugin hot-swap with rollback
// =============================================================================

/**
 * @brief Manages hot-reloading of ThemisDB modules without a database restart.
 *
 * Key features:
 *  - **Atomic swap**: the old module handle stays alive until the new one is
 *    fully initialised; failure keeps the old module running.
 *  - **State preservation**: optional save/restore callbacks allow stateful
 *    plugins to survive a reload.
 *  - **Rollback**: one backup slot is kept per module; calling rollback()
 *    re-activates the previous version.
 *  - **Reload notifications**: registered callbacks are invoked at each phase
 *    (BEFORE_UNLOAD, AFTER_UNLOAD, AFTER_LOAD, ROLLBACK).
 *
 * Thread safety: all public methods are thread-safe. Read-only queries hold
 * a shared lock (std::shared_lock) to allow concurrent readers; write
 * operations (reloadModule, rollback, registration) hold an exclusive lock
 * (std::unique_lock) on the internal std::shared_mutex.
 *
 * Typical usage:
 * @code
 *   HotReloadManager mgr;
 *   mgr.registerModule("themis_storage", loader);
 *
 *   // Later, when a new build arrives on disk:
 *   auto result = mgr.reloadModule("themis_storage", "/path/to/new.so");
 *   if (!result.success) {
 *       LOG_ERROR("Reload failed: {}", result.errorMessage);
 *   }
 *
 *   // If something breaks, roll back:
 *   mgr.rollback("themis_storage");
 * @endcode
 */
class HotReloadManager {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * @brief Configuration options for HotReloadManager.
     */
    struct Config {
        bool     verifySignature    = true;  ///< Verify new module before swap
        bool     preserveState      = true;  ///< Invoke state save/restore if set
        bool     enableRollback     = true;  ///< Retain backup for rollback

        /// Optional resource limits applied to every module loaded by this manager.
        /// When set, each successfully loaded module is wrapped in a ModuleSandbox
        /// with the given configuration (memory cap, CPU share, etc.).
        /// When empty (default), no sandboxing is applied.
        std::optional<ModuleSandbox::Config> sandboxConfig;
    };

    // -------------------------------------------------------------------------
    // Reload phase enumeration (for callbacks)
    // -------------------------------------------------------------------------

    /**
     * @brief Lifecycle phase emitted during a hot-reload or rollback.
     */
    enum class ReloadPhase {
        BEFORE_UNLOAD, ///< Old module about to be unloaded
        AFTER_UNLOAD,  ///< Old module unloaded; new one not yet live
        AFTER_LOAD,    ///< New module live (reload succeeded)
        ROLLBACK       ///< Rollback completed
    };

    /// @brief Callback invoked at each reload phase.
    using ReloadCallback =
        std::function<void(const std::string& module_name, ReloadPhase phase)>;

    /// @brief Optional callback to save module state before unload.
    /// Returns serialised state string (empty = nothing to save).
    using StateSaveCallback =
        std::function<std::string(const std::string& module_name)>;

    /// @brief Optional callback to restore state after load.
    /// Returns true on success.
    using StateRestoreCallback =
        std::function<bool(const std::string& module_name, const std::string& state)>;

    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    HotReloadManager();
    explicit HotReloadManager(const Config& config);
    ~HotReloadManager();

    HotReloadManager(const HotReloadManager&)            = delete;
    HotReloadManager& operator=(const HotReloadManager&) = delete;

    // -------------------------------------------------------------------------
    // Module registration
    // -------------------------------------------------------------------------

    /**
     * @brief Register a module so it can be hot-reloaded later.
     *
     * @param module_name  Logical name (e.g. "themis_storage").
     * @param loader       The ModuleLoader that currently owns the module.
     */
    void registerModule(const std::string& module_name, ModuleLoader& loader);

    /**
     * @brief Unregister a module and free any backup slots.
     * @param module_name Logical name previously passed to registerModule().
     */
    void unregisterModule(const std::string& module_name);

    // -------------------------------------------------------------------------
    // Core hot-reload operations
    // -------------------------------------------------------------------------

    /**
     * @brief Hot-reload a module from a new path without stopping the database.
     *
     * Steps:
     *  1. Optionally save state (if StateSaveCallback is set).
     *  2. Emit BEFORE_UNLOAD.
     *  3. Load new binary via the registered ModuleLoader.
     *  4. On success: unload old binary, emit AFTER_UNLOAD, emit AFTER_LOAD,
     *     optionally restore state.
     *  5. On failure: keep old binary running, return error result.
     *
     * @param module_name  Logical module name (must be registered).
     * @param new_path     Filesystem path to the updated binary.
     * @return HotReloadResult describing outcome, duration, and versions.
     */
    HotReloadResult reloadModule(const std::string& module_name,
                                 const std::string& new_path);

    /**
     * @brief Roll back to the previous version of a module.
     *
     * Only available when enableRollback=true and a successful reload has
     * already been performed (making the prior version the backup).
     *
     * @param module_name Logical module name.
     * @return HotReloadResult describing rollback outcome.
     */
    HotReloadResult rollback(const std::string& module_name);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get the current version of a registered module.
     * @return Version info, or std::nullopt if the module is not registered
     *         or not currently loaded.
     */
    std::optional<ModuleVersion> getCurrentVersion(
        const std::string& module_name) const;

    /**
     * @brief Check whether a rollback is available for a module.
     */
    bool isRollbackAvailable(const std::string& module_name) const;

    /**
     * @brief Return the list of all registered module names.
     */
    std::vector<std::string> registeredModules() const;

    /**
     * @brief Get sandbox resource-usage statistics for a loaded module.
     *
     * @return SandboxStats sampled from the active sandbox, or std::nullopt if
     *         sandboxing is not configured for this manager, or the module is
     *         not registered / not currently sandboxed.
     */
    std::optional<SandboxStats> getSandboxStats(
        const std::string& module_name) const;

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    void setStateSaveCallback(StateSaveCallback cb);
    void setStateRestoreCallback(StateRestoreCallback cb);

    void addReloadCallback(ReloadCallback cb);
    void clearReloadCallbacks();

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    /**
     * @brief Cumulative reload statistics.
     */
    struct Stats {
        uint64_t totalReloads       = 0;
        uint64_t successfulReloads  = 0;
        uint64_t failedReloads      = 0;
        uint64_t rollbacks          = 0;
        uint64_t statesSaved        = 0;
        uint64_t statesRestored     = 0;
    };

    Stats getStats() const;
    void  resetStats();

private:
    // -------------------------------------------------------------------------
    // Internal state per registered module
    // -------------------------------------------------------------------------

    struct ModuleSlot {
        std::string  name;
        ModuleLoader* loader = nullptr;        ///< Non-owning ptr to the loader
        uint64_t registration_id = 0;          ///< Monotonic id to detect rebinds

        // Current live version
        std::string  current_path;
        ModuleVersion current_version;

        // Backup slot for rollback (holds the *path* of the previous binary)
        bool         has_backup     = false;
        std::string  backup_path;
        ModuleVersion backup_version;

        // Active resource-limit sandbox (null when sandboxing is not configured)
        std::unique_ptr<ModuleSandbox> sandbox;
    };

    Config config_;
    mutable std::shared_mutex mutex_;

    std::unordered_map<std::string, ModuleSlot> slots_;
    uint64_t next_registration_id_ = 1;

    StateSaveCallback    state_save_cb_;
    StateRestoreCallback state_restore_cb_;
    std::vector<ReloadCallback> reload_cbs_;

    Stats stats_;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    void notify(const std::string& name, ReloadPhase phase);
    std::string saveState(const std::string& name);
    bool restoreState(const std::string& name, const std::string& state);

    static ModuleVersion versionFromLoader(ModuleLoader& loader,
                                           const std::string& module_name);
};

} // namespace modules
} // namespace themis
