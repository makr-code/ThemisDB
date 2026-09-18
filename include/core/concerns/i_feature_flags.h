/**
 * @file i_feature_flags.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <memory>

namespace themis {
namespace core {
namespace concerns {

class IFeatureFlags {
public:
    /**
     * @brief IFeature Flags.
     * @return Return value.
     */
    virtual ~IFeatureFlags() = default;

    // -----------------------------------------------------------------------
    // Core query
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual bool isEnabled(std::string_view name) const = 0;

    // -----------------------------------------------------------------------
    // Mutation
    // -----------------------------------------------------------------------

    /**
     * @brief Set Value.
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     */
    virtual void setValue(std::string_view name, bool value) = 0;

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::unordered_map<std::string, bool> getAllFlags() const = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    virtual void flush() noexcept {}

    virtual void shutdown() noexcept {}

    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

// ---------------------------------------------------------------------------
// In-process implementation
// ---------------------------------------------------------------------------

class InMemoryFeatureFlags : public IFeatureFlags {
public:
    InMemoryFeatureFlags() = default;

    explicit InMemoryFeatureFlags(std::unordered_map<std::string, bool> initial)
        : flags_(std::move(initial)) {}

    bool isEnabled(std::string_view name) const override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = flags_.find(std::string(name));
        return it != flags_.end() && it->second;
    }

    void setValue(std::string_view name, bool value) override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        flags_[std::string(name)] = value;
    }

    std::unordered_map<std::string, bool> getAllFlags() const override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return flags_;
    }

    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, bool> flags_;
};

} // namespace concerns
} // namespace core
} // namespace themis
