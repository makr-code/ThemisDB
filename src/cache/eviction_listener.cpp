/**
 * @file eviction_listener.cpp
 * @brief Implementation of cache eviction listener manager for Phase 3 integration.
 * @version 1.0.0
 * 
 * Implements the EvictionListenerManager interface for registering and emitting
 * cache eviction events to consumers (e.g., AccessCoordinator).
 * 
 * @see include/cache/eviction_listener.h
 * @see src/cache/ROADMAP.md Phase 3
 */

#include "cache/eviction_listener.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace themis {
namespace cache {

/**
 * @brief Default implementation of EvictionListenerManager.
 * 
 * Thread-safe manager for registering and emitting eviction events.
 * Uses mutex to protect listener list and incremental handles.
 */
class EvictionListenerManagerImpl : public EvictionListenerManager {
public:
    EvictionListenerManagerImpl() : next_handle_(1) {}

    ~EvictionListenerManagerImpl() override = default;

    uint64_t registerListener(std::shared_ptr<IEvictionListener> listener) override {
        if (!listener) {
            return 0;  // Invalid handle
        }

        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t handle = next_handle_++;
        listeners_[handle] = listener;
        return handle;
    }

    void unregisterListener(uint64_t handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.erase(handle);
    }

    void emitEvictionEvent(const CacheEvictionEvent& event) override {
        std::vector<std::shared_ptr<IEvictionListener>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& pair : listeners_) {
                snapshot.push_back(pair.second);
            }
        }

        // Call listeners outside lock to avoid deadlocks
        for (const auto& listener : snapshot) {
            if (listener) {
                try {
                    listener->onCacheEvicted(event);
                } catch (const std::exception& ex) {
                    // Log error but continue with other listeners
                    // In production, would use structured logging here
                    (void)ex;  // Silence unused variable warning
                }
            }
        }
    }

    void emitCapacityPressure(TierLevel from_tier,
                             uint32_t current_capacity_percent,
                             std::size_t recommended_eviction_count) override {
        std::vector<std::shared_ptr<IEvictionListener>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& pair : listeners_) {
                snapshot.push_back(pair.second);
            }
        }

        // Call listeners outside lock to avoid deadlocks
        for (const auto& listener : snapshot) {
            if (listener) {
                try {
                    listener->onCapacityPressure(from_tier, current_capacity_percent,
                                                recommended_eviction_count);
                } catch (const std::exception& ex) {
                    // Log error but continue with other listeners
                    (void)ex;  // Silence unused variable warning
                }
            }
        }
    }

    std::size_t getListenerCount() const noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<IEvictionListener>> listeners_;
    uint64_t next_handle_;
};

std::unique_ptr<EvictionListenerManager> createEvictionListenerManager() {
    return std::make_unique<EvictionListenerManagerImpl>();
}

}  // namespace cache
}  // namespace themis
