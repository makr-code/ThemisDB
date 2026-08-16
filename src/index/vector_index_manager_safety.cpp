/**
 * @file vector_index_manager_safety.cpp
 * @brief Vector Index Manager iterator safety implementation
 * @version 0.0.1
 * @note Phase 2 A-2: Iterator Invalidation Safety (Gaps A-2-05 to A-2-08)
 */

#include "index/vector_index_manager_safety.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>

namespace themis {

/**
 * @brief Vector index data container
 */
class VectorIndexData {
public:
    VectorIndexData(uint32_t id, const std::string& name, uint32_t dimension)
        : id_(id), name_(name), dimension_(dimension) {}
    
    uint32_t id() const { return id_; }
    const std::string& name() const { return name_; }
    uint32_t dimension() const { return dimension_; }

private:
    uint32_t id_;
    std::string name_;
    uint32_t dimension_;
};

// ============================================================================
// VectorIndexHandle Implementation
// ============================================================================

bool VectorIndexHandle::isValid() const {
    if (!manager_) return false;
    return manager_->CurrentGeneration(index_id_) == generation_;
}

// ============================================================================
// VectorIndexManagerSafety Implementation
// ============================================================================

VectorIndexManagerSafety::VectorIndexManagerSafety() : next_id_(1) {
    THEMIS_INFO("VectorIndexManagerSafety initialized");
}

VectorIndexManagerSafety::~VectorIndexManagerSafety() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    indices_.clear();
    generation_map_.clear();
    THEMIS_INFO("VectorIndexManagerSafety destroyed");
}

VectorIndexHandle VectorIndexManagerSafety::CreateIndex(
    const std::string& name, uint32_t dimension) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t id = next_id_++;
    uint64_t generation = 0;
    
    auto data = std::make_shared<VectorIndexData>(id, name, dimension);
    IndexMetadata metadata{id, name, dimension, data, generation};
    
    indices_[id] = metadata;
    generation_map_[id] = generation;
    
    THEMIS_DEBUG("Created vector index: id={}, name={}, dimension={}, generation={}",
                 id, name, dimension, generation);
    
    return VectorIndexHandle(id, generation, this);
}

bool VectorIndexManagerSafety::RemoveVectorIndex(uint32_t index_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = indices_.find(index_id);
    if (it == indices_.end()) {
        THEMIS_WARN("Vector index not found for removal: id={}", index_id);
        return false;
    }
    
    // Gap A-2-06: Increment generation to invalidate all existing handles
    generation_map_[index_id]++;
    uint64_t new_generation = generation_map_[index_id];
    
    // Remove index
    indices_.erase(it);
    
    THEMIS_INFO("Removed vector index: id={}, new_generation={}", index_id, new_generation);
    return true;
}

bool VectorIndexManagerSafety::UpdateVectorIndex(uint32_t index_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = indices_.find(index_id);
    if (it == indices_.end()) {
        THEMIS_WARN("Vector index not found for update: id={}", index_id);
        return false;
    }
    
    try {
        // Gap A-2-07: Snapshot-based rebuild for iterator safety
        // 1. Take snapshot of current index state
        IndexMetadata original = it->second;
        
        // 2. Create new index with same metadata but new data
        auto new_data = std::make_shared<VectorIndexData>(
            original.id, original.name, original.dimension);
        
        // 3. Increment generation to invalidate old handles
        generation_map_[index_id]++;
        uint64_t new_generation = generation_map_[index_id];
        
        // 4. Create updated metadata with new generation and data
        IndexMetadata updated{
            original.id,
            original.name,
            original.dimension,
            new_data,
            new_generation
        };
        
        // 5. Replace in map (atomic from caller perspective)
        indices_[index_id] = updated;
        
        THEMIS_DEBUG("Updated vector index: id={}, new_generation={}", index_id, new_generation);
        return true;
    }
    catch (const std::exception& e) {
        THEMIS_ERROR("UpdateVectorIndex failed: {}", e.what());
        return false;
    }
}

std::shared_ptr<VectorIndexData> VectorIndexManagerSafety::GetIndexByHandle(
    const VectorIndexHandle& handle) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Gap A-2-08: Validate handle generation before accessing index
    auto gen_it = generation_map_.find(handle.index_id_);
    if (gen_it == generation_map_.end() || gen_it->second != handle.generation_) {
        // Handle is invalid (index removed or updated)
        THEMIS_DEBUG("Handle validation failed: id={}, expected_gen={}, current_gen={}",
                     handle.index_id_, handle.generation_,
                     gen_it != generation_map_.end() ? gen_it->second : 0);
        return nullptr;
    }
    
    auto idx_it = indices_.find(handle.index_id_);
    if (idx_it == indices_.end()) {
        return nullptr;
    }
    
    return idx_it->second.data;
}

std::shared_ptr<VectorIndexData> VectorIndexManagerSafety::GetIndexById(uint32_t index_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = indices_.find(index_id);
    if (it == indices_.end()) {
        return nullptr;
    }
    
    return it->second.data;
}

std::vector<uint32_t> VectorIndexManagerSafety::GetIndexIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<uint32_t> ids;
    for (const auto& [id, metadata] : indices_) {
        ids.push_back(id);
    }
    
    return ids;
}

size_t VectorIndexManagerSafety::GetIndexCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return indices_.size();
}

uint64_t VectorIndexManagerSafety::CurrentGeneration(uint32_t index_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = generation_map_.find(index_id);
    if (it == generation_map_.end()) {
        return 0;  // Not found
    }
    
    return it->second;
}

} // namespace themis
