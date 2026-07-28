/**
 * @file adapter_registry.h
 * @brief Thread-safe typed adapter registry with hot-swap drain semantics.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 91/100
 * @note Gap Summary: total=1; TODO=0, Stub=1, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 */

#pragma once

#include "core/concerns/adapter_metadata.h"

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
// STUB/SIMULATION NOTE — plugin-based adapter loading
//
// Purpose:    Placeholder for future plugin-based adapter loading that avoids
//             core-module recompilation (Issue #1706, Target: Q4 2026).
// Activation: NOT active — dlopen/LoadLibrary support requires a platform-
//             specific implementation that has not yet been written.
// Production Delta: Adapters are registered programmatically only; no dynamic
//             library loading occurs at runtime.
// Removal Plan: Replace loadFromPlugin() stub with plugin_loader.h when
//             Issue #1706 is implemented (Target: Q4 2026).
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe typed adapter registry with hot-swap drain semantics.
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
 */
class AdapterRegistry {
public:
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
     * @throws std::invalid_argument if @p meta.apiVersion == 0.
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
        if (meta.apiVersion == 0) {
            throw std::invalid_argument(
                "[AdapterRegistry] registerAdapter: apiVersion must be >= 1 (got 0) "
                "for adapter id='" + id + "'");
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
            constexpr int kMaxIterations = 100; // 100 × 1 ms = kHotSwapTimeoutMs
            for (int i = 0; i < kMaxIterations; ++i) {
                if (old_adapter_void.use_count() <= 1) {
                    return true; // all callers have released the old adapter
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            // Timeout — emit structured warning
            std::cerr << "[AdapterRegistry] hot-swap drain timeout for type "
                      << typeid(T).name()
                      << " after 100ms\n";
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
    // Plugin loading stub
    // -----------------------------------------------------------------------

    /**
     * @brief Load an adapter from a dynamic library path.
     *
     * @note STUB — always returns @c false.  See the STUB/SIMULATION NOTE in
     *       the file header.  Plugin loading is tracked as Issue #1706
     *       (Target: Q4 2026).
     *
     * @param path        Path to the dynamic library (ignored).
     * @param adapter_id  Adapter identifier to load (ignored).
     * @return            Always @c false.
     */
    bool loadFromPlugin(const std::string& path, const std::string& adapter_id);

private:
    /// Reader-writer lock protecting registry_.
    mutable std::shared_mutex registry_mutex_;

    /// Type-keyed adapter storage.
    std::unordered_map<std::type_index, std::shared_ptr<void>> registry_;
};

} // namespace concerns
} // namespace core
} // namespace themis
