/**
 * @file redundancy_strategy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB RAID-like Redundancy Strategy
 * 
 * Provides RAID-inspired data distribution and redundancy patterns
 * for distributed sharding with configurable modes per collection.
 * 
 * Supported Modes:
 * - NONE:         No redundancy, pure sharding
 * - MIRROR:       Full replication (RAID-1 like)
 * - STRIPE:       Data striping for throughput (RAID-0 like)
 * - STRIPE_MIRROR: Striping + Mirroring (RAID-10 like)
 * - PARITY:       Erasure coding (RAID-5/6 like)
 * - GEO_MIRROR:   Geo-distributed replication
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <future>

#include "sharding/write_concern.h"
#include "sharding/truetime.h"

namespace themisdb {
namespace sharding {
class RaftShardManager;
}
}

namespace themis {
namespace sharding {

// Forward declarations
class ConsistentHashRing;
class ShardTopology;
struct ShardInfo;

/** @brief RAID-inspired data distribution and redundancy modes. */
enum class RedundancyMode {
    NONE,           // No redundancy, only consistent hash sharding
    MIRROR,         // Full replication to N shards (RAID-1)
    STRIPE,         // Data striping across shards (RAID-0)
    STRIPE_MIRROR,  // Striping with mirroring (RAID-10)
    PARITY,         // Erasure coding with parity (RAID-5)
    RAID6,          // Erasure coding with dual parity (RAID-6)
    GEO_MIRROR      // Geo-distributed replication
};

/** @brief Read-routing policy for replicated data. */
enum class ReadPreference {
    PRIMARY,        // Always read from primary
    NEAREST,        // Read from nearest replica (latency-based)
    ROUND_ROBIN,    // Load-balance across all replicas
    RANDOM,         // Random replica selection
    SECONDARY_ONLY, // Only read from secondaries
    FOLLOWER,       // Follower-reads (any follower, possibly stale)
    LOCAL_REGION    // Prefer shards in the local region (geo-locality)
};

/** @brief Conflict-resolution policy for async/multi-master replication flows. */
enum class ConflictResolution {
    LAST_WRITE_WINS,    // Timestamp-based
    FIRST_WRITE_WINS,   // First value preserved
    HIGHEST_NODE_ID,    // Deterministic by node ID
    CUSTOM              // Application-defined
};

/** @brief Erasure-coding backend algorithm selection. */
enum class ErasureCodingAlgorithm {
    REED_SOLOMON,       // Classic Reed-Solomon
    CAUCHY,             // Cauchy Reed-Solomon (faster)
    LRC,                // Local Reconstruction Code (Azure-style)
    HAMMING             // Hamming code (RAID-2 style, single-error correction via XOR parities)
};

/** @brief Configuration parameters for erasure-coded redundancy modes. */
struct ErasureCodingConfig {
    uint32_t data_shards = 4;       // k: Number of data chunks
    uint32_t parity_shards = 2;     // m: Number of parity chunks
    ErasureCodingAlgorithm algorithm = ErasureCodingAlgorithm::REED_SOLOMON;
    uint32_t min_document_size_kb = 1024;  // Minimum size to apply EC
    
    // Total shards needed = data_shards + parity_shards
    uint32_t totalShards() const { return data_shards + parity_shards; }
    
    // Storage efficiency = data_shards / total_shards
    double storageEfficiency() const { 
        return static_cast<double>(data_shards) / totalShards(); 
    }
    
    // Fault tolerance = parity_shards
    uint32_t faultTolerance() const { return parity_shards; }
};

/** @brief Configuration for geo-distributed replication and failover behavior. */
struct GeoReplicationConfig {
    std::string primary_datacenter;
    std::vector<std::string> replica_datacenters;

    // Region/zone placement: map from region name to list of allowed shard IDs
    // Empty map means no placement constraint (any shard is acceptable)
    std::map<std::string, std::vector<std::string>> region_shards;

    // Per-region minimum quorum for writes (region -> required acks)
    // E.g. {{"us-east", 2}, {"eu-west", 1}} means 2 acks in us-east AND 1 in eu-west
    std::map<std::string, uint32_t> region_write_quorums;

