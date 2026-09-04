/**
 * @file cuda_utils.h
 * @brief RAII utilities and error-checking macros for CUDA device memory
 *        used throughout the ThemisDB index module.
 *
 * ### Motivation
 * Raw `cudaMalloc` / `cudaFree` calls scattered across GPU code paths are
 * error-prone: a single early-return or exception can leak device memory and
 * exhaust VRAM without any host-side indication.  This header provides:
 *
 *  - `CudaDeleter<T>` — a custom deleter that calls `cudaFree()`.
 *  - `CudaUniquePtr<T>` — `std::unique_ptr<T, CudaDeleter<T>>`, the RAII
 *    owner for a device allocation.  Destructs via `cudaFree` even when an
 *    exception propagates or a function returns early.
 *  - `cudaMakeUnique<T>(n)` — allocates `n` elements on the current CUDA
 *    device and wraps the pointer in a `CudaUniquePtr<T>`.  Returns a
 *    null wrapper on allocation failure (no throw).
 *  - `THEMIS_CUDA_CHECK(expr, error_code)` — asserts a `cudaError_t`
 *    expression and returns `error_code` on failure.
 *  - `THEMIS_CUDA_CHECK_BOOL(expr)` — asserts a `cudaError_t` expression
 *    and returns `false` on failure.
 *
 * ### Usage
 * @code
 * #ifdef THEMIS_ENABLE_CUDA
 * #include "index/cuda_utils.h"
 *
 * themis::index::CudaUniquePtr<float> d_buf = themis::index::cudaMakeUnique<float>(1024);
 * if (!d_buf) { return IndexErrorCode::GpuMemoryError; }
 * THEMIS_CUDA_CHECK(cudaMemcpy(d_buf.get(), h_buf, 1024 * sizeof(float),
 *                               cudaMemcpyHostToDevice),
 *                   IndexErrorCode::GpuKernelError);
 * #endif
 * @endcode
 *
 * @note All symbols are conditional on `THEMIS_ENABLE_CUDA`.  Include this
 *       header unconditionally; the `#ifdef` guard makes non-CUDA builds safe.
 *
 * @version 0.1.0
 * @date    2026-08-26
 */

#pragma once

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <memory>

#include "utils/logger.h"

namespace themis::index {

// ─────────────────────────────────────────────────────────────────────────────
// CudaDeleter — custom deleter that calls cudaFree()
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Custom deleter for CUDA device memory.
 *
 * Calls `cudaFree()` on destruction.  A null pointer is a no-op
 * (`cudaFree(nullptr)` is well-defined and returns `cudaSuccess`).
 *
 * @tparam T Element type of the device allocation.
 */
template <typename T>
struct CudaDeleter {
    /**
     * @brief Release a device pointer via `cudaFree()`.
     * @param ptr Device pointer to free.  May be null.
     */
    void operator()(T* ptr) const noexcept {
        if (ptr) {
          cudaFree(ptr);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// CudaUniquePtr — RAII owner for a CUDA device allocation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RAII wrapper for CUDA device memory.
 *
 * Behaves identically to `std::unique_ptr<T[]>` with the exception that
 * `cudaFree()` is used instead of `delete[]`.  Ownership may be transferred
 * with `std::move`; copying is deleted.
 *
 * @tparam T Element type of the device allocation.
 *
 * @par Thread Safety
 * Not thread-safe.  Use external synchronisation when a single `CudaUniquePtr`
 * is accessed from multiple threads.
 */
template <typename T>
using CudaUniquePtr = std::unique_ptr<T, CudaDeleter<T>>;

// ─────────────────────────────────────────────────────────────────────────────
// cudaMakeUnique — factory that wraps cudaMalloc in RAII
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Allocate `n` elements of type `T` on the current CUDA device.
 *
 * On success the returned `CudaUniquePtr<T>` owns the allocation.  On failure
 * (any `cudaError_t != cudaSuccess`) a null `CudaUniquePtr<T>` is returned;
 * no exception is thrown.
 *
 * @tparam T    Element type to allocate.
 * @param  n    Number of elements (the allocation is `n * sizeof(T)` bytes).
 *              Passing `n == 0` is implementation-defined in CUDA; the function
 *              returns a null wrapper for `n == 0` to avoid ambiguity.
 * @return RAII wrapper owning the allocation, or a null wrapper on failure.
 */
template <typename T>
[[nodiscard]] CudaUniquePtr<T> cudaMakeUnique(size_t n) {
    if (n == 0) return CudaUniquePtr<T>{nullptr};
    T* raw = nullptr;
    if (cudaMalloc(&raw, n * sizeof(T)) != cudaSuccess) {
        return CudaUniquePtr<T>{nullptr};
    }
    return CudaUniquePtr<T>{raw};
}

} // namespace themis::index

// ─────────────────────────────────────────────────────────────────────────────
// THEMIS_CUDA_CHECK — error-checking macros
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Evaluate a CUDA expression; return `error_code` on failure.
 *
 * Logs an error message that includes the file, line number, and the human-
 * readable CUDA error string before returning.  Intended for use inside
 * functions that return an `IndexErrorCode` or compatible integral type.
 *
 * @param expr       A `cudaError_t`-producing expression (e.g. a kernel launch
 *                   or `cudaMemcpy` call).
 * @param error_code The value to return when `expr != cudaSuccess`.
 *
 * @par Example
 * @code
 * THEMIS_CUDA_CHECK(cudaMemcpy(dst, src, bytes, cudaMemcpyHostToDevice),
 *                   IndexErrorCode::GpuKernelError);
 * @endcode
 */
#define THEMIS_CUDA_CHECK(expr, error_code)                                     \
    do {                                                                         \
        cudaError_t _themis_cuda_err_ = (expr);                                 \
        if (_themis_cuda_err_ != cudaSuccess) {                                 \
            THEMIS_ERROR("CUDA error at {}:{} — {}",                            \
                         __FILE__, __LINE__,                                     \
                         cudaGetErrorString(_themis_cuda_err_));                \
            return (error_code);                                                 \
        }                                                                        \
    } while (0)

/**
 * @brief Evaluate a CUDA expression; return `false` on failure.
 *
 * Variant of `THEMIS_CUDA_CHECK` for use inside boolean-returning functions.
 * Logs the same diagnostic information before returning `false`.
 *
 * @param expr A `cudaError_t`-producing expression.
 *
 * @par Example
 * @code
 * THEMIS_CUDA_CHECK_BOOL(cudaMalloc(&ptr, bytes));
 * @endcode
 */
#define THEMIS_CUDA_CHECK_BOOL(expr)                                            \
    do {                                                                         \
        cudaError_t _themis_cuda_err_ = (expr);                                 \
        if (_themis_cuda_err_ != cudaSuccess) {                                 \
            THEMIS_ERROR("CUDA error at {}:{} — {}",                            \
                         __FILE__, __LINE__,                                     \
                         cudaGetErrorString(_themis_cuda_err_));                \
            return false;                                                        \
        }                                                                        \
    } while (0)

#endif // THEMIS_ENABLE_CUDA
