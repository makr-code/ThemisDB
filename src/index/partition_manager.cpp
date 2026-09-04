/**
 * @file partition_manager.cpp
 * @brief Partition manager implementation with iterator invalidation safety
 * @version 0.0.1
 * @note Phase 2 A-2: Iterator Invalidation Safety (Gaps A-2-01 to A-2-04)
 */

#include "index/partition_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>

namespace themis {

/**
 * @brief Partition data container
 */
class PartitionData {
public:
    PartitionData(uint32_t id, const std::string& name)
        : id_(id), name_(name) {}
    
    uint32_t id() const { return id_; }
    const std::string& name() const { return name_; }
    
private:
    uint32_t id_;
    std::string name_;
};

// ============================================================================
// PartitionHandle Implementation
// ============================================================================

bool PartitionHandle::isValid() const {
    if (!manager_) return false;
    return manager_->CurrentEpoch(partition_id_) == epoch_;
}

// ============================================================================
// PartitionManager Implementation
// ============================================================================

PartitionManager::PartitionManager() : next_id_(1) {
    THEMIS_INFO("PartitionManager initialized");
}

PartitionManager::~PartitionManager() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    partitions_.clear();
    epoch_counters_.clear();
    THEMIS_INFO("PartitionManager destroyed");
}

PartitionHandle PartitionManager::AddPartition(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t id = next_id_++;
    uint64_t epoch = 0;
    
    auto data = std::make_shared<PartitionData>(id, name);
    PartitionMetadata metadata{id, name, data, epoch};
    
    partitions_[id] = metadata;
    epoch_counters_[id] = epoch;
    
    THEMIS_DEBUG("Added partition: id={}, name={}, epoch={}", id, name, epoch);
    
    return PartitionHandle(id, epoch, this);
}

bool PartitionManager::RemovePartition([[maybe_unused]] uint32_t partition_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = partitions_.find(partition_id);
    if (it == partitions_.end()) {
        THEMIS_WARN("Partition not found for removal: id={}", partition_id);
        return false;
    }
    
    // Gap A-2-02: Increment epoch to invalidate all existing handles
    epoch_counters_[partition_id]++;
    uint64_t new_epoch = epoch_counters_[partition_id];
    
    // Remove partition
    partitions_.erase(it);
    
    THEMIS_INFO("Removed partition: id={}, new_epoch={}", partition_id, new_epoch);
    return true;
}

void PartitionManager::RebuildPartitions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Gap A-2-03: Save partition IDs and rebuild with epoch invalidation
    std::vector<std::pair<uint32_t, std::string>> partition_list;
    
    for (const auto& [id, metadata] : partitions_) {
        partition_list.push_back({id, metadata.name});
    }
    
    // Clear all partitions
    partitions_.clear();
    
    // Rebuild partitions with new data structures
    // This invalidates all previous handles via epoch increment
    for (const auto& [id, name] : partition_list) {
        // Increment epoch for this partition
        epoch_counters_[id]++;
        uint64_t new_epoch = epoch_counters_[id];
        
        // Create new partition with new epoch
        auto data = std::make_shared<PartitionData>(id, name);
        PartitionMetadata metadata{id, name, data, new_epoch};
        partitions_[id] = metadata;
        
        THEMIS_DEBUG("Rebuilt partition: id={}, name={}, epoch={}", id, name, new_epoch);
    }
    
    THEMIS_INFO("Rebuilt {} partitions with epoch invalidation", partition_list.size());
}

void PartitionManager::CompactPartitions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Gap A-2-04: Bounds checking and exception safety during compaction
    if (partitions_.empty()) {
        THEMIS_DEBUG("CompactPartitions: no partitions to compact");
        return;
    }
    
    try {
        // Collect partition IDs to compact
        std::vector<uint32_t> partition_ids;
        for (const auto& [id, metadata] : partitions_) {
            // Bounds check: verify ID is within valid range
            if (id > 0 && id < next_id_) {
                partition_ids.push_back(id);
            }
        }
        
        // Create compacted map with bounds checking
        std::unordered_map<uint32_t, PartitionMetadata> compacted;
        
        for (uint32_t id : partition_ids) {
            auto it = partitions_.find(id);
            if (it != partitions_.end()) {
                // Increment epoch for each partition during compaction
                epoch_counters_[id]++;
                uint64_t new_epoch = epoch_counters_[id];
                
                // Copy partition with new epoch
                PartitionMetadata updated = it->second;
                updated.epoch = new_epoch;
                compacted[id] = updated;
                
                THEMIS_DEBUG("Compacted partition: id={}, epoch={}", id, new_epoch);
            }
        }
        
        // Replace partitions with compacted version
        partitions_ = std::move(compacted);
        
        THEMIS_INFO("Compaction complete: {} partitions", partitions_.size());
    }
    catch (const std::exception& e) {
        THEMIS_ERROR("CompactPartitions failed: {}", e.what());
        throw;
    }
}

uint64_t PartitionManager::CurrentEpoch([[maybe_unused]] uint32_t partition_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = epoch_counters_.find(partition_id);
    if (it == epoch_counters_.end()) {
        return 0;  // Not found
    }
    
    return it->second;
}

std::shared_ptr<PartitionData> PartitionManager::GetPartitionById([[maybe_unused]] uint32_t partition_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = partitions_.find(partition_id);
    if (it == partitions_.end()) {
        return nullptr;
    }
    
    return it->second.data;
}

std::shared_ptr<PartitionData> PartitionManager::GetPartitionByHandle(const PartitionHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Gap A-2-01: Validate handle epoch before returning partition
    auto epoch_it = epoch_counters_.find(handle.partition_id_);
    if (epoch_it == epoch_counters_.end() || epoch_it->second != handle.epoch_) {
        // Handle is invalid (partition removed or rebuilt)
        return nullptr;
    }
    
    auto partition_it = partitions_.find(handle.partition_id_);
    if (partition_it == partitions_.end()) {
        return nullptr;
    }
    
    return partition_it->second.data;
}

std::vector<uint32_t> PartitionManager::GetPartitionIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<uint32_t> ids;
    for (const auto& [id, metadata] : partitions_) {
        ids.push_back(id);
    }
    
    return ids;
}

size_t PartitionManager::GetPartitionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return partitions_.size();
}

} // namespace themis
