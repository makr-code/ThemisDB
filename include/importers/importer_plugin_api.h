/**
 * @file importer_plugin_api.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "importers/importer_interface.h"
#include "importers/importer_plugin.h"
#include "plugins/plugin_interface.h"

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <atomic>
#include <future>
#include <thread>
#include <chrono>
#include <cstdlib>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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
        // Dispatch via vtable to the derived IImporter::initialize(const std::string&)
        return static_cast<IImporter*>(this)->initialize(
            config_json ? std::string(config_json) : std::string{});
    }

    // Prevent name-hiding of IImporter::initialize(const std::string&)
    using IImporter::initialize;

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
// PluginSandboxConfig
// ============================================================================

/**
 * @brief Resource limits for a plugin loaded via `ImporterPluginRegistry::loadPlugin()`.
 *
 * Each import job launched by a V1 plugin runs in a sandboxed thread.  The
 * sandbox enforces the limits specified here:
 *
 *  - **Memory limit** — the host-provided allocator callbacks in
 *    `ThemisImporterAllocator` track cumulative byte allocations.  When a job
 *    exceeds `memory_limit_bytes` the allocator returns `nullptr`, preventing
 *    further allocation and causing the job to fail gracefully.  Plugins that
 *    bypass the allocator (use `malloc` directly) are not subject to this limit.
 *
 *  - **Timeout** — the import thread is given at most `timeout_ms` milliseconds
 *    to complete.  If the deadline is reached `cancel()` is signalled and the
 *    job returns an error after the thread joins.
 *
 * Set a field to 0 to disable the corresponding limit.
 */
struct PluginSandboxConfig {
    /// Maximum bytes a single import job may allocate via the sandbox
    /// allocator.  0 disables per-job memory limiting.
    size_t   memory_limit_bytes = 256UL * 1024UL * 1024UL;  ///< 256 MiB

    /// Maximum wall-clock time (milliseconds) an import job may run.
    /// 0 disables the timeout.
    uint32_t timeout_ms = 300'000;  ///< 5 minutes
};

// ============================================================================
// V1ImporterAdapter (internal helper)
// ============================================================================

/**
 * @brief `IImporter` adapter that wraps a `THEMIS_IMPORTER_PLUGIN_V1` instance.
 *
 * Created internally by `ImporterPluginRegistry::loadPlugin()`.  Each call to
 * `importData()` runs in a dedicated thread under the configured
 * `PluginSandboxConfig` constraints.
 */
class V1ImporterAdapter : public IImporter {
public:
    V1ImporterAdapter(const THEMIS_IMPORTER_PLUGIN_V1* desc,
                      PluginSandboxConfig               sandbox)
        : desc_(desc), sandbox_(sandbox)
    {
        alloc_ctx_.memory_limit_bytes = sandbox_.memory_limit_bytes;
        ThemisImporterAllocator alloc = makeAllocator();
        instance_ = desc_->create_instance(
            sandbox_.memory_limit_bytes > 0 ? &alloc : nullptr);
    }

    ~V1ImporterAdapter() override {
        if (instance_ && desc_ && desc_->destroy_instance) {
            ThemisImporterAllocator alloc = makeAllocator();
            desc_->destroy_instance(instance_,
                sandbox_.memory_limit_bytes > 0 ? &alloc : nullptr);
        }
    }

    // Non-copyable
    V1ImporterAdapter(const V1ImporterAdapter&)            = delete;
    V1ImporterAdapter& operator=(const V1ImporterAdapter&) = delete;

    // ----------------------------------------------------------------
    // IImporter interface
    // ----------------------------------------------------------------

    const char* getName() const override {
        return (desc_ && desc_->name) ? desc_->name : "v1_plugin";
    }

    std::vector<std::string> getSupportedTypes() const override { return {}; }

