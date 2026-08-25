/**
 * @file gpu_compression.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB GPU-Accelerated Compression
 *
 * Provides GPU-accelerated compression/decompression using CUDA (NVIDIA nvCOMP)
 * and HIP (ROCm) for the following algorithms:
 *   - Zstd  (NVIDIA nvCOMP / CPU fallback)
 *   - Snappy (nvCOMP Snappy variant / CPU fallback)
 *   - LZ4   (nvCOMP parallel decompress / CPU fallback)
 *
 * Expected improvement: 5-10× compression throughput vs. CPU-only path.
 * Automatic CPU fallback when no GPU is available or when data is too small
 * to amortise device-transfer overhead.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// ============================================================================
// THEMIS_CUDA_CHECK — fail-closed CUDA error checking macro
// ============================================================================
//
// Wraps any CUDA API call and returns ErrorCode::CudaError (logging the error)
// when the call does not return cudaSuccess.  This satisfies the
// unchecked_cuda_call gap class: every cuda*() call that affects correctness
// or resource ownership MUST be wrapped with this macro rather than ignoring
// the return value.
//
// Usage:
//   THEMIS_CUDA_CHECK(cudaMalloc(&ptr, size));
//   THEMIS_CUDA_CHECK(cudaMemcpyAsync(dst, src, n, kind, stream));
//
// The macro is a no-op when THEMIS_ENABLE_CUDA is not defined.
// ============================================================================
#ifdef THEMIS_ENABLE_CUDA
#  include <cuda_runtime.h>
// Requires spdlog or THEMIS_LOG_ERROR to be available at the call site.
// Include utils/logger.h before this header if THEMIS_LOG_ERROR is preferred.
#  ifndef THEMIS_CUDA_CHECK
#    define THEMIS_CUDA_CHECK(call)                                            \
       do {                                                                    \
           cudaError_t _themis_cuda_err = (call);                             \
           if (_themis_cuda_err != cudaSuccess) {                             \
               THEMIS_ERROR("CUDA error at {}:{}: {}",                        \
                            __FILE__, __LINE__,                               \
                            cudaGetErrorString(_themis_cuda_err));            \
               return ErrorCode::CudaError;                                   \
           }                                                                  \
       } while (0)
#  endif // THEMIS_CUDA_CHECK
#else
#  define THEMIS_CUDA_CHECK(call) static_cast<void>(0)
#endif // THEMIS_ENABLE_CUDA

namespace themis {
namespace storage {

// ============================================================================
// GPU Acceleration Type
// ============================================================================

/**
 * @brief Select which hardware backend drives compression.
 */
enum class GpuAccelerationType {
    CPU_ONLY,   ///< No GPU; always use the CPU implementation
    CUDA,       ///< NVIDIA CUDA + nvCOMP library
    HIP,        ///< AMD HIP/ROCm backend
    AUTO        ///< Auto-detect the best available backend at runtime
};

// ============================================================================
// Supported GPU-accelerated algorithms
// ============================================================================

/**
 * @brief Compression algorithm to use.
 *
 * All three algorithms have a GPU-native path (nvCOMP / ROCm) and an
 * equivalent CPU fallback so that code always compiles and works without
 * any GPU hardware present.
 */
enum class GpuCompressionAlgorithm {
    ZSTD,   ///< Zstandard via NVIDIA nvCOMP (CPU: zstd_codec)
    SNAPPY, ///< Snappy via nvCOMP Snappy variant (CPU: snappy library)
    LZ4     ///< LZ4 with parallel GPU decompression (CPU: lz4 library)
};

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Configuration for GpuCompressionManager.
 */
struct GpuCompressionConfig {
    GpuAccelerationType accel_type = GpuAccelerationType::AUTO;

    int  device_id           = 0;       ///< CUDA/HIP device index

    /// When true (default), a GPU failure transparently falls back to the
    /// CPU implementation.  When false, GPU failures surface as a failed
    /// GpuCompressionResult / empty decompression result instead of silently
    /// routing to CPU.
    bool fallback_cpu        = true;

    /// Reserved for future implementation: use non-blocking GPU streams and
    /// expose a wait/poll interface.  Currently all GPU calls are synchronous.
    bool async_compute       = true;

    /// Minimum uncompressed bytes before GPU path is attempted.
    /// Below this threshold the CPU path is always used to avoid
    /// PCIe / HBM transfer overhead dominating latency.
    /// Must be > 0; a value of 0 is treated as if the threshold were 1.
    size_t min_size_for_gpu  = 256 * 1024;   // 256 KB

