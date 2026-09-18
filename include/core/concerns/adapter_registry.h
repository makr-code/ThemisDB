/**
 * @file adapter_registry.h
 * @brief Thread-safe typed adapter registry with hot-swap drain and plugin loading.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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

enum class AdapterTrustPolicy : uint8_t {
    kTrustAll        = 0, ///< Accept all plugins without signature verification.
    kRequireSignature = 1, ///< Reject plugins without a valid SHA-256 signature.
};

class AdapterRegistry {
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    AdapterRegistry() = default;

    ~AdapterRegistry();
    // -----------------------------------------------------------------------
    // SLO constant
    // -----------------------------------------------------------------------

    static constexpr std::chrono::milliseconds kHotSwapTimeoutMs{100};

    // -----------------------------------------------------------------------
    // Registration
    // -----------------------------------------------------------------------

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

        /**
         * @brief Lock.
         * @param[in] registry_mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::shared_mutex> lock(registry_mutex_);
        registry_[std::type_index(typeid(T))] = std::shared_ptr<void>(adapter);
    }

    // -----------------------------------------------------------------------
    // Resolution
    // -----------------------------------------------------------------------

    template<typename T>
    std::shared_ptr<T> resolve() const {
        /**
         * @brief Lock.
         * @param[in] registry_mutex_ Input parameter.
         * @return Return value.
         */
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

    template<typename T>
    bool hotSwap(std::shared_ptr<T> new_adapter, [[maybe_unused]] AdapterMetadata meta = {}) {
        if (!new_adapter) {
            throw std::invalid_argument(
                "[AdapterRegistry] hotSwap: new_adapter must not be nullptr");
        }

        std::shared_ptr<void> old_adapter_void;
        {
            /**
             * @brief Lock.
             * @param[in] registry_mutex_ Input parameter.
             * @return Return value.
             */
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
     * @brief Count.
     * @return Return value.
     */
    size_t count() const;

    /**
     * @brief Has Adapter.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasAdapter(std::type_index type) const;

    // -----------------------------------------------------------------------
    // Plugin loading
    // -----------------------------------------------------------------------

    /**
     * @brief Set Trust Policy.
     * @param[in] policy Input parameter.
     */
    void setTrustPolicy(AdapterTrustPolicy policy);

    [[nodiscard]] bool loadFromPlugin(const std::string& path,
                                      const std::string& adapter_id);

private:
    // -----------------------------------------------------------------------
    // PluginHandle — RAII OS library handle
    // -----------------------------------------------------------------------

    struct PluginHandle {
        void*       handle = nullptr; ///< OS-specific library handle.
        std::string path;             ///< Path used to open the library.

        PluginHandle() = default;
        /**
         * @brief Plugin Handle.
         * @param[in,out] h Input/output parameter.
         * @param[in] p Input parameter.
         * @return Return value.
         */
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

    mutable std::shared_mutex registry_mutex_;

    std::unordered_map<std::type_index, std::shared_ptr<void>> registry_;

    std::unordered_map<std::string, PluginHandle> plugin_handles_;

    AdapterTrustPolicy trust_policy_ = AdapterTrustPolicy::kTrustAll;
};

} // namespace concerns
} // namespace core
} // namespace themis
