/**
 * @file gpu_memory_oversubscription.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class PrefetchStrategy {
    NONE,       ///< On-demand loading only; no background prefetch.
    LRU,        ///< Prefetch cold partition closest to becoming hot (LRU heuristic).
    MRU,        ///< Prefetch cold partition that was most recently accessed.
    SEQUENTIAL  ///< Prefetch next partition by insertion-order ID.
};

class GPUMemoryOversubscriptionManager {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        bool enable_oversubscription = false;

        size_t vram_budget_mb = 0;

        size_t host_ram_budget_mb = 0;

        size_t partition_vectors = 65536;

        PrefetchStrategy prefetch_strategy = PrefetchStrategy::LRU;

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
    /**
     * @brief GPUMemory Oversubscription Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GPUMemoryOversubscriptionManager(const Config& config);
    ~GPUMemoryOversubscriptionManager();

    GPUMemoryOversubscriptionManager(const GPUMemoryOversubscriptionManager&) = delete;
    GPUMemoryOversubscriptionManager& operator=(const GPUMemoryOversubscriptionManager&) = delete;

    // -----------------------------------------------------------------------
    // Partition management
    // -----------------------------------------------------------------------

    size_t addPartition(const std::vector<float>& flat_data,
                        size_t num_vectors,
                        size_t dimension,
                        const std::string& tag = "");

    /**
     * @brief Remove Partition.
     * @param[in] partition_id Identifier of the partition.
     * @return True when the operation succeeds.
     */
    bool removePartition(size_t partition_id);

    /**
     * @brief Access Partition.
     * @param[in] partition_id Identifier of the partition.
     * @return True when the operation succeeds.
     */
    bool accessPartition(size_t partition_id);

    /**
     * @brief Evict Partition.
     * @param[in] partition_id Identifier of the partition.
     * @return True when the operation succeeds.
     */
    bool evictPartition(size_t partition_id);

    // -----------------------------------------------------------------------
    // Data access (read-only)
    // -----------------------------------------------------------------------

    /**
     * @brief Get Partition Data.
     * @param[in] partition_id Identifier of the partition.
     * @return Pointer to the result.
     */
    const std::vector<float>* getPartitionData(size_t partition_id) const;

    /**
     * @brief Get Partition Vector Count.
     * @param[in] partition_id Identifier of the partition.
     * @return Return value.
     */
    size_t getPartitionVectorCount(size_t partition_id) const;

    /**
     * @brief Is Partition In VRAM.
     * @param[in] partition_id Identifier of the partition.
     * @return True when the operation succeeds.
     */
    bool isPartitionInVRAM(size_t partition_id) const;

    /**
     * @brief Get Hot Partitions.
     * @return Return value.
     */
    std::vector<size_t> getHotPartitions() const;

    /**
     * @brief Get Cold Partitions.
     * @return Return value.
     */
    std::vector<size_t> getColdPartitions() const;

    /**
     * @brief Get All Partition Ids.
     * @return Return value.
     */
    std::vector<size_t> getAllPartitionIds() const;

    // -----------------------------------------------------------------------
    // Prefetch control
    // -----------------------------------------------------------------------

    /**
     * @brief Prefetch Partition.
     * @param[in] partition_id Identifier of the partition.
     */
    void prefetchPartition(size_t partition_id);

    /**
     * @brief Set Prefetch Strategy.
     * @param[in] strategy Input parameter.
     */
    void setPrefetchStrategy(PrefetchStrategy strategy);

    /**
     * @brief Get Prefetch Strategy.
     * @return Return value.
     */
    PrefetchStrategy getPrefetchStrategy() const;

    // -----------------------------------------------------------------------
    // Budget control
    // -----------------------------------------------------------------------

    /**
     * @brief Set VRAMBudget MB.
     * @param[in] mb Input parameter.
     */
    void setVRAMBudgetMB(size_t mb);

    /**
     * @brief Get VRAMBudget Bytes.
     * @return Return value.
     */
    size_t getVRAMBudgetBytes() const;

    /**
     * @brief Get VRAMUsed Bytes.
     * @return Return value.
     */
    size_t getVRAMUsedBytes() const;

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    /**
     * @brief Get Partition Info.
     * @param[in] partition_id Identifier of the partition.
     * @return Return value.
     */
    PartitionInfo getPartitionInfo(size_t partition_id) const;

    /**
     * @brief Partition Count.
     * @return Return value.
     */
    size_t partitionCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace index
} // namespace themis
