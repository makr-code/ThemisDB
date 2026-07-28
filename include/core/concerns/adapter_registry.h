/**
 * @file adapter_registry.h
 * @brief Thread-safe typed adapter registry with hot-swap drain and plugin loading.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 */

#pragma once

#include "core/concerns/adapter_metadata.h"
#include "core/concerns/plugin_api.h"

#include <memory>
#include <mutex>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <iostream>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// AdapterTrustPolicy
// ---------------------------------------------------------------------------

/**
 * @brief Trust policy governing signature verification during plugin loading.
 *
 * - @c kTrustAll  — load any plugin regardless of signing state (development
 *                   default).  Adapters that carry an @c AdapterSignature are
 *                   still verified if one is provided via
 *                   @c registerAdapter, but @c loadFromPlugin does not require
 *                   a signature file to be present.
 * - @c kRequireSignature — @c loadFromPlugin rejects a plugin library unless
 *                   a `.sig` metadata file is found alongside the library and
 *                   the SHA-256 digest in that file matches the library's
 *                   contents.  Recommended for production deployments.
 */
enum class AdapterTrustPolicy : uint8_t {
    kTrustAll        = 0, ///< Accept all plugins without signature verification.
    kRequireSignature = 1, ///< Reject plugins without a valid SHA-256 signature.
};

/**
 * @brief Thread-safe typed adapter registry with hot-swap drain and runtime plugin loading.
 *
 * AdapterRegistry provides O(1) type-keyed storage and retrieval of adapters
 * expressed as @c std::shared_ptr<void> with @c std::type_index keys.
 * Reader-writer semantics are used so concurrent @c resolve() calls do not
 * block each other; only @c registerAdapter() and @c hotSwap() take an
 * exclusive write lock.
 *
 * ## Thread-safety
 * All public methods are safe to call concurrently.  @c resolve() acquires a
 * shared (read) lock; @c registerAdapter() and @c hotSwap() acquire an
 * exclusive (write) lock for the map update only, then drain outside the lock.
 *
 * ## Hot-swap SLO
 * @c hotSwap() attempts to drain in-flight references within
 * @c kHotSwapTimeoutMs.  If drain does not complete within the SLO it emits
 * a structured warning to @c std::cerr and returns @c false.
 *
 * ## Plugin loading
 * @c loadFromPlugin() loads a shared library via @c dlopen (POSIX) or
 * @c LoadLibraryA (Windows), resolves the @c themis_plugin_register entry
 * point, and invokes it so the plugin can call @c registerAdapter<T>() for
 * each adapter it provides.  Loaded library handles are kept alive for the
 * lifetime of the @c AdapterRegistry and released in the destructor.
 */
class AdapterRegistry {
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    AdapterRegistry() = default;

    /**
     * @brief Destructor — releases all dynamically-loaded plugin library handles.
     *
     * Any adapter shared_ptrs stored in the registry that originated from
     * plugin-loaded code may outlive the registry (callers hold shared_ptrs).
     * The library handles are closed here; any access through surviving
     * shared_ptrs after this point is undefined behaviour.  Callers must
     * ensure no shared_ptrs from plugin adapters remain live before destroying
     * the registry.
     */
    ~AdapterRegistry();
    // -----------------------------------------------------------------------
    // SLO constant
    // -----------------------------------------------------------------------

    /**
     * @brief Maximum time budget for hot-swap in-flight drain.
     *
     * hotSwap() polls for 1 ms × kHotSwapTimeoutMs.count() iterations before
     * declaring a drain timeout.
     */
    static constexpr std::chrono::milliseconds kHotSwapTimeoutMs{100};