    // Per-region minimum quorum for reads (region -> required acks)
    std::map<std::string, uint32_t> region_read_quorums;

    // Local region for this node (used for LOCAL_REGION read preference)
    std::string local_region;

    enum class ReplicationMode {
        SYNC,       // Synchronous (high latency, strong consistency)
        SEMI_SYNC,  // Wait for at least one remote DC
        ASYNC       // Asynchronous (low latency, eventual consistency)
    } replication_mode = ReplicationMode::ASYNC;

    ConflictResolution conflict_resolution = ConflictResolution::LAST_WRITE_WINS;
    ReadPreference read_preference = ReadPreference::NEAREST;

    // Bounded-staleness: maximum acceptable replication lag for follower reads (ms)
    // 0 = no bound (pure follower/async reads)
    uint32_t max_staleness_ms = 0;

    // Maximum replication lag before alerts (milliseconds)
    uint32_t max_lag_ms = 10000;

    // Local datacenter optimization
    bool prefer_local_reads = true;
    bool prefer_local_writes = false;  // Only for ASYNC mode

    // Geo-failover: automatically exclude regions that have too many unhealthy shards
    bool enable_geo_failover = false;

    // Minimum fraction of healthy shards in a region before it is considered failed
    // E.g. 0.5 means a region is failed-out if <50% of its shards are healthy
    double region_failure_threshold = 0.5;

    // Regions currently marked as failed-out (populated at runtime, not set by user)
    mutable std::vector<std::string> failed_regions;
};

/** @brief Striping layout and throughput tuning parameters. */
struct StripeConfig {
    uint32_t stripe_size_kb = 64;           // Chunk size in KB
    uint32_t min_stripe_shards = 4;         // Minimum shards for striping
    bool stripe_large_documents_only = true;
    uint32_t large_document_threshold_kb = 1024;  // 1MB default
    bool parallel_stripe_io = true;         // Parallel read/write of chunks
    uint32_t max_parallel_io = 8;           // Max concurrent I/O operations
};

/** @brief Hot-spare failover and rebuild tuning parameters. */
struct HotSpareConfigSimple {
    bool enable = false;
    std::vector<std::string> spare_shards;
    bool auto_rebuild = true;
    uint32_t rebuild_throttle_mbps = 100;
    std::chrono::seconds health_check_interval{30};
};

/** @brief Top-level redundancy policy configuration for one collection/strategy. */
struct RedundancyConfig {
    RedundancyMode mode = RedundancyMode::MIRROR;
    
    // Replication settings (for MIRROR, STRIPE_MIRROR, GEO_MIRROR)
    uint32_t replication_factor = 3;
    ReadPreference read_preference = ReadPreference::NEAREST;
    WriteConcern write_concern = WriteConcern::MAJORITY;
    
    // Quorum settings
    uint32_t read_quorum = 1;   // For quorum reads
    uint32_t write_quorum = 2;  // For quorum writes
    bool enable_quorum_enforcement = false;  // Enable quorum-based consistency (default: OFF for RC1)
    bool enable_partition_detection = false;  // Enable network partition detection
    bool enable_raft_consensus = false;       // Enable Raft consensus for writes
    
    // Stripe settings (for STRIPE, STRIPE_MIRROR)
    StripeConfig stripe;
    
    // Erasure coding settings (for PARITY mode)
    ErasureCodingConfig erasure_coding;
    
    // Geo-replication settings (for GEO_MIRROR mode)
    GeoReplicationConfig geo_replication;
    
    // Hot spare settings (for automatic failover and rebuild)
    HotSpareConfigSimple hot_spare;
    
    // Timing
    std::chrono::milliseconds replication_timeout{5000};
    std::chrono::milliseconds health_check_interval{1000};
    
    // Recovery
    bool auto_recovery = true;
    uint32_t recovery_parallelism = 4;
    
    // Validate configuration
    bool validate() const;
    
    // Get storage efficiency (0.0 to 1.0)
    double getStorageEfficiency() const;
    
    // Get fault tolerance (number of failures tolerated)
    uint32_t getFaultTolerance() const;
    
    // Get effective replication factor
    uint32_t getEffectiveReplicationFactor() const;
};

