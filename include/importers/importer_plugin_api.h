/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            importer_plugin_api.h                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-02-28                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 BETA (API stable from v1.5.0)                ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     N/A                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 Beta — breaking changes possible before v1.5.0           ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file importer_plugin_api.h
 * @brief Plugin API for third-party importer extensions.
 *
 * This header is the single entry point for authors of external importer
 * plugins distributed as shared libraries (.so / .dll / .dylib).
 *
 * ## Quick-start
 *
 * 1. Derive your importer class from `ImporterPluginBase`.
 * 2. Implement all pure-virtual methods (IImporter + IThemisPlugin).
 * 3. Place `THEMIS_IMPORTER_PLUGIN_IMPL(YourImporterClass)` in one
 *    translation unit of your shared library.
 * 4. Optionally ship a `plugin.json` alongside the binary (see
 *    `PluginManifest` in `plugins/plugin_interface.h`).
 *
 * @code
 * // my_csv_importer.h
 * #include "importers/importer_plugin_api.h"
 *
 * class MyCsvImporter : public themis::importers::ImporterPluginBase {
 * public:
 *     const char* getName()    const override { return "my_csv_importer"; }
 *     const char* getVersion() const override { return "0.1.0"; }
 *
 *     std::vector<std::string> getSupportedTypes() const override {
 *         return {"csv"};
 *     }
 *
 *     // ... implement remaining IImporter methods ...
 * };
 *
 * THEMIS_IMPORTER_PLUGIN_IMPL(MyCsvImporter)
 * @endcode
 *
 * ## Versioning & stability
 *
 * The importer plugin API will be stabilised in ThemisDB v1.5.0.
 * Breaking changes are possible in earlier releases; version-gate your
 * plugins using `THEMIS_IMPORTER_PLUGIN_API_VERSION` if needed.
 *
 * ## Thread-safety
 *
 * `ImporterPluginRegistry` is thread-safe.  Individual `IImporter` instances
 * are **not** thread-safe by default; do not share a single instance across
 * threads without external synchronisation.
 */

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// ---------------------------------------------------------------------------
// API version token (bump on every incompatible change)
// ---------------------------------------------------------------------------

/// Numeric API version, suitable for compile-time guards.
/// Format: (major * 10000 + minor * 100 + patch)
#define THEMIS_IMPORTER_PLUGIN_API_VERSION 10000  // 1.0.0

namespace themis {
namespace importers {

// ============================================================================
// ImporterPluginBase
// ============================================================================

/**
 * @brief Convenience base class for third-party importer plugins.
 *
 * Combines `IImporter` and `plugins::IThemisPlugin` into a single class so
 * that plugin authors only need one base, and provides sensible default
 * implementations for the `IThemisPlugin` lifecycle methods.
 *
 * Derived classes **must** implement all pure-virtual methods from `IImporter`
 * plus the following `IThemisPlugin` identifiers:
 *   - `getName()`
 *   - `getVersion()`
 *
 * The remaining `IThemisPlugin` methods have working defaults but may be
 * overridden for customisation.
 *
 * ### Minimal implementation checklist
 *   - `getName()`           – unique plugin identifier (snake_case recommended)
 *   - `getVersion()`        – semantic version string, e.g. "1.0.0"
 *   - `getSupportedTypes()` – source-type tokens, e.g. {"csv", "tsv"}
 *   - `initialize(config)`  – parse JSON config; return false on invalid input
 *   - `validateSource()`    – pre-flight check of the source path
 *   - `importData()`        – synchronous import; populate ImportStats
 *   - `importDataAsync()`   – launch background thread; return ImportHandle
 *   - `cancel()`            – signal cancellation to the running import
 *   - `getSourceSchema()`   – return source schema as JSON
 *
 * @note `getInstance()` is already implemented and returns `this` so that
 *       the PluginManager can recover an `IImporter*` from an
 *       `IThemisPlugin*` without an extra wrapper object.
 */
class ImporterPluginBase : public IImporter, public plugins::IThemisPlugin {
public:
    ~ImporterPluginBase() override = default;

    // ----------------------------------------------------------------
    // IThemisPlugin – type and capability defaults
    // ----------------------------------------------------------------

    /// Always returns `PluginType::IMPORTER`.
    plugins::PluginType getType() const override {
        return plugins::PluginType::IMPORTER;
    }

    /**
     * @brief Default capabilities: no special hardware or streaming support.
     *
     * Override to advertise streaming, batching, or GPU acceleration.
     */
    plugins::PluginCapabilities getCapabilities() const override {
        return plugins::PluginCapabilities{};
    }

