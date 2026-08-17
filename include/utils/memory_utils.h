/**
 * @file memory_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <stdexcept>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

namespace themis {
namespace utils {
namespace memory {

#ifdef THEMIS_ENABLE_CUDA

/**
 * @brief Custom deleter for CUDA device memory
 * 
 * Automatically calls cudaFree when the unique_ptr is destroyed.
 * Exception-safe and follows RAII principles.
 */
struct CudaDeleter {
    void operator()(void* ptr) const noexcept {
        if (ptr) {
            cudaFree(ptr);
        }
    }
};

/**
 * @brief Custom deleter for CUDA pinned host memory
 * 
 * Automatically calls cudaFreeHost when the unique_ptr is destroyed.
 */
struct CudaPinnedDeleter {
    void operator()(void* ptr) const noexcept {
        if (ptr) {
            cudaFreeHost(ptr);
        }
    }
};

/**
 * @brief RAII wrapper for CUDA device memory
 * 
 * Provides exception-safe CUDA memory allocation with automatic cleanup.
 * Uses std::unique_ptr with custom deleter for proper resource management.
 * 
 * Example usage:
 * @code
 *   auto gpu_buffer = make_cuda_unique<float>(1024);
 *   float* ptr = gpu_buffer.get();
 *   // cudaFree automatically called when gpu_buffer goes out of scope
 * @endcode
 */
template<typename T>
using CudaUniquePtr = std::unique_ptr<T, CudaDeleter>;

/**
 * @brief Factory function for CUDA device memory allocation
 * 
 * @param count Number of elements to allocate (default: 1)
 * @return CudaUniquePtr<T> Smart pointer managing the allocated memory
 * @throws std::runtime_error if cudaMalloc fails
 */
template<typename T>
CudaUniquePtr<T> make_cuda_unique(size_t count = 1) {
    T* ptr = nullptr;
    cudaError_t err = cudaMalloc(&ptr, count * sizeof(T));
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string("CUDA allocation failed: ") + 
                                 cudaGetErrorString(err));
    }
    return CudaUniquePtr<T>(ptr);
}

/**
 * @brief RAII wrapper for CUDA pinned host memory
 * 
 * Manages pinned (page-locked) host memory that can be accessed by GPU
 * with higher bandwidth. Automatically calls cudaFreeHost on destruction.
 * 
 * Example usage:
 * @code
 *   PinnedMemory<float> host_buffer(1024);
 *   float* ptr = host_buffer.get();
 *   size_t size = host_buffer.size();
 * @endcode
 */
template<typename T>
class PinnedMemory {
public:
    /**
     * @brief Allocate pinned host memory
     * 
     * @param count Number of elements to allocate
     * @throws std::runtime_error if cudaMallocHost fails
     */
    explicit PinnedMemory(size_t count) 
        : ptr_(nullptr), count_(count) {
        T* raw_ptr = nullptr;
        cudaError_t err = cudaMallocHost(&raw_ptr, count * sizeof(T));
        if (err != cudaSuccess) {
            throw std::runtime_error(std::string("Pinned allocation failed: ") + 
                                     cudaGetErrorString(err));
        }
        ptr_.reset(raw_ptr);
    }
    
    // Prevent copying
    PinnedMemory(const PinnedMemory&) = delete;
    PinnedMemory& operator=(const PinnedMemory&) = delete;
    
    // Allow moving
    PinnedMemory(PinnedMemory&&) noexcept noexcept = default;
    PinnedMemory& operator=(PinnedMemory&&) noexcept noexcept = default;
    
    /**
     * @brief Get raw pointer to pinned memory
     * @return T* Raw pointer (non-owning)
     */
    T* get() const noexcept { 
        return ptr_.get(); 
    }
    
    /**
     * @brief Get number of elements
     * @return size_t Element count
     */
    size_t size() const noexcept { 
        return count_; 
    }
    
    /**
     * @brief Get total size in bytes
     * @return size_t Byte count
     */
    size_t bytes() const noexcept { 
        return count_ * sizeof(T); 
    }
    
    /**
     * @brief Check if memory is allocated
     * @return bool True if pointer is non-null
     */
    explicit operator bool() const noexcept { 
        return ptr_ != nullptr; 
    }

private:
    std::unique_ptr<T, CudaPinnedDeleter> ptr_;
    size_t count_;
};

/**
 * @brief RAII wrapper for raw CUDA device pointers (void*)
 * 
 * For cases where type information is not available at compile time.
 * Manages void* pointers with automatic cudaFree on destruction.
 * 
 * Example usage:
 * @code
 *   auto gpu_buffer = make_cuda_buffer(1024 * sizeof(float));
 *   void* ptr = gpu_buffer.get();
 * @endcode
 */