/** @brief Metadata for one striped/parity chunk of a logical document. */
struct ChunkInfo {
    std::string chunk_id;
    std::string document_id;
    uint32_t chunk_index;
    uint32_t total_chunks;
    uint64_t offset;
    uint64_t size;
    std::string shard_id;
    std::string checksum;           // CRC32 or SHA256
    bool is_parity = false;         // For erasure coding
    
    // Serialize to binary
    std::vector<uint8_t> serialize() const;
    static std::optional<ChunkInfo> deserialize(const std::vector<uint8_t>& data);
};

/** @brief Logical chunk group representing one striped/parity document layout. */
struct StripeGroup {
    std::string document_id;
    std::vector<ChunkInfo> data_chunks;
    std::vector<ChunkInfo> parity_chunks;  // For PARITY mode
    std::chrono::system_clock::time_point created_at;
    uint64_t total_size;
    
    // Check if all chunks are available
    bool isComplete() const;
    
    // Get missing chunk indices
    std::vector<uint32_t> getMissingChunks() const;
    
    // Can recover from erasure coding?
    bool canRecover(uint32_t data_shards, uint32_t parity_shards) const;
};

/** @brief Result payload for write path with redundancy fanout metadata. */
struct WriteResult {
    bool success;
    std::string document_id;
    std::vector<std::string> written_shards;
    std::vector<std::string> failed_shards;
    uint32_t acknowledgements;
    std::chrono::milliseconds latency;
    std::string error_message;
    
    static WriteResult successful(const std::string& doc_id, 
                                  const std::vector<std::string>& shards,
                                  std::chrono::milliseconds lat);
    static WriteResult failed(const std::string& doc_id, 
                             const std::string& error);
};

/** @brief Result payload for read path including source, chunk, and snapshot-version metadata. */
struct ReadResult {
    bool success;
    std::string document_id;
    std::string data;
    std::string source_shard;
    std::chrono::milliseconds latency;
    bool from_replica;
    uint32_t chunks_read;  // For striped documents
    uint64_t version_token = 0;  // Monotonic token for merged/snapshotted reads
    std::string error_message;
};

/** @brief Aggregated runtime statistics for redundancy strategy activity. */
struct RedundancyStats {
    uint64_t total_documents;
    uint64_t total_replicas;
    uint64_t total_chunks;
    uint64_t parity_chunks;
    
    // Storage metrics
    uint64_t logical_bytes;     // Actual data size
    uint64_t physical_bytes;    // Storage used (with redundancy)
    double storage_efficiency;  // logical / physical
    
    // Performance metrics
    uint64_t reads_from_primary;
    uint64_t reads_from_replica;
    uint64_t stripe_reads;
    uint64_t degraded_reads;    // Reads that required recovery
    
    // Recovery metrics
    uint64_t recovery_operations;
    uint64_t recovered_bytes;
    std::chrono::milliseconds avg_recovery_time;
    
    // Health
    uint32_t healthy_shards;
    uint32_t unhealthy_shards;
    uint32_t degraded_documents;  // Documents with missing replicas
};

/** @brief Interface for erasure-coding implementations used by parity modes. */
class ErasureCoder {
public:
    virtual ~ErasureCoder() = default;
    
    // Encode data into data + parity chunks
    virtual std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) = 0;
    
    // Decode/recover original data from available chunks
    // available_chunks: map from chunk index to chunk data
    // missing_indices: indices of missing chunks
    virtual std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) = 0;
    
    // Factory method
    static std::unique_ptr<ErasureCoder> create(ErasureCodingAlgorithm algorithm);
};

/**
 * Reed-Solomon Erasure Coder
 * Uses a systematic Vandermonde-based encoding matrix for full multi-chunk
 * erasure recovery (up to parity_shards simultaneous failures).
 */
class ReedSolomonCoder : public ErasureCoder {
public:
    std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;
    