    /**
     * @brief Initialise the plugin from a JSON configuration string.
     *
     * The default implementation delegates to
     * `IImporter::initialize(std::string)` so that existing IImporter
     * implementations only need to implement one `initialize` method.
     *
     * @param config_json  Null-terminated JSON configuration string.
     * @return `true` on success, `false` on invalid/unsupported configuration.
     */
    bool initialize(const char* config_json) override {
        return initialize(config_json ? std::string(config_json) : std::string{});
    }

    /**
     * @brief Default no-op shutdown.  Override to release resources.
     */
    void shutdown() override {}

    /**
     * @brief Returns `this` cast to `void*`.
     *
     * Callers that know the concrete type can retrieve the `IImporter*` via:
     * @code
     *   auto* importer = static_cast<IImporter*>(plugin->getInstance());
     * @endcode
     */
    void* getInstance() override {
        return static_cast<IImporter*>(this);
    }
};

// ============================================================================
// ImporterPluginDescriptor
// ============================================================================

/**
 * @brief Lightweight descriptor for a registered importer plugin.
 *
 * Can be populated from any `ImporterPluginBase`-derived instance and used to
 * discover which importer plugins are currently registered without
 * instantiating a fresh instance.  See `ImporterPluginRegistry::listPlugins()`.
 */
struct ImporterPluginDescriptor {
    std::string name;              ///< Plugin identifier (matches getName())
    std::string version;           ///< Semantic version string
    std::vector<std::string> supported_types;  ///< Source-type tokens
    plugins::PluginCapabilities capabilities;  ///< Advertised capabilities
};

// ============================================================================
// ImporterPluginRegistry
// ============================================================================

/**
 * @brief Singleton registry for importer plugins.
 *
 * Provides importer-specific factory registration and lookup:
 *   - Register a factory by name.
 *   - Create an `IImporter` instance directly without a void* cast.
 *   - List all registered importer plugin names.
 *   - Query whether a plugin name is registered.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * ### Typical usage inside ThemisDB
 * @code
 *   // Register built-in importers at startup:
 *   ImporterPluginRegistry::instance().registerFactory(
 *       "postgres_importer",
 *       []() -> std::unique_ptr<IImporter> {
 *           return std::make_unique<PostgreSQLImporter>();
 *       });
 *
 *   // Retrieve and use:
 *   auto importer = ImporterPluginRegistry::instance().create("postgres_importer");
 *   if (importer) {
 *       importer->initialize("{}");
 *       auto stats = importer->importData("/tmp/dump.sql", opts);
 *   }
 * @endcode
 */
class ImporterPluginRegistry {
public:
    using Factory = std::function<std::unique_ptr<IImporter>()>;

    /// Returns the process-wide singleton.
    static ImporterPluginRegistry& instance() {
        static ImporterPluginRegistry reg;
        return reg;
    }

    /**
     * @brief Register an importer factory under @p name.
     *
     * If a factory with the same name already exists it is replaced; this
     * allows runtime override of built-in importers by user plugins.
     *
     * @param name     Unique plugin identifier (e.g. "my_csv_importer").
     * @param factory  Factory callable returning a heap-allocated `IImporter`.
     */
    void registerFactory(const std::string& name, Factory factory) {
        std::lock_guard<std::mutex> lk(mutex_);
        factories_[name] = std::move(factory);
    }

    /**
     * @brief Create a new `IImporter` instance by plugin @p name.
     *
     * @param name  Plugin identifier passed to `registerFactory()`.
     * @return      New instance, or `nullptr` if the name is not registered.
     */
    std::unique_ptr<IImporter> create(const std::string& name) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = factories_.find(name);
        if (it == factories_.end()) {
            return nullptr;
        }
        return it->second();
    }

    /**
     * @brief Returns the names of all registered importer plugins.
     */
    std::vector<std::string> listPlugins() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<std::string> names;
        names.reserve(factories_.size());
        for (const auto& kv : factories_) {
            names.push_back(kv.first);
        }
        return names;
    }

    /**
     * @brief Returns `true` if a plugin named @p name is registered.
     */
    bool hasPlugin(const std::string& name) const {
        std::lock_guard<std::mutex> lk(mutex_);
        return factories_.count(name) > 0;
    }

    /**
     * @brief Remove a registered factory.
     *
     * Safe to call even if @p name is not registered (no-op in that case).
     *
     * @param name  Plugin identifier to remove.
     */
    void unregisterFactory(const std::string& name) {
        std::lock_guard<std::mutex> lk(mutex_);
        factories_.erase(name);
    }

    /**
     * @brief Remove all registered factories.
     *
     * Primarily intended for unit-test teardown.
     */
    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        factories_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, Factory> factories_;
};

// ============================================================================
// ImporterPluginLoader
// ============================================================================