    bool initialize(const std::string& config_json) override {
        if (!instance_ || !desc_ || !desc_->initialize) {
          return false;
        }
        return desc_->initialize(instance_, config_json.c_str()) == 0;
    }

    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override {
        if (!instance_ || !desc_) {
            errors.push_back("Plugin instance not created");
            return false;
        }
        if (!desc_->validate_source) {
          return true;
        }
        char errbuf[1024] = {};
        int rc = desc_->validate_source(instance_, source_path.c_str(),
                                        errbuf, sizeof(errbuf));
        if (rc != 0 && errbuf[0] != '\0') {
            errors.emplace_back(errbuf);
        }
        return rc == 0;
    }

    ImportStats importData(const std::string&  source_path,
                           const ImportOptions& options,
                           ProgressCallback     /*cb*/ = nullptr) override {
        if (!instance_ || !desc_ || !desc_->import_data) {
            ImportStats s;
            s.errors.push_back("Plugin instance not initialized");
            return s;
        }
        alloc_ctx_.bytes_used.store(0, std::memory_order_relaxed);
        alloc_ctx_.limit_exceeded.store(false, std::memory_order_relaxed);

        if (sandbox_.timeout_ms > 0) {
            // Run with timeout enforcement
            auto future = std::async(std::launch::async,
                [this, &source_path, &options]() -> ImportStats {
                    return runImportV1(source_path, options);
                });
            auto status = future.wait_for(
                std::chrono::milliseconds(sandbox_.timeout_ms));
            if (status == std::future_status::timeout) {
                if (desc_->cancel) {
                  desc_->cancel(instance_);
                }
                future.wait();
                ImportStats s;
                s.errors.push_back("Plugin import timed out after "
                    + std::to_string(sandbox_.timeout_ms) + " ms");
                return s;
            }
            return future.get();
        }
        return runImportV1(source_path, options);
    }

    std::shared_ptr<ImportHandle> importDataAsync(
            const std::string&  source_path,
            const ImportOptions& options) override {
        auto handle  = std::make_shared<ImportHandle>();
        handle->id   = std::string(getName()) + "-async-job";
        handle->running.store(true);
        auto promise = std::make_shared<std::promise<ImportStats>>();
        handle->future = promise->get_future().share();
        std::thread([this, source_path, options, handle, promise]() {
            auto stats = importData(source_path, options);
            handle->running.store(false);
            promise->set_value(std::move(stats));
        }).detach();
        return handle;
    }

    void cancel() override {
        if (instance_ && desc_ && desc_->cancel) {
            desc_->cancel(instance_);
        }
    }

    json getSourceSchema(const std::string& source_path) override {
        if (!instance_ || !desc_ || !desc_->get_schema) {
          return json::object();
        }
        const char* s = desc_->get_schema(instance_, source_path.c_str());
        if (!s || s[0] == '\0') {
          return json::object();
        }
        try {
            return json::parse(s);
        } catch (...) {
            return json::object();
        }
    }

private:
    // ----------------------------------------------------------------
    // Sandbox allocator helpers
    // ----------------------------------------------------------------

    struct AllocContext {
        std::atomic<size_t> bytes_used{0};
        std::atomic<bool>   limit_exceeded{false};
        size_t              memory_limit_bytes{0};
    };

    // Header stored before each allocation so sandboxFree can subtract
    // the correct byte count without a separate lookup table.
    struct alignas(std::max_align_t) AllocHeader {
        size_t total_bytes = 0;  ///< sizeof(AllocHeader) + user-requested bytes
    };

    static void* sandboxAlloc(size_t bytes, void* user_data) {
        auto* ctx = static_cast<AllocContext*>(user_data);
        if (ctx->limit_exceeded.load(std::memory_order_relaxed)) {
          return nullptr;
        }
        const size_t total = sizeof(AllocHeader) + bytes;
        if (ctx->memory_limit_bytes > 0) {
            size_t prev = ctx->bytes_used.fetch_add(total, std::memory_order_relaxed);
            if (prev + total > ctx->memory_limit_bytes) {
                ctx->bytes_used.fetch_sub(total, std::memory_order_relaxed);
                ctx->limit_exceeded.store(true, std::memory_order_relaxed);
                return nullptr;
            }
        }
        auto* header = static_cast<AllocHeader*>(std::malloc(total));
        if (!header) {
            if (ctx->memory_limit_bytes > 0) {
                ctx->bytes_used.fetch_sub(total, std::memory_order_relaxed);
            }
            return nullptr;
        }
        header->total_bytes = total;
        return header + 1;
    }