    std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;
    
private:
    // Galois Field GF(2^8) operations with irreducible polynomial x^8+x^4+x^3+x^2+1 (0x1d)
    uint8_t gf_mul(uint8_t a, uint8_t b);
    uint8_t gf_inv(uint8_t a);
    uint8_t gf_div(uint8_t a, uint8_t b);
    uint8_t gf_pow(uint8_t a, uint8_t exp);
    void gf_matrix_mul(const std::vector<std::vector<uint8_t>>& matrix,
                       const std::vector<uint8_t>& vec,
                       std::vector<uint8_t>& result);
    // Build Vandermonde parity matrix (parity_shards x data_shards)
    // V[p][j] = gf_pow(p+1, j)
    std::vector<std::vector<uint8_t>> buildVandermondeMatrix(uint32_t rows, uint32_t cols);
    // Gaussian elimination in GF(2^8) for matrix inversion
    bool invertMatrix(std::vector<std::vector<uint8_t>>& matrix);
};

/**
 * Cauchy Reed-Solomon Erasure Coder
 * Optimized for RAID 6 dual-parity encoding/decoding
 */
class CauchyReedSolomonCoder : public ErasureCoder {
public:
    std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;
    
    std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;
    
private:
    // Cauchy matrix operations
    std::vector<std::vector<uint8_t>> buildCauchyMatrix(uint32_t rows, uint32_t cols);
    
    // Optimized Galois Field operations for Cauchy
    uint8_t gf_mul(uint8_t a, uint8_t b);
    uint8_t gf_inv(uint8_t a);
    void gf_matrix_mul(const std::vector<std::vector<uint8_t>>& matrix,
                       const std::vector<uint8_t>& vec,
                       std::vector<uint8_t>& result);
    
    // Matrix inversion for recovery
    bool invertMatrix(std::vector<std::vector<uint8_t>>& matrix);
};

/**
 * Locally Repairable Code (LRC) Erasure Coder
 *
 * LRC organises @p data_shards into local groups.  Each group has one XOR
 * local-parity shard so a single failure in that group can be repaired by
 * reading only the other group members rather than all data shards.
 * The remaining parity budget (parity_shards − n_local_groups) is spent on
 * global Vandermonde parity shards that cover all data shards.
 *
 * Layout (total n = data_shards + parity_shards shards):
 *   [d0 … dk-1] [lp0 … lp(g-1)] [gp0 … gp(m-1)]
 *   where g = n_local_groups, m = parity_shards − g
 *
 * Default local group size is kDefaultLocalGroupSize (4).  The value is
 * capped so that g ≤ parity_shards.
 */
class LocallyRepairableCoder : public ErasureCoder {
public:
    static constexpr uint32_t kDefaultLocalGroupSize = 4;

    std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;

    std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;

private:
    // Return number of local groups for given data/parity counts.
    static uint32_t localGroupCount(uint32_t data_shards, uint32_t parity_shards);

    // GF(2^8) helpers (shared Vandermonde parity logic)
    static uint8_t gf_mul(uint8_t a, uint8_t b);
    static uint8_t gf_inv(uint8_t a);
    static uint8_t gf_pow(uint8_t a, uint8_t exp);
    static void gf_matrix_mul(const std::vector<std::vector<uint8_t>>& m,
                               const std::vector<uint8_t>& v,
                               std::vector<uint8_t>& result);
    static bool invertMatrix(std::vector<std::vector<uint8_t>>& matrix);
    static std::vector<std::vector<uint8_t>> buildVandermonde(uint32_t rows, uint32_t cols);
};

/**
 * Hamming Erasure Coder
 *
 * Implements a generalised RAID-2 / Hamming-code erasure coder operating at
 * shard (block) granularity rather than at the bit level.
 *
 * Parity assignment:
 *   Parity shard p (0-indexed) covers every data shard j (0-indexed) for
 *   which bit p is set in the 1-based position (j + 1):
 *
 *     parity[p] = XOR{ data[j]  for all j in [0, data_shards)
 *                      where ((j + 1) >> p) & 1 == 1 }
 *
 * Properties:
 *   - Encode / decode uses pure XOR — no Galois-Field arithmetic needed.
 *   - Single data-shard failure can always be recovered via syndrome
 *     detection (O(data_shards) work per byte).
 *   - A missing parity shard can be recomputed directly from data shards.
 *   - Best configured with parity_shards = ceil(log2(data_shards + r + 1))
 *     so that syndromes are unique for every possible single failure.
 *   - When more than one data shard is missing the coder attempts iterative
 *     repair using parity shards that cover exactly one of the missing
 *     shards; if impossible it throws std::runtime_error.
 *
 * Example: 4 data shards → 3 parity shards (Hamming(7,4) shard analogue)
 */
