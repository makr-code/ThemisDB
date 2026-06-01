/*
 * ThemisDB | File: whisper_plugin_registrar.cpp | Version: 0.0.1 | Last Modified: 2026-05-29 19:53:16
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 93/100 | Lines: 117
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "whisper/whisper_plugin_registrar.h"
#include <stdexcept>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_hot_plug_monitor.h"
#include <memory>

namespace themis {
namespace whisper {

// ── WhisperPluginAdapter ──────────────────────────────────────────────────────

WhisperPluginAdapter::WhisperPluginAdapter(
        std::unique_ptr<WhisperPlugin> plugin)
    : whisper_plugin_(std::move(plugin)) {}

plugins::PluginCapabilities WhisperPluginAdapter::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming    = false;
    caps.supports_batching     = false;
    caps.supports_transactions = false;
    caps.thread_safe           = true;
    caps.gpu_accelerated       = false; // enabled when THEMIS_ENABLE_WHISPER + GPU build
    return caps;
}

bool WhisperPluginAdapter::initialize(const char* config_json) {
    if (!config_json || config_json[0] == '\0') {
        return false;
    }
    try {
        const auto config = nlohmann::json::parse(config_json);
        if (config.contains("model_path") && config["model_path"].is_string()) {
            model_path_ = config["model_path"].get<std::string>();
            if (!model_path_.empty()) {
                return whisper_plugin_->initialize(model_path_, config);
            }
        }
        return false;
    } catch (const nlohmann::json::exception&) {
        return false;
    } catch (const std::exception&) {
        return false;
    } catch (const std::string&) {
        return false;
    } catch (const char*) {
        return false;
    }
}

void WhisperPluginAdapter::shutdown() {
    // Reset to a fresh stub state so the adapter can be safely re-used after
    // a hot-plug unload event.
    whisper_plugin_ = std::make_unique<WhisperPlugin>();
    model_path_.clear();
}

// ── WhisperPluginRegistrar — factory methods ──────────────────────────────────

std::unique_ptr<WhisperPlugin> WhisperPluginRegistrar::createPlugin(
        const json& config) {
    auto plugin = std::make_unique<WhisperPlugin>();
    if (config.contains("model_path") && config["model_path"].is_string()) {
        const std::string path = config["model_path"].get<std::string>();
        if (!path.empty()) {
            plugin->initialize(path, config);
        }
    }
    return plugin;
}

std::unique_ptr<WhisperPluginAdapter> WhisperPluginRegistrar::createAdapter(
        const json& config) {
    auto plugin = createPlugin(config);
    return std::make_unique<WhisperPluginAdapter>(std::move(plugin));
}

// ── WhisperPluginRegistrar — hot-plug ─────────────────────────────────────────

WhisperPluginRegistrar::ReloadCallback
WhisperPluginRegistrar::defaultReloadCallback() {
    return [](WhisperPlugin& plugin, const json& config) -> bool {
        if (config.contains("model_path") && config["model_path"].is_string()) {
            const std::string path = config["model_path"].get<std::string>();
            if (!path.empty()) {
                return plugin.initialize(path, config);
            }
        }
        return false;
    };
}

bool WhisperPluginRegistrar::enableHotPlug(
        plugins::PluginManager& manager,
        const std::string& directory) {
    plugins::HotPlugConfig cfg;
    cfg.enabled      = true;
    cfg.auto_load    = true;
    cfg.auto_reload  = true;
    cfg.auto_unload  = true;
    return manager.enableHotPlug(directory, cfg);
}

void WhisperPluginRegistrar::disableHotPlug(plugins::PluginManager& manager) {
    manager.disableHotPlug();
}

} // namespace whisper
} // namespace themis
