/**
 * @file gpu_memory_oversubscription.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "themis/gpu/unified_memory.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace index {

/**
 * @brief Prefetching strategy for the GPU memory oversubscription manager.
 *
 * Controls which cold partitions are proactively migrated into VRAM before
 * they are explicitly accessed by a search operation.
 *
 *  NONE       – No prefetching; partitions are loaded strictly on demand.
 *  LRU        – Prefetch the cold partition that was evicted least recently
 *               (i.e. the one most likely to become hot again soon).
 *  MRU        – Prefetch the cold partition that was used most recently
 *               (keep frequently-used data warm).
 *  SEQUENTIAL – When partition N is accessed, prefetch partition N+1 in
 *               insertion order.  Optimal for sequential scan workloads.
 */
enum class PrefetchStrategy {
    NONE,       ///< On-demand loading only; no background prefetch.
    LRU,        ///< Prefetch cold partition closest to becoming hot (LRU heuristic).
    MRU,        ///< Prefetch cold partition that was most recently accessed.
    SEQUENTIAL  ///< Prefetch next partition by insertion-order ID.
};

/**
 * @brief GPU memory oversubscription manager for the index domain.
 *
 * Enables vector datasets larger than the available GPU VRAM by partitioning
 * the index into fixed-size chunks ("partitions").  Hot partitions reside in
 * VRAM (or CUDA/HIP unified memory) for full-speed GPU queries; cold
 * partitions remain in host RAM and are streamed on demand.
 *
 * Features
 * --------
 * - **Unified Memory**: Uses GPUUnifiedMemoryAllocator (cudaMallocManaged /
 *   hipMallocManaged) for transparent page migration when available.
 * - **Streaming**: Partitions are loaded from host RAM into VRAM on access
 *   and evicted when the VRAM budget is exhausted.
 * - **LRU Eviction**: A Least-Recently-Used policy keeps the hottest
 *   partitions in VRAM and evicts the coldest ones first.
 * - **Prefetching**: Configurable strategy (NONE, LRU, MRU, SEQUENTIAL)
 *   predicts and prefetches upcoming partitions to hide PCIe latency.
 * - **Multi-GPU support**: Partition IDs are stable across GPU boundaries;
 *   MultiGPUVectorIndex distributes partitions across devices.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 *
 * CPU-only builds
 * ---------------
 * When neither THEMIS_ENABLE_CUDA nor THEMIS_ENABLE_HIP is defined the
 * allocator falls back to ordinary heap memory.  All API calls succeed and
 * statistics are maintained, but no actual GPU migration occurs.  This allows
 * the full control-plane logic (LRU eviction, prefetching, statistics) to be
 * exercised and tested without GPU hardware.
 */
class GPUMemoryOversubscriptionManager {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Enable oversubscription support (must be true to activate the manager).
        bool enable_oversubscription = false;

        /// Maximum VRAM budget in MB.  0 = unlimited (all partitions stay hot).
        size_t vram_budget_mb = 0;

        /// Maximum host RAM budget in MB.  0 = unlimited.
        size_t host_ram_budget_mb = 0;

        /// Number of vectors per partition chunk.
        size_t partition_vectors = 65536;

        /// Prefetch strategy applied whenever a partition is accessed.
        PrefetchStrategy prefetch_strategy = PrefetchStrategy::LRU;

