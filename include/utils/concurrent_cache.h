/**
 * @file concurrent_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <mutex>
#include <optional>
#include <unordered_map>

namespace themis {

/**
 * @brief Thread-safe cache wrapper using a std::unordered_map + mutex.
 *
 * Provides a simple concurrent cache implementation when TBB containers are
 * unavailable in the build environment.
 */
template <typename Key, typename Value>
class ConcurrentCache {
public:
    using MapType = std::unordered_map<Key, Value>;
    using Accessor = std::nullptr_t;
    using ConstAccessor = std::nullptr_t;

    ConcurrentCache() = default;
    ~ConcurrentCache() = default;

    // Disable copy, allow move
    ConcurrentCache(const ConcurrentCache&) = delete;
    ConcurrentCache& operator=(const ConcurrentCache&) = delete;
    ConcurrentCache(ConcurrentCache&&) noexcept = default;
    ConcurrentCache& operator=(ConcurrentCache&&) noexcept = default;

    /// Insert or overwrite value
    void insert(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_[key] = value;
    }

    /// Get value if exists
    std::optional<Value> get(const Key& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /// Update or insert with accessor
    bool try_update(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        it->second = value;
        return true;
    }

    /// Erase key
    bool erase(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.erase(key) > 0;
    }

    /// Check if key exists
    bool contains(const Key& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.find(key) != map_.end();
    }

    /// Get size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    /// Clear all entries
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
    }

    /// Execute function for each entry (snapshot iteration)
    template <typename Func>
    void for_each(Func fn) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& item : map_) {
            fn(item.first, item.second);
        }
    }

    /// Direct access to underlying map for advanced operations
    MapType& map() { return map_; }
    const MapType& map() const { return map_; }

private:
    mutable std::mutex mutex_;
    MapType map_;
};

} // namespace themis