    /// nvCOMP / ROCm chunk size for per-buffer compression (bytes).
    /// Must be > 0.
    size_t chunk_size        = 64 * 1024;    // 64 KB

    int zstd_level           = 3;       ///< Zstd compression level (1-22)

    /// Reserved for future implementation: cap GPU memory usage.
    /// Currently has no effect; 0 means unlimited.
    size_t max_gpu_memory_mb = 2048;
};

// ============================================================================
// Result type
// ============================================================================

/**
 * @brief Result of a single compress or decompress call.
 */
struct GpuCompressionResult {
    std::vector<uint8_t> data;
    GpuCompressionAlgorithm algorithm;
    bool used_gpu          = false; ///< true if GPU path was taken
    size_t original_size   = 0;
    float  compression_ratio = 1.0f;
    bool   success         = false;
    std::string error_message;      ///< Non-empty on failure

    GpuCompressionResult() : algorithm(GpuCompressionAlgorithm::LZ4) {}
};

// ============================================================================
// Forward declaration for platform-specific implementation
// ============================================================================

class GpuCompressionImpl;

// ============================================================================
// GpuCompressionManager
// ============================================================================

/**
 * @brief GPU-accelerated compression manager for Zstd, Snappy, and LZ4.
 *
 * ### Usage
 * ```cpp
 * #include "storage/gpu_compression.h"
 *
 * GpuCompressionConfig cfg;
 * cfg.accel_type = GpuAccelerationType::AUTO;
 * GpuCompressionManager mgr(cfg);
 *
 * // Compress with LZ4 GPU path (auto-falls-back to CPU when needed)
 * auto result = mgr.compress(data, GpuCompressionAlgorithm::LZ4);
 * auto original = mgr.decompress(result.data, GpuCompressionAlgorithm::LZ4);
 * ```
 *
 * ### Thread-safety
 * This class is **NOT thread-safe**.  Concurrent calls to `compress()` or
 * `decompress()` from multiple threads require external synchronization
 * (e.g., a `std::mutex`).  Construction and destruction must also be
 * externally serialized.  `set_config()` must only be called before any
 * concurrent use begins.
 */
class GpuCompressionManager {
public:
    explicit GpuCompressionManager(
        const GpuCompressionConfig& config = GpuCompressionConfig{}
    );

    ~GpuCompressionManager();

    // Non-copyable, non-movable (due to mutable std::mutex)
    GpuCompressionManager(const GpuCompressionManager&)            = delete;
    GpuCompressionManager& operator=(const GpuCompressionManager&) = delete;
    GpuCompressionManager(GpuCompressionManager&&) noexcept = delete;
    GpuCompressionManager& operator=(GpuCompressionManager&&) noexcept = delete;

    // -------------------------------------------------------------------------
    // Compress / Decompress
    // -------------------------------------------------------------------------

    /**
     * @brief Compress @p data using the specified @p algorithm.
     *
     * Automatically routes to the GPU path when:
     *   - GPU was initialised successfully, AND
     *   - `data.size() >= config.min_size_for_gpu`, AND
     *   - `!force_cpu_` flag is set.
     *
     * Falls back to the CPU implementation otherwise.
     */
    GpuCompressionResult compress(
        const std::vector<uint8_t>& data,
        GpuCompressionAlgorithm algorithm
    );

    /** @brief Convenience overload accepting a raw pointer. */
    GpuCompressionResult compress(
        const uint8_t* data,
        size_t size,
        GpuCompressionAlgorithm algorithm
    );

