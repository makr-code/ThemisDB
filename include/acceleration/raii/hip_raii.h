/**
 * @file hip_raii.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// RAII wrappers for HIP resources (AMD ROCm)
// Provides automatic resource cleanup and exception safety
// Header-only implementation for ease of use

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#include <hipblas/hipblas.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace themis {
namespace acceleration {
namespace raii {

// ============================================================================
// HIP Stream RAII Wrapper
// ============================================================================

/// @brief RAII wrapper for HIP stream (hipStream_t).
///
/// Manages the lifetime of a HIP stream created with hipStreamCreate().
/// Automatically destroys the stream on scope exit (exception-safe RAII).
/// 
/// Supports ownership semantics:
/// - **Owned streams**: created by HipStream itself, destroyed on scope exit.
/// - **Non-owned streams**: wrapped around externally-created streams, not destroyed.
///
/// Features:
/// - Move semantics: efficient transfer of stream ownership.
/// - Non-copyable: prevents accidental stream duplication.
/// - Exception-safe: stream is released even during unwinding.
/// - Equivalent API to CudaStream for portability.
///
/// Example usage:
/// ```cpp
/// HipStream stream(true);  // Create and manage a new stream
/// stream.synchronize();     // Wait for all work in the stream
/// // stream automatically destroyed on scope exit
/// ```
///
/// @note For wrapping externally-created streams, use wrap() static factory.
class HipStream {
public:
    /// @brief Default constructor; does not create a stream.
    HipStream() : stream_(nullptr), owned_(false) {}
    
    /// @brief Create a new HIP stream.
    /// @param createNow If true, create the stream immediately; if false, defer creation.
    /// @param flags Stream creation flags (default: 0).
    /// @throws std::runtime_error if stream creation fails and createNow is true.
    explicit HipStream(bool createNow, unsigned int flags = 0) 
        : stream_(nullptr), owned_(false) {
        if (createNow) {
            create(flags);
        }
    }
    
    /// @brief Wrap an existing stream without taking ownership.
    /// @param stream The HIP stream handle to wrap (not owned by the wrapper).
    /// @return A HipStream instance that wraps but does not own the stream.
    /// @note The stream will not be destroyed when this wrapper goes out of scope.
    static HipStream wrap(hipStream_t stream) {
        HipStream wrapper;
        wrapper.stream_ = stream;
        wrapper.owned_ = false;
        return wrapper;
    }
    
    // Non-copyable
    HipStream(const HipStream&) = delete;
    HipStream& operator=(const HipStream&) = delete;
    
    /// @brief Move constructor; transfers stream ownership.
    HipStream(HipStream&& other) noexcept 
        : stream_(other.stream_), owned_(other.owned_) {
        other.stream_ = nullptr;
        other.owned_ = false;
    }
    
    /// @brief Move assignment; transfers stream ownership.
    HipStream& operator=(HipStream&& other) noexcept {
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
    ~HipStream() {
        destroy();
    }
    
    /// @brief Create a new stream with specified flags.
    /// @param flags Stream creation flags (default: 0).
    /// @throws std::runtime_error if stream creation fails.
    /// @note If a stream is already owned, it is destroyed first.
    void create(unsigned int flags = 0) {
        if (stream_ && owned_) {
            destroy();
        }
        
        hipError_t err = hipStreamCreate(&stream_);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to create HIP stream: ") + 
                hipGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    /// @brief Create a stream with a specified priority.
    /// @param priority Stream priority (lower values have higher priority).
    /// @param flags Stream creation flags (default: hipStreamNonBlocking).
    /// @throws std::runtime_error if stream creation fails.
    void createWithPriority(int priority, unsigned int flags = hipStreamNonBlocking) {
        if (stream_ && owned_) {
            destroy();
        }
        
        hipError_t err = hipStreamCreateWithPriority(&stream_, flags, priority);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to create HIP stream with priority: ") + 
                hipGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    /// @brief Wait for all operations in this stream to complete.
    /// @throws std::runtime_error if synchronization fails.
    /// @note No-op if stream is invalid (nullptr).
    void synchronize() {
        if (stream_) {
            hipError_t err = hipStreamSynchronize(stream_);
            if (err != hipSuccess) {
                throw std::runtime_error(
                    std::string("HIP stream synchronization failed: ") + 
                    hipGetErrorString(err)
                );
            }
        }
    }
    
    /// @brief Check if the stream is valid and ready for use.
    /// @return true if the stream has been created and is non-null; false otherwise.
    bool valid() const { return stream_ != nullptr; }
    
    /// @brief Get the underlying HIP stream handle.
    /// @return The hipStream_t handle; nullptr if not created.
    /// @note The returned handle remains valid until this object is destroyed or reassigned.
    hipStream_t get() const { return stream_; }
    
    /// @brief Release ownership of the stream without destroying it.
    /// @return The underlying HIP stream handle.
    /// @note After calling release(), the caller is responsible for destroying the stream.
    hipStream_t release() {
        owned_ = false;
        hipStream_t tmp = stream_;
        stream_ = nullptr;
        return tmp;
    }
    
private:
    void destroy() {
        if (stream_ && owned_) {
            hipStreamDestroy(stream_);
        }
        stream_ = nullptr;
        owned_ = false;
    }
    
    hipStream_t stream_;
    bool owned_;
};

// ============================================================================
// HIP Device Memory RAII Wrapper
// ============================================================================

/// @brief RAII wrapper for HIP device memory (allocated via hipMalloc).
///
/// Manages the lifetime of GPU device memory on AMD ROCm. Automatically frees
/// memory on scope exit (exception-safe RAII).
///
/// Features:
/// - Type-agnostic: wraps raw device memory in bytes.
/// - Move semantics: efficient transfer of memory ownership.
/// - Non-copyable: prevents accidental memory duplication.
/// - Bounds-checked copy operations (host↔device).
/// - Exception-safe: memory is released even during unwinding.
/// - Equivalent API to CudaDeviceMemory for portability.
///
/// For type-safe allocation and copying, use HipDeviceBuffer<T> instead.
///
/// Example usage:
/// ```cpp
/// HipDeviceMemory dev_mem(1024);  // Allocate 1024 bytes
/// dev_mem.copyFrom(host_buffer, 1024);  // Copy host→device
/// dev_mem.copyTo(host_buffer, 1024);    // Copy device→host
/// // Memory automatically freed on scope exit
/// ```
///
/// @see HipDeviceBuffer for type-safe memory management.
class HipDeviceMemory {
public:
    /// @brief Default constructor; does not allocate memory.
    HipDeviceMemory() : ptr_(nullptr), size_(0) {}
    
    /// @brief Allocate device memory.
    /// @param size Number of bytes to allocate.
    /// @throws std::runtime_error if allocation fails (e.g., out of device memory).
    explicit HipDeviceMemory(size_t size) : ptr_(nullptr), size_(0) {
        if (size > 0) {
            allocate(size);
        }
    }
    
    // Non-copyable
    HipDeviceMemory(const HipDeviceMemory&) = delete;
    HipDeviceMemory& operator=(const HipDeviceMemory&) = delete;
    
    /// @brief Move constructor; transfers memory ownership.
    HipDeviceMemory(HipDeviceMemory&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    
    /// @brief Move assignment; transfers memory ownership.
    HipDeviceMemory& operator=(HipDeviceMemory&& other) noexcept {
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
    ~HipDeviceMemory() {
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
        
        hipError_t err = hipMalloc(&ptr_, size);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to allocate HIP device memory (") + 
                std::to_string(size) + " bytes): " + 
                hipGetErrorString(err)
            );
        }
        size_ = size;
    }
    
    /// @brief Copy data from host to device.
    /// @param host Source pointer on the host (must be valid for @p size bytes).
    /// @param size Number of bytes to copy (must be ≤ allocated size).
    /// @param stream Optional HIP stream for asynchronous copy; 0 for synchronous.
    /// @throws std::runtime_error if memory is unallocated, copy exceeds capacity, or copy fails.
    void copyFrom(const void* host, size_t size, hipStream_t stream = 0) {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy to unallocated HIP memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        hipError_t err = {};
        if (stream) {
            err = hipMemcpyAsync(ptr_, host, size, hipMemcpyHostToDevice, stream);
        } else {
            err = hipMemcpy(ptr_, host, size, hipMemcpyHostToDevice);
        }
        
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy to HIP device memory: ") + 
                hipGetErrorString(err)
            );
        }
    }
    
    /// @brief Copy data from device to host.
    /// @param host Destination pointer on the host (must be writable for @p size bytes).
    /// @param size Number of bytes to copy (must be ≤ allocated size).
    /// @param stream Optional HIP stream for asynchronous copy; 0 for synchronous.
    /// @throws std::runtime_error if memory is unallocated, copy exceeds capacity, or copy fails.
    void copyTo(void* host, size_t size, hipStream_t stream = 0) const {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy from unallocated HIP memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        hipError_t err = {};
        if (stream) {
            err = hipMemcpyAsync(host, ptr_, size, hipMemcpyDeviceToHost, stream);
        } else {
            err = hipMemcpy(host, ptr_, size, hipMemcpyDeviceToHost);
        }
        
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy from HIP device memory: ") + 
                hipGetErrorString(err)
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
    /// @note After calling release(), the caller is responsible for calling hipFree().
    void* release() {
        void* tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return tmp;
    }
    
private:
    void free() {
        if (ptr_) {
            hipFree(ptr_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }
    
    void* ptr_;
    size_t size_;
};

// ============================================================================
// Scoped HIP Device Setter
// ============================================================================

/// @brief RAII wrapper for HIP device context switching.
///
/// Saves the current HIP device on construction, sets a new device,
/// and restores the previous device on destruction. Enables scoped device
/// context management without manual save/restore logic.
///
/// Features:
/// - Non-copyable and non-movable: enforces single-threaded usage.
/// - Exception-safe: previous device is restored during unwinding.
/// - Automatic cleanup: no explicit reset() call required.
///
/// Example usage:
/// ```cpp
/// {
///     ScopedHipDevice scoped_dev(1);  // Switch to device 1
///     // All HIP calls use device 1
/// }  // Automatically switch back to original device
/// ```
///
/// @note Not thread-safe; use in single-threaded contexts only.
///       For multi-threaded scenarios, synchronize access or use device
///       thread-local storage.
class ScopedHipDevice {
public:
    /// @brief Switch to the specified HIP device.
    /// @param deviceId The device ID to switch to.
    /// @throws std::runtime_error if the device cannot be queried or set.
    explicit ScopedHipDevice(int deviceId) : previousDevice_(-1) {
        hipError_t err = hipGetDevice(&previousDevice_);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to get current HIP device: ") + 
                hipGetErrorString(err)
            );
        }
        
        err = hipSetDevice(deviceId);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to set HIP device to ") + 
                std::to_string(deviceId) + ": " + 
                hipGetErrorString(err)
            );
        }
    }
    
    ~ScopedHipDevice() {
        if (previousDevice_ >= 0) {
            hipSetDevice(previousDevice_);
        }
    }
    
    // No copy or move
    ScopedHipDevice(const ScopedHipDevice&) = delete;
    ScopedHipDevice& operator=(const ScopedHipDevice&) = delete;
    ScopedHipDevice(ScopedHipDevice&&) = delete;
    ScopedHipDevice& operator=(ScopedHipDevice&&) = delete;
    
private:
    int previousDevice_;
};

// ============================================================================
// hipBLAS Handle RAII Wrapper
// ============================================================================

/**
 * @brief RAII wrapper for hipblasHandle_t.
 *
 * Ensures the hipBLAS handle is destroyed even when exceptions are thrown
 * between hipblasCreate() and hipblasDestroy().
 *
 * @par Example
 * HipblasHandle blas;
 * if (!blas.create()) { return; } // creation failed — no leak
 * // use blas.get() for hipBLAS calls
 * // handle automatically destroyed on scope exit
 */
