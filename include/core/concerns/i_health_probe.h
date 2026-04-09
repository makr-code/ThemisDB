#pragma once

#include "core/concerns/lifecycle.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Abstract interface for a named health probe.
 *
 * Implementations encapsulate a single liveness or readiness check for a
 * specific subsystem (e.g. "rocksdb", "redis", "grpc-upstream"). Probes are
 * registered with a @c HealthProbeRegistry and queried collectively.
 *
 * Thread-safety: @c probe() must be safe to call concurrently from multiple
 * threads.
 */
class IHealthProbe {
public:
    virtual ~IHealthProbe() = default;

    /**
     * @brief Unique, human-readable probe name (e.g. "rocksdb", "redis").
     *
     * Names must not be empty and should be stable across process restarts.
     */
    virtual std::string name() const = 0;

    /**
     * @brief Execute the health check and return the result.
     *
     * Implementations must be non-blocking on the happy path; a slow backend
     * should be queried via a cached status that is refreshed in the background
     * rather than blocking the caller.
     */
    virtual ProbeResult probe() const = 0;
};

// ---------------------------------------------------------------------------
// Functional convenience wrapper
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight @c IHealthProbe backed by a callable.
 *
 * Suitable for registering ad-hoc probes without defining a full subclass:
 * @code
 *   registry.registerProbe(FunctionalHealthProbe::make("db",
 *       []{ return db.ping() ? ProbeResult::healthy() : ProbeResult::unhealthy("no ping"); }));
 * @endcode
 */
class FunctionalHealthProbe : public IHealthProbe {
public:
    using ProbeFunc = std::function<ProbeResult()>;

    FunctionalHealthProbe(std::string name, ProbeFunc fn)
        : name_(std::move(name)), fn_(std::move(fn)) {}

    std::string name() const override { return name_; }
    ProbeResult probe() const override { return fn_(); }

    static std::shared_ptr<IHealthProbe> make(std::string name, ProbeFunc fn) {
        return std::make_shared<FunctionalHealthProbe>(std::move(name), std::move(fn));
    }

private:
    std::string name_;
    ProbeFunc   fn_;
};

// ---------------------------------------------------------------------------
// Thread-safe registry
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe registry for named @c IHealthProbe instances.
 *
 * Components register their probes at startup; monitoring infrastructure
 * queries @c checkAll() to aggregate results for liveness and readiness
 * endpoints.
 *
 * Design:
 * - Registration and lookup are protected by a single @c std::mutex.
 * - @c checkAll() acquires the lock once to snapshot probe pointers, then
 *   calls each probe *outside* the lock so a slow probe cannot block
 *   concurrent registrations.
 */
class HealthProbeRegistry {
public:
    HealthProbeRegistry() = default;

    /**
     * @brief Register (or replace) a named probe.
     *
     * If a probe with the same name is already registered it is replaced.
     * @param probe Non-null probe instance.
     */
    void registerProbe(std::shared_ptr<IHealthProbe> probe) {
        if (!probe) return;
        std::lock_guard<std::mutex> lock(mutex_);
        probes_[probe->name()] = std::move(probe);
    }

    /**
     * @brief Remove the probe with the given name (no-op if absent).
     */
    void unregisterProbe(std::string_view name) {
        std::lock_guard<std::mutex> lock(mutex_);
        probes_.erase(std::string(name));
    }

    /**
     * @brief Check all registered probes and return name-result pairs.
     *
     * Probes are invoked outside the registry lock so slow checks do not
     * block concurrent registrations.
     */
    std::vector<std::pair<std::string, ProbeResult>> checkAll() const {
        // Snapshot probe pointers under the lock.
        std::vector<std::shared_ptr<IHealthProbe>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot.reserve(probes_.size());
            for (const auto& kv : probes_) {
                snapshot.push_back(kv.second);
            }
        }
        // Call each probe without holding the lock.
        std::vector<std::pair<std::string, ProbeResult>> results;
        results.reserve(snapshot.size());
        for (const auto& p : snapshot) {
            results.emplace_back(p->name(), p->probe());
        }
        return results;
    }

    /**
     * @brief Return true when every registered probe reports healthy.
     *
     * Returns true (vacuously healthy) when no probes are registered.
     */
    bool isHealthy() const {
        for (const auto& [name, result] : checkAll()) {
            if (!result.ok) return false;
        }
        return true;
    }

    /**
     * @brief Return the number of currently registered probes.
     */
    size_t probeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return probes_.size();
    }

    /**
     * @brief Liveness probe for the registry itself (always healthy).
     */
    ProbeResult selfProbe() const {
        return isHealthy()
            ? ProbeResult::healthy("all " + std::to_string(probeCount()) + " probes healthy")
            : ProbeResult::unhealthy("one or more health probes failed");
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<IHealthProbe>> probes_;
};

} // namespace concerns
} // namespace core
} // namespace themis