    static void sandboxFree(void* ptr, void* user_data) {
        if (!ptr) {
          return;
        }
        auto* header = static_cast<AllocHeader*>(ptr) - 1;
        auto* ctx    = static_cast<AllocContext*>(user_data);
        if (ctx && ctx->memory_limit_bytes > 0) {
            ctx->bytes_used.fetch_sub(header->total_bytes, std::memory_order_relaxed);
        }
        std::free(header);
    }

    ThemisImporterAllocator makeAllocator() {
        ThemisImporterAllocator alloc{};
        alloc.alloc     = &V1ImporterAdapter::sandboxAlloc;
        alloc.free      = &V1ImporterAdapter::sandboxFree;
        alloc.user_data = &alloc_ctx_;
        return alloc;
    }

    ImportStats runImportV1(const std::string&  source_path,
                            const ImportOptions& /*options*/) {
        ImportStats stats;
        ThemisImporterAllocator alloc = makeAllocator();
        // allocator passed at create_instance time
        uint64_t imported = 0, failed = 0;
        int rc = desc_->import_data(instance_, source_path.c_str(),
                                    nullptr, &imported, &failed);
        stats.imported_records = imported;
        stats.failed_records   = failed;
        stats.total_records    = imported + failed;
        if (rc != 0) {
            stats.errors.push_back(
                "Plugin import_data returned error code: " + std::to_string(rc));
        }
        if (alloc_ctx_.limit_exceeded.load(std::memory_order_relaxed)) {
            stats.errors.push_back(
                "Plugin exceeded sandbox memory limit of "
                + std::to_string(sandbox_.memory_limit_bytes) + " bytes");
        }
        return stats;
    }

    // ----------------------------------------------------------------
    // Members
    // ----------------------------------------------------------------
    const THEMIS_IMPORTER_PLUGIN_V1* desc_;
    PluginSandboxConfig              sandbox_;
    void*                            instance_{nullptr};
    AllocContext                     alloc_ctx_;
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
    using Factory = std::function<std::shared_ptr<IImporter>()>;

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
    std::shared_ptr<IImporter> create(const std::string& name) const {
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
     *
     * @warning This method removes factory entries but does **not** close the
     *          underlying shared-library handles opened by `loadPlugin()`.
     *          Always call `unloadPlugin(name)` for every plugin loaded via
     *          `loadPlugin()` before calling `clear()` to avoid resource leaks
     *          and dangling function pointers in the process image:
     * @code
     *   // Correct teardown sequence:
     *   auto& reg = ImporterPluginRegistry::instance();
     *   for (const auto& name : reg.listPlugins()) {
     *       reg.unloadPlugin(name);   // closes shared library
     *   }
     *   // reg.clear() is now a no-op since all factories were removed by unloadPlugin()
     * @endcode
     */
    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        factories_.clear();
    }

    // ----------------------------------------------------------------
    // V1 plugin loading (THEMIS_IMPORTER_PLUGIN_V1 ABI)
    // ----------------------------------------------------------------

