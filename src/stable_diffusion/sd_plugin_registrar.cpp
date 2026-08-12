/**
 * @file sd_plugin_registrar.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
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

void SDPluginAdapter::shutdown() {
    // Reset to a fresh stub state so the adapter can be safely re-used after
    // a hot-plug unload event.
    sd_plugin_ = std::make_unique<SDPlugin>();
    model_path_.clear();
}

// ── SDPluginRegistrar — factory methods ──────────────────────────────────────

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

std::unique_ptr<SDPluginAdapter> SDPluginRegistrar::createAdapter(
        const json& config) {
    auto plugin  = createPlugin(config);
    return std::make_unique<SDPluginAdapter>(std::move(plugin));
}

// ── SDPluginRegistrar — hot-plug ──────────────────────────────────────────────

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

void SDPluginRegistrar::disableHotPlug(plugins::PluginManager& manager) {
    manager.disableHotPlug();
}

} // namespace imggen
} // namespace themis
