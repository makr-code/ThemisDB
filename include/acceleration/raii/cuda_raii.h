/**
 * @file cuda_raii.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// @brief RAII wrapper for CUDA stream (cudaStream_t).
///
/// Manages the lifetime of a CUDA stream created with cudaStreamCreate().
/// Automatically destroys the stream on scope exit (exception-safe RAII).
/// 
/// Supports ownership semantics:
/// - **Owned streams**: created by CudaStream itself, destroyed on scope exit.
/// - **Non-owned streams**: wrapped around externally-created streams, not destroyed.
///
/// Features:
/// - Move semantics: efficient transfer of stream ownership.
/// - Non-copyable: prevents accidental stream duplication.
/// - Exception-safe: stream is released even during unwinding.
///
/// Example usage:
/// ```cpp
/// CudaStream stream(true);  // Create and manage a new stream
/// stream.synchronize();      // Wait for all work in the stream
/// // stream automatically destroyed on scope exit
/// ```
///
/// @note For wrapping externally-created streams, use wrap() static factory.
class CudaStream {
public:
    /// @brief Default constructor; does not create a stream.
    CudaStream() : stream_(nullptr), owned_(false) {}
    
    /// @brief Create a new CUDA stream.
    /// @param createNow If true, create the stream immediately; if false, defer creation.
    /// @param flags Stream creation flags (e.g., cudaStreamDefault, cudaStreamNonBlocking).
    /// @throws std::runtime_error if stream creation fails and createNow is true.
    explicit CudaStream(bool createNow, unsigned int flags = cudaStreamDefault) 
        : stream_(nullptr), owned_(false) {
        if (createNow) {
            create(flags);
        }
    }
    
    /// @brief Wrap an existing stream without taking ownership.
    /// @param stream The CUDA stream handle to wrap (not owned by the wrapper).
    /// @return A CudaStream instance that wraps but does not own the stream.
    /// @note The stream will not be destroyed when this wrapper goes out of scope.
    static CudaStream wrap(cudaStream_t stream) {
        CudaStream wrapper;
        wrapper.stream_ = stream;
        wrapper.owned_ = false;
        return wrapper;
    }
    
    // Non-copyable
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;
    
    /// @brief Move constructor; transfers stream ownership.
    CudaStream(CudaStream&& other) noexcept 
        : stream_(other.stream_), owned_(other.owned_) {
        other.stream_ = nullptr;
        other.owned_ = false;
    }
    
    /// @brief Move assignment; transfers stream ownership.
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
    
    /// @brief Destructor; destroys the stream if it is owned.
    ~CudaStream() {
        destroy();
    }
    
    /// @brief Create a new stream with specified flags.
    /// @param flags Stream creation flags (default: cudaStreamDefault).
    /// @throws std::runtime_error if stream creation fails.
    /// @note If a stream is already owned, it is destroyed first.
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
    
    /// @brief Create a stream with a specified priority.
    /// @param priority Stream priority (lower values have higher priority).
    /// @param flags Stream creation flags (default: cudaStreamNonBlocking).
    /// @throws std::runtime_error if stream creation fails.
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
    
    /// @brief Wait for all operations in this stream to complete.
    /// @throws std::runtime_error if synchronization fails.
    /// @note No-op if stream is invalid (nullptr).
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
    
    /// @brief Check if the stream is valid and ready for use.
    /// @return true if the stream has been created and is non-null; false otherwise.
    bool valid() const { return stream_ != nullptr; }
    
    /// @brief Get the underlying CUDA stream handle.
    /// @return The cudaStream_t handle; nullptr if not created.
    /// @note The returned handle remains valid until this object is destroyed or reassigned.
    cudaStream_t get() const { return stream_; }
    
    /// @brief Release ownership of the stream without destroying it.
    /// @return The underlying CUDA stream handle.
    /// @note After calling release(), the caller is responsible for destroying the stream.
    cudaStream_t release() {
        owned_ = false;
        cudaStream_t tmp = stream_;
        stream_ = nullptr;
        return tmp;
    }
    
private:
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

/// @brief RAII wrapper for CUDA device memory (allocated via cudaMalloc).
///
/// Manages the lifetime of GPU device memory. Automatically frees memory on
/// scope exit (exception-safe RAII).
///
/// Features:
/// - Type-agnostic: wraps raw device memory in bytes.
/// - Move semantics: efficient transfer of memory ownership.
/// - Non-copyable: prevents accidental memory duplication.
/// - Bounds-checked copy operations (host↔device).
/// - Exception-safe: memory is released even during unwinding.
///
/// For type-safe allocation and copying, use CudaDeviceBuffer<T> instead.
///
/// Example usage:
/// ```cpp
/// CudaDeviceMemory dev_mem(1024);  // Allocate 1024 bytes
/// host_data to_device(host_buffer, 1024);  // Copy host→device
/// dev_mem.copyTo(host_buffer, 1024);       // Copy device→host
/// // Memory automatically freed on scope exit
/// ```
///
/// @see CudaDeviceBuffer for type-safe memory management.
class CudaDeviceMemory {
public:
    /// @brief Default constructor; does not allocate memory.
    CudaDeviceMemory() : ptr_(nullptr), size_(0) {}
    
    /// @brief Allocate device memory.
    /// @param size Number of bytes to allocate.
    /// @throws std::runtime_error if allocation fails (e.g., out of device memory).
    explicit CudaDeviceMemory(size_t size) : ptr_(nullptr), size_(0) {
        if (size > 0) {
            allocate(size);
        }
    }
    
    // Non-copyable
    CudaDeviceMemory(const CudaDeviceMemory&) = delete;
    CudaDeviceMemory& operator=(const CudaDeviceMemory&) = delete;
    
    /// @brief Move constructor; transfers memory ownership.
    CudaDeviceMemory(CudaDeviceMemory&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    
    /// @brief Move assignment; transfers memory ownership.
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
    
    /// @brief Destructor; frees the allocated device memory.
    ~CudaDeviceMemory() {
        free();
    }
    
    /// @brief Allocate device memory.
    /// @param size Number of bytes to allocate.
    /// @throws std::runtime_error if allocation fails or if memory is already allocated.
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
    
    /// @brief Copy data from host to device.
    /// @param host Source pointer on the host (must be valid for @p size bytes).
    /// @param size Number of bytes to copy (must be ≤ allocated size).
    /// @param stream Optional CUDA stream for asynchronous copy; 0 for synchronous.
    /// @throws std::runtime_error if memory is unallocated, copy exceeds capacity, or copy fails.
    void copyFrom(const void* host, size_t size, cudaStream_t stream = 0) {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy to unallocated CUDA memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        cudaError_t err;
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
    
    /// @brief Copy data from device to host.
    /// @param host Destination pointer on the host (must be writable for @p size bytes).
    /// @param size Number of bytes to copy (must be ≤ allocated size).
    /// @param stream Optional CUDA stream for asynchronous copy; 0 for synchronous.
    /// @throws std::runtime_error if memory is unallocated, copy exceeds capacity, or copy fails.
    void copyTo(void* host, size_t size, cudaStream_t stream = 0) const {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy from unallocated CUDA memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        cudaError_t err;
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
    
    /// @brief Check if memory has been allocated.
    /// @return true if memory is allocated; false otherwise.
    bool valid() const { return ptr_ != nullptr; }
    
    /// @brief Get the raw device memory pointer.
    /// @return The device pointer; nullptr if unallocated.
    void* get() const { return ptr_; }
    
    /// @brief Get the allocated size in bytes.
    /// @return The size of the allocated memory; 0 if unallocated.
    size_t size() const { return size_; }
    
    /// @brief Release ownership of the memory without freeing it.
    /// @return The raw device pointer.
    /// @note After calling release(), the caller is responsible for calling cudaFree().
    void* release() {
        void* tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return tmp;
    }
    
private:
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

/**
 * @brief RAII wrapper for cublasHandle_t.
 *
 * Ensures the cuBLAS handle is destroyed even when exceptions are thrown
 * between cublasCreate() and cublasDestroy().
 *
 * @par Example
 * CublasHandle blas;
 * if (!blas.create()) { return; } // creation failed — no leak
 * // use blas.get() for cuBLAS calls
 * // handle automatically destroyed on scope exit
 */