/**
 * @brief Loads third-party importer plugins from shared libraries at runtime.
 *
 * Discovers the plugin entry points (`createPlugin` / `destroyPlugin`) from
 * a shared library, constructs the plugin, and registers its importer factory
 * in `ImporterPluginRegistry`.
 *
 * ### Expected shared-library exports
 *
 * Every importer plugin library must export two C-linkage symbols (provided
 * automatically by `THEMIS_IMPORTER_PLUGIN_IMPL`):
 * @code
 *   extern "C" {
 *     themis::plugins::IThemisPlugin* createPlugin();
 *     void destroyPlugin(themis::plugins::IThemisPlugin*);
 *   }
 * @endcode
 *
 * ### Platform notes
 * - Linux / macOS: uses `dlopen` / `dlsym` / `dlclose`.
 * - Windows: uses `LoadLibrary` / `GetProcAddress` / `FreeLibrary`.
 * - When the platform is unsupported, `load()` returns `false` immediately.
 *
 * ### Example
 * @code
 *   ImporterPluginLoader loader;
 *   if (!loader.load("/opt/themis/plugins/my_csv_importer.so")) {
 *       // handle error: loader.lastError()
 *   }
 *   auto importer = ImporterPluginRegistry::instance().create("my_csv_importer");
 * @endcode
 *
 * @note The loader keeps the shared library handle open until `unload()` is
 *       called (or the loader is destroyed).  Destroying the loader without
 *       calling `unload()` will close the library automatically.
 */
class ImporterPluginLoader {
public:
    ImporterPluginLoader() = default;

    /// Unloads the library if still open.
    ~ImporterPluginLoader() {
        unload();
    }

    // Non-copyable; move-only
    ImporterPluginLoader(const ImporterPluginLoader&) = delete;
    ImporterPluginLoader& operator=(const ImporterPluginLoader&) = delete;

    ImporterPluginLoader(ImporterPluginLoader&& other) noexcept
        : handle_(other.handle_)
        , loaded_name_(std::move(other.loaded_name_))
        , last_error_(std::move(other.last_error_))
        , destroy_fn_(other.destroy_fn_)
        , raw_plugin_(other.raw_plugin_)
    {
        other.handle_     = nullptr;
        other.destroy_fn_ = nullptr;
        other.raw_plugin_ = nullptr;
    }