    // -----------------------------------------------------------------------
    // Registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register an adapter for type @c T.
     *
     * Validates @p id and @p meta before inserting the adapter into the
     * registry.  If @p validator is non-null its @c validate() is also called;
     * a @c false return causes the registration to be rejected.
     *
     * @tparam T          Concrete adapter type.
     * @param  id         Unique non-empty string identifier for this adapter.
     * @param  adapter    Owning shared_ptr to the adapter instance.
     * @param  validator  Optional validator; nullptr means no additional check.
     * @param  meta       Optional metadata; id and apiVersion are validated.
     *
     * @throws std::invalid_argument if @p id is empty.
     * @throws std::invalid_argument if @p adapter is nullptr.
     * @throws std::invalid_argument if @p meta.apiVersion is below
     *         @c kCurrentApiVersion.
     * @throws std::invalid_argument if @p validator returns false.
     */
    template<typename T>
    void registerAdapter(
        std::string          id,
        std::shared_ptr<T>   adapter,
        AdapterValidator*    validator = nullptr,
        AdapterMetadata      meta      = {}
    ) {
        // Structural validation
        if (id.empty()) {
            throw std::invalid_argument(
                "[AdapterRegistry] registerAdapter: adapter id must not be empty");
        }
        if (!adapter) {
            throw std::invalid_argument(
                "[AdapterRegistry] registerAdapter: adapter must not be nullptr "
                "for adapter id='" + id + "'");
        }
        if (meta.apiVersion < kCurrentApiVersion) {
            throw std::invalid_argument(
                "[AdapterRegistry] registerAdapter: apiVersion must be >= " +
                std::to_string(kCurrentApiVersion) + " (got " +
                std::to_string(meta.apiVersion) + ") for adapter id='" + id + "'");
        }

        // Back-fill metadata id if not set
        if (meta.id.empty()) {
            meta.id = id;
        }

        // Optional validator gate
        if (validator && !validator->validate(meta)) {
            throw std::invalid_argument(
                "[AdapterRegistry] registerAdapter: AdapterValidator rejected "
                "adapter id='" + id + "'");
        }

        std::unique_lock<std::shared_mutex> lock(registry_mutex_);
        registry_[std::type_index(typeid(T))] = std::shared_ptr<void>(adapter);
    }

    // -----------------------------------------------------------------------
    // Resolution
    // -----------------------------------------------------------------------

    /**
     * @brief Resolve the registered adapter for type @c T.
     *
     * Acquires a shared (read) lock so concurrent resolves do not block each
     * other.  Returns a null shared_ptr if no adapter of type @c T has been
     * registered.
     *
     * @tparam T  Adapter type to look up.
     * @return    shared_ptr<T> to the active adapter, or @c nullptr.
     */
    template<typename T>
    std::shared_ptr<T> resolve() const {
        std::shared_lock<std::shared_mutex> lock(registry_mutex_);
        auto it = registry_.find(std::type_index(typeid(T)));
        if (it == registry_.end()) {
            return nullptr;
        }
        return std::static_pointer_cast<T>(it->second);
    }

    // -----------------------------------------------------------------------
    // Hot-swap
    // -----------------------------------------------------------------------