    /**
     * @brief Load a plugin from a shared library using the V1 C ABI.
     *
     * Opens the shared library at @p path, resolves the
     * `themis_importer_create` factory symbol
     * (THEMIS_IMPORTER_CREATE_SYMBOL), calls the factory to obtain the
     * `THEMIS_IMPORTER_PLUGIN_V1` descriptor, validates the ABI version, and
     * registers a factory in the registry under the plugin's reported name.
     *
     * The resulting factory wraps each `importData()` call in a dedicated
     * thread that enforces the limits in @p sandbox.  When the sandbox
     * memory allocator is exhausted the import job fails gracefully with an
     * error in `ImportStats::errors`.  If the timeout fires, `cancel()` is
     * signalled and the job returns an error after the thread joins.
     *
     * Performance targets (per the v1.9.0 specification):
     *   - Cold `dlopen` ≤ 50 ms.
     *   - API version check on load adds ≤ 1 ms overhead.
     *
     * @param path     Filesystem path to the plugin shared library
     *                 (.so / .dll / .dylib).
     * @param sandbox  Resource limits for each import job spawned by this
     *                 plugin.  Defaults to 256 MiB / 5-minute limits.
     * @return `true` on success; `false` otherwise.
     *         Call `lastLoadError()` for a human-readable description.
     *
     * @note The shared library is kept open until `unloadPlugin()` is called
     *       or the registry is destroyed.  Do not call `registerFactory()`
     *       with the same name between `loadPlugin()` and `unloadPlugin()`
     *       as the library handle will not be closed in that case.
     *
     * @see unloadPlugin(), lastLoadError(), THEMIS_IMPORTER_CREATE_SYMBOL
     */
    bool loadPlugin(const std::string&      path,
                    const PluginSandboxConfig& sandbox = PluginSandboxConfig{}) {
        std::lock_guard<std::mutex> lk(mutex_);
        last_load_error_.clear();

        using FactoryFn = const THEMIS_IMPORTER_PLUGIN_V1*(*)();
        void*     handle  = nullptr;
        FactoryFn factory = nullptr;

#if defined(_WIN32)
        handle = static_cast<void*>(::LoadLibraryA(path.c_str()));
        if (!handle) {
            last_load_error_ = "LoadLibrary failed for: " + path;
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        factory = reinterpret_cast<FactoryFn>(
            ::GetProcAddress(static_cast<HMODULE>(handle),
                             THEMIS_IMPORTER_CREATE_SYMBOL));
#elif defined(__unix__) || defined(__APPLE__)
        handle = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            const char* err = ::dlerror();
            last_load_error_ = err ? std::string(err)
                                   : ("dlopen failed for: " + path);
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        factory = reinterpret_cast<FactoryFn>(
            ::dlsym(handle, THEMIS_IMPORTER_CREATE_SYMBOL));
#else
        last_load_error_ = "Dynamic library loading not supported on this platform";
        return false;
#endif

        if (!factory) {
            last_load_error_ = std::string("Missing symbol '")
                + THEMIS_IMPORTER_CREATE_SYMBOL + "' in: " + path;
            closeLibraryHandle(handle);
            return false;
        }

        // --- ABI version check (target: ≤ 1 ms overhead) ---
        const THEMIS_IMPORTER_PLUGIN_V1* descriptor = factory();
        if (!descriptor) {
            last_load_error_ = std::string(THEMIS_IMPORTER_CREATE_SYMBOL)
                + "() returned nullptr for: " + path;
            closeLibraryHandle(handle);
            return false;
        }

        if (descriptor->abi_version != THEMIS_IMPORTER_PLUGIN_ABI_V1) {
            last_load_error_ =
                "Unsupported plugin ABI version "
                + std::to_string(descriptor->abi_version)
                + " (expected " + std::to_string(THEMIS_IMPORTER_PLUGIN_ABI_V1)
                + ") in: " + path;
            closeLibraryHandle(handle);
            return false;
        }

        if (descriptor->struct_size < sizeof(THEMIS_IMPORTER_PLUGIN_V1)) {
            last_load_error_ =
                "Plugin V1 struct_size " + std::to_string(descriptor->struct_size)
                + " < expected " + std::to_string(sizeof(THEMIS_IMPORTER_PLUGIN_V1))
                + " in: " + path;
            closeLibraryHandle(handle);
            return false;
        }

        if (!descriptor->name || !descriptor->create_instance
                || !descriptor->destroy_instance || !descriptor->initialize
                || !descriptor->import_data) {
            last_load_error_ =
                "Plugin V1 descriptor has NULL required function pointer(s) in: " + path;
            closeLibraryHandle(handle);
            return false;
        }

        const std::string plugin_name = descriptor->name;

        // Register a factory that creates V1ImporterAdapter instances.
        auto captured_descriptor = descriptor;
        auto captured_sandbox    = sandbox;
        factories_[plugin_name] = [captured_descriptor, captured_sandbox]()
                -> std::shared_ptr<IImporter> {
            return std::make_shared<V1ImporterAdapter>(
                captured_descriptor, captured_sandbox);
        };

        // Track the library handle so we can close it in unloadPlugin().
        loaded_v1_handles_[plugin_name] = handle;
        return true;
    }

    /**
     * @brief Unload a plugin previously loaded with `loadPlugin()`.
     *
     * Removes the plugin's factory from the registry and closes the shared
     * library.  Safe to call even if @p name was not loaded via `loadPlugin()`
     * (no-op for unknown names).
     *
     * @param name  Plugin name as reported by the V1 descriptor (i.e.
     *              `THEMIS_IMPORTER_PLUGIN_V1::name`).
     */
    void unloadPlugin(const std::string& name) {
        std::lock_guard<std::mutex> lk(mutex_);
        factories_.erase(name);
        auto it = loaded_v1_handles_.find(name);
        if (it != loaded_v1_handles_.end()) {
            closeLibraryHandle(it->second);
            loaded_v1_handles_.erase(it);
        }
    }

    /**
     * @brief Human-readable error description from the last failed
     *        `loadPlugin()` call.
     *
     * Returns an empty string if the last `loadPlugin()` succeeded or if
     * `loadPlugin()` has not been called.  The string is overwritten on the
     * next `loadPlugin()` call regardless of success or failure.
     */
    const std::string& lastLoadError() const {
        // Written only inside loadPlugin() which holds mutex_.
        // Safe to read without lock after loadPlugin() returns.
        return last_load_error_;
    }

private:
    // ----------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------
    static void closeLibraryHandle(void* handle) {
        if (!handle) {
          return;
        }
#if defined(_WIN32)
        ::FreeLibrary(static_cast<HMODULE>(handle));
#elif defined(__unix__) || defined(__APPLE__)
        ::dlclose(handle);
#endif
    }

    mutable std::mutex mutex_;
    std::map<std::string, Factory> factories_;
    std::map<std::string, void*>   loaded_v1_handles_;
    std::string                    last_load_error_;
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
            [captured_create, captured_destroy]() -> std::shared_ptr<IImporter> {
                auto* plugin_ptr = captured_create();
                if (!plugin_ptr) {
                  return nullptr;
                }
                auto* importer_ptr = static_cast<IImporter*>(plugin_ptr->getInstance());
                if (!importer_ptr) {
                    captured_destroy(plugin_ptr);
                    return nullptr;
                }
                // Wrap with a deleter that also destroys the plugin wrapper.
                return std::shared_ptr<IImporter>(
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
        if (!handle_) {
          return;
        }

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
        if (!handle_) {
          return;
        }
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
// ImporterRegistry — convenience alias for ImporterPluginRegistry
// ============================================================================

namespace themis {
namespace importers {

/**
 * @brief Alias for `ImporterPluginRegistry`.
 *
 * Provided for compatibility with the roadmap API surface
 * (`ImporterRegistry::loadPlugin(path)`).
 *
 * ### Example
 * @code
 *   // Load a V1 plugin:
 *   themis::importers::ImporterRegistry::instance()
 *       .loadPlugin("/opt/themis/plugins/oracle_importer.so");
 *
 *   // Create an importer instance:
 *   auto importer = themis::importers::ImporterRegistry::instance()
 *       .create("oracle_importer");
 * @endcode
 */
using ImporterRegistry = ImporterPluginRegistry;

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