class HammingCoder : public ErasureCoder {
public:
    /**
     * Encode @p data into (data_shards + parity_shards) chunks.
     *
     * The first data_shards chunks are systematic (raw data).
     * Chunks at indices [data_shards, data_shards + parity_shards) are parity.
     *
     * @throws std::invalid_argument if data is empty or shard counts are 0.
     */
    std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;

    /**
     * Recover original data from an incomplete set of chunks.
     *
     * @param available_chunks  Map of chunk index → chunk bytes for every
     *                          shard that is present.
     * @param missing_indices   Indices of shards that are unavailable.
     * @param data_shards       k (number of data shards used during encode).
     * @param parity_shards     r (number of parity shards used during encode).
     * @return                  Concatenated recovered data shards (may be
     *                          zero-padded at the end if encode padded).
     * @throws std::runtime_error if too many data shards are missing to recover.
     */
    std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;
};

/**
 * Redundancy Strategy
 * Main class for managing RAID-like redundancy
 */
class RedundancyStrategy {
public:
    using WriteHandler = std::function<bool(const std::string& shard_id, 
                                            const std::string& doc_id,
                                            const std::vector<uint8_t>& data)>;
    
    using ReadHandler = std::function<std::optional<std::vector<uint8_t>>(
                                            const std::string& shard_id,
                                            const std::string& doc_id)>;
    
    /** @brief Read handler with version token support for consistency checking */
    struct VersionedReadResult {
        std::optional<std::vector<uint8_t>> data;
        uint64_t version_token = 0;  // Monotonic version for consistency
        std::string shard_id;         // Source shard identifier
    };
    
    using ReadHandlerWithVersion = std::function<VersionedReadResult(
                                            const std::string& shard_id,
                                            const std::string& doc_id)>;
    
    /** @brief Versioned chunk with source metadata for consistency checking */
    struct VersionedChunk {
        std::vector<uint8_t> data;
        uint64_t version_token = 0;
        std::string shard_id;
    };

    /** @brief Construct strategy for a given redundancy configuration. */
    explicit RedundancyStrategy(const RedundancyConfig& config);
    /** @brief Destroy strategy and associated coder resources. */
    ~RedundancyStrategy();
    
    /** @brief Write document using the currently configured redundancy mode. */
    WriteResult write(
        const std::string& document_id,
        const std::vector<uint8_t>& data,
        const std::string& collection,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        WriteHandler handler
    );
    
    /** @brief Read document using configured read preference and redundancy mode.
     *  @return ReadResult annotated with a monotonic version_token for callers
     *          that need to detect stale cross-shard snapshots.
     */
    ReadResult read(
        const std::string& document_id,
        const std::string& collection,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandler handler
    );
    
    /** @brief Remove document from all replicas/chunks according to mode semantics. */
    bool remove(
        const std::string& document_id,
        const std::string& collection,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        WriteHandler handler  // Sends delete command
    );
    
    /** @brief Attempt recovery of degraded/unavailable document replicas/chunks. */
    bool recoverDocument(
        const std::string& document_id,
        const std::string& collection,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandler read_handler,
        WriteHandler write_handler
    );
    
    /** @brief Health snapshot for one logical document across redundancy layout. */
    struct DocumentHealth {
        /** @brief True when no required replica/chunk is missing. */
        bool is_healthy;
        /** @brief Number of replicas/chunks that are currently readable. */
        uint32_t available_replicas;
        /** @brief Replica/chunk count required by active redundancy mode. */
        uint32_t required_replicas;
        /** @brief Shards that should contain data but currently do not. */
        std::vector<std::string> missing_shards;
        /** @brief True when recovery path can reconstruct missing data. */
        bool can_recover;
    };
    
