/**
 * @file config_hot_reloader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

enum class ConfigChangeType {
    CREATED,   ///< A new key/file appeared.
    MODIFIED,  ///< An existing key/file changed value.
    DELETED,   ///< A key/file was removed.
    RELOADED,  ///< A force-reload was triggered (may bundle multiple changes).
};

// ---------------------------------------------------------------------------
// ConfigChange — a single key-level change record
// ---------------------------------------------------------------------------

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

using ConfigChangeCallback = std::function<void(const ConfigChange&)>;

// ---------------------------------------------------------------------------
// HotReloadResult — outcome of a reload() invocation
// ---------------------------------------------------------------------------

struct HotReloadResult {
    bool        success             = false;
    std::vector<ConfigChange> applied_changes;
    std::vector<std::string>  errors;
    double      reload_duration_ms  = 0.0;
};

// ---------------------------------------------------------------------------
// IConfigHotReloader — runtime config hot-reload interface
// ---------------------------------------------------------------------------

class IConfigHotReloader {
public:
    /**
     * @brief IConfig Hot Reloader.
     * @return Return value.
     */
    virtual ~IConfigHotReloader() = default;

    /**
     * @brief Watch.
     * @param[in] config_path_or_key Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool watch(const std::string& config_path_or_key) = 0;

    /**
     * @brief Unwatch.
     * @param[in] config_path_or_key Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool unwatch(const std::string& config_path_or_key) = 0;

    /**
     * @brief Reload.
     * @return Return value.
     */
    virtual HotReloadResult reload() = 0;

    /**
     * @brief On Config Change.
     * @param[in] key_prefix Input parameter.
     * @param[in] callback Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool onConfigChange(
        const std::string&  key_prefix,
        ConfigChangeCallback callback
    ) = 0;

    /**
     * @brief Last Reload Result.
     * @return Return value.
     */
    virtual HotReloadResult lastReloadResult() const = 0;

    /**
     * @brief Reload Interval.
     * @return Return value.
     */
    virtual std::chrono::milliseconds reloadInterval() const = 0;

    /**
     * @brief Set Reload Interval.
     * @param[in] interval Input parameter.
     */
    virtual void setReloadInterval(std::chrono::milliseconds interval) = 0;
};

} // namespace core
} // namespace themis
