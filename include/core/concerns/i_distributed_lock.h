#pragma once

#include "core/concerns/lifecycle.h"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// Distributed lock interface
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for cross-node distributed locking.
 *
 * Provides a named, optionally TTL-scoped exclusive lock that can span
 * multiple process instances in a cluster.  In-process implementations
 * (e.g. @c InMemoryDistributedLock) are suitable for single-node deployments
 * and unit tests; production deployments should use a backend-backed
 * implementation (Redis @c SET NX PX, etcd lease, ZooKeeper ephemeral node).
 *
 * Thread-safety: all public methods must be safe to call concurrently from
 * multiple threads.
 *
 * Lifecycle: honour @c flush() and @c shutdown() so that held locks are
 * released gracefully before the process exits.
 */
class IDistributedLock {
public:
    virtual ~IDistributedLock() = default;

    // -----------------------------------------------------------------------
    // Lock acquisition
    // -----------------------------------------------------------------------

    /**
     * @brief Attempt to acquire a named lock immediately (non-blocking).
     *
     * @param name  Lock name / resource identifier.  Must not be empty.
     * @param ttl   Time-to-live for the lock; @c 0 means no automatic
     *              expiry (the lock must be released explicitly).
     *              Non-zero TTL enables automatic release so that crashes
     *              cannot leave locks permanently held.
     *
     * @return true  — lock successfully acquired; the caller must call
     *                @c release(name) when done.
     * @return false — lock is currently held by another holder.
     */
    virtual bool tryAcquire(std::string_view name,
                             std::chrono::milliseconds ttl = std::chrono::milliseconds{0}) = 0;

    /**
     * @brief Attempt to acquire a named lock, waiting up to @p timeout.
     *
     * Spins / polls until either the lock is obtained or the timeout elapses.
     * Implementations are encouraged to use back-off strategies.
     *
     * @param name    Lock name.
     * @param timeout Maximum time to wait.
     * @param ttl     Lock TTL once acquired (0 = no expiry).
     *
     * @return true on success, false on timeout.
     */
    virtual bool tryAcquireFor(std::string_view name,
                                std::chrono::milliseconds timeout,
                                std::chrono::milliseconds ttl = std::chrono::milliseconds{0}) = 0;

    // -----------------------------------------------------------------------
    // Lock release
    // -----------------------------------------------------------------------

    /**
     * @brief Release a previously acquired lock.
     *
     * No-op if the lock is not currently held by this instance.
     *
     * @param name Lock name.
     */
    virtual void release(std::string_view name) = 0;

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    /**
     * @brief Return whether the named lock is currently held (by anyone).
     *
     * @param name Lock name.
     */
    virtual bool isLocked(std::string_view name) const = 0;

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Release all currently held locks and release resources.
     *
     * Safe to call multiple times.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down the lock provider and release all held locks.
     *
     * After @c shutdown() any further calls have undefined behaviour.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Return whether the lock provider is operational.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

// ---------------------------------------------------------------------------
// RAII lock guard
// ---------------------------------------------------------------------------

/**
 * @brief RAII guard that releases a distributed lock on scope exit.
 *
 * Usage:
 * @code
 *   if (auto guard = DistributedLockGuard::tryAcquire(*lock_provider, "my-resource")) {
 *       // exclusive access to "my-resource"
 *   } // lock released automatically
 * @endcode
 */
class DistributedLockGuard {
public:
    /**
     * @brief Attempt to acquire @p name on @p provider.
     *
     * @return A guard holding the lock if acquired, or an empty optional on
     *         failure.
     */
    static std::optional<DistributedLockGuard> tryAcquire(
            IDistributedLock& provider,
            std::string_view name,
            std::chrono::milliseconds ttl = std::chrono::milliseconds{0}) {
        if (provider.tryAcquire(name, ttl)) {
            return DistributedLockGuard(provider, std::string(name));
        }
        return std::nullopt;
    }

    /**
     * @brief Attempt to acquire @p name, waiting up to @p timeout.
     */
    static std::optional<DistributedLockGuard> tryAcquireFor(
            IDistributedLock& provider,
            std::string_view name,
            std::chrono::milliseconds timeout,
            std::chrono::milliseconds ttl = std::chrono::milliseconds{0}) {
        if (provider.tryAcquireFor(name, timeout, ttl)) {
            return DistributedLockGuard(provider, std::string(name));
        }
        return std::nullopt;
    }