    /**
     * @brief Atomically replace the adapter for type @c T and drain in-flight refs.
     *
     * The new adapter is written to the registry under an exclusive lock so
     * new @c resolve() calls immediately return the replacement.  The old
     * adapter's reference count is then polled until it drops to 1 (only the
     * local drain variable holds it) or until @c kHotSwapTimeoutMs elapses.
     *
     * On timeout a structured warning is emitted to @c std::cerr and the
     * method returns @c false.  The new adapter has already been installed;
     * the old one may still be held by in-flight callers.
     *
     * @tparam T           Adapter type.
     * @param  new_adapter Replacement adapter; must not be null.
     * @param  meta        Optional metadata for the new adapter.
     * @return @c true if drain completed within @c kHotSwapTimeoutMs,
     *         @c false on timeout.
     *
     * @throws std::invalid_argument if @p new_adapter is nullptr.
     */
    template<typename T>
    bool hotSwap(std::shared_ptr<T> new_adapter, AdapterMetadata meta = {}) {
        if (!new_adapter) {
            throw std::invalid_argument(
                "[AdapterRegistry] hotSwap: new_adapter must not be nullptr");
        }

        std::shared_ptr<void> old_adapter_void;
        {
            std::unique_lock<std::shared_mutex> lock(registry_mutex_);
            auto key = std::type_index(typeid(T));
            auto it  = registry_.find(key);
            if (it != registry_.end()) {
                old_adapter_void = it->second; // retain old for drain
            }
            registry_[key] = std::shared_ptr<void>(new_adapter);
        }
        // Lock released — new callers get the new adapter.

        // Drain: wait for old adapter ref count to drop to 1 (our local copy).
        if (old_adapter_void) {
            const auto kMaxIterations = static_cast<int>(kHotSwapTimeoutMs.count());
            for (int i = 0; i < kMaxIterations; ++i) {
                if (old_adapter_void.use_count() <= 1) {
                    return true; // all callers have released the old adapter
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            // Timeout — emit structured warning
            std::cerr << "[AdapterRegistry] hot-swap drain timeout for type "
                      << typeid(T).name()
                      << " after " << kHotSwapTimeoutMs.count() << "ms\n";
            return false;
        }
        return true; // no prior registration; swap trivially succeeded
    }

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    /**
     * @brief Return the total number of registered adapter types.
     * @return Count of registered type entries.
     */
    size_t count() const;

    /**
     * @brief Check whether an adapter is registered for the given type index.
     *
     * @param type  @c std::type_index of the adapter type to query.
     * @return      @c true if an adapter is registered for @p type.
     */
    bool hasAdapter(std::type_index type) const;

    // -----------------------------------------------------------------------
    // Plugin loading
    // -----------------------------------------------------------------------

    /**
     * @brief Set the trust policy applied during plugin loading.
     *
     * The default policy is @c AdapterTrustPolicy::kTrustAll.  Set to
     * @c AdapterTrustPolicy::kRequireSignature in production environments to
     * enforce that every plugin library carries a valid SHA-256 signature file
     * (`<library_path>.sig`) before loading is permitted.
     *
     * @param policy  New trust policy.
     */
    void setTrustPolicy(AdapterTrustPolicy policy);

    /**
     * @brief Load and register adapters from a dynamic library file.
     *
     * Opens the shared library at @p path using @c dlopen (POSIX) or
     * @c LoadLibraryA (Windows), locates the @c themis_plugin_register
     * symbol, and invokes it with @c (this, adapter_id, kPluginAbiVersion).
     * The plugin is expected to call @c registerAdapter<T>() for each adapter
     * it provides.
     *
     * The loaded library handle is retained until the @c AdapterRegistry
     * instance is destroyed so that code in the library remains valid.
     *
     * Error conditions that return @c false:
     *  - @p path is empty.
     *  - The library file does not exist or cannot be opened.
     *  - The library does not export @c themis_plugin_register.
     *  - The plugin init function returns non-zero.
     *  - @c AdapterTrustPolicy::kRequireSignature is active and the SHA-256
     *    signature file (@c path + ".sig") is absent or does not match.
     *
     * @param path        Filesystem path to the shared library.
     * @param adapter_id  Adapter identifier forwarded to the plugin's init
     *                    function; use this to select among multiple adapters
     *                    bundled in one library.
     * @return            @c true if the plugin was loaded and its init
     *                    function succeeded; @c false otherwise.
     */
    [[nodiscard]] bool loadFromPlugin(const std::string& path,
                                      const std::string& adapter_id);

private:
    // -----------------------------------------------------------------------
    // PluginHandle — RAII OS library handle
    // -----------------------------------------------------------------------

    /**
     * @brief RAII wrapper for an OS-specific dynamic library handle.
     *
     * Non-copyable and movable.  The handle is released in the destructor via
     * @c dlclose (POSIX) or @c FreeLibrary (Windows).
     */
    struct PluginHandle {
        void*       handle = nullptr; ///< OS-specific library handle.
        std::string path;             ///< Path used to open the library.

        PluginHandle() = default;
        explicit PluginHandle(void* h, std::string p)
            : handle(h), path(std::move(p)) {}

        PluginHandle(const PluginHandle&)            = delete;
        PluginHandle& operator=(const PluginHandle&) = delete;

        PluginHandle(PluginHandle&& o) noexcept
            : handle(o.handle), path(std::move(o.path)) {
            o.handle = nullptr;
        }
        PluginHandle& operator=(PluginHandle&& o) noexcept {
            if (this != &o) {
                handle   = o.handle;
                path     = std::move(o.path);
                o.handle = nullptr;
            }
            return *this;
        }

        ~PluginHandle(); // defined in adapter_registry.cpp
    };

    /// Reader-writer lock protecting registry_ and plugin_handles_.
    mutable std::shared_mutex registry_mutex_;

    /// Type-keyed adapter storage.
    std::unordered_map<std::type_index, std::shared_ptr<void>> registry_;

    /// Loaded plugin library handles keyed by library path.
    std::unordered_map<std::string, PluginHandle> plugin_handles_;

    /// Active trust policy for plugin loading.
    AdapterTrustPolicy trust_policy_ = AdapterTrustPolicy::kTrustAll;
};

} // namespace concerns
} // namespace core
} // namespace themis
