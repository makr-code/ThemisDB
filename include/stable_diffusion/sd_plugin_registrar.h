/**
 * @file sd_plugin_registrar.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: sd_plugin_registrar.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "stable_diffusion/sd_plugin.h"
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
namespace imggen {

using json = nlohmann::json;

// ────────────────────────────────────────────────────────────────────────────
// SDPluginAdapter
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief IThemisPlugin adapter for SDPlugin.
 *
 * Wraps an SDPlugin so that it can be managed by the unified
 * plugins::PluginManager, including hot-plug monitoring.
 *
 * On initialize() the adapter calls SDPlugin::initialize(model_path, config).
 * On shutdown() the adapter resets the inner plugin to a stub state.
 * getInstance() returns a pointer to the underlying SDPlugin for callers that
 * need direct access to the image-generation API.
 *
 * Thread-Safety: initialize() and shutdown() are not concurrently safe;
 * all other paths delegate to the thread-safe SDPlugin.
 */
class SDPluginAdapter : public plugins::IThemisPlugin {
public:
    /**
     * @brief Construct from an existing SDPlugin instance.
     * @param plugin Heap-allocated SDPlugin; adapter takes ownership.
     */
    explicit SDPluginAdapter(std::unique_ptr<SDPlugin> plugin);

    // ── IThemisPlugin ───────────────────────────────────────────────────────
    const char* getName()    const override { return "stable_diffusion"; }
    const char* getVersion() const override { return "2.3.0"; }

    plugins::PluginType getType() const override {
        return plugins::PluginType::IMAGE_GENERATION;
    }

    plugins::PluginCapabilities getCapabilities() const override;

    /**
     * @brief Initialize the underlying SDPlugin.
     *
     * Parses @p config_json and calls SDPlugin::initialize().
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
     * @brief Return a pointer to the underlying SDPlugin.
     *
     * Callers should cast the return value to `imggen::SDPlugin*`.
     */
    void* getInstance() override { return sd_plugin_.get(); }

    // ── Direct access ───────────────────────────────────────────────────────

    /** @return Non-owning pointer to the wrapped SDPlugin. */
    SDPlugin*       getSDPlugin()       { return sd_plugin_.get(); }
    const SDPlugin* getSDPlugin() const { return sd_plugin_.get(); }

private:
    std::unique_ptr<SDPlugin> sd_plugin_;
    std::string               model_path_; ///< saved for hot-reload
};

// ────────────────────────────────────────────────────────────────────────────
// SDPluginRegistrar
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory and registration helper for SDPlugin.
 *
 * Provides static utilities to:
 *  - Create a raw SDPlugin (createPlugin)
 *  - Wrap it in an SDPluginAdapter for use with plugins::PluginManager
 *    (createAdapter)
 *  - Enable PluginManager hot-plug monitoring for the plugin directory
 *    (enableHotPlug / disableHotPlug)
 *
 * Hot-plug lifecycle:
 *   When the PluginManager detects that a model file or plugin shared-library
 *   has been replaced on disk, registerReloadCallback() is called so that
 *   a consumer can reinitialise SDPlugin with the new model.
 *
 * Usage:
 * @code
 *   // Simple: create and use directly
 *   auto plugin = SDPluginRegistrar::createPlugin({});
 *
 *   // Advanced: wire into PluginManager hot-plug
 *   auto adapter = SDPluginRegistrar::createAdapter({{"model_path","sd.ckpt"}});
 *   // hand adapter to PluginManager::registerPlugin() or use enableHotPlug()
 * @endcode
 */
class SDPluginRegistrar {
public:
    // ── Factory methods ──────────────────────────────────────────────────

    /**
     * @brief Create a standalone SDPlugin instance.
     *
     * @param config  Optional JSON configuration.
     *                Key: "model_path" (string) — if present, calls
     *                SDPlugin::initialize(model_path, config).
     * @return Heap-allocated SDPlugin; caller owns.
     */
    static std::unique_ptr<SDPlugin> createPlugin(const json& config = {});

    /**
     * @brief Create an SDPluginAdapter wrapping a new SDPlugin.
     *
     * The adapter implements IThemisPlugin and can be handed directly to
     * plugins::PluginManager.
     *
     * @param config  Optional JSON configuration.
     * @return Heap-allocated SDPluginAdapter; caller owns.
     */
    static std::unique_ptr<SDPluginAdapter> createAdapter(
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
        std::function<bool(SDPlugin& plugin, const json& config)>;

    /**
     * @brief Default hot-plug reload callback.
     *
     * Calls SDPlugin::initialize(config["model_path"], config) when
     * "model_path" is present and non-empty. Missing or empty paths are
     * treated as a successful stub-mode no-op so hot-plug reload can keep the
     * plugin unloaded without failing the caller.
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
    SDPluginRegistrar() = delete;
};

} // namespace imggen
} // namespace themis
