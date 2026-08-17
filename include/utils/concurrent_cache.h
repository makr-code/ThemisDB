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

#include <tbb/concurrent_hash_map.h>
#include <optional>

namespace themis {

/**
 * @brief Thread-safe cache wrapper using TBB concurrent_hash_map
 * 
 * Provides convenient methods for concurrent read/write operations
 * without explicit locking. Lock-free for readers.
 */
template <typename Key, typename Value>
class ConcurrentCache {
public:
    using MapType = tbb::concurrent_hash_map<Key, Value>;
    using Accessor = typename MapType::accessor;
    using ConstAccessor = typename MapType::const_accessor;
    
    ConcurrentCache() = default;
    ~ConcurrentCache() = default;
    
    // Disable copy, allow move
    ConcurrentCache(const ConcurrentCache&) = delete;
    ConcurrentCache& operator=(const ConcurrentCache&) = delete;
    ConcurrentCache(ConcurrentCache&&) noexcept = default;
    ConcurrentCache& operator=(ConcurrentCache&&) noexcept = default;
    
    /// Insert or overwrite value
    void insert(const Key& key, const Value& value) {
        map_.insert({key, value});
    }
    
    /// Get value if exists
    std::optional<Value> get(const Key& key) const {
        ConstAccessor acc;
        if (map_.find(acc, key)) {
            return acc->second;
        }
        return std::nullopt;
    }
    
    /// Update or insert with accessor
    bool try_update(const Key& key, const Value& value) {
        Accessor acc;
        if (map_.find(acc, key)) {
            acc->second = value;
            return true;
        }
        return false;
    }
    
    /// Erase key
    bool erase(const Key& key) {
        return map_.erase(key);
    }
    
    /// Check if key exists
    bool contains(const Key& key) const {
        ConstAccessor acc;
        return map_.find(acc, key);
    }
    
    /// Get size
    size_t size() const {
        return map_.size();
    }
    
    /// Clear all entries
    void clear() {
        map_.clear();
    }
    
    /// Execute function for each entry (snapshot iteration)
    template <typename Func>
    void for_each(Func fn) const {
        for (const auto& item : map_) {
            fn(item.first, item.second);
        }
    }
    
    /// Direct access to underlying map for advanced operations
    MapType& map() { return map_; }
    const MapType& map() const { return map_; }
    
private:
    MapType map_;
};

} // namespace themis
