/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wal_shipper.h                                      ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:43:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     323                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 16db53f833  2026-03-12  feat(sharding): implement Raft snapshot compaction and lo... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 1ee010fe7f  2026-02-25  fix(replication/audit): fix div-by-zero in WALShipper sta... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    
    // Compression configuration
    enum class CompressionType {
        None,       // No compression
        LZ4,        // Fast compression (2-4x, lower CPU)
        Zstd        // Better compression (3-10x, higher CPU)
    };
    CompressionType compression = CompressionType::Zstd;  // Default to Zstd
    int compression_level = 3;  // Zstd/LZ4 compression level (1-22 for Zstd, 1-12 for LZ4)
    
    // Adaptive batching
    bool adaptive_batch_size = false;  // Adjust batch size based on network conditions
    size_t min_batch_size = 10;        // Minimum batch size (when adaptive)
    size_t max_batch_size = 1000;      // Maximum batch size (when adaptive)
    
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
    uint64_t total_bytes_uncompressed = 0;  // Bytes before compression
    uint64_t total_batches = 0;
    uint64_t failed_ships = 0;
    uint64_t retries = 0;
    std::chrono::milliseconds avg_ship_time{0};
    std::chrono::milliseconds max_lag{0};
    double avg_compression_ratio = 1.0;  // Bytes_uncompressed / Bytes_compressed
    uint64_t total_snapshot_chunks_sent = 0;
    uint64_t total_snapshot_bytes_sent = 0;
};

/**
 * A single chunk of a snapshot being transferred to a lagging replica.
 *
 * Snapshot data is compressed on the sender side (e.g. with ZSTD) and split
 * into fixed-size pieces.  Each piece carries a SHA-256 checksum of its own
 * payload so the receiver can detect corruption independently of adjacent
 * chunks, tolerating network interruption and partial retries.
 */
struct SnapshotChunk {
    uint64_t snapshot_index;    ///< Identifies the snapshot this chunk belongs to
    uint64_t snapshot_term;     ///< Raft term of the last entry covered by the snapshot
    uint64_t chunk_index;       ///< 0-based index of this chunk within the snapshot
    uint64_t total_chunks;      ///< Total number of chunks for this snapshot
    std::vector<uint8_t> data;  ///< Compressed chunk payload
    std::string checksum;       ///< SHA-256 of this chunk's data (hex string)
    bool last_chunk = false;    ///< True if this is the final chunk
};

/**
 * Result of a snapshot send operation
 */
struct SnapshotTransferResult {
    bool success = false;
    uint64_t chunks_sent = 0;
    uint64_t bytes_sent = 0;
    std::string error_message;
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
    
    /**
     * Set Prometheus metrics exporter (optional)
     */
    void setMetricsExporter(std::shared_ptr<class PrometheusMetrics> metrics);
    
    /**
     * Phase 3: Calculate optimal batch size based on network and system metrics
     * 
     * Adapts batch size dynamically based on:
     * - Network latency (lower latency = smaller batches for lower lag)
     * - CPU utilization (lower CPU = larger batches for compression)
     * - Disk IOPS available (higher IOPS = larger batches)
     * 
     * @param network_latency_ms Average network latency in milliseconds
     * @param cpu_utilization CPU utilization (0.0 - 1.0)
     * @param disk_iops_available Available disk IOPS
     * @return Optimal batch size
     */
    size_t calculateOptimalBatchSize(double network_latency_ms,
                                     double cpu_utilization,
                                     size_t disk_iops_available) const;
    
    /**
     * Phase 3: Select optimal compression type based on payload characteristics
     * 
     * Analyzes payload to select best compression:
     * - Small payloads (<4KB): No compression (overhead not worth it)
     * - Large, repetitive payloads: Zstd (best ratio)
     * - Large, random payloads: LZ4 or None (faster)
     * - CPU constrained: LZ4 or None
     * 
     * @param payload_size Size of payload in bytes
     * @param is_repetitive Whether payload has high repetition (JSON, text)
     * @param cpu_utilization Current CPU utilization (0.0 - 1.0)
     * @return Recommended compression type
     */
    WALShipperConfig::CompressionType selectCompressionType(size_t payload_size,
                                                            bool is_repetitive,
                                                            double cpu_utilization) const;

    // ------------------------------------------------------------------
    // Snapshot transfer API (for lagging-replica catch-up)
    // ------------------------------------------------------------------

    /**
     * @brief Transfer a full snapshot to a lagging replica in fixed-size chunks.
     *
     * Each chunk is accompanied by a SHA-256 checksum of its payload.  If the
     * connection drops mid-transfer the caller may retry from the last
     * confirmed chunk index; the receiver should verify each chunk's checksum
     * before writing it to stable storage.
     *
     * @param replica_id   Target replica identifier
     * @param chunks       Ordered sequence of chunks (chunk_index 0 … N-1)
     * @return Transfer result with success/failure details
     */
    SnapshotTransferResult sendSnapshot(const std::string& replica_id,
                                        const std::vector<SnapshotChunk>& chunks);

    /**
     * @brief Verify a snapshot chunk's integrity using its embedded checksum.
     *
     * Computes SHA-256 of chunk.data and compares it against chunk.checksum.
     * Should be called by the receiver before accepting each chunk.
     *
     * @param chunk  The chunk to verify
     * @return true if the checksum matches
     */
    static bool verifyChunkChecksum(const SnapshotChunk& chunk);

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
    
    // Prometheus metrics (optional)
    std::shared_ptr<class PrometheusMetrics> metrics_;
    
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