        /// Use cudaMallocManaged / hipMallocManaged when available.
        bool use_unified_memory = true;
    };

    // -----------------------------------------------------------------------
    // Per-partition information (read-only view)
    // -----------------------------------------------------------------------

    struct PartitionInfo {
        size_t   partition_id   = 0;
        size_t   num_vectors    = 0;
        size_t   dimension      = 0;
        bool     in_vram        = false;
        uint64_t last_access_ns = 0;
        size_t   access_count   = 0;
        std::string tag;
    };

    // -----------------------------------------------------------------------
    // Aggregate statistics
    // -----------------------------------------------------------------------

    struct Stats {
        size_t total_partitions    = 0;  ///< All managed partitions.
        size_t hot_partitions      = 0;  ///< Partitions currently in VRAM.
        size_t cold_partitions     = 0;  ///< Partitions currently in host RAM only.
        size_t vram_used_bytes     = 0;  ///< Bytes currently resident in VRAM.
        size_t host_ram_used_bytes = 0;  ///< Bytes resident in host RAM.
        size_t vram_budget_bytes   = 0;  ///< Effective VRAM budget (0 = unlimited).
        size_t evictions           = 0;  ///< Total LRU evictions performed.
        size_t loads               = 0;  ///< Total partition loads into VRAM.
        size_t prefetch_requests   = 0;  ///< Prefetch requests issued.
        size_t prefetch_hits       = 0;  ///< Prefetch requests already hot.
        double prefetch_hit_rate   = 0.0;///< prefetch_hits / prefetch_requests.
    };

    // -----------------------------------------------------------------------
    // Constructors / Destructor
    // -----------------------------------------------------------------------

    GPUMemoryOversubscriptionManager();
    explicit GPUMemoryOversubscriptionManager(const Config& config);
    ~GPUMemoryOversubscriptionManager();

    GPUMemoryOversubscriptionManager(const GPUMemoryOversubscriptionManager&) = delete;
    GPUMemoryOversubscriptionManager& operator=(const GPUMemoryOversubscriptionManager&) = delete;

    // -----------------------------------------------------------------------
    // Partition management
    // -----------------------------------------------------------------------

    /**
     * @brief Add a pre-flattened block of vectors as a new partition.
     *
     * The data is copied into host RAM.  The partition starts cold; call
     * accessPartition() to migrate it to VRAM.
     *
     * @param flat_data   Row-major float array: [v0_d0…v0_dD, v1_d0…, …].
     * @param num_vectors Number of vectors in this partition.
     * @param dimension   Dimensionality of each vector.
     * @param tag         Optional diagnostic label.
     * @return Assigned partition ID (monotonically increasing from 0).
     */
    size_t addPartition(const std::vector<float>& flat_data,
                        size_t num_vectors,
                        size_t dimension,
                        const std::string& tag = "");

    /**
     * @brief Remove a partition from the manager.
     *
     * If the partition is currently VRAM-resident it is evicted first.
     *
     * @return true on success; false if the partition ID is unknown.
     */
    bool removePartition(size_t partition_id);

    /**
     * @brief Access a partition, loading it into VRAM if necessary.
     *
     * If the VRAM budget would be exceeded the LRU partition is evicted
     * before loading.  The partition's LRU access timestamp is updated and
     * the configured prefetch strategy is applied.
     *
     * @return true when the partition is (now) VRAM-resident (or simulated
     *         as VRAM-resident on a CPU-only build).
     */
    bool accessPartition(size_t partition_id);

    /**
     * @brief Explicitly evict a partition from VRAM back to host RAM.
     *
     * @return true on success; false if the partition is unknown or already cold.
     */
    bool evictPartition(size_t partition_id);

    // -----------------------------------------------------------------------
    // Data access (read-only)
    // -----------------------------------------------------------------------

    /**
     * @brief Return a const pointer to the flat float data of a partition.
     *
     * The returned pointer refers to the host-side copy and is valid for the
     * lifetime of the partition.  Callers that intend to run a GPU kernel on
     * the data should first call accessPartition() to ensure VRAM residency.
     *
     * @return nullptr when the partition ID is unknown.
     */
    const std::vector<float>* getPartitionData(size_t partition_id) const;

    /**
     * @brief Return the number of vectors in a partition (0 if unknown).
     */
    size_t getPartitionVectorCount(size_t partition_id) const;

    /**
     * @brief Return true when the partition is currently VRAM-resident.
     */
    bool isPartitionInVRAM(size_t partition_id) const;

    /**
     * @brief Return IDs of all partitions currently resident in VRAM.
     */
    std::vector<size_t> getHotPartitions() const;

    /**
     * @brief Return IDs of all partitions currently in host RAM only.
     */
    std::vector<size_t> getColdPartitions() const;

    /**
     * @brief Return all partition IDs in insertion order.
     */
    std::vector<size_t> getAllPartitionIds() const;

    // -----------------------------------------------------------------------
    // Prefetch control
    // -----------------------------------------------------------------------

    /**
     * @brief Explicitly request a prefetch of the given partition.
     *
     * If the VRAM budget allows the partition is loaded without evicting
     * any existing hot partition and without updating the LRU access time.
     * This is a best-effort hint — if the budget is full the request is
     * recorded in statistics but the partition stays cold.
     */
    void prefetchPartition(size_t partition_id);

    /**
     * @brief Change the prefetch strategy at runtime.
     */
    void setPrefetchStrategy(PrefetchStrategy strategy);

    /**
     * @brief Return the current prefetch strategy.
     */
    PrefetchStrategy getPrefetchStrategy() const;

    // -----------------------------------------------------------------------
    // Budget control
    // -----------------------------------------------------------------------

    /**
     * @brief Update the VRAM budget.  If the new budget is smaller than the
     *        current VRAM usage, excess hot partitions are evicted (LRU first).
     */
    void setVRAMBudgetMB(size_t mb);

    /**
     * @brief Return the effective VRAM budget in bytes (0 = unlimited).
     */
    size_t getVRAMBudgetBytes() const;

    /**
     * @brief Return the number of VRAM bytes currently in use.
     */
    size_t getVRAMUsedBytes() const;

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    /**
     * @brief Return aggregate statistics.
     */
    Stats getStats() const;

    /**
     * @brief Return per-partition information.
     *
     * Returns a zeroed PartitionInfo with partition_id == SIZE_MAX when the
     * ID is unknown.
     */
    PartitionInfo getPartitionInfo(size_t partition_id) const;

    /**
     * @brief Return the total number of managed partitions.
     */
    size_t partitionCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace index
} // namespace themis
