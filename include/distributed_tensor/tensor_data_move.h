/**
 * @file tensor_data_move.h
 * @brief Tensor data structures with explicit move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <cstring>

namespace themis {
namespace tensor {

/// Data type enumeration
enum class DataType {
    UNKNOWN = 0,
    FLOAT32 = 1,
    FLOAT64 = 2,
    INT32 = 3,
    INT64 = 4,
    UINT32 = 5,
    UINT64 = 6,
};

/// Tensor shape descriptor
struct TensorShape {
    std::vector<int64_t> dims;
    
    TensorShape() = default;
    
    TensorShape(const std::vector<int64_t>& d) : dims(d) {}
    
    // Explicit move semantics
    TensorShape(TensorShape&& other) noexcept 
        : dims(std::move(other.dims)) {}
    
    TensorShape& operator=(TensorShape&& other) noexcept {
        if (this != &other) {
            dims = std::move(other.dims);
        }
        return *this;
    }
    
    TensorShape(const TensorShape&) = default;
    TensorShape& operator=(const TensorShape&) = default;
    
    /// Calculate total element count
    int64_t getElementCount() const {
        int64_t count = 1;
        for (auto d : dims) {
            count *= d;
        }
        return count;
    }
};

/**
 * @brief Tensor data container with explicit move semantics
 * 
 * Holds large data buffer with shape and type information.
 * Designed for efficient memory transfer via move semantics.
 * 
 * Thread-safety:
 * - NOT thread-safe for concurrent moves
 * - Only move during initialization/teardown
 * 
 * Move Semantics:
 * - Explicit move constructor transfers data buffer
 * - Explicit move assignment transfers ownership
 * - Copy semantics are deleted (data is too large)
 * - All operations marked noexcept
 * 
 * Invariants:
 * - After move: source is in valid empty state
 * - data_ vector is empty for moved-from objects
 * - dtype_ is UNKNOWN for empty entries
 * 
 * @code
 * TensorData tensor1 = loadLargeTensor(path);
 * TensorData tensor2 = std::move(tensor1);  // ✅ Efficient transfer
 * 
 * // tensor1 is now empty but valid
 * EXPECT_TRUE(tensor1.data_.empty());
 * EXPECT_EQ(tensor1.dtype_, DataType::UNKNOWN);
 * @endcode
 */
class TensorData {
private:
    std::vector<float> data_;
    TensorShape shape_;
    DataType dtype_ = DataType::UNKNOWN;

public:
    /// Default constructor - creates empty tensor
    TensorData() = default;

    /// Constructor with shape and dtype
    TensorData(const TensorShape& shape, DataType dtype)
        : shape_(shape), dtype_(dtype) {
        data_.resize(shape.getElementCount());
    }

    /**
     * @brief Move constructor - transfers data buffer
     * 
     * @param[in,out] other Source tensor (will be empty after move)
     * 
     * @post this->data_ contains all elements from other
     * @post this->shape_ = old other.shape_
     * @post this->dtype_ = old other.dtype_
     * @post other.data_.empty()
     * @post other.dtype_ = DataType::UNKNOWN
     * @post other.shape_.dims.empty()
     * 
     * Exception safety: noexcept
     */
    TensorData(TensorData&& other) noexcept
        : data_(std::move(other.data_)),
          shape_(std::move(other.shape_)),
          dtype_(other.dtype_) {
        other.dtype_ = DataType::UNKNOWN;
    }

    /**
     * @brief Move assignment operator - transfers data buffer
     * 
     * @param[in,out] other Source tensor (will be empty after move)
     * @return Reference to this
     * 
     * @post this->data_ contains all elements from other
     * @post other.data_.empty()
     * 
     * Exception safety: noexcept
     */
    TensorData& operator=(TensorData&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            shape_ = std::move(other.shape_);
            dtype_ = other.dtype_;
            other.dtype_ = DataType::UNKNOWN;
        }
        return *this;
    }

    /// Delete copy constructor
    TensorData(const TensorData&) = delete;
    
    /// Delete copy assignment operator
    TensorData& operator=(const TensorData&) = delete;

    /// Destructor
    ~TensorData() = default;

    // Accessors

    /// @brief Get data buffer
    std::vector<float>& data() { return data_; }
    const std::vector<float>& data() const { return data_; }

    /// @brief Get tensor shape
    const TensorShape& shape() const { return shape_; }

    /// @brief Get data type
    DataType dtype() const { return dtype_; }

    /// @brief Get total element count
    int64_t size() const { return shape_.getElementCount(); }

    /// @brief Check if tensor is empty
    bool empty() const { return data_.empty(); }

    /// @brief Get memory size in bytes
    size_t memorySize() const {
        return data_.size() * sizeof(float);
    }
};

