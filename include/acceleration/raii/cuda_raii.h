/**
 * @file cuda_raii.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// RAII wrappers for CUDA resources
// Provides automatic resource cleanup and exception safety
// Header-only implementation for ease of use

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace themis {
namespace acceleration {
namespace raii {

// ============================================================================
// CUDA Stream RAII Wrapper
// ============================================================================

class CudaStream {
public:
    CudaStream() : stream_(nullptr), owned_(false) {}
    
    explicit CudaStream(bool createNow, unsigned int flags = cudaStreamDefault) 
        : stream_(nullptr), owned_(false) {
        if (createNow) {
            create(flags);
        }
    }
    
    /**
     * @brief Wrap.
     * @param[in] stream Input parameter.
     * @return Return value.
     * @details Implements wrap without additional internal calls.
     */
    static CudaStream wrap(cudaStream_t stream) {
        CudaStream wrapper;
        wrapper.stream_ = stream;
        wrapper.owned_ = false;
        return wrapper;
    }
    
    // Non-copyable
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;
    
    CudaStream(CudaStream&& other) noexcept 
        : stream_(other.stream_), owned_(other.owned_) {
        other.stream_ = nullptr;
        other.owned_ = false;
    }
    
    CudaStream& operator=(CudaStream&& other) noexcept {
        if (this != &other) {
            destroy();
            stream_ = other.stream_;
            owned_ = other.owned_;
            other.stream_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    ~CudaStream() {
        destroy();
    }
    
    void create(unsigned int flags = cudaStreamDefault) {
        if (stream_ && owned_) {
            destroy();
        }
        
        cudaError_t err = cudaStreamCreate(&stream_);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to create CUDA stream: ") + 
                cudaGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    void createWithPriority(int priority, unsigned int flags = cudaStreamNonBlocking) {
        if (stream_ && owned_) {
            destroy();
        }
        
        cudaError_t err = cudaStreamCreateWithPriority(&stream_, flags, priority);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to create CUDA stream with priority: ") + 
                cudaGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    /**
     * @brief Synchronize.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: cudaStreamSynchronize(), std::string(), cudaGetErrorString().
     */
    void synchronize() {
        if (stream_) {
            cudaError_t err = cudaStreamSynchronize(stream_);
            if (err != cudaSuccess) {
                throw std::runtime_error(
                    std::string("CUDA stream synchronization failed: ") + 
                    cudaGetErrorString(err)
                );
            }
        }
    }
    
    bool valid() const { return stream_ != nullptr; }
    
    cudaStream_t get() const { return stream_; }
    
    /**
     * @brief Release.
     * @return Return value.
     * @details Implements release without additional internal calls.
     */
    cudaStream_t release() {
        owned_ = false;
        cudaStream_t tmp = stream_;
        stream_ = nullptr;
        return tmp;
    }
    
private:
    /**
     * @brief Destroy.
     * @details Calls: cudaStreamDestroy().
     */
    void destroy() {
        if (stream_ && owned_) {
            cudaStreamDestroy(stream_);
        }
        stream_ = nullptr;
        owned_ = false;
    }
    
    cudaStream_t stream_;
    bool owned_;
};

// ============================================================================
// CUDA Device Memory RAII Wrapper
// ============================================================================

class CudaDeviceMemory {
public:
    CudaDeviceMemory() : ptr_(nullptr), size_(0) {}
    
    explicit CudaDeviceMemory(size_t size) : ptr_(nullptr), size_(0) {
        if (size > 0) {
            allocate(size);
        }
    }
    
    // Non-copyable
    CudaDeviceMemory(const CudaDeviceMemory&) = delete;
    CudaDeviceMemory& operator=(const CudaDeviceMemory&) = delete;
    
    CudaDeviceMemory(CudaDeviceMemory&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    
    CudaDeviceMemory& operator=(CudaDeviceMemory&& other) noexcept {
        if (this != &other) {
            free();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    ~CudaDeviceMemory() {
        free();
    }
    
    /**
     * @brief Allocate.
     * @param[in] size Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: free(), cudaMalloc(), std::string(), std::to_string(), cudaGetErrorString().
     */
    void allocate(size_t size) {
        if (ptr_) {
            free();
        }
        
        if (size == 0) {
            return;
        }
        
        cudaError_t err = cudaMalloc(&ptr_, size);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to allocate CUDA device memory (") + 
                std::to_string(size) + " bytes): " + 
                cudaGetErrorString(err)
            );
        }
        size_ = size;
    }
    
    void copyFrom(const void* host, size_t size, cudaStream_t stream = 0) {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy to unallocated CUDA memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        cudaError_t err = {};
        if (stream) {
            err = cudaMemcpyAsync(ptr_, host, size, cudaMemcpyHostToDevice, stream);
        } else {
            err = cudaMemcpy(ptr_, host, size, cudaMemcpyHostToDevice);
        }
        
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy to CUDA device memory: ") + 
                cudaGetErrorString(err)
            );
        }
    }
    
    void copyTo(void* host, size_t size, cudaStream_t stream = 0) const {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy from unallocated CUDA memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        cudaError_t err = {};
        if (stream) {
            err = cudaMemcpyAsync(host, ptr_, size, cudaMemcpyDeviceToHost, stream);
        } else {
            err = cudaMemcpy(host, ptr_, size, cudaMemcpyDeviceToHost);
        }
        
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy from CUDA device memory: ") + 
                cudaGetErrorString(err)
            );
        }
    }
    
    bool valid() const { return ptr_ != nullptr; }
    
    void* get() const { return ptr_; }
    
    size_t size() const { return size_; }
    
    /**
     * @brief Release.
     * @return Pointer to the result.
     * @details Implements release without additional internal calls.
     */
    void* release() {
        void* tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return tmp;
    }
    
private:
    /**
     * @brief Free.
     * @details Calls: cudaFree().
     */
    void free() {
        if (ptr_) {
            cudaFree(ptr_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }
    
    void* ptr_;
    size_t size_;
};

// ============================================================================
// cuBLAS Handle RAII Wrapper
// ============================================================================

class CublasHandle {
public:
    CublasHandle() noexcept : handle_(nullptr) {}

    explicit CublasHandle(bool createNow) : handle_(nullptr) {
        if (createNow) {
            createOrThrow();
        }
    }

    // Non-copyable
    CublasHandle(const CublasHandle&)            = delete;
    CublasHandle& operator=(const CublasHandle&) = delete;

    // Movable
    CublasHandle(CublasHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    CublasHandle& operator=(CublasHandle&& other) noexcept {
        if (this != &other) {
            destroy();
            handle_       = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~CublasHandle() { destroy(); }

    bool create() noexcept {
        if (handle_) { destroy(); }
        return cublasCreate(&handle_) == CUBLAS_STATUS_SUCCESS;
    }

    /**
     * @brief Create Or Throw.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: destroy(), cublasCreate().
     */
    void createOrThrow() {
        if (handle_) { destroy(); }
        if (cublasCreate(&handle_) != CUBLAS_STATUS_SUCCESS) {
            throw std::runtime_error("Failed to create cuBLAS handle");
        }
    }

    cublasHandle_t get() const noexcept { return handle_; }

    bool valid() const noexcept { return handle_ != nullptr; }

    cublasHandle_t release() noexcept {
        cublasHandle_t tmp = handle_;
        handle_            = nullptr;
        return tmp;
    }

private:
    void destroy() noexcept {
        if (handle_) {
            cublasDestroy(handle_);
            handle_ = nullptr;
        }
    }

    cublasHandle_t handle_;
};

// ============================================================================
// Typed CUDA Device Buffer (RAII, exception-safe)
// ============================================================================

template<typename T>
class CudaDeviceBuffer {
public:
    CudaDeviceBuffer() noexcept : ptr_(nullptr), count_(0) {}

    explicit CudaDeviceBuffer(size_t count) : ptr_(nullptr), count_(0) {
        if (count > 0) { allocate(count); }
    }

    // Non-copyable
    CudaDeviceBuffer(const CudaDeviceBuffer&)            = delete;
    CudaDeviceBuffer& operator=(const CudaDeviceBuffer&) = delete;

    // Movable
    CudaDeviceBuffer(CudaDeviceBuffer&& other) noexcept
        : ptr_(other.ptr_), count_(other.count_) {
        other.ptr_   = nullptr;
        other.count_ = 0;
    }

    CudaDeviceBuffer& operator=(CudaDeviceBuffer&& other) noexcept {
        if (this != &other) {
            free();
            ptr_         = other.ptr_;
            count_       = other.count_;
            other.ptr_   = nullptr;
            other.count_ = 0;
        }
        return *this;
    }

    ~CudaDeviceBuffer() { free(); }

    /**
     * @brief Allocate.
     * @param[in] count Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: free(), cudaMalloc(), std::string(), std::to_string(), cudaGetErrorString().
     */
    void allocate(size_t count) {
        if (ptr_) { free(); }
        if (count == 0) { return; }
        cudaError_t err = cudaMalloc(&ptr_, count * sizeof(T));
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("CudaDeviceBuffer: cudaMalloc failed (") +
                std::to_string(count * sizeof(T)) + " bytes): " +
                cudaGetErrorString(err));
        }
        count_ = count;
    }

    bool tryAllocate(size_t count) noexcept {
        if (ptr_) { free(); }
        if (count == 0) { return true; }
        if (cudaMalloc(&ptr_, count * sizeof(T)) != cudaSuccess) {
            ptr_   = nullptr;
            count_ = 0;
            return false;
        }
        count_ = count;
        return true;
    }

    /**
     * @brief Copy From.
     * @param[in] src Input parameter.
     * @param[in] count Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: cudaMemcpy(), std::string(), cudaGetErrorString().
     */
    void copyFrom(const T* src, size_t count) {
        if (!ptr_) {
            throw std::runtime_error("CudaDeviceBuffer: copyFrom on unallocated buffer");
        }
        if (count > count_) {
            throw std::runtime_error("CudaDeviceBuffer: copyFrom size exceeds allocation");
        }
        cudaError_t err = cudaMemcpy(ptr_, src, count * sizeof(T), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("CudaDeviceBuffer: cudaMemcpy H2D failed: ") +
                cudaGetErrorString(err));
        }
    }

    void copyTo(T* dst, size_t count) const {
        if (!ptr_) {
            throw std::runtime_error("CudaDeviceBuffer: copyTo on unallocated buffer");
        }
        if (count > count_) {
            throw std::runtime_error("CudaDeviceBuffer: copyTo size exceeds allocation");
        }
        cudaError_t err = cudaMemcpy(dst, ptr_, count * sizeof(T), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("CudaDeviceBuffer: cudaMemcpy D2H failed: ") +
                cudaGetErrorString(err));
        }
    }

    T*     get() const noexcept { return ptr_; }
    size_t count() const noexcept { return count_; }
    bool   valid() const noexcept { return ptr_ != nullptr; }

    T* release() noexcept {
        T* tmp   = ptr_;
        ptr_     = nullptr;
        count_   = 0;
        return tmp;
    }

private:
    void free() noexcept {
        if (ptr_) {
            cudaFree(ptr_);
            ptr_   = nullptr;
            count_ = 0;
        }
    }

    T*     ptr_;
    size_t count_;
};

} // namespace raii
} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
