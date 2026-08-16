/**
 * @file vector_index_manager_safety.h
 * @brief Vector Index Manager iterator safety wrapper
 * @version 0.0.1
 * @note Phase 2 A-2: Iterator Invalidation Safety (Gaps A-2-05 to A-2-08)
 * 
 * Provides safe iterator patterns for vector index operations by:
 * - Using stable handle-based access instead of raw iterators
 * - Implementing fallback to stable ID-based lookup
 * - Adding exception safety guarantees
 * - Protecting against concurrent modifications
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace themis {

/// Forward declaration
class VectorIndexData;

/**
 * @brief Safe handle for vector index access
 * 
 * Replaces raw iterators with stable, invalidation-aware handles.
 * 
 * @note Gap A-2-05: Handle-based access for vector indices
 */
class VectorIndexHandle {
public:
    /**
     * @brief Check if this handle is still valid
     * @return true if the index still exists and hasn't been removed
     */
    bool isValid() const;
    
    /**
     * @brief Get the index ID
     * @return Index identifier
     */
    uint32_t id() const { return index_id_; }
    
    /**
     * @brief Get the generation when this handle was created
     * @return Generation value for invalidation detection
     */
    uint64_t generation() const { return generation_; }

private:
    friend class VectorIndexManagerSafety;
    
    uint32_t index_id_;
    uint64_t generation_;
    class VectorIndexManagerSafety* manager_;
    
    VectorIndexHandle(uint32_t id, uint64_t gen, VectorIndexManagerSafety* mgr)
        : index_id_(id), generation_(gen), manager_(mgr) {}
};

/**
 * @brief Safe wrapper for VectorIndexManager with iterator safety
 * 
 * Features:
 * - Replace raw iterators with stable handles
 * - Automatic invalidation detection on index removal
 * - Fallback to stable ID-based lookup patterns
 * - Exception safety during iterator cleanup
 * 
 * @note Gaps A-2-05 to A-2-08: Complete iterator safety for vector indices
 */
class VectorIndexManagerSafety {
public:
    VectorIndexManagerSafety();
    ~VectorIndexManagerSafety() noexcept;
    
    /**
     * @brief Create a vector index
     * @param name Index name
     * @param dimension Vector dimension
     * @return Handle for safe index access
     * @note Gap A-2-05: Returns handle instead of raw iterator
     */
    VectorIndexHandle CreateIndex(const std::string& name, uint32_t dimension);
    
    /**
     * @brief Remove a vector index
     * 
     * Invalidates all handles to this index by incrementing its generation.
     * 
     * @param index_id ID of index to remove
     * @return true if index was removed, false if not found
     * @note Gap A-2-06: Generation increment invalidates all existing handles
     */
    bool RemoveVectorIndex(uint32_t index_id);
    
    /**
     * @brief Update a vector index
     * 
     * May trigger internal reorganization. Uses snapshot pattern
     * to prevent iterator invalidation.
     * 
     * @param index_id Index to update
     * @return true if update succeeded
     * @note Gap A-2-07: Snapshot-based rebuild for iterator safety
     */
    bool UpdateVectorIndex(uint32_t index_id);
    
    /**
     * @brief Get index by handle (safe access)
     * @param handle Previously obtained index handle
     * @return Shared pointer to index data if valid, nullptr if invalidated
     * @note Gap A-2-08: Validates handle and detects invalidation
     */
    std::shared_ptr<VectorIndexData> GetIndexByHandle(const VectorIndexHandle& handle);
    
    /**
     * @brief Get index by ID (fallback access pattern)
     * @param index_id Index identifier
     * @return Shared pointer to index data, or nullptr if not found
     * @note Stable ID-based lookup for iterator safety
     */
    std::shared_ptr<VectorIndexData> GetIndexById(uint32_t index_id);
    
    /**
     * @brief Get all index IDs
     * @return Vector of current index IDs
     */
    std::vector<uint32_t> GetIndexIds() const;
    
    /**
     * @brief Get index count
     * @return Number of active indices
     */
    size_t GetIndexCount() const;
    
    /**
     * @brief Get current generation for an index
     * @param index_id Index identifier
     * @return Current generation value (0 if not found)
     */
    uint64_t CurrentGeneration(uint32_t index_id) const;

private:
    struct IndexMetadata {
        uint32_t id;
        std::string name;
        uint32_t dimension;
        std::shared_ptr<VectorIndexData> data;
        uint64_t generation;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<uint32_t, IndexMetadata> indices_;
    uint32_t next_id_;
    std::unordered_map<uint32_t, uint64_t> generation_map_;
};

/**
 * @brief RAII guard for vector index lifetime management
 * 
 * Ensures index is not accessed after removal.
 * 
 * @note Gap A-2-07: Exception-safe lifetime tracking with snapshot
 */
class VectorIndexGuard {
public:
    VectorIndexGuard(VectorIndexManagerSafety& mgr, const VectorIndexHandle& handle)
        : manager_(mgr), handle_(handle), index_(nullptr) {
        index_ = manager_.GetIndexByHandle(handle_);
    }
    
    ~VectorIndexGuard() noexcept = default;
    
    /**
     * @brief Check if index is still valid
     * @return true if index can be safely accessed
     */
    bool IsValid() const { return index_ != nullptr && handle_.isValid(); }
    
    /**
     * @brief Get index data
     * @return Index data pointer, or nullptr if invalid
     */
    std::shared_ptr<VectorIndexData> Get() const { return index_; }

private:
    VectorIndexManagerSafety& manager_;
    VectorIndexHandle handle_;
    std::shared_ptr<VectorIndexData> index_;
};

} // namespace themis
