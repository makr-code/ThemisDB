#include "whisper/whisper_plugin_registrar.h"
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
        // STUB/SIMULATION NOTE:
        // Purpose: Allow WhisperPluginAdapter to be constructed and initialized
        //   without a real Whisper model, for integration environments where
        //   whisper.cpp is not installed or where no model file is available
        //   (CI, unit tests, development builds without speech-to-text).
        // Activation: config_json is null, empty, or contains no "model_path"
        //   key (or model_path is empty).
        // Production Delta: transcribe() will return empty strings or error
        //   responses because no model is loaded; the WhisperPlugin underlying
        //   object is in its default (uninitialized) state.
        // Removal Plan: Provide a valid model_path in the plugin configuration
        //   (e.g., "model_path": "/opt/models/ggml-base.en.bin").  The real
        //   initialize() path will then load the model via whisper.cpp.
        // Roadmap ref: src/llm/FUTURE_ENHANCEMENTS.md §"Whisper Plugin Activation"
        // Stub mode — no model required
        return true;
    }
    try {
        const auto config = nlohmann::json::parse(config_json);
        if (config.contains("model_path") && config["model_path"].is_string()) {
            model_path_ = config["model_path"].get<std::string>();
            if (!model_path_.empty()) {
                return whisper_plugin_->initialize(model_path_, config);
            }
        }
        // No model_path → stub mode
        return true;
    } catch (...) {
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
        // STUB/SIMULATION NOTE:
        // Purpose: Allow makeReloadCallback() to return a functional callback even
        //          when no Whisper model was loaded during initialize() (stub mode).
        // Activation: Called when config contains no "model_path" or model_path is
        //          empty (same condition as the stub initialize() path above).
        // Production Delta: The reload callback returns true without re-loading any
        //          model; subsequent transcribe() calls continue to return empty
        //          results (no speech recognition).
        // Removal Plan: Provide a valid model_path; the non-stub initialize() path
        //          then sets the loaded model, and the reload callback re-initializes
        //          it correctly.  See src/llm/FUTURE_ENHANCEMENTS.md §"Whisper Reload".
        // Stub mode — no model to reload; treat as successbool WhisperPluginRegistrar::enableHotPlug(
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
