/**
 * @file llama_cpp_registrar.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: llama_cpp_registrar.cpp | Version: 0.0.10 | Last Modified: 2026-06-01 04:20:37
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 87/100 | Lines: 91
 * Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=1, L=0
 * PR History (last 5): #4556 feat(llama_cpp): v2.1.0 â€”... (2026-04-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "llama_cpp/llama_cpp_registrar.h"
#include <stdexcept>
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
        // Production Delta: In explicit stub-mode test builds, callback
        //   reports success without loading a model. In production builds,
        //   callback fails closed.
        // Removal Plan: Ensure the hot-plug config always provides a valid
        //   model_path before invoking reload.  Once THEMIS_MODEL_DIR is set
        //   and model files are present, this path should never be reached.
        // Roadmap ref: src/llm/FUTURE_ENHANCEMENTS.md §"LlamaCpp Plugin Model Reload"
        // Fail closed: reloading without a model path must signal failure
        // in non-stub production builds.
    #ifdef THEMIS_LLAMA_CPP_STUB_MODE
        return true;
    #else
        return false;
    #endif
    };
}

// ── initFromServerConfig ──────────────────────────────────────────────────────

bool LlamaCppPluginRegistrar::initFromServerConfig(const json& server_config) {
    if (!server_config.contains("llm")) {
        return true; // no LLM section — stub / CI mode, no plugin needed
    }
    const auto& llm_cfg = server_config["llm"];
    if (!llm_cfg.contains("model_path")) {
        return true; // no model_path key — stub mode OK
    }
    const std::string model_path = llm_cfg.value("model_path", "");
    if (model_path.empty()) {
        return true; // empty path — stub mode OK
    }
    auto& mgr = themis::llm::LLMPluginManager::instance();
    return registerWithLLMManager(mgr, "llama_cpp", llm_cfg);
}

} // namespace llamacpp
} // namespace themis

