/**
 * @file whisper_plugin_registrar.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "whisper/whisper_plugin.h"
#include "plugins/plugin_interface.h"
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace themis {
namespace plugins {
class PluginManager;
} // namespace plugins
} // namespace themis

namespace themis {
namespace whisper {

using json = nlohmann::json;

// ────────────────────────────────────────────────────────────────────────────
// WhisperPluginAdapter
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief IThemisPlugin adapter for WhisperPlugin.
 *
 * Wraps a WhisperPlugin so that it can be managed by the unified
 * plugins::PluginManager, including hot-plug monitoring.
 *
 * On initialize() the adapter calls WhisperPlugin::initialize(model_path,
 * config). On shutdown() the adapter resets the inner plugin to its default
 * stub state. getInstance() returns a pointer to the underlying WhisperPlugin
 * for callers that need direct access to the transcription API.
 *
 * Thread-Safety: initialize() and shutdown() are not concurrently safe;
 * all other paths delegate to the thread-safe WhisperPlugin.
 */
class WhisperPluginAdapter : public plugins::IThemisPlugin {
public:
    /**
     * @brief Construct from an existing WhisperPlugin instance.
     * @param plugin Heap-allocated WhisperPlugin; adapter takes ownership.
     */
    explicit WhisperPluginAdapter(std::unique_ptr<WhisperPlugin> plugin);

    // ── IThemisPlugin ───────────────────────────────────────────────────────
    const char* getName()    const override { return "whisper"; }
    const char* getVersion() const override { return "2.3.0"; }

    plugins::PluginType getType() const override {
        return plugins::PluginType::AUDIO_PROCESSING;
    }

    plugins::PluginCapabilities getCapabilities() const override;

    /**
     * @brief Initialize the underlying WhisperPlugin.
     *
     * Parses @p config_json and calls WhisperPlugin::initialize().
     * Expected JSON keys: "model_path" (string, optional).
     *
     * @param config_json JSON configuration string.
     * @return true on success; false when config is missing a non-empty model_path.
     */
    bool initialize(const char* config_json) override;

    /**
     * @brief Shutdown: reset the inner plugin to its default stub state.
     */
    void shutdown() override;

    /**
     * @brief Return a pointer to the underlying WhisperPlugin.
     *
     * Callers should cast the return value to `whisper::WhisperPlugin*`.
     */
    void* getInstance() override { return whisper_plugin_.get(); }

    // ── Direct access ───────────────────────────────────────────────────────

    /** @return Non-owning pointer to the wrapped WhisperPlugin. */
    WhisperPlugin*       getWhisperPlugin()       { return whisper_plugin_.get(); }
    const WhisperPlugin* getWhisperPlugin() const { return whisper_plugin_.get(); }

private:
    std::unique_ptr<WhisperPlugin> whisper_plugin_;
    std::string                    model_path_; ///< saved for hot-reload
};

// ────────────────────────────────────────────────────────────────────────────
// WhisperPluginRegistrar
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory and registration helper for WhisperPlugin.
 *
 * Provides static utilities to:
 *  - Create a raw WhisperPlugin (createPlugin)
 *  - Wrap it in a WhisperPluginAdapter for use with plugins::PluginManager
 *    (createAdapter)
 *  - Enable PluginManager hot-plug monitoring for the plugin directory
 *    (enableHotPlug / disableHotPlug)
 *
 * Hot-plug lifecycle:
 *   When the PluginManager detects that a model file or plugin shared-library
 *   has been replaced on disk, registerReloadCallback() is called so that a
 *   consumer can reinitialise WhisperPlugin with the new model.
 *
 * Usage:
 * @code
 *   // Simple: create and use directly
 *   auto plugin = WhisperPluginRegistrar::createPlugin({});
 *
 *   // Advanced: wire into PluginManager hot-plug
 *   auto adapter = WhisperPluginRegistrar::createAdapter(
 *       {{"model_path", "ggml-base.bin"}});
 * @endcode
 */
class WhisperPluginRegistrar {
public:
    // ── Factory methods ──────────────────────────────────────────────────

    /**
     * @brief Create a standalone WhisperPlugin instance.
     *
     * @param config  Optional JSON configuration.
     *                Key: "model_path" (string) — if present, calls
     *                WhisperPlugin::initialize(model_path, config).
     * @return Heap-allocated WhisperPlugin; caller owns.
     */
    static std::unique_ptr<WhisperPlugin> createPlugin(const json& config = {});

    /**
     * @brief Create a WhisperPluginAdapter wrapping a new WhisperPlugin.
     *
     * The adapter implements IThemisPlugin and can be handed directly to
     * plugins::PluginManager.
     *
     * @param config  Optional JSON configuration.
     * @return Heap-allocated WhisperPluginAdapter; caller owns.
     */
    static std::unique_ptr<WhisperPluginAdapter> createAdapter(
        const json& config = {});

    // ── Hot-plug ─────────────────────────────────────────────────────────

    /**
     * @brief Callback type invoked when the plugin is hot-reloaded.
     *
     * @param plugin  Freshly created replacement plugin (not yet loaded).
     * @param config  Configuration originally passed to the registrar.
     * @return true if reload should proceed; false aborts it.
     */
    using ReloadCallback =
        std::function<bool(WhisperPlugin& plugin, const json& config)>;

    /**
     * @brief Default hot-plug reload callback.
     *
     * Calls WhisperPlugin::initialize(model_path, config) when "model_path"
     * is present; otherwise returns false.
     */
    static ReloadCallback defaultReloadCallback();

    /**
     * @brief Enable PluginManager hot-plug monitoring for a directory.
     *
     * Calls manager.enableHotPlug(directory, config) with default options
     * (auto_load=true, auto_reload=true, auto_unload=true).
     *
     * @param manager    PluginManager instance.
     * @param directory  Directory to watch.
     * @return true on success.
     */
    static bool enableHotPlug(
        plugins::PluginManager& manager,
        const std::string& directory);

    /**
     * @brief Disable PluginManager hot-plug monitoring.
     *
     * @param manager  PluginManager instance.
     */
    static void disableHotPlug(plugins::PluginManager& manager);

private:
    WhisperPluginRegistrar() = delete;
};

} // namespace whisper
} // namespace themis