/**
 * @brief Tensor metadata with explicit move semantics
 */
class TensorMetadata {
private:
    std::string tensor_id_;
    TensorShape shape_;
    DataType dtype_ = DataType::UNKNOWN;
    std::string device_location_;  // CPU or GPU device ID
    size_t memory_offset_ = 0;

public:
    TensorMetadata() = default;

    /**
     * @brief Move constructor - transfers metadata
     */
    TensorMetadata(TensorMetadata&& other) noexcept
        : tensor_id_(std::move(other.tensor_id_)),
          shape_(std::move(other.shape_)),
          dtype_(other.dtype_),
          device_location_(std::move(other.device_location_)),
          memory_offset_(other.memory_offset_) {
        other.dtype_ = DataType::UNKNOWN;
        other.memory_offset_ = 0;
    }

    /**
     * @brief Move assignment operator
     */
    TensorMetadata& operator=(TensorMetadata&& other) noexcept {
        if (this != &other) {
            tensor_id_ = std::move(other.tensor_id_);
            shape_ = std::move(other.shape_);
            dtype_ = other.dtype_;
            device_location_ = std::move(other.device_location_);
            memory_offset_ = other.memory_offset_;
            other.dtype_ = DataType::UNKNOWN;
            other.memory_offset_ = 0;
        }
        return *this;
    }

    TensorMetadata(const TensorMetadata&) = delete;
    TensorMetadata& operator=(const TensorMetadata&) = delete;

    // Accessors
    const std::string& getTensorId() const { return tensor_id_; }
    const TensorShape& getShape() const { return shape_; }
    DataType getDtype() const { return dtype_; }
    const std::string& getDeviceLocation() const { return device_location_; }
    size_t getMemoryOffset() const { return memory_offset_; }
};

/**
 * @brief Shard metadata with explicit move semantics
 */
class ShardMetadata {
private:
    std::string shard_id_;
    std::vector<int64_t> shard_ranges_;
    std::string shard_location_;

public:
    ShardMetadata() = default;

    /**
     * @brief Move constructor - transfers shard data
     */
    ShardMetadata(ShardMetadata&& other) noexcept
        : shard_id_(std::move(other.shard_id_)),
          shard_ranges_(std::move(other.shard_ranges_)),
          shard_location_(std::move(other.shard_location_)) {}

    /**
     * @brief Move assignment operator
     */
    ShardMetadata& operator=(ShardMetadata&& other) noexcept {
        if (this != &other) {
            shard_id_ = std::move(other.shard_id_);
            shard_ranges_ = std::move(other.shard_ranges_);
            shard_location_ = std::move(other.shard_location_);
        }
        return *this;
    }

    ShardMetadata(const ShardMetadata&) = delete;
    ShardMetadata& operator=(const ShardMetadata&) = delete;

    const std::string& getShardId() const { return shard_id_; }
    const std::vector<int64_t>& getShardRanges() const { return shard_ranges_; }
    const std::string& getShardLocation() const { return shard_location_; }
};

/**
 * @brief Tensor registry with explicit move semantics
 */
class TensorRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<TensorData>> tensors_;
    std::unordered_map<std::string, TensorMetadata> metadata_;
    std::unordered_map<std::string, std::vector<ShardMetadata>> shards_;

public:
    TensorRegistry() = default;

    /**
     * @brief Move constructor - transfers all registrations
     */
    TensorRegistry(TensorRegistry&& other) noexcept
        : tensors_(std::move(other.tensors_)),
          metadata_(std::move(other.metadata_)),
          shards_(std::move(other.shards_)) {}

    /**
     * @brief Move assignment operator
     */
    TensorRegistry& operator=(TensorRegistry&& other) noexcept {
        if (this != &other) {
            tensors_ = std::move(other.tensors_);
            metadata_ = std::move(other.metadata_);
            shards_ = std::move(other.shards_);
        }
        return *this;
    }

    TensorRegistry(const TensorRegistry&) = delete;
    TensorRegistry& operator=(const TensorRegistry&) = delete;

    /// Register a tensor
    void registerTensor(const std::string& id, std::shared_ptr<TensorData> data) {
        tensors_[id] = data;
    }

    /// Get registered tensor
    std::shared_ptr<TensorData> getTensor(const std::string& id) {
        auto it = tensors_.find(id);
        return it != tensors_.end() ? it->second : nullptr;
    }

    /// Clear all registrations
    void clear() noexcept {
        tensors_.clear();
        metadata_.clear();
        shards_.clear();
    }

    /// Get registry size
    size_t size() const noexcept { return tensors_.size(); }
};

}  // namespace tensor
}  // namespace themis
