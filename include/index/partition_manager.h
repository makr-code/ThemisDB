/**
 * @file partition_manager.h
 * @brief Partition manager with iterator invalidation safety
 * @version 0.0.1
 * @note Phase 2 A-2: Iterator Invalidation Safety
 * 
 * Implements handle-based partition access with epoch-based validity tracking
 * to prevent use-after-free when partitions are removed or rebuilt.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <optional>

namespace themis {

/// Forward declaration
class PartitionData;

/**
 * @brief Handle for safe partition access with lifetime tracking
 * 
 * Provides stable access to partition data even after structural modifications.
 * Uses epoch counters to detect invalidation.
 * 
 * @note Gap A-2-01: Handle-based validity tracking for iterator safety
 */
class PartitionHandle {
public:
    /**
     * @brief Check if this handle is still valid
     * @return true if partition still exists and hasn't been rebuilt
     */
    bool isValid() const;
    
    /**
     * @brief Get the partition ID
     * @return Partition identifier
     */
    uint32_t id() const { return partition_id_; }
    
    /**
     * @brief Get the epoch when this handle was created
     * @return Epoch value
     */
    uint64_t epoch() const { return epoch_; }

private:
    friend class PartitionManager;
    
    uint32_t partition_id_;
    uint64_t epoch_;
    PartitionManager* manager_;
    
    PartitionHandle(uint32_t id, uint64_t epoch, PartitionManager* mgr)
        : partition_id_(id), epoch_(epoch), manager_(mgr) {}
};

/**
 * @brief Manages partitions with iterator invalidation safety
 * 
 * Features:
 * - Epoch-based validity tracking for all handles
 * - Automatic iterator invalidation on partition removal
 * - Stable ID-based access fallback patterns
 * - RAII guards for partition lifetime management
 * 
 * @note Gaps A-2-01 to A-2-04: Complete iterator safety implementation
 */
class PartitionManager {
public:
    PartitionManager();
    ~PartitionManager() noexcept;
    
    /**
     * @brief Add a new partition
     * @param name Partition name
     * @return Handle for safe partition access
     * @note Gap A-2-01: Returns handle with epoch tracking
     */
    PartitionHandle AddPartition(const std::string& name);
    
    /**
     * @brief Remove a partition by ID
     * 
     * Invalidates all handles to this partition by incrementing its epoch.
     * 
     * @param partition_id ID of partition to remove
     * @return true if partition was removed, false if not found
     * @note Gap A-2-02: Epoch increment invalidates all existing handles
     */
    bool RemovePartition(uint32_t partition_id);
    
    /**
     * @brief Rebuild partitions after compaction or optimization
     * 
     * Creates new partition instances, invalidating previous handles.
     * Uses fallback to stable ID-based lookup for active clients.
     * 
     * @note Gap A-2-03: RAII guard + epoch invalidation during rebuild
     */
    void RebuildPartitions();
    
    /**
     * @brief Compact and reorganize partitions
     * 
     * Consolidates partitions, potentially changing internal structure.
     * All handles must be re-validated after this operation.
     * 
     * @note Gap A-2-04: Bounds checking + exception safety during compact
     */
    void CompactPartitions();
    
    /**
     * @brief Get current epoch for a partition
     * @param partition_id Partition identifier
     * @return Current epoch value (0 if not found)
     */
    uint64_t CurrentEpoch(uint32_t partition_id) const;
    
    /**
     * @brief Get partition data by ID (fallback access pattern)
     * @param partition_id Partition identifier
     * @return Pointer to partition data, or nullptr if not found
     * @note Stable ID-based lookup for iterator safety
     */
    std::shared_ptr<PartitionData> GetPartitionById(uint32_t partition_id);
    
    /**
     * @brief Get partition data by handle
     * @param handle Previously obtained partition handle
     * @return Pointer to partition data if valid, nullptr if invalidated
     * @note Gap A-2-01: Validates handle before access
     */
    std::shared_ptr<PartitionData> GetPartitionByHandle(const PartitionHandle& handle);
    
    /**
     * @brief Get all partition IDs (stable list)
     * @return Vector of current partition IDs
     */
    std::vector<uint32_t> GetPartitionIds() const;
    
    /**
     * @brief Get partition count
     * @return Number of active partitions
     */
    size_t GetPartitionCount() const;

private:
    struct PartitionMetadata {
        uint32_t id;
        std::string name;
        std::shared_ptr<PartitionData> data;
        uint64_t epoch;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<uint32_t, PartitionMetadata> partitions_;
    uint32_t next_id_;
    std::unordered_map<uint32_t, uint64_t> epoch_counters_;
};

/**
 * @brief RAII guard for partition lifetime management
 * 
 * Ensures partition is not accessed after removal.
 * 
 * @note Gap A-2-03: Exception-safe lifetime tracking
 */
class PartitionGuard {
public:
    PartitionGuard(PartitionManager& mgr, const PartitionHandle& handle)
        : manager_(mgr), handle_(handle), partition_(nullptr) {
        partition_ = manager_.GetPartitionByHandle(handle_);
    }
    
    ~PartitionGuard() noexcept = default;
    
    /**
     * @brief Check if partition is still valid
     * @return true if partition can be safely accessed
     */
    bool IsValid() const { return partition_ != nullptr && handle_.isValid(); }
    
    /**
     * @brief Get partition data
     * @return Partition data pointer, or nullptr if invalid
     */
    std::shared_ptr<PartitionData> Get() const { return partition_; }

private:
    PartitionManager& manager_;
    PartitionHandle handle_;
    std::shared_ptr<PartitionData> partition_;
};

} // namespace themis
