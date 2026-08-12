/**
 * @file feature_flags_examples.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Example: Using Performance Feature Flags in ThemisDB Code
//
// This file demonstrates how to use the performance optimization feature flags
// in actual ThemisDB components.
//
// See: include/performance/feature_flags.h
// See: docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md

#pragma once

#include <performance/feature_flags.h>
#include <memory>
#include <cstdlib>

#ifdef THEMIS_ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

namespace themis {
namespace examples {

// Example 1: Memory Allocation with Mimalloc
// Based on: "Mimalloc: Free List Sharding in Action" (ISMM'19)
/** @brief Based on: "Mimalloc: Free List Sharding in Action" (ISMM'19). */
class AllocatorExample {
public:
    static void* allocate(size_t size) {
        if (THEMIS_PERF_MIMALLOC_ENABLED()) {
            // Use mimalloc if enabled
            #ifdef THEMIS_ENABLE_MIMALLOC
            return mi_malloc(size);
            #else
            // Fall back to standard malloc if not compiled in
            return std::malloc(size);
            #endif
        } else {
            // Standard allocation
            return std::malloc(size);
        }
    }
    
    static void deallocate(void* ptr) {
        if (THEMIS_PERF_MIMALLOC_ENABLED()) {
            #ifdef THEMIS_ENABLE_MIMALLOC
            mi_free(ptr);
            #else
            std::free(ptr);
            #endif
        } else {
            std::free(ptr);
        }
    }
};

// Example 2: Cache Implementation with LIRS Policy
// Based on: "LIRS: An Efficient Low Inter-reference Recency Set" (SIGMETRICS'02)
template<typename Key, typename Value>
/** @brief Cache usage example. */
class CacheExample {
public:
    void put(const Key& key, const Value& value) {
        if (THEMIS_PERF_LIRS_CACHE_ENABLED()) {
            // Use LIRS cache replacement policy
            put_lirs(key, value);
        } else {
            // Use standard LRU policy
            put_lru(key, value);
        }
    }
    
    Value get(const Key& key) {
        if (THEMIS_PERF_LIRS_CACHE_ENABLED()) {
            return get_lirs(key);
        } else {
            return get_lru(key);
        }
    }

private:
    void put_lru(const Key& key, const Value& value) {
        // Standard LRU implementation
        // ...
    }
    
    void put_lirs(const Key& key, const Value& value) {
        // LIRS implementation (when available)
        // Better cache hit rate for database workloads
        // ...
    }
    
    Value get_lru(const Key& key) {
        // Standard LRU get
        return Value{};
    }
    
    Value get_lirs(const Key& key) {
        // LIRS get
        return Value{};
    }
};

// Example 3: Index Access with RCU
// Based on: "Scalable Read-Mostly Synchronization Using RCU" (ASPLOS'10)
template<typename Key, typename Value>
/** @brief Index usage example. */
class IndexExample {
public:
    Value lookup(const Key& key) {
        if (THEMIS_PERF_RCU_INDEX_ENABLED()) {
            // Use RCU for lock-free reads (ideal for read-heavy workloads)
            return lookup_rcu(key);
        } else {
            // Use standard locking
            return lookup_locked(key);
        }
    }
    
    void insert(const Key& key, const Value& value) {
        if (THEMIS_PERF_RCU_INDEX_ENABLED()) {
            insert_rcu(key, value);
        } else {
            insert_locked(key, value);
        }
    }

private:
    Value lookup_locked(const Key& key) {
        // Traditional read-write lock
        // std::shared_lock lock(mutex_);
        // return index_.find(key);
        return Value{};
    }
    
    Value lookup_rcu(const Key& key) {
        // RCU read-side (zero cost for readers)
        // No locks needed!
        // return rcu_index_.find(key);
        return Value{};
    }
    
    void insert_locked(const Key& key, const Value& value) {
        // std::unique_lock lock(mutex_);
        // index_.insert(key, value);
    }
    
    void insert_rcu(const Key& key, const Value& value) {
        // RCU write-side (requires synchronization)
        // auto* new_index = index_.clone();
        // new_index->insert(key, value);
        // rcu_assign_pointer(index_, new_index);
    }
};

// Example 4: Storage Engine with WiscKey Value Separation
// Based on: "WiscKey: Separating Keys from Values" (FAST'16)
/** @brief Based on: "WiscKey: Separating Keys from Values" (FAST'16). */
class StorageExample {
public:
    void write(const std::string& key, const std::string& value) {
        if (THEMIS_PERF_WISCKEY_ENABLED() && value.size() > 1024) {
            // Large values: use WiscKey separation
            write_separated(key, value);
        } else {
            // Small values or WiscKey disabled: traditional LSM
            write_traditional(key, value);
        }
    }

private:
    void write_traditional(const std::string& key, const std::string& value) {
        // Traditional LSM: key + value together
        // Higher write amplification but simpler
    }
    
    void write_separated(const std::string& key, const std::string& value) {
        // WiscKey: store value in value log, only pointer in LSM
        // 1. Append value to value log
        // 2. Get offset/pointer
        // 3. Store key + pointer in LSM
        // Result: Much lower write amplification for large values
    }
};

// Example 5: Feature Flag Configuration at Startup
/** @brief Example 5: Feature Flag Configuration at Startup. */
class ServerStartupExample {
public:
    static void configure_performance_flags() {
        auto& flags = performance::PerformanceFeatureFlags::instance();
        
        // Load from configuration file
        // auto config = load_config("config/performance_optimizations.json");
        std::unordered_map<std::string, bool> config = {
            {"enable_mimalloc", true},
            {"enable_huge_pages", false},
            {"enable_rcu_index", true},
            {"enable_lirs_cache", true}
        };
        
        flags.load_from_config(config);
        
        // Log enabled optimizations
        auto all_flags = flags.get_all_flags();
        for (const auto& [name, enabled] : all_flags) {
            if (enabled) {
                // LOG(INFO) << "Performance optimization enabled: " << name;
            }
        }
    }
};

// Example 6: Runtime Monitoring
/** @brief Example 6: Runtime Monitoring. */
class MonitoringExample {
public:
    static void report_feature_flags_status() {
        auto& flags = performance::PerformanceFeatureFlags::instance();
        auto all_flags = flags.get_all_flags();
        
        // Send to metrics/monitoring system
        for (const auto& [name, enabled] : all_flags) {
            // metrics.gauge("performance.feature." + name, enabled ? 1 : 0);
        }
    }
};

} // namespace examples
} // namespace themis
