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
 *
 * @tparam Key Key type used for map lookup.
 * @tparam Value Value type stored in the cache.
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

    /**
     * @brief Inserts or overwrites a value for a key.
     * @param key Key to insert or update.
     * @param value Value to store.
     */
    void insert(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_[key] = value;
    }

    /**
     * @brief Gets a copy of the value for a key.
     * @param key Key to look up.
     * @return Stored value when present, otherwise std::nullopt.
     */
    std::optional<Value> get(const Key& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Updates an existing value for a key.
     * @param key Key to update.
     * @param value Replacement value.
     * @return true when key exists and was updated; false when key is missing.
     */
    bool try_update(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        it->second = value;
        return true;
    }

    /**
     * @brief Removes a key from the cache.
     * @param key Key to erase.
     * @return true when an entry was removed, otherwise false.
     */
    bool erase(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.erase(key) > 0;
    }

    /**
     * @brief Checks whether a key exists.
     * @param key Key to probe.
     * @return true when the key exists, otherwise false.
     */
    bool contains(const Key& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.find(key) != map_.end();
    }

    /**
     * @brief Returns the number of entries in the cache.
     * @return Entry count.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    /**
     * @brief Removes all entries from the cache.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
    }

    /**
     * @brief Executes a callback for each entry in the current snapshot.
     * @tparam Func Callable type accepting `(const Key&, const Value&)`.
     * @param fn Callback invoked for each cache entry.
     * @return No value.
     */
    template <typename Func>
    void for_each(Func fn) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& item : map_) {
            fn(item.first, item.second);
        }
    }

    /**
     * @brief Returns mutable access to the underlying map.
     * @return Mutable reference to the backing map.
     */
    MapType& map() { return map_; }

    /**
     * @brief Returns read-only access to the underlying map.
     * @return Const reference to the backing map.
     */
    const MapType& map() const { return map_; }

private:
    mutable std::mutex mutex_;
    MapType map_;
};

} // namespace themis
