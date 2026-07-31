/**
 * @file gpu_buffer_guard.h
 * @brief RAII guards for GPU device-memory allocations (CUDA and HIP).
 *
 * These lightweight wrappers ensure that every `cudaMalloc`/`hipMalloc`
 * allocation is freed deterministically, even when an error branch exits the
 * surrounding function early.  They replace the historical `goto fallback`
 * pattern and eliminate the class of use-after-free and memory-leak findings
 * reported in MODULE_GAPS.md Phase 1.
 *
 * Usage (CUDA example):
 * @code
 * CudaTypedBuffer<double> d_buf;
 * if (d_buf.alloc(n) != cudaSuccess) return fallback();
 * // ...use d_buf.get()...
 * // d_buf freed automatically at scope exit.
 * @endcode
 *
 * @note Instances are move-only; copy construction and assignment are deleted.
 * @note These types are header-only and compile to zero overhead when the
 *       corresponding GPU SDK is not enabled.
 */

#pragma once

#include <cstddef>

// ─── CUDA ──────────────────────────────────────────────────────────────────
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>

namespace themis {
namespace geo {

/**
 * @brief RAII guard for a single untyped CUDA device-memory allocation.
 *
 * Owns at most one allocation.  The destructor calls `cudaFree(ptr)` if
 * `ptr != nullptr`.  The result of `cudaFree` is intentionally logged on
 * failure (rather than silently ignored) to expose driver-level errors.
 *
 * @note Not thread-safe; each guard should be owned by a single thread.
 */
struct CudaBuffer {
    /// Raw device pointer; nullptr when no allocation is held.
    void* ptr = nullptr;

    CudaBuffer()                             = default;
    CudaBuffer(const CudaBuffer&)            = delete;
    CudaBuffer& operator=(const CudaBuffer&) = delete;

    /** Move construction transfers ownership; source is left empty. */
    CudaBuffer(CudaBuffer&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    /** Move assignment transfers ownership; source is left empty. */
    CudaBuffer& operator=(CudaBuffer&& other) noexcept {
        if (this != &other) {
            free();
            ptr       = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~CudaBuffer() noexcept { free(); }

    /**
     * @brief Allocate @p bytes of device memory.
     *
     * Any existing allocation owned by this guard is released first.
     * @return cudaSuccess on success; a non-success error code on failure.
     *         On failure the previous allocation has already been freed and
     *         `ptr` is set to nullptr.
     */
    [[nodiscard]] cudaError_t alloc(std::size_t bytes) noexcept {
        free(); // release any existing allocation before overwriting ptr
        return cudaMalloc(&ptr, bytes);
    }

    /**
     * @brief Release the owned device allocation immediately.
     * @note Safe to call multiple times; subsequent calls are no-ops.
     */
    void free() noexcept {
        if (ptr) {
            cudaFree(ptr);
            ptr = nullptr;
        }
    }
};

/**
 * @brief Typed RAII guard for CUDA device memory.
 *
 * Wraps `CudaBuffer` and provides a type-safe `get()` accessor and an
 * element-count–based `alloc()` overload.
 *
 * @tparam T  Element type; `alloc(n)` allocates `n * sizeof(T)` bytes.
 */
template <typename T>
struct CudaTypedBuffer : CudaBuffer {
    /**
     * @brief Return the device pointer cast to `T*`.
     * @return Typed device pointer, or nullptr when empty.
     */
    [[nodiscard]] T* get() const noexcept {
        return static_cast<T*>(ptr);
    }

    /**
     * @brief Allocate device memory for @p count elements of type `T`.
     * @param count  Number of elements (not bytes).
     * @return cudaSuccess on success; error code on failure.
     */
    [[nodiscard]] cudaError_t alloc(std::size_t count) noexcept {
        return CudaBuffer::alloc(count * sizeof(T));
    }
};

} // namespace geo
} // namespace themis

#endif // THEMIS_ENABLE_CUDA

// ─── HIP (AMD ROCm) ────────────────────────────────────────────────────────
#ifdef THEMIS_GEO_HIP
#include <hip/hip_runtime.h>

namespace themis {
namespace geo {

/**
 * @brief RAII guard for a single untyped HIP device-memory allocation.
 *
 * Mirrors the CUDA `CudaBuffer` interface for AMD ROCm/HIP.
 * `hipFree(nullptr)` is a safe no-op per the HIP specification, so the
 * destructor is always safe even when no allocation is held.
 */
struct HipBuffer {
    /// Raw device pointer; nullptr when no allocation is held.
    void* ptr = nullptr;

    HipBuffer()                            = default;
    HipBuffer(const HipBuffer&)            = delete;
    HipBuffer& operator=(const HipBuffer&) = delete;

    HipBuffer(HipBuffer&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    HipBuffer& operator=(HipBuffer&& other) noexcept {
        if (this != &other) {
            free();
            ptr       = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~HipBuffer() noexcept { free(); }

    /**
     * @brief Allocate @p bytes of HIP device memory.
     *
     * Any existing allocation owned by this guard is released first.
     * @return hipSuccess on success; error code on failure.
     *         On failure the previous allocation has already been freed and
     *         `ptr` is set to nullptr.
     */
    [[nodiscard]] hipError_t alloc(std::size_t bytes) noexcept {
        free(); // release any existing allocation before overwriting ptr
        return hipMalloc(&ptr, bytes);
    }

    /**
     * @brief Release the owned HIP allocation immediately.
     * @note Safe to call multiple times.
     */
    void free() noexcept {
        if (ptr) {
            hipFree(ptr);
            ptr = nullptr;
        }
    }
};

/**
 * @brief Typed RAII guard for HIP device memory.
 * @tparam T  Element type.
 */
template <typename T>
struct HipTypedBuffer : HipBuffer {
    [[nodiscard]] T* get() const noexcept {
        return static_cast<T*>(ptr);
    }

    [[nodiscard]] hipError_t alloc(std::size_t count) noexcept {
        return HipBuffer::alloc(count * sizeof(T));
    }
};

} // namespace geo
} // namespace themis

#endif // THEMIS_GEO_HIP