class CudaBuffer {
public:
    /**
     * @brief Allocate device memory
     * 
     * @param bytes Number of bytes to allocate
     * @throws std::runtime_error if cudaMalloc fails
     */
    explicit CudaBuffer(size_t bytes) 
        : ptr_(nullptr), size_(bytes) {
        void* raw_ptr = nullptr;
        cudaError_t err = cudaMalloc(&raw_ptr, bytes);
        if (err != cudaSuccess) {
            throw std::runtime_error(std::string("CUDA buffer allocation failed: ") + 
                                     cudaGetErrorString(err));
        }
        ptr_.reset(raw_ptr);
    }
    
    // Prevent copying
    CudaBuffer(const CudaBuffer&) = delete;
    CudaBuffer& operator=(const CudaBuffer&) = delete;
    
    // Allow moving
    CudaBuffer(CudaBuffer&&) noexcept noexcept = default;
    CudaBuffer& operator=(CudaBuffer&&) noexcept noexcept = default;
    
    /**
     * @brief Get raw pointer
     * @return void* Raw pointer (non-owning)
     */
    void* get() const noexcept { 
        return ptr_.get(); 
    }
    
    /**
     * @brief Release ownership of the pointer
     * 
     * Returns the raw pointer and relinquishes ownership.
     * The caller becomes responsible for freeing the memory.
     * Size is reset after releasing to maintain consistency.
     * 
     * @return void* Raw pointer (caller owns)
     */
    void* release() noexcept {
        void* result = ptr_.release();
        size_ = 0;  // Reset size to 0 after release to indicate no memory is owned
        return result;
    }
    
    /**
     * @brief Get size in bytes
     * 
     * Note: Returns 0 after release() is called.
     * 
     * @return size_t Byte count
     */
    size_t size() const noexcept { 
        return size_; 
    }
    
    /**
     * @brief Check if memory is allocated
     * @return bool True if pointer is non-null
     */
    explicit operator bool() const noexcept { 
        return ptr_ != nullptr; 
    }

private:
    std::unique_ptr<void, CudaDeleter> ptr_;
    size_t size_;
};

/**
 * @brief Factory function for CUDA buffer allocation
 * 
 * @param bytes Number of bytes to allocate
 * @return CudaBuffer RAII wrapper managing the buffer
 * @throws std::runtime_error if cudaMalloc fails
 */
inline CudaBuffer make_cuda_buffer(size_t bytes) {
    return CudaBuffer(bytes);
}

#else // !THEMIS_ENABLE_CUDA

// CPU fallback implementations when CUDA is not available

/**
 * @brief CPU fallback deleter using std::free
 */
struct CpuDeleter {
    void operator()(void* ptr) const noexcept {
        if (ptr) {
            std::free(ptr);
        }
    }
};

/**
 * @brief CPU fallback for CudaBuffer
 * 
 * Uses malloc/free for CPU-only builds.
 */
class CudaBuffer {
public:
    explicit CudaBuffer(size_t bytes) 
        : ptr_(nullptr), size_(bytes) {
        void* raw_ptr = std::malloc(bytes);
        if (!raw_ptr && bytes > 0) {
            throw std::runtime_error("CPU buffer allocation failed");
        }
        ptr_.reset(raw_ptr);
    }
    
    CudaBuffer(const CudaBuffer&) = delete;
    CudaBuffer& operator=(const CudaBuffer&) = delete;
    CudaBuffer(CudaBuffer&&) noexcept noexcept = default;
    CudaBuffer& operator=(CudaBuffer&&) noexcept noexcept = default;
    
    /**
     * @brief Get raw pointer
     * @return void* Raw pointer (non-owning)
     */
    void* get() const noexcept { return ptr_.get(); }
    
    /**
     * @brief Release ownership of the pointer
     * 
     * Returns the raw pointer and relinquishes ownership.
     * The caller becomes responsible for freeing the memory.
     * After calling release(), size() will return 0.
     * 
     * @return void* Raw pointer (caller owns)
     */
    void* release() noexcept { 
        void* result = ptr_.release();
        size_ = 0;  // Reset size to 0 after release to indicate no memory is owned
        return result;
    }
    
    /**
     * @brief Get size in bytes
     * 
     * Note: Returns 0 after release() is called.
     * 
     * @return size_t Byte count
     */
    size_t size() const noexcept { return size_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    std::unique_ptr<void, CpuDeleter> ptr_;
    size_t size_;
};

inline CudaBuffer make_cuda_buffer(size_t bytes) {
    return CudaBuffer(bytes);
}

#endif // THEMIS_ENABLE_CUDA

} // namespace memory
} // namespace utils
} // namespace themis
