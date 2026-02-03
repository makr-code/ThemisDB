// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_RESOURCE_LIMITS_H
#define THEMISDB_SHARDING_RESOURCE_LIMITS_H

#include <cstddef>
#include <chrono>

namespace themisdb {
namespace sharding {

/**
 * @brief Global Resource Limits Configuration
 * 
 * Centralized configuration for resource management across all components.
 * Prevents resource exhaustion and cascading failures under load.
 */
struct ResourceLimits {
    // Transaction Management
    size_t max_pending_transactions{10000};
    size_t max_transaction_history{100000};
    std::chrono::milliseconds transaction_ttl{3600000};  // 1 hour
    
    // Memory Management
    size_t max_metadata_cache_entries{100000};
    size_t max_metadata_cache_bytes{1073741824};  // 1 GB
    size_t max_log_entries_in_memory{1000000};
    std::chrono::milliseconds cache_entry_ttl{600000};  // 10 minutes
    
    // Connection Management
    size_t max_connections_per_node{50};
    size_t max_total_connections{1000};
    std::chrono::milliseconds connection_idle_timeout{300000};  // 5 minutes
    
    // Thread Management
    size_t max_worker_threads{256};
    size_t thread_pool_size{64};
    
    // WAL Management
    size_t max_wal_segment_size{1073741824};  // 1 GB per segment
    size_t max_total_wal_size{10737418240};  // 10 GB total
    size_t max_wal_entries_per_segment{1000000};
    
    // Replication
    size_t max_pending_replications{10000};
    size_t max_replica_backlog_entries{1000000};
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_RESOURCE_LIMITS_H
