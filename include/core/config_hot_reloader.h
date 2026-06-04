/*
 * ThemisDB | File: config_hot_reloader.h | Version: 0.1.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 161
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file config_hot_reloader.h
 * @brief Runtime configuration hot-reload interface.
 *
 * IConfigHotReloader watches one or more configuration sources (files,
 * Consul KV, etcd, Vault) and notifies registered callbacks when values
 * change, allowing subsystems to update their runtime state without a
 * process restart.
 *
 * ### Reliability contract
 * - `reload()` is atomic from the subscriber's perspective: callbacks
 *   receive either the full new configuration or none at all.
 * - Failed reloads do not evict the previous configuration.
 * - `lastReloadResult()` always reflects the outcome of the most recent
 *   `reload()` call, whether triggered automatically or explicitly.
 */

#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace core {

// ---------------------------------------------------------------------------
// ConfigChangeType — type of change detected in a config source
// ---------------------------------------------------------------------------

/**
 * @brief The nature of a configuration value change.
 */
enum class ConfigChangeType {
    CREATED,   ///< A new key/file appeared.
    MODIFIED,  ///< An existing key/file changed value.
    DELETED,   ///< A key/file was removed.
    RELOADED,  ///< A force-reload was triggered (may bundle multiple changes).
};

// ---------------------------------------------------------------------------
// ConfigChange — a single key-level change record
// ---------------------------------------------------------------------------

/**
 * @brief Record describing a single configuration change.
 *
 * `old_value_json` and `new_value_json` are JSON-serialised representations
 * of the previous and current value (empty string when not applicable).
 */
struct ConfigChange {
    std::string      config_key;
    ConfigChangeType change_type = ConfigChangeType::MODIFIED;
    std::string      old_value_json;
    std::string      new_value_json;
    std::chrono::system_clock::time_point changed_at;
};

// ---------------------------------------------------------------------------
// ConfigChangeCallback — callback type invoked when a watched key changes
// ---------------------------------------------------------------------------

/**
 * @brief Callback invoked on the hot-reloader's background thread when a
 *        watched key matching the registered `key_prefix` changes.
 *
 * The callback must complete quickly (< 5 ms); long-running reactions should
 * be dispatched to a separate thread pool.
 */
using ConfigChangeCallback = std::function<void(const ConfigChange&)>;

// ---------------------------------------------------------------------------
// HotReloadResult — outcome of a reload() invocation
// ---------------------------------------------------------------------------

/**
 * @brief Result of a reload() invocation.
 */
struct HotReloadResult {
    bool        success             = false;
    std::vector<ConfigChange> applied_changes;
    std::vector<std::string>  errors;
    double      reload_duration_ms  = 0.0;
};

// ---------------------------------------------------------------------------
// IConfigHotReloader — runtime config hot-reload interface
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for runtime configuration hot-reload.
 *
 * ### Thread safety
 * All methods must be safe to call concurrently from multiple threads.
 */
class IConfigHotReloader {
public:
    virtual ~IConfigHotReloader() = default;

    /**
     * @brief Start watching @p config_path_or_key for changes.
     *
    * The watched source must exist and must not already be registered.
    * Implementations should treat invalid paths/keys as non-fatal input
    * errors and return false instead of throwing.
    *
     * @return `false` if the path/key is not accessible or already watched.
     */
    virtual bool watch(const std::string& config_path_or_key) = 0;

    /**
     * @brief Stop watching @p config_path_or_key.
     *
    * Unwatching an unknown source is a no-op failure and should not mutate
    * the current reload state.
    *
     * @return `false` if the path/key was not being watched.
     */
    virtual bool unwatch(const std::string& config_path_or_key) = 0;

    /**
     * @brief Force an immediate reload from all watched sources.
     *
     * Registered callbacks are invoked synchronously on the calling thread
    * before this method returns. Implementations should preserve the last
    * known-good configuration if the reload fails.
    *
    * @return HotReloadResult describing the applied changes, any errors, and
    *         the observed reload duration.
     */
    virtual HotReloadResult reload() = 0;

    /**
     * @brief Register a callback for changes to keys matching @p key_prefix.
     *
     * The callback fires whenever a watched key whose name starts with
     * @p key_prefix changes.  Multiple callbacks may share the same prefix.
     * Callbacks run on the reloader's dispatch context, so they should avoid
     * blocking work and must tolerate duplicate notification across batched
     * reloads.
     *
     * @param key_prefix Prefix used to match watched keys.
     * @param callback Callback invoked for matching changes.
     * @return `false` if registration failed (e.g., too many callbacks).
     */
    virtual bool onConfigChange(
        const std::string&  key_prefix,
        ConfigChangeCallback callback
    ) = 0;

    /**
     * @brief Return the result of the most recent reload() call.
     *
     * @return Snapshot of the last reload attempt, including failures.
     */
    virtual HotReloadResult lastReloadResult() const = 0;

    /**
     * @brief Return the current automatic reload polling interval.
     *
     * @return Polling interval; zero means automatic polling is disabled.
     */
    virtual std::chrono::milliseconds reloadInterval() const = 0;

    /**
     * @brief Update the automatic reload polling interval.
     *
     * Setting @p interval to zero disables automatic polling (manual
     * `reload()` calls remain functional). Negative durations are not valid
     * and should be rejected by the implementation contract.
     *
     * @param interval New polling interval.
     */
    virtual void setReloadInterval(std::chrono::milliseconds interval) = 0;
};

} // namespace core
} // namespace themis