    DistributedLockGuard(DistributedLockGuard&& other) noexcept
        : provider_(other.provider_),
          name_(std::move(other.name_)),
          released_(other.released_) {
        other.released_ = true;
    }

    DistributedLockGuard& operator=(DistributedLockGuard&& other) noexcept {
        if (this != &other) {
            if (!released_) provider_.get().release(name_);
            provider_  = other.provider_;
            name_      = std::move(other.name_);
            released_  = other.released_;
            other.released_ = true;
        }
        return *this;
    }

    DistributedLockGuard(const DistributedLockGuard&)            = delete;
    DistributedLockGuard& operator=(const DistributedLockGuard&) = delete;

    ~DistributedLockGuard() {
        if (!released_) provider_.get().release(name_);
    }

    /**
     * @brief Explicitly release the lock before scope exit.
     *
     * After calling @c release() the destructor is a no-op.
     */
    void release() {
        if (!released_) {
            released_ = true;
            provider_.get().release(name_);
        }
    }

    /// Return the lock name held by this guard.
    const std::string& lockName() const { return name_; }

private:
    DistributedLockGuard(IDistributedLock& provider, std::string name)
        : provider_(provider), name_(std::move(name)), released_(false) {}

    std::reference_wrapper<IDistributedLock> provider_;
    std::string name_;
    bool released_ = false;
};

// ---------------------------------------------------------------------------
// In-memory implementation (single-process)
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe in-memory distributed lock provider.
 *
 * Uses a @c std::mutex-protected map of lock names to their expiry time.
 * TTL-based expiry is checked lazily on each @c tryAcquire() call so no
 * background thread is required.
 *
 * Suitable for unit tests and single-node deployments.  Does NOT coordinate
 * across process boundaries; for cross-node locking use a Redis- or etcd-
 * backed implementation.
 */
class InMemoryDistributedLock : public IDistributedLock {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    bool tryAcquire(std::string_view name,
                    std::chrono::milliseconds ttl = std::chrono::milliseconds{0}) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = std::string(name);
        auto it  = locks_.find(key);
        if (it != locks_.end()) {
            // Check TTL expiry.
            if (it->second != TimePoint{} && Clock::now() > it->second) {
                // Expired — consider it released.
                it->second = ttlToExpiry(ttl);
                return true;
            }
            return false; // held and not expired
        }
        locks_.emplace(key, ttlToExpiry(ttl));
        return true;
    }

    bool tryAcquireFor(std::string_view name,
                       std::chrono::milliseconds timeout,
                       std::chrono::milliseconds ttl = std::chrono::milliseconds{0}) override {
        auto deadline = Clock::now() + timeout;
        // Spin with exponential back-off (capped at 16 ms).
        std::chrono::milliseconds backoff{1};
        while (Clock::now() < deadline) {
            if (tryAcquire(name, ttl)) return true;
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - Clock::now());
            auto sleep_ms = remaining.count() > 0
                ? std::min(backoff, remaining)
                : std::chrono::milliseconds{0};
            if (sleep_ms.count() > 0) {
                std::this_thread::sleep_for(sleep_ms);
            }
            backoff = std::min(backoff * 2, std::chrono::milliseconds{16});
        }
        return tryAcquire(name, ttl); // one last attempt at the deadline
    }

    void release(std::string_view name) override {
        std::lock_guard<std::mutex> lock(mutex_);
        locks_.erase(std::string(name));
    }

    bool isLocked(std::string_view name) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = locks_.find(std::string(name));
        if (it == locks_.end()) return false;
        // Treat expired TTL locks as not held.
        if (it->second != TimePoint{} && Clock::now() > it->second) return false;
        return true;
    }

    void flush() noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        locks_.clear();
    }

    void shutdown() noexcept override { flush(); }

    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

private:
    static TimePoint ttlToExpiry(std::chrono::milliseconds ttl) {
        if (ttl.count() == 0) return TimePoint{}; // no expiry
        return Clock::now() + ttl;
    }

    mutable std::mutex mutex_;
    /// Maps lock name → expiry time (TimePoint{} = no expiry).
    std::unordered_map<std::string, TimePoint> locks_;
};

} // namespace concerns
} // namespace core
} // namespace themis
