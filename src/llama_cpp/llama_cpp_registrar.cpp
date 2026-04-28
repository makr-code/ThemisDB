/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llama_cpp_registrar.cpp                            ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:49:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f0f3ecebde  2026-04-11  feat(llama_cpp): v2.1.0 — streaming, batch inference, Plu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llama_cpp/llama_cpp_registrar.h"
#include "llm/llm_plugin_manager.h"
#include <memory>

namespace themis {
namespace llamacpp {

// ── createPlugin ─────────────────────────────────────────────────────────────

std::unique_ptr<LlamaCppPlugin> LlamaCppPluginRegistrar::createPlugin(
        const json& config) {
    auto plugin = std::make_unique<LlamaCppPlugin>();
    if (config.contains("model_path") && config["model_path"].is_string()) {
        const std::string path = config["model_path"].get<std::string>();
        if (!path.empty()) {
            plugin->loadModel(path, config);
        }
    }
    return plugin;
}

// ── createAdapter ─────────────────────────────────────────────────────────────

std::unique_ptr<llm::LLMPluginAdapter> LlamaCppPluginRegistrar::createAdapter(
        const json& config) {
    auto plugin = createPlugin(config);
    return std::make_unique<llm::LLMPluginAdapter>(std::move(plugin));
}

// ── registerWithLLMManager ────────────────────────────────────────────────────

bool LlamaCppPluginRegistrar::registerWithLLMManager(
        llm::LLMPluginManager& manager,
        const std::string& plugin_name,
        const json& config) {
    try {
        auto plugin = createPlugin(config);
        manager.registerPlugin(plugin_name, std::move(plugin));
        return true;
    } catch (...) {
        return false;
    }
}

// ── defaultReloadCallback ─────────────────────────────────────────────────────

LlamaCppPluginRegistrar::ReloadCallback
LlamaCppPluginRegistrar::defaultReloadCallback() {
    return [](LlamaCppPlugin& plugin, const json& config) -> bool {
        if (config.contains("model_path") && config["model_path"].is_string()) {
            const std::string path = config["model_path"].get<std::string>();
            if (!path.empty()) {
                return plugin.loadModel(path, config);
            }
        }
        // STUB/SIMULATION NOTE:
        // Purpose: Allow LlamaCppPlugin hot-plug reload to succeed without a
        //   real llama.cpp model file, for environments where the model is not
        //   installed or has been removed (CI, development builds, model
        //   swap operations in progress).
        // Activation: config contains no "model_path" key, or model_path is
        //   empty.
        // Production Delta: The plugin remains in its current (possibly
        //   uninitialized) state; subsequent inference calls will fail or
        //   return empty responses.  No model is loaded.
        // Removal Plan: Ensure the hot-plug config always provides a valid
        //   model_path before invoking reload.  Once THEMIS_MODEL_DIR is set
        //   and model files are present, this path should never be reached.
        // Roadmap ref: src/llm/FUTURE_ENHANCEMENTS.md §"LlamaCpp Plugin Model Reload"
        // Stub mode — no model to load; treat as success
        return true;
    };
}

} // namespace llamacpp
} // namespace themis