    /**
     * @brief Evaluate document health across expected replicas/chunks.
     * @param document_id Logical document identifier.
     * @param collection Collection name (reserved for collection-level policy context).
     * @param ring Hash-ring resolver for primary/replica placement.
     * @param topology Topology source used for shard-health filtering.
     * @param handler Read callback used to probe replica/chunk availability.
     * @return Health summary with availability and recoverability assessment.
     */
    DocumentHealth checkDocumentHealth(
        const std::string& document_id,
        const std::string& collection,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandler handler
    );
    
    /** @brief Return currently active strategy configuration. */
    const RedundancyConfig& getConfig() const { return config_; }
    
    /** @brief Update strategy configuration (dynamic reconfiguration). */
    void updateConfig(const RedundancyConfig& config);
    
    /** @brief Return aggregated redundancy runtime statistics. */
    RedundancyStats getStats() const;
    
    /** @brief Export strategy metrics in Prometheus text exposition format. */
    std::string exportPrometheusMetrics() const;
    
    /**
     * @brief Set Raft shard manager for consensus-based writes
     * @param raft_manager Shared pointer to RaftShardManager
     */
    void setRaftShardManager(std::shared_ptr<themisdb::sharding::RaftShardManager> raft_manager);

    /**
     * @brief Record an observed round-trip latency for a shard.
     *
     * Callers (e.g. the RPC layer) should call this after each successful read
     * so that ReadPreference::NEAREST can select the shard with the lowest
     * recent latency.  The value is incorporated into a per-shard exponential
     * moving average (α = 0.2).
     *
     * @param shard_id  Identifier of the shard that was contacted
     * @param latency_ms Observed round-trip latency in milliseconds
     */
    void recordShardLatency(const std::string& shard_id, double latency_ms);

private:
    RedundancyConfig config_;
    std::unique_ptr<ErasureCoder> erasure_coder_;
    mutable std::shared_mutex mutex_;
    
    // Raft shard manager for consensus-based writes (optional)
    std::shared_ptr<themisdb::sharding::RaftShardManager> raft_manager_;

    // TrueTime clock for globally consistent timestamps in read operations
    std::unique_ptr<TrueTime> truetime_;

    // Per-shard exponential moving average latency (ms) for NEAREST routing.
    // Protected by latency_mutex_ (separate from mutex_ to avoid blocking reads
    // while latency updates are in progress).
    mutable std::mutex latency_mutex_;
    std::unordered_map<std::string, double> shard_latency_ewma_ms_;
    static constexpr double kLatencyEwmaAlpha = 0.2;  // smoothing factor

    /// @brief LOCK ORDERING (CANONICAL):
    /// Tier 1: mutex_          — protects config, erasure_coder, mode-specific state
    ///   ↓ can acquire Tier 2 while holding this lock
    /// Tier 2: latency_mutex_  — protects per-shard latency EWMA metrics (independent cache)
    ///   ↓ terminal tier; no further acquisitions
    ///
    /// RATIONALE: mutex_ guards primary write/read state; latency_mutex_ is independent
    /// to allow latency recording without blocking core read/write operations.
    /// All acquisitions must follow this hierarchy to avoid deadlock.
    ///
    
    // Statistics
    // Note: Atomic counters are relaxed since they're diagnostic; no synchronization required with other fields.
    std::atomic<uint64_t> stats_writes_{0};
    std::atomic<uint64_t> stats_reads_{0};
    std::atomic<uint64_t> stats_recoveries_{0};
    std::atomic<uint64_t> stats_bytes_written_{0};
    std::atomic<uint64_t> stats_bytes_read_{0};
    
    /**
     * @brief Check if Raft consensus is enabled and write should go through leader
     * @param shard_id Shard to check
     * @return true if Raft is enabled and shard has leader
     */
    bool shouldUseRaftConsensus(const std::string& shard_id) const;
    
    /**
     * @brief Propose write through Raft consensus (for leader enforcement)
     * @param shard_id Target shard
     * @param document_id Document ID
     * @param data Data to write
     * @return true if write was successfully proposed and committed
     */
    bool proposeRaftWrite(const std::string& shard_id,
                         const std::string& document_id,
                         const std::vector<uint8_t>& data);
    
    // Internal write methods for each mode
    WriteResult writeMirror(
        const std::string& document_id,
        const std::vector<uint8_t>& data,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        WriteHandler handler
    );
    
