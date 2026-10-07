#include "llama_cpp/llama_cpp_registrar.h"
#include <stdexcept>
#include "llm/llm_plugin_manager.h"
#include <memory>

namespace themis {
namespace llamacpp {

/**
 * @brief ── createPlugin ─────────────────────────────────────────────────────────────
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: contains(), is_string(), empty(), loadModel().
 */

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

/**
 * @brief ── createAdapter ─────────────────────────────────────────────────────────────
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: createPlugin(), std::move().
 */

std::unique_ptr<llm::LLMPluginAdapter> LlamaCppPluginRegistrar::createAdapter(
        const json& config) {
    auto plugin = createPlugin(config);
    return std::make_unique<llm::LLMPluginAdapter>(std::move(plugin));
}

/**
 * @brief ── registerWithLLMManager ────────────────────────────────────────────────────
 * @param[in,out] manager Input/output parameter.
 * @param[in] plugin_name Name of the plugin.
 * @param[in] config Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: createPlugin(), registerPlugin(), std::move().
 */

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
        // No model path means reload is not possible in production; the
        // registrar must fail closed unless a stub/test override is active.
    #ifdef THEMIS_LLAMA_CPP_STUB_MODE
        return true;
    #else
        return false;
    #endif
    };
}

/**
 * @brief ── initFromServerConfig ──────────────────────────────────────────────────────
 * @param[in] server_config Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: contains(), value(), empty(), themis::llm::LLMPluginManager::instance(), registerWithLLMManager().
 */

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

