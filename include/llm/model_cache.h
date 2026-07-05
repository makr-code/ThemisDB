/**
 * @file model_cache.h
 * @brief Model cache entry with explicit move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <utility>

namespace themis {
namespace llm {

/// Forward declaration
class Model;

/**
 * @brief Cache entry for loaded models with explicit move semantics
 * 
 * Thread-safety:
 * - NOT thread-safe for concurrent access to move operations
 * - Only safe to move during initialization/teardown
 * 
 * Move Semantics:
 * - Explicit move constructor transfers ownership of model_ and resets source
 * - Explicit move assignment closes old resources before moving
 * - Copy semantics are deleted
 * - All operations marked noexcept
 * 
 * @code
 * ModelCacheEntry entry1 = createEntry();
 * ModelCacheEntry entry2 = std::move(entry1);  // ✅ Move construction
 * 
 * ModelCacheEntry entry3;
 * entry3 = std::move(entry2);  // ✅ Move assignment
 * 
 * // entry1 is now in valid but empty state
 * EXPECT_EQ(entry1.access_count(), 0);
 * @endcode
 * 
 * @invariant After move construction/assignment:
 * - Destination contains all members from source
 * - Source is in valid empty state (access_count = 0)
 * - model_ is nullptr for empty entries
 */
class ModelCacheEntry {
private:
    std::shared_ptr<Model> model_;
    std::string model_id_;
    std::chrono::system_clock::time_point load_time_;
    size_t access_count_ = 0;

public:
    /// Default constructor - creates empty entry
    ModelCacheEntry() = default;

    /**
     * @brief Move constructor - transfers ownership from other
     * 
     * @param[in,out] other Source entry to move from (will be in valid empty state after)
     * 
     * @post this->model_ == old other.model_
     * @post this->access_count_ == old other.access_count_
     * @post other.access_count_ == 0
     * @post other.model_ == nullptr
     * 
     * Exception safety: noexcept
     */
    ModelCacheEntry(ModelCacheEntry&& other) noexcept
        : model_(std::move(other.model_)),
          model_id_(std::move(other.model_id_)),
          load_time_(std::move(other.load_time_)),
          access_count_(other.access_count_) {
        other.access_count_ = 0;
    }

    /**
     * @brief Move assignment operator - transfers ownership from other
     * 
     * Self-assignment safe and is a no-op
     * 
     * @param[in,out] other Source entry to move from (will be in valid empty state after)
     * @return Reference to this
     * 
     * @post this->model_ == old other.model_
     * @post this->access_count_ == old other.access_count_
     * @post other.access_count_ == 0
     * @post other.model_ == nullptr
     * 
     * Exception safety: noexcept
     */
    ModelCacheEntry& operator=(ModelCacheEntry&& other) noexcept {
        if (this != &other) {
            model_ = std::move(other.model_);
            model_id_ = std::move(other.model_id_);
            load_time_ = std::move(other.load_time_);
            access_count_ = other.access_count_;
            other.access_count_ = 0;
        }
        return *this;
    }

    /// Delete copy constructor
    ModelCacheEntry(const ModelCacheEntry&) = delete;
    
    /// Delete copy assignment operator
    ModelCacheEntry& operator=(const ModelCacheEntry&) = delete;

    /// Default destructor - model_ shared_ptr handles cleanup
    ~ModelCacheEntry() = default;

    // Accessors

    /// @brief Get the cached model
    /// @return Shared pointer to the cached model (may be nullptr)
    std::shared_ptr<Model> getModel() const { return model_; }

    /// @brief Get the model identifier
    /// @return Model ID string
    const std::string& getModelId() const { return model_id_; }

    /// @brief Get load time
    /// @return Timestamp when model was loaded
    std::chrono::system_clock::time_point getLoadTime() const { return load_time_; }

    /// @brief Get access count
    /// @return Number of times this entry has been accessed
    size_t access_count() const { return access_count_; }

    /// @brief Increment access count
    void incrementAccessCount() { ++access_count_; }

    /// @brief Check if entry is valid (has a model)
    /// @return true if model_ is not nullptr
    bool isValid() const { return static_cast<bool>(model_); }
};

}  // namespace llm
}  // namespace themis
