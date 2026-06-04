/*
 * ThemisDB | File: i_feature_flags.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 155
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2845 [core] Feature flag interfa... (2026-03-12) | #2691 [core] Feature flag interfa... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * @brief Abstract interface for feature flag management.
 *
 * Provides a unified interface for querying and toggling feature flags at
 * runtime without redeployment.  Implementations can source flag values from
 * a static in-memory map, a remote feature-flag service, environment variables,
 * or any other provider.
 *
 * Thread-safety: all methods must be safe to call concurrently from multiple
 * threads.
 *
 * Lifecycle: implementations should honour flush() and shutdown() so that
 * any pending writes (e.g. audit records or remote-sync state) are flushed
 * before the process exits. shutdown() should be idempotent.
 */
class IFeatureFlags {
public:
    virtual ~IFeatureFlags() = default;

    // -----------------------------------------------------------------------
    // Core query
    // -----------------------------------------------------------------------

    /**
     * @brief Return whether the named feature flag is currently enabled.
     *
    * Unknown flags are treated as disabled by the default in-memory provider.
    * Remote providers should document whether they fall back to disabled or
    * report a backend error via isHealthy().
    *
     * @param name Flag name (UTF-8, not required to be NUL-terminated).
     * @return true when the flag is enabled, false when disabled or unknown.
     */
    [[nodiscard]] virtual bool isEnabled(std::string_view name) const = 0;

    // -----------------------------------------------------------------------
    // Mutation
    // -----------------------------------------------------------------------

    /**
     * @brief Enable or disable a named feature flag.
     *
     * Creates the flag entry if it does not yet exist.
    * Providers that persist state remotely should treat this as a durable
    * update request and surface replication or write failures through health
    * checks rather than by throwing.
     *
     * @param name  Flag name.
     * @param value true = enable, false = disable.
     */
    virtual void setValue(std::string_view name, bool value) = 0;

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    /**
     * @brief Return a snapshot of all currently defined flag values.
     *
     * The returned map is a copy; modifications do not affect the provider.
    * The snapshot reflects a moment-in-time view and may already be stale by
    * the time the caller inspects it.
     */
    [[nodiscard]] virtual std::unordered_map<std::string, bool> getAllFlags() const = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    /**
     * @brief Flush any pending state (e.g. audit records, remote syncs).
     *
    * No-op for in-memory providers. Implementations that batch changes should
    * use this as the durability boundary for best-effort persistence.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down the provider and release resources.
     *
    * After shutdown() any further calls have undefined behaviour unless the
    * implementation explicitly documents idempotent post-shutdown access.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Probe whether the feature flag provider is operational.
     *
     * @return ProbeResult with ok=true when the provider is healthy.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

// ---------------------------------------------------------------------------
// In-process implementation
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe in-memory feature flag provider.
 *
 * Suitable for unit tests, single-process deployments, and as a starting
 * point for more sophisticated providers (file-backed, remote-service, etc.).
 *
 * All flag values default to *disabled* (false) until explicitly set.
 */
class InMemoryFeatureFlags : public IFeatureFlags {
public:
    InMemoryFeatureFlags() = default;

    /**
     * @brief Construct with a pre-populated set of flags.
     * @param initial Initial flag values.
     */
    explicit InMemoryFeatureFlags(std::unordered_map<std::string, bool> initial)
        : flags_(std::move(initial)) {}

    bool isEnabled(std::string_view name) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = flags_.find(std::string(name));
        return it != flags_.end() && it->second;
    }

    void setValue(std::string_view name, bool value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        flags_[std::string(name)] = value;
    }

    std::unordered_map<std::string, bool> getAllFlags() const override {
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