class HipblasHandle {
public:
    /// @brief Default constructor; does not create a handle.
    HipblasHandle() noexcept : handle_(nullptr) {}

    /// @brief Construct and immediately create a handle.
    /// @throws std::runtime_error if hipblasCreate fails.
    explicit HipblasHandle(bool createNow) : handle_(nullptr) {
        if (createNow) {
            createOrThrow();
        }
    }

    // Non-copyable
    HipblasHandle(const HipblasHandle&)            = delete;
    HipblasHandle& operator=(const HipblasHandle&) = delete;

    // Movable
    HipblasHandle(HipblasHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    HipblasHandle& operator=(HipblasHandle&& other) noexcept {
        if (this != &other) {
            destroy();
            handle_       = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~HipblasHandle() { destroy(); }

    /**
     * @brief Create a hipBLAS handle.
     * @return true on success, false on failure (no exception).
     */
    bool create() noexcept {
        if (handle_) { destroy(); }
        return hipblasCreate(&handle_) == HIPBLAS_STATUS_SUCCESS;
    }

    /**
     * @brief Create a hipBLAS handle, throwing on failure.
     * @throws std::runtime_error if hipblasCreate fails.
     */
    void createOrThrow() {
        if (handle_) { destroy(); }
        if (hipblasCreate(&handle_) != HIPBLAS_STATUS_SUCCESS) {
            throw std::runtime_error("Failed to create hipBLAS handle");
        }
    }

    /// @brief Returns the underlying handle (nullptr if not created).
    hipblasHandle_t get() const noexcept { return handle_; }

    /// @brief Returns true if a valid handle has been created.
    bool valid() const noexcept { return handle_ != nullptr; }

    /// @brief Release ownership without destroying; caller takes responsibility.
    hipblasHandle_t release() noexcept {
        hipblasHandle_t tmp = handle_;
        handle_             = nullptr;
        return tmp;
    }

private:
    void destroy() noexcept {
        if (handle_) {
            hipblasDestroy(handle_);
            handle_ = nullptr;
        }
    }

    hipblasHandle_t handle_;
};

// ============================================================================
// Typed HIP Device Buffer (RAII, exception-safe)
// ============================================================================

/**
 * @brief Type-safe RAII wrapper for a HIP device buffer of element type T.
 *
 * Allocates @p count elements of type T on the device and frees them on
 * destruction.  All copy operations to/from host memory are bounds-checked.
 *
 * @tparam T Element type (must be trivially copyable for host↔device copies).
 *
 * @par Example
 * HipDeviceBuffer<float> d_a(n);
 * d_a.copyFrom(host_vec.data(), n);
 * // use d_a.get() with hipBLAS / kernel calls
 * // freed automatically on scope exit — exception-safe
 */
template<typename T>
class HipDeviceBuffer {
public:
    /// @brief Default constructor; allocates nothing.
    HipDeviceBuffer() noexcept : ptr_(nullptr), count_(0) {}

    /**
     * @brief Allocate @p count elements of T on the device.
     * @param count Number of elements to allocate.
     * @throws std::runtime_error if hipMalloc fails.
     */
    explicit HipDeviceBuffer(size_t count) : ptr_(nullptr), count_(0) {
        if (count > 0) { allocate(count); }
    }

    // Non-copyable
    HipDeviceBuffer(const HipDeviceBuffer&)            = delete;
    HipDeviceBuffer& operator=(const HipDeviceBuffer&) = delete;

    // Movable
    HipDeviceBuffer(HipDeviceBuffer&& other) noexcept
        : ptr_(other.ptr_), count_(other.count_) {
        other.ptr_   = nullptr;
        other.count_ = 0;
    }

    HipDeviceBuffer& operator=(HipDeviceBuffer&& other) noexcept {
        if (this != &other) {
            free();
            ptr_         = other.ptr_;
            count_       = other.count_;
            other.ptr_   = nullptr;
            other.count_ = 0;
        }
        return *this;
    }

    ~HipDeviceBuffer() { free(); }

    /**
     * @brief Allocate @p count elements on the device.
     * @throws std::runtime_error on allocation failure.
     */
    void allocate(size_t count) {
        if (ptr_) { free(); }
        if (count == 0) { return; }
        hipError_t err = hipMalloc(&ptr_, count * sizeof(T));
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("HipDeviceBuffer: hipMalloc failed (") +
                std::to_string(count * sizeof(T)) + " bytes): " +
                hipGetErrorString(err));
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
        if (hipMalloc(&ptr_, count * sizeof(T)) != hipSuccess) {
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
            throw std::runtime_error("HipDeviceBuffer: copyFrom on unallocated buffer");
        }
        if (count > count_) {
            throw std::runtime_error("HipDeviceBuffer: copyFrom size exceeds allocation");
        }
        hipError_t err = hipMemcpy(ptr_, src, count * sizeof(T), hipMemcpyHostToDevice);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("HipDeviceBuffer: hipMemcpy H2D failed: ") +
                hipGetErrorString(err));
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
            throw std::runtime_error("HipDeviceBuffer: copyTo on unallocated buffer");
        }
        if (count > count_) {
            throw std::runtime_error("HipDeviceBuffer: copyTo size exceeds allocation");
        }
        hipError_t err = hipMemcpy(dst, ptr_, count * sizeof(T), hipMemcpyDeviceToHost);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("HipDeviceBuffer: hipMemcpy D2H failed: ") +
                hipGetErrorString(err));
        }
    }

    /// @brief Returns a typed device pointer.
    T*     get() const noexcept { return ptr_; }
    /// @brief Returns the allocated element count.
    size_t count() const noexcept { return count_; }
    /// @brief Returns true if allocated and non-empty.
    bool   valid() const noexcept { return ptr_ != nullptr; }

    /// @brief Release ownership; caller is responsible for hipFree.
    T* release() noexcept {
        T* tmp   = ptr_;
        ptr_     = nullptr;
        count_   = 0;
        return tmp;
    }

private:
    void free() noexcept {
        if (ptr_) {
            hipFree(ptr_);
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

#endif // THEMIS_ENABLE_HIP