class CublasHandle {
public:
    /// @brief Default constructor; does not create a handle.
    CublasHandle() noexcept : handle_(nullptr) {}

    /// @brief Construct and immediately create a handle.
    /// @throws std::runtime_error if cublasCreate fails.
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

    /**
     * @brief Create a cuBLAS handle.
     * @return true on success, false on failure (no exception).
     */
    bool create() noexcept {
        if (handle_) { destroy(); }
        return cublasCreate(&handle_) == CUBLAS_STATUS_SUCCESS;
    }

    /**
     * @brief Create a cuBLAS handle, throwing on failure.
     * @throws std::runtime_error if cublasCreate fails.
     */
    void createOrThrow() {
        if (handle_) { destroy(); }
        if (cublasCreate(&handle_) != CUBLAS_STATUS_SUCCESS) {
            throw std::runtime_error("Failed to create cuBLAS handle");
        }
    }

    /// @brief Returns the underlying handle (nullptr if not created).
    cublasHandle_t get() const noexcept { return handle_; }

    /// @brief Returns true if a valid handle has been created.
    bool valid() const noexcept { return handle_ != nullptr; }

    /// @brief Release ownership without destroying; caller takes responsibility.
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

/**
 * @brief Type-safe RAII wrapper for a CUDA device buffer of element type T.
 *
 * Allocates @p count elements of type T on the device and frees them on
 * destruction.  All copy operations to/from host memory are bounds-checked.
 *
 * @tparam T Element type (must be trivially copyable for host↔device copies).
 *
 * @par Example
 * CudaDeviceBuffer<float> d_a(n);
 * d_a.copyFrom(host_vec.data(), n);
 * // use d_a.get() with cuBLAS / kernel calls
 * // freed automatically on scope exit — exception-safe
 */
template<typename T>
class CudaDeviceBuffer {
public:
    /// @brief Default constructor; allocates nothing.
    CudaDeviceBuffer() noexcept : ptr_(nullptr), count_(0) {}

