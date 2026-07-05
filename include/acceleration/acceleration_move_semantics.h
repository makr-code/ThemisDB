/**
 * @file acceleration_move_semantics.h
 * @brief GPU acceleration classes with explicit move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace themis {
namespace acceleration {

/// GPU device identifier type
using GpuDevice = int;

/// GPU stream identifier type
using GpuStream = void*;

/**
 * @brief GPU kernel handle with explicit move semantics
 * 
 * Manages GPU kernel compilation and execution resources.
 * Thread-safety: NOT thread-safe for concurrent move operations.
 * Only move during initialization/teardown.
 */
class GPUKernelHandle {
private:
    void* kernel_handle_ = nullptr;  // CUfunction equivalent
    std::string kernel_name_;
    GpuDevice device_ = 0;
    std::unique_ptr<char[]> ptx_code_;
    size_t ptx_size_ = 0;

public:
    GPUKernelHandle() = default;

    /**
     * @brief Move constructor - transfers kernel resources
     * 
     * @param[in,out] other Source handle (will be empty after move)
     * 
     * @post this->kernel_handle_ = old other.kernel_handle_
     * @post this->device_ = old other.device_
     * @post other.kernel_handle_ = nullptr
     * @post other.device_ = 0
     * 
     * Exception safety: noexcept
     */
    GPUKernelHandle(GPUKernelHandle&& other) noexcept
        : kernel_handle_(other.kernel_handle_),
          kernel_name_(std::move(other.kernel_name_)),
          device_(other.device_),
          ptx_code_(std::move(other.ptx_code_)),
          ptx_size_(other.ptx_size_) {
        other.kernel_handle_ = nullptr;
        other.device_ = 0;
        other.ptx_size_ = 0;
    }

    /**
     * @brief Move assignment operator - transfers kernel resources
     * 
     * @param[in,out] other Source handle (will be empty after move)
     * @return Reference to this
     * 
     * @post this->kernel_handle_ = old other.kernel_handle_
     * @post other.kernel_handle_ = nullptr
     * 
     * Exception safety: noexcept
     */
    GPUKernelHandle& operator=(GPUKernelHandle&& other) noexcept {
        if (this != &other) {
            if (kernel_handle_ != nullptr) {
                // TODO: Call cuFunctionUnload or equivalent
            }
            kernel_handle_ = other.kernel_handle_;
            kernel_name_ = std::move(other.kernel_name_);
            device_ = other.device_;
            ptx_code_ = std::move(other.ptx_code_);
            ptx_size_ = other.ptx_size_;
            other.kernel_handle_ = nullptr;
            other.device_ = 0;
            other.ptx_size_ = 0;
        }
        return *this;
    }

    GPUKernelHandle(const GPUKernelHandle&) = delete;
    GPUKernelHandle& operator=(const GPUKernelHandle&) = delete;

    ~GPUKernelHandle() = default;

    // Accessors
    const std::string& getKernelName() const { return kernel_name_; }
    GpuDevice getDevice() const noexcept { return device_; }
    bool isValid() const noexcept { return kernel_handle_ != nullptr; }
};

/**
 * @brief GPU batch processor with explicit move semantics
 */
class GPUBatchProcessor {
private:
    std::vector<GPUKernelHandle> kernels_;
    std::string processor_name_;
    size_t batch_size_ = 0;
    void* batch_buffer_ = nullptr;

public:
    GPUBatchProcessor() = default;

    /**
     * @brief Move constructor - transfers kernels and batch data
     */
    GPUBatchProcessor(GPUBatchProcessor&& other) noexcept
        : kernels_(std::move(other.kernels_)),
          processor_name_(std::move(other.processor_name_)),
          batch_size_(other.batch_size_),
          batch_buffer_(other.batch_buffer_) {
        other.batch_size_ = 0;
        other.batch_buffer_ = nullptr;
    }

    /**
     * @brief Move assignment operator
     */
    GPUBatchProcessor& operator=(GPUBatchProcessor&& other) noexcept {
        if (this != &other) {
            if (batch_buffer_ != nullptr) {
                // TODO: Call cuMemFree or equivalent
            }
            kernels_ = std::move(other.kernels_);
            processor_name_ = std::move(other.processor_name_);
            batch_size_ = other.batch_size_;
            batch_buffer_ = other.batch_buffer_;
            other.batch_size_ = 0;
            other.batch_buffer_ = nullptr;
        }
        return *this;
    }