    ImporterPluginLoader& operator=(ImporterPluginLoader&& other) noexcept {
        if (this != &other) {
            unload();
            handle_      = other.handle_;
            loaded_name_ = std::move(other.loaded_name_);
            last_error_  = std::move(other.last_error_);
            destroy_fn_  = other.destroy_fn_;
            raw_plugin_  = other.raw_plugin_;
            other.handle_     = nullptr;
            other.destroy_fn_ = nullptr;
            other.raw_plugin_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Load the shared library at @p path and register its importer.
     *
     * On success, the plugin's importer factory is registered in
     * `ImporterPluginRegistry::instance()` under the name returned by
     * `IThemisPlugin::getName()`.
     *
     * @param path  Filesystem path to the shared library.
     * @return      `true` on success; `false` otherwise (see `lastError()`).
     */
    bool load(const std::string& path) {
        unload();  // release any previously loaded library
        last_error_.clear();

        using CreateFn  = plugins::IThemisPlugin*(*)();
        using DestroyFn = void(*)(plugins::IThemisPlugin*);
        CreateFn  create_fn  = nullptr;
        DestroyFn local_destroy = nullptr;

#if defined(_WIN32)
        handle_ = static_cast<void*>(::LoadLibraryA(path.c_str()));
        if (!handle_) {
            last_error_ = "LoadLibrary failed for: " + path;
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        create_fn     = reinterpret_cast<CreateFn>(
            ::GetProcAddress(static_cast<HMODULE>(handle_), "createPlugin"));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        local_destroy = reinterpret_cast<DestroyFn>(
            ::GetProcAddress(static_cast<HMODULE>(handle_), "destroyPlugin"));
#elif defined(__unix__) || defined(__APPLE__)
        handle_ = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle_) {
            const char* err = ::dlerror();
            last_error_ = err ? err : ("dlopen failed for: " + path);
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        create_fn     = reinterpret_cast<CreateFn>(::dlsym(handle_, "createPlugin"));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        local_destroy = reinterpret_cast<DestroyFn>(::dlsym(handle_, "destroyPlugin"));
#else
        last_error_ = "Dynamic library loading not supported on this platform";
        return false;
#endif

        destroy_fn_ = local_destroy;

        if (!create_fn || !destroy_fn_) {
            last_error_ = "Missing entry points createPlugin/destroyPlugin in: " + path;
            destroy_fn_ = nullptr;
            closeHandle();
            return false;
        }

        raw_plugin_ = create_fn();
        if (!raw_plugin_) {
            last_error_ = "createPlugin() returned nullptr for: " + path;
            destroy_fn_ = nullptr;
            closeHandle();
            return false;
        }

        if (raw_plugin_->getType() != plugins::PluginType::IMPORTER) {
            last_error_ = std::string("Plugin '") + raw_plugin_->getName()
                        + "' is not an IMPORTER plugin (type="
                        + std::to_string(static_cast<int>(raw_plugin_->getType())) + ")";
            destroy_fn_(raw_plugin_);
            raw_plugin_  = nullptr;
            destroy_fn_  = nullptr;
            closeHandle();
            return false;
        }

        loaded_name_ = raw_plugin_->getName();

        // Register a factory that creates new instances from this library.
        // The factory captures the create/destroy function pointers.
        auto captured_create  = create_fn;
        auto captured_destroy = destroy_fn_;

        ImporterPluginRegistry::instance().registerFactory(
            loaded_name_,
            [captured_create, captured_destroy]() -> std::unique_ptr<IImporter> {
                auto* plugin_ptr = captured_create();
                if (!plugin_ptr) return nullptr;
                auto* importer_ptr = static_cast<IImporter*>(plugin_ptr->getInstance());
                if (!importer_ptr) {
                    captured_destroy(plugin_ptr);
                    return nullptr;
                }
                // Wrap with a deleter that also destroys the plugin wrapper.
                return std::unique_ptr<IImporter>(
                    importer_ptr,
                    [captured_destroy, plugin_ptr](IImporter* /*p*/) {
                        captured_destroy(plugin_ptr);
                    });
            });

        return true;
    }

    /**
     * @brief Unregister and close the loaded library.
     *
     * Safe to call even when no library is loaded (no-op).
     */
    void unload() {
        if (!handle_) return;

        // Remove from registry before closing the library so no dangling
        // function pointers remain in registered factories.
        if (!loaded_name_.empty()) {
            ImporterPluginRegistry::instance().unregisterFactory(loaded_name_);
            loaded_name_.clear();
        }

        if (raw_plugin_ && destroy_fn_) {
            destroy_fn_(raw_plugin_);
            raw_plugin_  = nullptr;
            destroy_fn_  = nullptr;
        }

        closeHandle();
    }

    /// Returns `true` if a library is currently loaded.
    bool isLoaded() const { return handle_ != nullptr; }

    /// Returns the plugin name reported by the last successfully loaded plugin.
    const std::string& loadedName() const { return loaded_name_; }

    /// Returns a human-readable error description from the last failed `load()`.
    const std::string& lastError() const { return last_error_; }

private:
    void closeHandle() {
        if (!handle_) return;
#if defined(_WIN32)
        ::FreeLibrary(static_cast<HMODULE>(handle_));
#elif defined(__unix__) || defined(__APPLE__)
        ::dlclose(handle_);
#endif
        handle_ = nullptr;
    }

    void*                             handle_      = nullptr;
    std::string                       loaded_name_;
    std::string                       last_error_;
    void (*destroy_fn_)(plugins::IThemisPlugin*) = nullptr;
    plugins::IThemisPlugin*           raw_plugin_ = nullptr;
};

} // namespace importers
} // namespace themis

// ============================================================================
// THEMIS_IMPORTER_PLUGIN_IMPL – convenience macro for plugin authors
// ============================================================================

/**
 * @brief Export the required C-linkage entry points for an importer plugin.
 *
 * Place this macro exactly **once** in one translation unit of your plugin
 * shared library.  @p PluginClass must:
 *   - Derive from `themis::importers::ImporterPluginBase` (or implement both
 *     `themis::importers::IImporter` and `themis::plugins::IThemisPlugin`).
 *   - Be default-constructible.
 *
 * Example:
 * @code
 *   class MyCsvImporter : public themis::importers::ImporterPluginBase {
 *       // ... implementation ...
 *   };
 *
 *   THEMIS_IMPORTER_PLUGIN_IMPL(MyCsvImporter)
 * @endcode
 *
 * This expands to:
 * @code
 *   extern "C" {
 *     THEMIS_PLUGIN_EXPORT IThemisPlugin* createPlugin()  { return new MyCsvImporter(); }
 *     THEMIS_PLUGIN_EXPORT void destroyPlugin(IThemisPlugin* p) { delete p; }
 *   }
 * @endcode
 */
#define THEMIS_IMPORTER_PLUGIN_IMPL(PluginClass)                            \
    extern "C" {                                                             \
        THEMIS_PLUGIN_EXPORT                                                 \
        themis::plugins::IThemisPlugin* createPlugin() {                    \
            return new PluginClass();                                        \
        }                                                                    \
        THEMIS_PLUGIN_EXPORT                                                 \
        void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {        \
            delete plugin;                                                   \
        }                                                                    \
    }
