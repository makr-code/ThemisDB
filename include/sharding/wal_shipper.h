#pragma once

#include "sharding/wal_manager.h"
#include "sharding/mtls_client.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace themis::sharding {

/**
 * WAL Shipper
 * 
 * Asynchronously ships WAL entries to replica shards for replication.
 * Inspired by PostgreSQL's WAL sender process.
 * 
 * Features:
 * - Background thread for continuous shipping
 * - Retry logic with exponential backoff
 * - Replication lag monitoring
 * - Batch shipping for efficiency
 * - Automatic recovery from network failures
 */

/**
 * Replica information
 */
struct ReplicaInfo {
    std::string replica_id;
    std::string endpoint;
    LSN last_confirmed_lsn;  // Last LSN confirmed by replica
    uint64_t lag_bytes = 0;  // Replication lag in bytes
    uint64_t lag_ms = 0;     // Replication lag in milliseconds
    bool is_healthy = true;
    uint64_t last_success_ts = 0;  // Timestamp of last successful ship
    uint64_t consecutive_failures = 0;
};

/**
 * WAL Shipper Configuration
 */
struct WALShipperConfig {
    std::string primary_id;
    size_t batch_size = 100;           // Max entries per batch
    size_t max_batch_bytes = 1024 * 1024;  // 1 MB max batch size
    uint64_t ship_interval_ms = 100;   // Ship every 100ms
    uint64_t retry_delay_ms = 1000;    // Initial retry delay
    uint64_t max_retry_delay_ms = 60000;  // Max retry delay (1 min)
    size_t max_retries = 5;
    uint64_t health_check_interval_ms = 10000;  // 10 seconds
    
    // mTLS configuration
    std::string cert_path;
    std::string key_path;
    std::string ca_cert_path;
};

/**
 * WAL Shipper Statistics
 */
struct WALShipperStats {
    uint64_t total_entries_shipped = 0;
    uint64_t total_bytes_shipped = 0;
    uint64_t total_batches = 0;
    uint64_t failed_ships = 0;
    uint64_t retries = 0;
    std::chrono::milliseconds avg_ship_time{0};
    std::chrono::milliseconds max_lag{0};
};

/**
 * WAL Shipper
 * 
 * Ships WAL entries from primary to replicas asynchronously
 */
class WALShipper {
public:
    WALShipper(std::shared_ptr<WALManager> wal_manager,
               const WALShipperConfig& config);
    ~WALShipper();
    
    /**
     * Add replica to ship to
     */
    void addReplica(const std::string& replica_id, const std::string& endpoint);
    
    /**
     * Remove replica
     */
    void removeReplica(const std::string& replica_id);
    
    /**
     * Start shipping
     */
    void start();
    
    /**
     * Stop shipping
     */
    void stop();
    
    /**
     * Check if shipping is active
     */
    bool isRunning() const;
    
    /**
     * Get replica information
     */
    std::vector<ReplicaInfo> getReplicaInfo() const;
    
    /**
     * Get statistics
     */
    WALShipperStats getStatistics() const;
    
    /**
     * Force immediate ship (for testing or manual trigger)
     */
    void forceShip();

private:
    WALShipperConfig config_;
    std::shared_ptr<WALManager> wal_manager_;
    
    // Replicas
    mutable std::mutex replicas_mutex_;
    std::map<std::string, ReplicaInfo> replicas_;
    
    // Shipping thread
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> shipper_thread_;
    std::mutex cv_mutex_;
    std::condition_variable cv_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    WALShipperStats stats_;
    
    // mTLS client
    std::shared_ptr<MTLSClient> mtls_client_;
    
    /**
     * Main shipping loop
     */
    void shippingLoop();
    
    /**
     * Ship to single replica
     */
    bool shipToReplica(const std::string& replica_id, ReplicaInfo& replica);
    
    /**
     * Ship batch of entries to replica
     */
    bool shipBatch(const std::string& endpoint,
                   const std::vector<WALEntry>& entries);
    
    /**
     * Update replica status after ship attempt
     */
    void updateReplicaStatus(ReplicaInfo& replica, bool success, size_t bytes_shipped);
    
    /**
     * Calculate replication lag
     */
    void calculateLag(ReplicaInfo& replica);
    
    /**
     * Perform health check on replica
     */
    void healthCheck(ReplicaInfo& replica);
};

} // namespace themis::sharding
