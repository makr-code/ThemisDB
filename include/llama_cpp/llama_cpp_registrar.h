/**
 * @file llama_cpp_registrar.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/llm_plugin_interface.h"
#include <memory>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

// Forward declarations
namespace themis {
namespace llm {
class LLMPluginManager;
} // namespace llm
} // namespace themis

namespace themis {
namespace llamacpp {

using json = nlohmann::json;

/**
 * @brief Factory and registration helper for LlamaCppPlugin.
 *
 * Provides static utilities to:
 *  - Create a raw LlamaCppPlugin (createPlugin)
 *  - Wrap it in an LLMPluginAdapter for use with the PluginManager
 *    (createAdapter)
 *  - Perform one-shot registration with LLMPluginManager
 *    (registerWithLLMManager)
 *
 * Hot-plug lifecycle:
 *   LlamaCppPlugin can be registered with plugins::PluginManager via the
 *   LLMPluginAdapter.  When the PluginManager hot-reloads the plugin (e.g.
 *   after a model file is replaced on disk), a reload listener can call
 *   registerReloadListener() to receive BEFORE_UNLOAD / AFTER_LOAD
 *   notifications and tear-down / re-initialise state accordingly.
 *
 * Usage:
 * @code
 *   // Simple: register with LLMPluginManager
 *   auto& mgr = themis::llm::LLMPluginManager::instance();
 *   LlamaCppPluginRegistrar::registerWithLLMManager(mgr, "llama_cpp", {});
 *
 *   // Advanced: create an adapter for PluginManager
 *   auto adapter = LlamaCppPluginRegistrar::createAdapter({});
 * @endcode
 */
class LlamaCppPluginRegistrar {
public:
    // ── Factory methods ──────────────────────────────────────────────────

    /**
     * @brief Create a standalone LlamaCppPlugin instance.
     *
     * @param config  Optional JSON configuration forwarded to loadModel().
     *                Keys: "model_path" (string), "n_ctx" / "context_length"
     *                      (integer), "n_gpu_layers" (integer).
     * @return Heap-allocated plugin, caller owns.
     */
    static std::unique_ptr<LlamaCppPlugin> createPlugin(const json& config = {});

    /**
     * @brief Create an LLMPluginAdapter wrapping a LlamaCppPlugin.
     *
     * The adapter implements IThemisPlugin so it can be handed to the unified
     * plugins::PluginManager.  Internally it delegates to LlamaCppPlugin for
     * all LLM operations.
     *
     * @param config  Optional JSON configuration.
     * @return Heap-allocated adapter, caller owns.
     */
    static std::unique_ptr<llm::LLMPluginAdapter> createAdapter(
        const json& config = {});

    // ── Registration helpers ─────────────────────────────────────────────

    /**
     * @brief Register a new LlamaCppPlugin instance with an LLMPluginManager.
     *
     * Creates a fresh plugin, optionally calls loadModel() when
     * config["model_path"] is present, and hands ownership to the manager.
     *
     * @param manager     Target LLMPluginManager.
     * @param plugin_name Name under which the plugin is registered.
     * @param config      Optional JSON configuration.
     * @return true on success.
     */
    static bool registerWithLLMManager(
        llm::LLMPluginManager& manager,
        const std::string& plugin_name = "llama_cpp",
        const json& config = {});

    // ── Hot-plug callback type ────────────────────────────────────────────

    /**
     * @brief Callback invoked when the plugin is hot-reloaded.
     *
     * @param plugin   The freshly created replacement plugin (not yet loaded).
     * @param config   The configuration that was originally passed to the
     *                 registrar; the callback may use it to re-load the model.
     * @return true if the callback handled the event; false aborts the reload.
     */
    using ReloadCallback =
        std::function<bool(LlamaCppPlugin& plugin, const json& config)>;

    /**
     * @brief Default hot-plug reload callback.
     *
     * Calls loadModel(config["model_path"], config) when "model_path" is
     * present; otherwise returns false (or true in stub mode).
     */
    static ReloadCallback defaultReloadCallback();

    // ── Server-startup integration ───────────────────────────────────────

    /**
     * @brief Initialize the LLM plugin subsystem from server configuration.
     *
     * Called during server startup. Reads the LLM configuration block from
     * @p server_config, and if a model path is configured, registers a
     * LlamaCppPlugin with the global LLMPluginManager singleton under the
     * name @c "llama_cpp".
     *
     * This method is a no-op (returns @c true) when:
     *  - @p server_config contains no @c "llm" key, or
     *  - the @c "llm" object has no @c "model_path" key, or
     *  - @c model_path is an empty string.
     *
     * In those cases the server starts in stub / CI mode without any model
     * loaded. When a non-empty @c model_path is present the plugin is
     * created, @c loadModel() is called, and the plugin is registered with
     * @c LLMPluginManager::instance().
     *
     * @param server_config  Full server configuration JSON object.
     *                       Expected key path: @c config["llm"]["model_path"]
     * @return @c true if registration succeeded or no plugin was needed;
     *         @c false if plugin creation or model loading failed.
     *
     * @code
     *   json cfg = load_server_config("config.json");
     *   if (!LlamaCppPluginRegistrar::initFromServerConfig(cfg)) {
     *       LOG_ERROR("LlamaCpp plugin failed to initialise");
     *   }
     * @endcode
     */
    static bool initFromServerConfig(const json& server_config);

private:
    LlamaCppPluginRegistrar() = delete;
};

} // namespace llamacpp
} // namespace themis