    /**
     * @brief Decompress @p compressed_data that was produced by compress().
     *
     * Uses the same GPU/CPU routing logic as compress().
     *
     * @return Decompressed bytes, or empty vector on failure.
     */
    std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& compressed_data,
        GpuCompressionAlgorithm algorithm,
        size_t original_size = 0
    );

    // -------------------------------------------------------------------------
    // Batch operations
    // -------------------------------------------------------------------------

    /**
     * @brief Compress multiple independent buffers in a single GPU dispatch.
     *
     * When a GPU backend is available all buffers are transferred to the device
     * and compressed in a single nvCOMP batched call (one kernel launch), then
     * the results are copied back in one pass.  This amortises host-device
     * transfer overhead and is the primary mechanism through which 5-10×
     * throughput is achieved versus sequential per-buffer GPU calls.
     *
     * When no GPU is available the implementation falls back to sequential
     * CPU compression (one buffer at a time).
     *
     * @param buffers    Input buffers (may have different sizes).
     * @param algorithm  Algorithm to use for all buffers.
     * @return One GpuCompressionResult per input buffer, in the same order.
     */
    std::vector<GpuCompressionResult> compress_batch(
        const std::vector<std::vector<uint8_t>>& buffers,
        GpuCompressionAlgorithm algorithm
    );

    /**
     * @brief Decompress multiple buffers in a single GPU dispatch.
     */
    std::vector<std::vector<uint8_t>> decompress_batch(
        const std::vector<std::vector<uint8_t>>& compressed_buffers,
        GpuCompressionAlgorithm algorithm,
        const std::vector<size_t>& original_sizes = {}
    );

    // -------------------------------------------------------------------------
    // Runtime introspection
    // -------------------------------------------------------------------------

    /** @brief Returns true if a GPU backend is initialised and operational. */
    bool is_gpu_available() const;

    /** @brief Returns the actually active acceleration type. */
    GpuAccelerationType active_accel_type() const;

    /** @brief Force CPU-only mode (useful for testing). */
    void force_cpu_fallback(bool enable);

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    const GpuCompressionConfig& get_config() const {
        std::lock_guard<std::mutex> lk(mu_);
        return config_;
    }
    void set_config(const GpuCompressionConfig& cfg);

    // -------------------------------------------------------------------------
    // Performance statistics
    // -------------------------------------------------------------------------

    /**
     * @brief Cumulative performance counters.
     */
    struct Stats {
        uint64_t total_compress_ops   = 0;
        uint64_t gpu_compress_ops     = 0;
        uint64_t total_decompress_ops = 0;
        uint64_t gpu_decompress_ops   = 0;
        uint64_t cpu_fallbacks        = 0;
        uint64_t bytes_in             = 0; ///< Total uncompressed bytes processed
        uint64_t bytes_out            = 0; ///< Total compressed bytes produced
        double   avg_gpu_compress_ms  = 0.0;
        double   avg_cpu_compress_ms  = 0.0;
        double   avg_gpu_decompress_ms = 0.0;
        double   avg_cpu_decompress_ms = 0.0;
    };

    Stats get_stats() const { std::lock_guard<std::mutex> lk(mu_); return stats_; }
    void  reset_stats();

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static std::string algorithm_to_string(GpuCompressionAlgorithm algorithm);
    static std::string accel_type_to_string(GpuAccelerationType type);

private:
    // -------------------------------------------------------------------------
    // GPU backend
    // -------------------------------------------------------------------------
    bool init_gpu();
    bool should_use_gpu(size_t data_size) const;

    // -------------------------------------------------------------------------
    // CPU fallback implementations
    // -------------------------------------------------------------------------
    GpuCompressionResult cpu_compress_zstd(const uint8_t* data, size_t size);
    GpuCompressionResult cpu_compress_snappy(const uint8_t* data, size_t size);
    GpuCompressionResult cpu_compress_lz4(const uint8_t* data, size_t size);

    std::vector<uint8_t> cpu_decompress_zstd(
        const std::vector<uint8_t>& data, size_t original_size);
    std::vector<uint8_t> cpu_decompress_snappy(
        const std::vector<uint8_t>& data, size_t original_size);
    std::vector<uint8_t> cpu_decompress_lz4(
        const std::vector<uint8_t>& data, size_t original_size);

    /// CPU-side decoder for data produced by the GPU (nvCOMP) path.
    /// Parses the GPU container format and decompresses each chunk with
    /// the corresponding native CPU library.
    std::vector<uint8_t> cpu_decompress_gpu_container(
        const std::vector<uint8_t>& compressed,
        GpuCompressionAlgorithm algorithm);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------
    mutable std::mutex               mu_;          ///< Protects config_ and stats_ from concurrent access.
    GpuCompressionConfig             config_;
    GpuAccelerationType              active_accel_ = GpuAccelerationType::CPU_ONLY;
    bool                             force_cpu_    = false;
    std::unique_ptr<GpuCompressionImpl> impl_;
    mutable Stats                    stats_;
};

// ============================================================================
// Factory helper
// ============================================================================

/**
 * @brief Create a GpuCompressionManager with the best available backend.
 *
 * Equivalent to constructing with `GpuAccelerationType::AUTO`.
 */
std::unique_ptr<GpuCompressionManager> create_gpu_compression_manager(
    const GpuCompressionConfig& config = GpuCompressionConfig{}
);

} // namespace storage
} // namespace themis
