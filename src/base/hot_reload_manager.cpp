/**
 * @file hot_reload_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.21
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Hot-reload manager implementation.
//
// See include/themis/base/hot_reload_manager.h for the public API.

#include "themis/base/hot_reload_manager.h"
#include <stdexcept>

#include <chrono>
#include <shared_mutex>
#include <spdlog/spdlog.h>

namespace themis {
namespace modules {

// =============================================================================
// Constructor / Destructor
// =============================================================================

HotReloadManager::HotReloadManager() : config_{}, stats_{} {}

HotReloadManager::HotReloadManager(const Config &config) : config_(config), stats_{} {}

HotReloadManager::~HotReloadManager() = default;

// =============================================================================
// Module registration
// =============================================================================

void HotReloadManager::registerModule(const std::string &module_name, ModuleLoader &loader) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto &slot  = slots_[module_name];
    slot.name   = module_name;
    slot.loader = &loader;
    slot.registration_id = next_registration_id_++;

    // Record the current version if the module is already loaded.
    auto info = loader.getModuleInfo(module_name);
    if (info.has_value()) {
        slot.current_path    = info->path;
        slot.current_version = ModuleVersion::fromMetadata(info->metadata);
    }

    spdlog::info("HotReloadManager: registered module '{}'", module_name);
}

void HotReloadManager::unregisterModule(const std::string &module_name) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    slots_.erase(module_name);
    spdlog::info("HotReloadManager: unregistered module '{}'", module_name);
}

// =============================================================================
// reloadModule – core hot-swap logic
// =============================================================================

HotReloadResult HotReloadManager::reloadModule(const std::string &module_name, const std::string &new_path) {
    auto wall_start = std::chrono::steady_clock::now();

    HotReloadResult result;
    result.rollbackAvailable = false;

    // --- Validate registration under lock --------------------------------
    ModuleLoader *loader_ptr = nullptr;
    std::string prior_path;
    ModuleVersion prior_version;
    uint64_t registration_id = 0;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = slots_.find(module_name);
        if (it == slots_.end()) {
            result.errorMessage = "Module '" + module_name + "' is not registered with HotReloadManager";
            spdlog::error("HotReloadManager::reloadModule: {}", result.errorMessage);
            stats_.totalReloads++;
            stats_.failedReloads++;
            return result;
        }
        loader_ptr    = it->second.loader;
        prior_path    = it->second.current_path;
        prior_version = it->second.current_version;
        registration_id = it->second.registration_id;
        if (!loader_ptr) {
            result.errorMessage = "Module '" + module_name + "' has a null loader";
            spdlog::error("HotReloadManager::reloadModule: {}", result.errorMessage);
            stats_.totalReloads++;
            stats_.failedReloads++;
            return result;
        }
    }

    result.previousVersion = prior_version.toString();

    // --- Save state before unloading (optional) ---------------------------
    std::string saved_state;
    if (config_.preserveState) {
        saved_state = saveState(module_name);
        if (!saved_state.empty()) {
            spdlog::debug("HotReloadManager: saved state for '{}' ({} bytes)", module_name, saved_state.size());
        }
    }

    // --- Phase: BEFORE_UNLOAD --------------------------------------------
    notify(module_name, ReloadPhase::BEFORE_UNLOAD);

    // --- Atomically load the new binary BEFORE unloading the old one ------
    // This ensures the old module keeps running if the new one fails to load.
    const std::string new_module_key = module_name + "__hot_reload_candidate__";

    auto load_result = loader_ptr->loadModule(new_path, new_module_key);
    if (!load_result.success) {
        result.errorMessage = "Failed to load new binary '" + new_path + "': " + load_result.errorMessage;
        spdlog::error("HotReloadManager::reloadModule: {}", result.errorMessage);
        // Old module is still live – do not unload.
        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            stats_.totalReloads++;
            stats_.failedReloads++;
        }
        return result;
    }

    // Capture new version before we remove the candidate entry.
    ModuleVersion new_version;
    {
        auto new_info = loader_ptr->getModuleInfo(new_module_key);
        if (new_info.has_value()) {
            new_version = ModuleVersion::fromMetadata(new_info->metadata);
        } else {
            // Fallback: mark with path only.
            new_version.version = new_path;
        }
    }

    // --- Unload old binary -----------------------------------------------
    loader_ptr->unloadModule(module_name);
    notify(module_name, ReloadPhase::AFTER_UNLOAD);

    // --- Rename candidate entry to the canonical name ---------------------
    // The loader tracks modules by name; we loaded under a temp key.
    // Unload the temp key and re-load directly under the real name so that
    // callers can still call loader.getModuleInfo(module_name).
    loader_ptr->unloadModule(new_module_key);

    auto final_result = loader_ptr->loadModule(new_path, module_name);
    if (!final_result.success) {
        // Extremely unlikely but handle: new binary disappeared between the
        // two loads.  Attempt to reload the old binary from the backup path.
        result.errorMessage = "Final load of new binary failed: " + final_result.errorMessage;
        spdlog::error("HotReloadManager::reloadModule: {}", result.errorMessage);

        if (!prior_path.empty()) {
            spdlog::warn("HotReloadManager: attempting emergency restore of '{}'", prior_path);
            loader_ptr->loadModule(prior_path, module_name);
        }

        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            stats_.totalReloads++;
            stats_.failedReloads++;
        }
        return result;
    }

    // --- Preserve backup for rollback ------------------------------------
    // --- Launch sandbox for the new module (if configured) ---------------
    // Replacing slot.sandbox drops the old sandbox, calling ~ModuleSandbox()
    // which invokes shutdown() automatically.
    std::unique_ptr<ModuleSandbox> new_sandbox;
    if (config_.sandboxConfig) {
        new_sandbox = std::make_unique<ModuleSandbox>(*config_.sandboxConfig);
        if (!new_sandbox->launch(module_name)) {
            spdlog::warn("HotReloadManager: sandbox launch warning for '{}': {}", module_name,
                         new_sandbox->lastError());
        }
        for (const auto &w : new_sandbox->launchWarnings()) {
            spdlog::debug("HotReloadManager: sandbox [{}]: {}", module_name, w);
        }
        // sandbox is committed to the module slot under lock below.
        // Keep local until slot re-validation is complete.
    }

    // Update slot metadata under lock. The module can be unregistered while
    // reload I/O is running, so re-validate ownership before mutating state.
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = slots_.find(module_name);
        if (it == slots_.end()
            || it->second.loader != loader_ptr
            || it->second.registration_id != registration_id) {
            result.errorMessage = "Module '" + module_name
                                  + "' was unregistered/rebound during reload; aborting";
            spdlog::warn("HotReloadManager::reloadModule: {}", result.errorMessage);
            stats_.totalReloads++;
            stats_.failedReloads++;
            lock.unlock();
            loader_ptr->unloadModule(module_name);
            return result;
        }

        ModuleSlot &slot = it->second;
        if (config_.enableRollback && !prior_path.empty()) {
            slot.has_backup     = true;
            slot.backup_path    = prior_path;
            slot.backup_version = prior_version;
        }

        slot.current_path    = new_path;
        slot.current_version = new_version;
        slot.sandbox         = std::move(new_sandbox);
        result.rollbackAvailable = slot.has_backup;
    }

    // --- Restore state (optional) ----------------------------------------
    if (config_.preserveState && !saved_state.empty()) {
        bool restored = restoreState(module_name, saved_state);
        if (!restored) {
            spdlog::warn("HotReloadManager: state restore failed for '{}'", module_name);
        }
    }

    // --- Phase: AFTER_LOAD -----------------------------------------------
    notify(module_name, ReloadPhase::AFTER_LOAD);

    auto wall_end = std::chrono::steady_clock::now();
    result.reloadDurationMs
        = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(wall_end - wall_start).count());

    result.success           = true;
    result.newVersion        = new_version.toString();

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        stats_.totalReloads++;
        stats_.successfulReloads++;
    }

    spdlog::info("HotReloadManager: '{}' hot-reloaded in {}ms ({} -> {})", module_name, result.reloadDurationMs,
                 result.previousVersion, result.newVersion);
    return result;
}

// =============================================================================
// rollback
// =============================================================================

HotReloadResult HotReloadManager::rollback(const std::string &module_name) {
    auto wall_start = std::chrono::steady_clock::now();

    HotReloadResult result;

    // Validate registration and extract backup info under lock.
    ModuleLoader *loader_ptr = nullptr;
    std::string backup_path;
    ModuleVersion backup_version;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto it = slots_.find(module_name);
        if (it == slots_.end()) {
            result.errorMessage = "Module '" + module_name + "' is not registered";
            spdlog::error("HotReloadManager::rollback: {}", result.errorMessage);
            return result;
        }

        ModuleSlot &slot = it->second;

        if (!slot.has_backup || slot.backup_path.empty()) {
            result.errorMessage = "No rollback available for module '" + module_name + "'";
            spdlog::warn("HotReloadManager::rollback: {}", result.errorMessage);
            return result;
        }

        result.previousVersion = slot.current_version.toString();
        loader_ptr             = slot.loader;
        backup_path            = slot.backup_path;
        backup_version         = slot.backup_version;
        if (!loader_ptr) {
            result.errorMessage = "Module '" + module_name + "' has a null loader";
            spdlog::error("HotReloadManager::rollback: {}", result.errorMessage);
            return result;
        }
    }

    // Perform the unload/load outside the mutex to avoid holding the lock
    // while doing potentially slow I/O and to prevent deadlock with notify().
    loader_ptr->unloadModule(module_name);

    auto load_result = loader_ptr->loadModule(backup_path, module_name);
    if (!load_result.success) {
        result.errorMessage = "Failed to restore backup binary '" + backup_path + "': " + load_result.errorMessage;
        spdlog::error("HotReloadManager::rollback: {}", result.errorMessage);
        return result;
    }

    // Launch sandbox for the restored module (if configured).
    // Created outside the lock (same pattern as reloadModule) for consistency.
    std::unique_ptr<ModuleSandbox> rollback_sandbox;
    if (config_.sandboxConfig) {
        rollback_sandbox = std::make_unique<ModuleSandbox>(*config_.sandboxConfig);
        if (!rollback_sandbox->launch(module_name)) {
            spdlog::warn("HotReloadManager: sandbox launch warning for rollback '{}': {}", module_name,
                         rollback_sandbox->lastError());
        }
        for (const auto &w : rollback_sandbox->launchWarnings()) {
            spdlog::debug("HotReloadManager: sandbox rollback [{}]: {}", module_name, w);
        }
    }

    // Update slot metadata under lock.
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = slots_.find(module_name);
        if (it != slots_.end()) {
            ModuleSlot &slot     = it->second;
            slot.current_path    = backup_path;
            slot.current_version = backup_version;
            slot.has_backup      = false;
            slot.backup_path.clear();
            slot.backup_version = {};
            // Replacing sandbox drops the old one (auto-shutdown via ~ModuleSandbox).
            slot.sandbox = std::move(rollback_sandbox);
        }
        stats_.rollbacks++;
    }

    // Notify outside of mutex to avoid re-entrancy deadlock.
    notify(module_name, ReloadPhase::ROLLBACK);

    auto wall_end = std::chrono::steady_clock::now();
    result.reloadDurationMs
        = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(wall_end - wall_start).count());

    result.success           = true;
    result.newVersion        = backup_version.toString();
    result.rollbackAvailable = false;

    spdlog::info("HotReloadManager: '{}' rolled back in {}ms (to {})", module_name, result.reloadDurationMs,
                 result.newVersion);
    return result;
}

// =============================================================================
// Queries
// =============================================================================

std::optional<ModuleVersion> HotReloadManager::getCurrentVersion(const std::string &module_name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = slots_.find(module_name);
    if (it == slots_.end()) {
        return std::nullopt;
    }

    const ModuleVersion &v = it->second.current_version;
    if (v.version.empty() && v.major == 0 && v.minor == 0 && v.patch == 0) {
        return std::nullopt; // Not yet loaded.
    }
    return v;
}

bool HotReloadManager::isRollbackAvailable(const std::string &module_name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = slots_.find(module_name);
    return it != slots_.end() && it->second.has_backup;
}

std::vector<std::string> HotReloadManager::registeredModules() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<std::string> names;
    names.reserve(slots_.size());
    for (const auto &[name, _] : slots_) {
        names.push_back(name);
    }
    return names;
}

std::optional<SandboxStats> HotReloadManager::getSandboxStats(const std::string &module_name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = slots_.find(module_name);
    // Short-circuit: null check before isActive() to avoid null dereference.
    if (it == slots_.end() || !it->second.sandbox || !it->second.sandbox->isActive()) {
        return std::nullopt;
    }
    return it->second.sandbox->stats();
}

// =============================================================================
// Callbacks
// =============================================================================

void HotReloadManager::setStateSaveCallback(StateSaveCallback cb) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    state_save_cb_ = std::move(cb);
}

void HotReloadManager::setStateRestoreCallback(StateRestoreCallback cb) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    state_restore_cb_ = std::move(cb);
}

void HotReloadManager::addReloadCallback(ReloadCallback cb) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    reload_cbs_.push_back(std::move(cb));
}

void HotReloadManager::clearReloadCallbacks() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    reload_cbs_.clear();
}

// =============================================================================
// Statistics
// =============================================================================

HotReloadManager::Stats HotReloadManager::getStats() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return stats_;
}

void HotReloadManager::resetStats() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    stats_ = {};
}

// =============================================================================
// Private helpers
// =============================================================================

void HotReloadManager::notify(const std::string &name, ReloadPhase phase) {
    // Capture callbacks under shared lock (read-only), then invoke outside
    // of lock to prevent re-entrancy deadlocks.
    std::vector<ReloadCallback> cbs;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        cbs = reload_cbs_;
    }
    for (const auto &cb : cbs) {
        try {
            cb(name, phase);
        } catch (const std::exception &ex) {
            spdlog::warn("HotReloadManager: reload callback threw: {}", ex.what());
        } catch (const std::string &) {
            spdlog::warn("HotReloadManager: reload callback threw unknown exception");
        } catch (const char *) {
            spdlog::warn("HotReloadManager: reload callback threw unknown exception");
        }
    }
}

std::string HotReloadManager::saveState(const std::string &name) {
    StateSaveCallback cb;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        cb = state_save_cb_;
    }
    if (!cb) {
        return std::string{};
    }
    try {
        auto state = cb(name);
        if (!state.empty()) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            stats_.statesSaved++;
        }
        return state;
    } catch (const std::exception &ex) {
        spdlog::warn("HotReloadManager: state-save callback threw: {}", ex.what());
        return std::string{};
    }
}

bool HotReloadManager::restoreState(const std::string &name, const std::string &state) {
    StateRestoreCallback cb;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        cb = state_restore_cb_;
    }
    if (!cb) {
        return false;
    }
    try {
        bool ok = cb(name, state);
        if (ok) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            stats_.statesRestored++;
        }
        return ok;
    } catch (const std::exception &ex) {
        spdlog::warn("HotReloadManager: state-restore callback threw: {}", ex.what());
        return false;
    }
}

/*static*/ ModuleVersion HotReloadManager::versionFromLoader(ModuleLoader &loader, const std::string &module_name) {
    auto info = loader.getModuleInfo(module_name);
    if (info.has_value()) {
        return ModuleVersion::fromMetadata(info->metadata);
    }
    return ModuleVersion{};
}

} // namespace modules
} // namespace themis