    /**
     * @brief Allocate @p count elements of T on the device.
     * @param count Number of elements to allocate.
     * @throws std::runtime_error if cudaMalloc fails.
     */
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
     * @brief Allocate @p count elements on the device.
     * @throws std::runtime_error on allocation failure.
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

    /**
     * @brief Try to allocate; returns false instead of throwing on failure.
     * @param count Number of elements to allocate.
     * @return true on success.
     */
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
     * @brief Copy @p count elements from host @p src into device buffer.
     * @param src   Host pointer with at least @p count valid elements.
     * @param count Number of elements to copy (must be ≤ capacity).
     * @throws std::runtime_error on copy failure or bounds violation.
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

    /**
     * @brief Copy @p count elements from device buffer to host @p dst.
     * @param dst   Host pointer with capacity for at least @p count elements.
     * @param count Number of elements to copy (must be ≤ capacity).
     * @throws std::runtime_error on copy failure or bounds violation.
     */
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

    /// @brief Returns a typed device pointer.
    T*     get() const noexcept { return ptr_; }
    /// @brief Returns the allocated element count.
    size_t count() const noexcept { return count_; }
    /// @brief Returns true if allocated and non-empty.
    bool   valid() const noexcept { return ptr_ != nullptr; }

    /// @brief Release ownership; caller is responsible for cudaFree.
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