    GPUBatchProcessor(const GPUBatchProcessor&) = delete;
    GPUBatchProcessor& operator=(const GPUBatchProcessor&) = delete;

    ~GPUBatchProcessor() = default;

    void addKernel(GPUKernelHandle kernel) {
        kernels_.push_back(std::move(kernel));
    }

    size_t getKernelCount() const noexcept { return kernels_.size(); }
    size_t getBatchSize() const noexcept { return batch_size_; }
};

/**
 * @brief GPU memory pool with explicit move semantics
 */
class GPUMemoryPool {
private:
    void* pool_handle_ = nullptr;
    size_t total_size_ = 0;
    size_t allocated_size_ = 0;
    GpuDevice device_ = 0;

public:
    GPUMemoryPool() = default;

    /**
     * @brief Move constructor - transfers memory pool
     */
    GPUMemoryPool(GPUMemoryPool&& other) noexcept
        : pool_handle_(other.pool_handle_),
          total_size_(other.total_size_),
          allocated_size_(other.allocated_size_),
          device_(other.device_) {
        other.pool_handle_ = nullptr;
        other.total_size_ = 0;
        other.allocated_size_ = 0;
        other.device_ = 0;
    }

    /**
     * @brief Move assignment operator
     */
    GPUMemoryPool& operator=(GPUMemoryPool&& other) noexcept {
        if (this != &other) {
            if (pool_handle_ != nullptr) {
                // TODO: Call pool cleanup
            }
            pool_handle_ = other.pool_handle_;
            total_size_ = other.total_size_;
            allocated_size_ = other.allocated_size_;
            device_ = other.device_;
            other.pool_handle_ = nullptr;
            other.total_size_ = 0;
            other.allocated_size_ = 0;
            other.device_ = 0;
        }
        return *this;
    }

    GPUMemoryPool(const GPUMemoryPool&) = delete;
    GPUMemoryPool& operator=(const GPUMemoryPool&) = delete;

    ~GPUMemoryPool() = default;

    void* allocate(size_t size);
    void deallocate(void* ptr) noexcept;
    
    size_t getTotalSize() const noexcept { return total_size_; }
    size_t getAllocatedSize() const noexcept { return allocated_size_; }
    size_t getAvailableSize() const noexcept { return total_size_ - allocated_size_; }
    bool isFull() const noexcept { return allocated_size_ >= total_size_; }
};

/**
 * @brief GPU stream wrapper with explicit move semantics
 */
class GPUStreamWrapper {
private:
    GpuStream stream_ = nullptr;
    GpuDevice device_ = 0;
    bool is_recording_ = false;

public:
    GPUStreamWrapper() = default;

    /**
     * @brief Move constructor - transfers stream handle
     */
    GPUStreamWrapper(GPUStreamWrapper&& other) noexcept
        : stream_(other.stream_),
          device_(other.device_),
          is_recording_(other.is_recording_) {
        other.stream_ = nullptr;
        other.device_ = 0;
        other.is_recording_ = false;
    }

    /**
     * @brief Move assignment operator
     */
    GPUStreamWrapper& operator=(GPUStreamWrapper&& other) noexcept {
        if (this != &other) {
            if (stream_ != nullptr) {
                // TODO: Call cuStreamDestroy or equivalent
            }
            stream_ = other.stream_;
            device_ = other.device_;
            is_recording_ = other.is_recording_;
            other.stream_ = nullptr;
            other.device_ = 0;
            other.is_recording_ = false;
        }
        return *this;
    }

    GPUStreamWrapper(const GPUStreamWrapper&) = delete;
    GPUStreamWrapper& operator=(const GPUStreamWrapper&) = delete;

    ~GPUStreamWrapper() = default;

    GpuStream getStream() const noexcept { return stream_; }
    GpuDevice getDevice() const noexcept { return device_; }
    bool isRecording() const noexcept { return is_recording_; }
    
    void synchronize() noexcept;
};

}  // namespace acceleration
}  // namespace themis
