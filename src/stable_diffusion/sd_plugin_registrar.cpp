/**
 * @file sd_plugin_registrar.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "stable_diffusion/sd_plugin_registrar.h"
#include <stdexcept>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_hot_plug_monitor.h"
#include <memory>

namespace themis {
namespace imggen {

// ── SDPluginAdapter ───────────────────────────────────────────────────────────

SDPluginAdapter::SDPluginAdapter(std::unique_ptr<SDPlugin> plugin)
    : sd_plugin_(std::move(plugin)) {}

plugins::PluginCapabilities SDPluginAdapter::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming  = false;
    caps.supports_batching   = true;  // SDPlugin::generateBatch
    caps.supports_transactions = false;
    caps.thread_safe         = true;
    caps.gpu_accelerated     = false; // enabled when THEMIS_ENABLE_STABLE_DIFFUSION + GPU build
    return caps;
}

/**
 * @brief Initialize.
 * @param[in] config_json Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: nlohmann::json::parse(), contains(), is_string(), empty().
 */
bool SDPluginAdapter::initialize(const char* config_json) {
    if (!config_json || config_json[0] == '\0') {
        return false;
    }
    try {
        const auto config = nlohmann::json::parse(config_json);
        if (config.contains("model_path") && config["model_path"].is_string()) {
            model_path_ = config["model_path"].get<std::string>();
            if (!model_path_.empty()) {
                return sd_plugin_->initialize(model_path_, config);
            }
        }
        return false;
    } catch (...) {
        return false;
    }
}

/**
 * @brief Shutdown.
 * @details Calls: clear().
 */
void SDPluginAdapter::shutdown() {
    // Reset to a fresh stub state so the adapter can be safely re-used after
    // a hot-plug unload event.
    sd_plugin_ = std::make_unique<SDPlugin>();
    model_path_.clear();
}

/**
 * @brief ── SDPluginRegistrar — factory methods ──────────────────────────────────────
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: contains(), is_string(), empty(), initialize().
 */

std::unique_ptr<SDPlugin> SDPluginRegistrar::createPlugin(const json& config) {
    auto plugin = std::make_unique<SDPlugin>();
    if (config.contains("model_path") && config["model_path"].is_string()) {
        const std::string path = config["model_path"].get<std::string>();
        if (!path.empty()) {
            plugin->initialize(path, config);
        }
    }
    return plugin;
}

/**
 * @brief Create Adapter.
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: createPlugin(), std::move().
 */
std::unique_ptr<SDPluginAdapter> SDPluginRegistrar::createAdapter(
        const json& config) {
    auto plugin  = createPlugin(config);
    return std::make_unique<SDPluginAdapter>(std::move(plugin));
}

/**
 * @brief ── SDPluginRegistrar — hot-plug ──────────────────────────────────────────────
 * @return Return value.
 * @details Calls: contains(), is_string(), empty(), initialize().
 */

SDPluginRegistrar::ReloadCallback SDPluginRegistrar::defaultReloadCallback() {
    return [](SDPlugin& plugin, const json& config) -> bool {
        if (config.contains("model_path") && config["model_path"].is_string()) {
            const std::string path = config["model_path"].get<std::string>();
            if (!path.empty()) {
                return plugin.initialize(path, config);
            }
        }
        return true;
    };
}

/**
 * @brief Enable Hot Plug.
 * @param[in,out] manager Input/output parameter.
 * @param[in] directory Input parameter.
 * @return True when the operation succeeds.
 * @details Implements enableHotPlug without additional internal calls.
 */
bool SDPluginRegistrar::enableHotPlug(
        plugins::PluginManager& manager,
        const std::string& directory) {
    plugins::HotPlugConfig cfg;
    cfg.enabled      = true;
    cfg.auto_load    = true;
    cfg.auto_reload  = true;
    cfg.auto_unload  = true;
    return manager.enableHotPlug(directory, cfg);
}

/**
 * @brief Disable Hot Plug.
 * @param[in,out] manager Input/output parameter.
 * @details Implements disableHotPlug without additional internal calls.
 */
void SDPluginRegistrar::disableHotPlug(plugins::PluginManager& manager) {
    manager.disableHotPlug();
}

} // namespace imggen
} // namespace themis
