/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concurrent_cache.h                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     117                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    ConcurrentCache(ConcurrentCache&&) = default;
    ConcurrentCache& operator=(ConcurrentCache&&) = default;
    
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
