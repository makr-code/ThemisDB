#pragma once

#include "core/concerns/lifecycle.h"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// Change-listener callback interface
// ---------------------------------------------------------------------------

/**
 * @brief Callback interface for configuration hot-reload notifications.
 *
 * Implement this interface and pass an instance to
 * @c IConfigHotReloader::addListener() to receive asynchronous notifications
 * whenever a configuration value changes.
 *
 * Thread-safety: @c onConfigChanged() may be called from any thread; the
 * listener implementation must therefore be thread-safe.  The call is
 * guaranteed to be non-blocking on the calling thread's side — delivery is
 * dispatched asynchronously by the reloader implementation.
 */
class IConfigChangeListener {
public:
    virtual ~IConfigChangeListener() = default;

    /**
     * @brief Invoked after a configuration key has been updated.
     *
     * @param key       The configuration key that changed.
     * @param old_value The previous value (empty string if the key is new).
     * @param new_value The updated value.
     */
    virtual void onConfigChanged(std::string_view key,
                                 std::string_view old_value,
                                 std::string_view new_value) = 0;
};

// ---------------------------------------------------------------------------
// Hot-reloader interface
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for runtime configuration hot-reload.
 *
 * Provides a key/value store whose changes are broadcast asynchronously to
 * registered @c IConfigChangeListener instances without blocking the thread
 * that calls @c set() or @c reload().
 *
 * Typical use cases:
 *  - Adjusting log levels without restarting the database process.
 *  - Switching tracing backends at runtime.
 *  - Enabling / disabling features via config changes instead of feature flags.
 *
 * Thread-safety: all public methods must be safe to call concurrently from
 * multiple threads.
 *
 * Lifecycle: honour @c flush() and @c shutdown() so that in-flight listener
 * notifications are completed before the process exits.
 */
class IConfigHotReloader {
public:
    virtual ~IConfigHotReloader() = default;

    // -----------------------------------------------------------------------
    // Listener management
    // -----------------------------------------------------------------------

    /**
     * @brief Register a change listener.
     *
     * Listeners are held as @c weak_ptr internally when possible so that
     * expired listeners are pruned automatically on the next change.
     *
     * @param listener Non-null listener to register.
     */
    virtual void addListener(std::shared_ptr<IConfigChangeListener> listener) = 0;

    /**
     * @brief Remove a previously registered listener.
     *
     * No-op if the listener is not currently registered.
     */
    virtual void removeListener(const std::shared_ptr<IConfigChangeListener>& listener) = 0;

    // -----------------------------------------------------------------------
    // Configuration access
    // -----------------------------------------------------------------------

    /**
     * @brief Return the current value for a key, or @c nullopt if absent.
     */
    virtual std::optional<std::string> get(std::string_view key) const = 0;

    /**
     * @brief Return the current value for a key with a fallback default.
     */
    virtual std::string getOrDefault(std::string_view key,
                                     std::string_view default_value) const {
        auto val = get(key);
        return val ? *val : std::string(default_value);
    }

    /**
     * @brief Set a configuration key to a new value and notify listeners.
     *
     * Listener notifications are dispatched asynchronously; this method
     * returns immediately after updating the stored value.
     *
     * @param key   Configuration key.
     * @param value New value.
     */
    virtual void set(std::string_view key, std::string_view value) = 0;

    /**
     * @brief Remove a key from the configuration store.
     *
     * Notifies listeners with @c new_value = "" if the key existed.
     */
    virtual void remove(std::string_view key) = 0;

    /**
     * @brief Return all current key/value pairs as a snapshot.
     */
    virtual std::unordered_map<std::string, std::string> snapshot() const = 0;

    /**
     * @brief Re-load configuration from the backing source (e.g. file, etcd).
     *
     * In-memory implementations perform a no-op.  File-backed or remote
     * implementations re-read the source and notify listeners for any keys
     * whose values have changed.
     */
    virtual void reload() {}

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Flush pending async listener notifications.
     *
     * Blocks until all queued notifications have been delivered.  Safe to
     * call multiple times.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down background threads and release resources.
     *
     * After @c shutdown() any further calls have undefined behaviour.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Return whether the reloader is operational.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

// ---------------------------------------------------------------------------
// In-memory implementation (single-process, synchronous notification)
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe in-memory configuration hot-reloader.
 *
 * Changes made via @c set() / @c remove() are applied atomically under a
 * mutex.  Listener notifications are delivered *synchronously* (still within
 * the @c set() call) but without holding the internal lock, so listeners may
 * call back into the reloader without deadlocking.
 *
 * Suitable for unit tests and single-process deployments.  For production
 * multi-node deployments use a distributed backing store (etcd, Consul) and
 * implement a remote-backed subclass.
 */
class InMemoryConfigHotReloader : public IConfigHotReloader {
public:
    InMemoryConfigHotReloader() = default;

    /**
     * @brief Construct with initial key/value pairs.
     */
    explicit InMemoryConfigHotReloader(
            std::unordered_map<std::string, std::string> initial)
        : config_(std::move(initial)) {}

    // -- IConfigHotReloader --------------------------------------------------

    void addListener(std::shared_ptr<IConfigChangeListener> listener) override {
        if (!listener) return;
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.push_back(listener);
    }

    void removeListener(const std::shared_ptr<IConfigChangeListener>& listener) override {
        if (!listener) return;
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.erase(
            std::remove_if(listeners_.begin(), listeners_.end(),
                [&](const std::weak_ptr<IConfigChangeListener>& wp) {
                    auto sp = wp.lock();
                    return !sp || sp == listener;
                }),
            listeners_.end());
    }

    std::optional<std::string> get(std::string_view key) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = config_.find(std::string(key));
        if (it == config_.end()) return std::nullopt;
        return it->second;
    }

    void set(std::string_view key, std::string_view value) override {
        std::string old_value;
        std::vector<std::shared_ptr<IConfigChangeListener>> live_listeners;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = config_.find(std::string(key));
            if (it != config_.end()) old_value = it->second;
            config_[std::string(key)] = std::string(value);
            live_listeners = collectLiveListeners();
        }

        // Notify outside the lock.
        for (const auto& l : live_listeners) {
            l->onConfigChanged(key, old_value, value);
        }
    }

    void remove(std::string_view key) override {
        std::string old_value;
        bool existed = false;
        std::vector<std::shared_ptr<IConfigChangeListener>> live_listeners;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = config_.find(std::string(key));
            if (it != config_.end()) {
                old_value = it->second;
                existed   = true;
                config_.erase(it);
            }
            if (existed) live_listeners = collectLiveListeners();
        }

        if (existed) {
            for (const auto& l : live_listeners) {
                l->onConfigChanged(key, old_value, "");
            }
        }
    }

    std::unordered_map<std::string, std::string> snapshot() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    void reload() override { /* in-memory: no external source to re-read */ }
    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

private:
    /**
     * @brief Collect live listener shared_ptrs and prune expired weak_ptrs.
     *
     * Must be called while holding @c mutex_.
     */
    std::vector<std::shared_ptr<IConfigChangeListener>> collectLiveListeners() {
        std::vector<std::shared_ptr<IConfigChangeListener>> live;
        std::vector<std::weak_ptr<IConfigChangeListener>>  kept;
        for (auto& wp : listeners_) {
            if (auto sp = wp.lock()) {
                live.push_back(sp);
                kept.push_back(wp);
            }
        }
        listeners_ = std::move(kept);
        return live;
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> config_;
    std::vector<std::weak_ptr<IConfigChangeListener>> listeners_;
};

} // namespace concerns
} // namespace core
} // namespace themis