    WriteResult writeStripe(
        const std::string& document_id,
        const std::vector<uint8_t>& data,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        WriteHandler handler
    );
    
    WriteResult writeStripeMirror(
        const std::string& document_id,
        const std::vector<uint8_t>& data,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        WriteHandler handler
    );
    
    WriteResult writeParity(
        const std::string& document_id,
        const std::vector<uint8_t>& data,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        WriteHandler handler
    );
    
    WriteResult writeGeoMirror(
        const std::string& document_id,
        const std::vector<uint8_t>& data,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        WriteHandler handler
    );
    
    // Internal read methods
    ReadResult readMirror(
        const std::string& document_id,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandler handler
    );
    
    /** @brief Version-aware read with consistency checking */
    ReadResult readMirrorWithVersion(
        const std::string& document_id,
        const std::string& collection,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandlerWithVersion handler
    );

    ReadResult readGeoMirror(
        const std::string& document_id,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandler handler
    );
    
    ReadResult readStripe(
        const std::string& document_id,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandler handler
    );
    
    ReadResult readParity(
        const std::string& document_id,
        ConsistentHashRing& ring,
        ShardTopology& topology,
        ReadHandler handler
    );
    
    // Utility methods
    std::vector<std::vector<uint8_t>> splitIntoChunks(
        const std::vector<uint8_t>& data,
        size_t chunk_size
    );
    
    std::vector<uint8_t> mergeChunks(
        const std::vector<std::vector<uint8_t>>& chunks
    );
    
    /**
     * @brief Merge chunks with version consistency checking and conflict resolution
     * 
     * Resolves GAP: undefined_conflict_resolution, unspecified_consistency, missing_version_tracking
     */
    std::vector<uint8_t> mergeChunksWithConsistency(
        const std::vector<VersionedChunk>& versioned_chunks,
        ConflictResolution conflict_resolution,
        uint64_t& result_version
    );
    
    std::string selectReadShard(
        const std::vector<std::string>& available_shards,
        ShardTopology& topology
    );

    // Select the best shard from candidates, preferring shards in local_region
    // when config is GEO_MIRROR with LOCAL_REGION or FOLLOWER read preference.
    std::string selectGeoReadShard(
        const std::vector<std::string>& candidates,
        ShardTopology& topology,
        const std::string& local_region
    );

    // Evaluate geo-failover: mark regions as failed-out based on health thresholds
    void evaluateGeoFailover(ShardTopology& topology) const;
    
    bool waitForWriteConcern(
        const std::vector<std::future<bool>>& futures,
        WriteConcern concern,
        uint32_t total_shards
    );
};

/**
 * Per-Collection Redundancy Manager
 * Allows different redundancy configurations per collection
 */
class CollectionRedundancyManager {
public:
    /** @brief Construct collection-level redundancy manager. */
    CollectionRedundancyManager();
    /** @brief Destroy manager and owned strategy instances. */
    ~CollectionRedundancyManager();
    
    /** @brief Set default redundancy config used when collection override is absent. */
    void setDefaultConfig(const RedundancyConfig& config);
    
    /** @brief Set or replace per-collection redundancy configuration. */
    void setCollectionConfig(const std::string& collection, 
                            const RedundancyConfig& config);
    
    /** @brief Get effective config for collection (default fallback when unset). */
    RedundancyConfig getConfig(const std::string& collection) const;
    
    /** @brief Get lazily-created strategy instance for collection. */
    std::shared_ptr<RedundancyStrategy> getStrategy(const std::string& collection);
    
    /** @brief List collections with explicit configuration overrides. */
    std::vector<std::string> listCollections() const;
    
    /** @brief Remove collection override and associated cached strategy. */
    void removeCollectionConfig(const std::string& collection);
    
private:
    RedundancyConfig default_config_;
    std::map<std::string, RedundancyConfig> collection_configs_;
    std::map<std::string, std::shared_ptr<RedundancyStrategy>> strategies_;
    mutable std::shared_mutex mutex_;
};

} // namespace sharding
} // namespace themis

// Backward compatibility shim: expose under themisdb::sharding
namespace themisdb {
namespace sharding {
using namespace themis::sharding;
}
}
