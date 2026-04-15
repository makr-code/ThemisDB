/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llama_cpp_registrar.h                              ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:03:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f0f3ecebde  2026-04-11  feat(llama_cpp): v2.1.0 — streaming, batch inference, Plu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
     * present; otherwise does nothing and returns true (stub mode).
     */
    static ReloadCallback defaultReloadCallback();

private:
    LlamaCppPluginRegistrar() = delete;
};

} // namespace llamacpp
} // namespace themis
