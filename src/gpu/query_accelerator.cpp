/**
 * @file query_accelerator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=9; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=6, Debt=0, C=5, H=17, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/query_accelerator.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <chrono>

// Phase 1 GPU Infrastructure: Error handling, RAII memory, timeout enforcement
#include "themis/gpu/gpu_error.h"
#include "themis/gpu/gpu_memory.h"
#include "themis/gpu/gpu_timeout.h"
#include "themis/gpu/gpu_cuda_error_hardening.h"
#include "themis/gpu/gpu_backend_dispatch_diagnostics.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
// Thrust (CUDA) — sort, reduce, copy
#include <thrust/binary_search.h>
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/functional.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
// cuBLAS — dot product / matrix-vector (FP32, FP16, BF16)
#include <cublas_v2.h>
#include <cuda_bf16.h>
#include <cuda_fp16.h>
// RAII wrappers — exception-safe CUDA resource management
#include "acceleration/raii/cuda_raii.h"
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
// ROCm Thrust (same API as CUDA Thrust)
#include <thrust/binary_search.h>
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/functional.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
// hipBLAS — dot product / GEMM (FP32, FP16)
#include <hip/hip_fp16.h>
#include <hipblas/hipblas.h>
// RAII wrappers — exception-safe HIP resource management
#include "acceleration/raii/hip_raii.h"
#endif

#ifdef THEMIS_ENABLE_CUVS
#include <cuvs/distance/distance_types.hpp>
#include <cuvs/neighbors/ivf_flat.hpp>
#include <raft/core/copy.hpp>
#include <raft/core/device_mdarray.hpp>
#include <raft/core/device_resources.hpp>
#endif

namespace themis {
namespace gpu {

namespace {

constexpr auto kGpuDispatchTimeout = std::chrono::seconds(5);

} // namespace

#if defined(THEMIS_ENABLE_CUDA)
#define THEMIS_GPU_QUERY_ACCEL_SYNC() CHECKED_CUDA(cudaDeviceSynchronize())
#elif defined(THEMIS_ENABLE_HIP)
#define THEMIS_GPU_QUERY_ACCEL_SYNC() CHECKED_HIP(hipDeviceSynchronize())
#endif

// ---------------------------------------------------------------------------
// FP16 / BF16 quantisation helpers (CPU simulation of Tensor Core precision)
// ---------------------------------------------------------------------------

/// Encode a float32 to IEEE 754 FP16 bits (uint16_t).
static uint16_t fp32_to_fp16(float f) noexcept {
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    const uint32_t sign   = (bits >> 31) & 0x1u;
    const int32_t exp32   = static_cast<int32_t>((bits >> 23) & 0xFFu) - 127;
    const uint32_t mant32 = bits & 0x7FFFFFu;

    // Special cases
    if (exp32 == 128) {
        // Inf or NaN
        return static_cast<uint16_t>((sign << 15) | 0x7C00u | (mant32 ? 0x0200u : 0u)); // preserve NaN signal
    }
    if (exp32 < -24) {
        // Too small: flush to ±0
        return static_cast<uint16_t>(sign << 15);
    }
    if (exp32 < -14) {
        // Subnormal FP16
        uint32_t shift  = static_cast<uint32_t>(-14 - exp32);
        uint32_t mant16 = (mant32 | 0x800000u) >> (shift + 13);
        return static_cast<uint16_t>((sign << 15) | mant16);
    }
    if (exp32 > 15) {
        // Overflow: return ±Inf
        return static_cast<uint16_t>((sign << 15) | 0x7C00u);
    }
    // Normalised
    uint32_t exp16  = static_cast<uint32_t>(exp32 + 15);
    uint32_t mant16 = mant32 >> 13;
    // Round to nearest even
    uint32_t round = mant32 & 0x1FFFu;
    if (round > 0x1000u || (round == 0x1000u && (mant16 & 1u))) {
        ++mant16;
    }
    if (mant16 >= 0x400u) {
        ++exp16;
        mant16 = 0;
    }
    return static_cast<uint16_t>((sign << 15) | (exp16 << 10) | (mant16 & 0x3FFu));
}

/// Decode IEEE 754 FP16 bits back to float32.
static float fp16_to_fp32(uint16_t h) noexcept {
    const uint32_t sign   = static_cast<uint32_t>((h >> 15) & 0x1u);
    const uint32_t exp16  = (h >> 10) & 0x1Fu;
    const uint32_t mant16 = h & 0x3FFu;

    uint32_t bits;
    if (exp16 == 0x1F) {
        // Inf or NaN
        bits = (sign << 31) | 0x7F800000u | (mant16 << 13);
    } else if (exp16 == 0) {
        // Zero or subnormal
        if (mant16 == 0) {
            bits = sign << 31;
        } else {
            // Normalise the subnormal
            uint32_t m = mant16;
            int32_t e  = -14;
            while ((m & 0x400u) == 0) {
                m <<= 1;
                --e;
            }
            m &= 0x3FFu;
            bits = (sign << 31) | (static_cast<uint32_t>(e + 127) << 23) | (m << 13);
        }
    } else {
        bits = (sign << 31) | ((exp16 + 112u) << 23) | (mant16 << 13);
    }
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

/// Round-trip a float through FP16 to simulate Tensor Core precision loss.
static float quantise_fp16(float f) noexcept {
    return fp16_to_fp32(fp32_to_fp16(f));
}

/// Encode a float32 to BF16 bits (uint16_t) — top 16 bits of FP32 with RNE.
static uint16_t fp32_to_bf16(float f) noexcept {
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    // Round to nearest even
    bits += 0x7FFFu + ((bits >> 16) & 1u);
    return static_cast<uint16_t>(bits >> 16);
}

/// Decode BF16 bits back to float32 — restore the truncated mantissa bits as 0.
static float bf16_to_fp32(uint16_t b) noexcept {
    uint32_t bits = static_cast<uint32_t>(b) << 16;
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

/// Round-trip a float through BF16 to simulate Tensor Core precision loss.
static float quantise_bf16(float f) noexcept {
    return bf16_to_fp32(fp32_to_bf16(f));
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GPUQueryAccelerator::GPUQueryAccelerator() : GPUQueryAccelerator(Config{}) {}

GPUQueryAccelerator::GPUQueryAccelerator(const Config &config)
    : config_(config), graph_cache_enabled_(config.enable_graph_cache) {}

// ---------------------------------------------------------------------------
// Graph cache control
// ---------------------------------------------------------------------------

void GPUQueryAccelerator::enableGraphCache() {
    graph_cache_enabled_.store(true, std::memory_order_relaxed);
}

void GPUQueryAccelerator::disableGraphCache() {
    graph_cache_enabled_.store(false, std::memory_order_relaxed);
}

GPUGraphCache::Stats GPUQueryAccelerator::getGraphCacheStats() const {
    return graph_cache_.getStats();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool GPUQueryAccelerator::shouldUseGPU(size_t num_rows) const noexcept {
    if (config_.force_cpu) {
        return false;
    }
    return num_rows >= config_.gpu_threshold_rows;
}

void GPUQueryAccelerator::recordOp(size_t rows, uint64_t bytes, bool gpu_used) {
    stats_.rows_processed += rows;
    stats_.bytes_scanned += bytes;
    if (gpu_used) {
        ++stats_.gpu_ops;
    } else {
        ++stats_.cpu_fallback_ops;
    }
}

// static
QueryShape GPUQueryAccelerator::makeShape(QueryShape::OpType op, size_t row_count, uint64_t param_hash) noexcept {
    QueryShape s;
    s.op         = op;
    s.row_count  = row_count;
    s.param_hash = param_hash;
    return s;
}

// ---------------------------------------------------------------------------
// scan
// ---------------------------------------------------------------------------

GPUQueryAccelerator::ScanResult GPUQueryAccelerator::scan(const std::vector<Row> &rows, FilterFn filter) {
    ScanResult result;
    result.rows_scanned = rows.size();

    // Determine path ---------------------------------------------------------
    bool use_gpu    = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;

    // Graph cache check ------------------------------------------------------
    // For recurring scans with the same row count: on a cache hit we replay the
    // captured graph (CPU simulation: execute normally but note the cache hit).
    // On a miss we capture the new shape for future replays.
    if (graph_cache_enabled_) {
        QueryShape shape = makeShape(QueryShape::OpType::SCAN, rows.size());
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // GPU path — parallel select using Thrust (activated when filter is nullptr;
    // host-function predicates cannot execute on-device and fall through to CPU).
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
    if (use_gpu && !filter) {
        bool gpu_done = false;
        try {
            KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
            // Pass-all scan: use thrust::copy_if with an always-true device
            // predicate to exercise a real GPU select primitive.  Host-callable
            // filter predicates fall through to the CPU path below.
            const size_t n = rows.size();

            // Generate row indices [0, 1, ..., n-1] on device.
            thrust::device_vector<uint64_t> d_idx(n);
            thrust::sequence(thrust::device, d_idx.begin(), d_idx.end(), uint64_t{0});

            // Device-side select with always-true predicate (copy_if).
            struct AlwaysTrue {
                __host__ __device__ bool operator()(const uint64_t &) const {
                    return true;
                }
            };

            thrust::device_vector<uint64_t> d_selected(n);
            auto d_end = thrust::copy_if(thrust::device, d_idx.begin(), d_idx.end(), d_selected.begin(), AlwaysTrue{});
            const size_t selected_count = static_cast<size_t>(d_end - d_selected.begin());

            // Copy selected indices back to host.
            std::vector<uint64_t> h_idx(selected_count);
            thrust::copy(thrust::device, d_selected.begin(), d_end, h_idx.begin());

            result.rows.reserve(selected_count);
            for (uint64_t i : h_idx) {
                result.rows.push_back(rows[static_cast<size_t>(i)]);
            }
            THEMIS_GPU_QUERY_ACCEL_SYNC();
            if (!kernel_guard.checkTimeoutDeadline()) {
                result.rows_passed = result.rows.size();
                gpu_done           = true;
            } else {
                result.rows.clear();
            }
        } catch (const std::bad_alloc &ex) {
            // Memory allocation failure during GPU operation
            result.rows.clear();
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("scan GPU path: memory allocation failure: {}", ex.what());
            }
        } catch (const std::runtime_error &ex) {
            // Thrust system_error or runtime error — fall through to CPU
            result.rows.clear();
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->debug("scan GPU path: runtime error: {}", ex.what());
            }
        } catch (const std::exception &ex) {
            // Other standard exceptions
            result.rows.clear();
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("scan GPU path: unexpected exception: {}", ex.what());
            }
        }
        if (gpu_done) {
            uint64_t bytes = 0;
            for (const auto &r : rows)
                bytes += r.data.size();
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_scans;
            recordOp(rows.size(), bytes, true);
            return result;
        }
        result.used_gpu = false;
    }
#endif

    // CPU sequential scan ----------------------------------------------------
    for (const auto &row : rows) {
        if (!filter || filter(row)) {
            result.rows.push_back(row);
        }
    }
    result.rows_passed = result.rows.size();

    // Stats ------------------------------------------------------------------
    uint64_t bytes = 0;
    for (const auto &r : rows) {
        bytes += r.data.size();
    }
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_scans;
    recordOp(rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// sort
// ---------------------------------------------------------------------------

GPUQueryAccelerator::SortResult GPUQueryAccelerator::sort(std::vector<Row> rows, KeyFn key_fn, SortOrder order) {
    SortResult result;
    bool use_gpu    = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;

    // Graph cache check — include sort order in the param hash ---------------
    if (graph_cache_enabled_) {
        uint64_t param   = static_cast<uint64_t>(order);
        QueryShape shape = makeShape(QueryShape::OpType::SORT, rows.size(), param);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // GPU path — Thrust stable_sort_by_key (CUDA) / ROCm Thrust (HIP).
    //
    // Key extractor is a host function, so keys are extracted on the host then
    // uploaded.  Indices are sorted on-device; rows are gathered back on host.
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
    if (use_gpu) {
        bool gpu_done = false;
        try {
            KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
            const size_t n = rows.size();

            // 1. Extract numeric keys and row indices on host.
            // Use uint64_t for indices to support datasets larger than 2^32 rows.
            std::vector<double> h_keys(n);
            std::vector<uint64_t> h_idx(n);
            for (size_t i = 0; i < n; ++i) {
                h_keys[i] = key_fn(rows[i]);
                h_idx[i]  = static_cast<uint64_t>(i);
            }

            // 2. Upload to device.
            thrust::device_vector<double> d_keys(h_keys.begin(), h_keys.end());
            thrust::device_vector<uint64_t> d_idx(h_idx.begin(), h_idx.end());

            // 3. Stable sort indices by key (ascending or descending).
            if (order == SortOrder::ASC) {
                thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_idx.begin());
            } else {
                thrust::stable_sort_by_key(d_keys.begin(), d_keys.end(), d_idx.begin(), thrust::greater<double>());
            }

            // 4. Copy sorted indices back to host.
            std::vector<uint64_t> sorted_idx(n);
            thrust::copy(d_idx.begin(), d_idx.end(), sorted_idx.begin());
            THEMIS_GPU_QUERY_ACCEL_SYNC();

            // 5. Gather rows into sorted order.
            std::vector<Row> sorted_rows(n);
            for (size_t i = 0; i < n; ++i) {
                sorted_rows[i] = std::move(rows[static_cast<size_t>(sorted_idx[i])]);
            }
            if (!kernel_guard.checkTimeoutDeadline()) {
                rows     = std::move(sorted_rows);
                gpu_done = true;
            }
        } catch (const std::bad_alloc &ex) {
            // Memory allocation failure during GPU operation
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("sort GPU path: memory allocation failure: {}", ex.what());
            }
        } catch (const std::runtime_error &ex) {
            // Thrust system_error or runtime error — fall through to CPU
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->debug("sort GPU path: runtime error: {}", ex.what());
            }
        } catch (const std::exception &ex) {
            // Other standard exceptions
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("sort GPU path: unexpected exception: {}", ex.what());
            }
        }
        if (gpu_done) {
            result.rows    = std::move(rows);
            uint64_t bytes = 0;
            for (const auto &r : result.rows)
                bytes += r.data.size();
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_sorts;
            recordOp(result.rows.size(), bytes, true);
            return result;
        }
        result.used_gpu = false;
    }
#endif

    // CPU stable-sort path:
    std::stable_sort(rows.begin(), rows.end(), [&](const Row &a, const Row &b) {
        double ka = key_fn(a);
        double kb = key_fn(b);
        return (order == SortOrder::ASC) ? ka < kb : ka > kb;
    });
    result.rows = std::move(rows);

    uint64_t bytes = 0;
    for (const auto &r : result.rows) {
        bytes += r.data.size();
    }
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_sorts;
    recordOp(result.rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// aggregate
// ---------------------------------------------------------------------------

GPUQueryAccelerator::AggResult GPUQueryAccelerator::aggregate(const std::vector<Row> &rows, AggFunc func,
                                                              KeyFn value_fn) {
    AggResult result;
    if (rows.empty()) {
        return result;
    }

    bool use_gpu    = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;
    result.count    = rows.size();

    // Graph cache check — include AggFunc in the param hash ------------------
    if (graph_cache_enabled_) {
        uint64_t param   = static_cast<uint64_t>(func);
        QueryShape shape = makeShape(QueryShape::OpType::AGGREGATE, rows.size(), param);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // GPU path — Thrust device reduce for SUM, MIN, MAX (CUDA/HIP).
    //
    // Value extractor is a host function; values are extracted on host then
    // uploaded to device.  COUNT is computed locally without a GPU kernel.
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
    if (use_gpu && func != AggFunc::COUNT) {
        bool gpu_done = false;
        try {
            KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
            const size_t n = rows.size();

            // 1. Extract values on host.
            std::vector<double> h_values(n);
            for (size_t i = 0; i < n; ++i)
                h_values[i] = value_fn(rows[i]);

            // 2. Upload to device.
            thrust::device_vector<double> d_values(h_values.begin(), h_values.end());

            // 3. Reduce on device.
            double gpu_result = 0.0;
            switch (func) {
                case AggFunc::SUM:
                    gpu_result = thrust::reduce(d_values.begin(), d_values.end(), 0.0, thrust::plus<double>());
                    break;
                case AggFunc::MIN:
                    gpu_result = thrust::reduce(d_values.begin(), d_values.end(), std::numeric_limits<double>::max(),
                                                thrust::minimum<double>());
                    break;
                case AggFunc::MAX:
                    gpu_result = thrust::reduce(d_values.begin(), d_values.end(), std::numeric_limits<double>::lowest(),
                                                thrust::maximum<double>());
                    break;
                case AggFunc::AVG: {
                    double s   = thrust::reduce(d_values.begin(), d_values.end(), 0.0, thrust::plus<double>());
                    gpu_result = s / static_cast<double>(n);
                    break;
                }
                default:
                    break;
            }
            THEMIS_GPU_QUERY_ACCEL_SYNC();
            if (!kernel_guard.checkTimeoutDeadline()) {
                result.value = gpu_result;
                gpu_done     = true;
            }
        } catch (const std::bad_alloc &ex) {
            // Memory allocation failure during GPU operation
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("aggregate GPU path: memory allocation failure: {}", ex.what());
            }
        } catch (const std::runtime_error &ex) {
            // Thrust system_error or runtime error — fall through to CPU
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->debug("aggregate GPU path: runtime error: {}", ex.what());
            }
        } catch (const std::exception &ex) {
            // Other standard exceptions
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("aggregate GPU path: unexpected exception: {}", ex.what());
            }
        }
        if (gpu_done) {
            uint64_t bytes = 0;
            for (const auto &r : rows)
                bytes += r.data.size();
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_aggregates;
            recordOp(rows.size(), bytes, true);
            return result;
        }
        result.used_gpu = false;
    }
#endif

    // CPU sequential path:
    double sum = 0.0;
    double mn  = std::numeric_limits<double>::max();
    double mx  = std::numeric_limits<double>::lowest();

    for (const auto &row : rows) {
        double v = value_fn(row);
        sum += v;
        if (v < mn)
            mn = v;
        if (v > mx)
            mx = v;
    }

    switch (func) {
        case AggFunc::SUM:
            result.value = sum;
            break;
        case AggFunc::COUNT:
            result.value = static_cast<double>(rows.size());
            break;
        case AggFunc::MIN:
            result.value = mn;
            break;
        case AggFunc::MAX:
            result.value = mx;
            break;
        case AggFunc::AVG:
            result.value = sum / static_cast<double>(rows.size());
            break;
    }

    uint64_t bytes = 0;
    for (const auto &r : rows) {
        bytes += r.data.size();
    }
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_aggregates;
    recordOp(rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// hashJoin
// ---------------------------------------------------------------------------

GPUQueryAccelerator::JoinResult GPUQueryAccelerator::hashJoin(const std::vector<Row> &left,
                                                              const std::vector<Row> &right, JoinKeyFn left_key,
                                                              JoinKeyFn right_key) {
    JoinResult result;
    if (left.empty() || right.empty()) {
        return result;
    }

    bool use_gpu    = shouldUseGPU(left.size() + right.size());
    result.used_gpu = use_gpu;

    // Graph cache check — key on total row count -----------------------------
    if (graph_cache_enabled_) {
        size_t total     = left.size() + right.size();
        QueryShape shape = makeShape(QueryShape::OpType::JOIN, total);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // Select build side (smaller) and probe side (larger) for optimal
    // memory usage on both GPU and CPU paths.
    const std::vector<Row> *build_side = &left;
    const std::vector<Row> *probe_side = &right;
    JoinKeyFn build_key                = left_key;
    JoinKeyFn probe_key                = right_key;
    bool swapped                       = false;

    if (right.size() < left.size()) {
        std::swap(build_side, probe_side);
        std::swap(build_key, probe_key);
        swapped = true;
    }

    // GPU path — two-phase sort-based join using Thrust (CUDA/HIP).
    //
    // Phase 1 (Build): copy build-side keys to device, sort with row indices
    //   using thrust::stable_sort_by_key — this is where the GPU is used.
    // Phase 2 (Probe): download sorted keys+indices to host; for each probe
    //   key run std::lower/upper_bound over the host-side sorted array.
    //   The probe is sequential since JoinKeyFn is a host functor; the GPU
    //   work is the sort in Phase 1.
    //
    // Sort-based joins deliver predictable O(n log n) device-side work without
    // requiring custom hash-table kernels, and leverage Thrust's tuned sort
    // for both CUDA and ROCm/HIP backends.
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
    if (use_gpu) {
        bool gpu_done = false;
        try {
            KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
            // Extract keys on host (JoinKeyFn is a host functor).
            const size_t bn = build_side->size();
            const size_t pn = probe_side->size();

            // Use uint64_t for indices to support tables larger than 2^32 rows.
            std::vector<uint64_t> h_build_keys(bn);
            std::vector<uint64_t> h_build_idx(bn);
            for (size_t i = 0; i < bn; ++i) {
                h_build_keys[i] = build_key((*build_side)[i]);
                h_build_idx[i]  = static_cast<uint64_t>(i);
            }

            // Upload build-side keys and sort on device.
            thrust::device_vector<uint64_t> d_bkeys(h_build_keys.begin(), h_build_keys.end());
            thrust::device_vector<uint64_t> d_bidx(h_build_idx.begin(), h_build_idx.end());
            thrust::stable_sort_by_key(d_bkeys.begin(), d_bkeys.end(), d_bidx.begin());

            // Download sorted build keys and indices for probe phase.
            std::vector<uint64_t> sorted_bkeys(bn);
            std::vector<uint64_t> sorted_bidx(bn);
            thrust::copy(d_bkeys.begin(), d_bkeys.end(), sorted_bkeys.begin());
            thrust::copy(d_bidx.begin(), d_bidx.end(), sorted_bidx.begin());
            THEMIS_GPU_QUERY_ACCEL_SYNC();

            // Probe phase: for each probe key binary-search the host-side
            // sorted build keys (GPU sort already paid for O(n log n) work).
            for (size_t pi = 0; pi < pn; ++pi) {
                uint64_t pk = probe_key((*probe_side)[pi]);
                auto lo     = std::lower_bound(sorted_bkeys.begin(), sorted_bkeys.end(), pk);
                auto hi     = std::upper_bound(sorted_bkeys.begin(), sorted_bkeys.end(), pk);
                for (auto it = lo; it != hi; ++it) {
                    size_t bi = static_cast<size_t>(
                        sorted_bidx[static_cast<size_t>(std::distance(sorted_bkeys.begin(), it))]);
                    if (!swapped) {
                        result.pairs.emplace_back((*build_side)[bi], (*probe_side)[pi]);
                    } else {
                        result.pairs.emplace_back((*probe_side)[pi], (*build_side)[bi]);
                    }
                }
            }
            gpu_done = !kernel_guard.checkTimeoutDeadline();
        } catch (const std::bad_alloc &ex) {
            // Memory allocation failure during GPU operation
            result.pairs.clear();
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("join GPU path: memory allocation failure: {}", ex.what());
            }
        } catch (const std::runtime_error &ex) {
            // Thrust system_error or runtime error — fall through to CPU
            result.pairs.clear();
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->debug("join GPU path: runtime error: {}", ex.what());
            }
        } catch (const std::exception &ex) {
            // Other standard exceptions
            result.pairs.clear();
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("join GPU path: unexpected exception: {}", ex.what());
            }
        }
        }
        if (gpu_done) {
            uint64_t bytes = 0;
            for (const auto &r : left)
                bytes += r.data.size();
            for (const auto &r : right)
                bytes += r.data.size();
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_joins;
            recordOp(left.size() + right.size(), bytes, true);
            return result;
        }
        result.used_gpu = false;
    }
#endif

    // CPU path uses an unordered_multimap on the smaller side (build_side /
    // probe_side already selected above):
    std::unordered_multimap<uint64_t, const Row *> ht;
    ht.reserve(build_side->size());
    for (const auto &row : *build_side) {
        ht.emplace(build_key(row), &row);
    }

    for (const auto &row : *probe_side) {
        uint64_t k      = probe_key(row);
        auto [beg, end] = ht.equal_range(k);
        for (auto it = beg; it != end; ++it) {
            if (!swapped) {
                result.pairs.emplace_back(*it->second, row);
            } else {
                result.pairs.emplace_back(row, *it->second);
            }
        }
    }

    uint64_t bytes = 0;
    for (const auto &r : left) {
        bytes += r.data.size();
    }
    for (const auto &r : right) {
        bytes += r.data.size();
    }
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_joins;
    recordOp(left.size() + right.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// dotProduct
// ---------------------------------------------------------------------------

GPUQueryAccelerator::DotProductResult GPUQueryAccelerator::dotProduct(const std::vector<float> &a,
                                                                      const std::vector<float> &b) {
    DotProductResult result;
    result.precision_used = config_.precision_mode;

    if (a.empty() || a.size() != b.size()) {
        std::lock_guard<std::mutex> lk(mutex_);
        ++stats_.total_dot_products;
        recordOp(0, 0, false);
        return result;
    }

    bool use_gpu    = shouldUseGPU(a.size());
    result.used_gpu = use_gpu;

    // GPU path — cuBLAS (CUDA) / hipBLAS (HIP) dispatch.
    //
    // FP32: cublasSdot / hipblasSdot — single-precision dot product.
    // FP16: cublasGemmEx / hipblasGemmEx with CUDA_R_16F / HIPBLAS_R_16F inputs
    //       and CUDA_R_32F / HIPBLAS_R_32F output, treating vectors as 1×n × n×1
    //       matrices.  FP32 output avoids __half overflow/saturation on long vectors.
    // BF16: cublasGemmEx with CUDA_R_16BF inputs + CUDA_R_32F output (CUDA only;
    //       HIP BF16 support varies by ROCm version and falls to CPU).
    //
    // The cuBLAS handle is created per-call for simplicity.  In a production
    // deployment the handle should be owned by GpuModule and reused across
    // calls to avoid the ~10 µs initialisation overhead.
#ifdef THEMIS_ENABLE_CUDA
    if (use_gpu) {
        bool gpu_done = false;
        // CublasHandle RAII — destroyed automatically on scope exit even if an
        // exception is thrown, preventing cublasHandle_t leaks.
        themis::acceleration::raii::CublasHandle blas;
        try {
            if (blas.create()) {
                const int n = static_cast<int>(a.size());

                if (config_.precision_mode == PrecisionMode::FP32) {
                    // --- FP32: cublasSdot with RAII GPU memory ---
                    // Use unique_gpu_ptr for automatic cleanup on scope exit
                    // CHECKED_CUDA wraps all CUDA calls for error handling
                    try {
                        auto d_a = themis::gpu::make_unique_gpu<float>(n);
                        auto d_b = themis::gpu::make_unique_gpu<float>(n);
                        
                        // Copy vectors to device with error checking
                        CHECKED_CUDA(cudaMemcpy(d_a.get(), a.data(), n * sizeof(float), cudaMemcpyHostToDevice));
                        CHECKED_CUDA(cudaMemcpy(d_b.get(), b.data(), n * sizeof(float), cudaMemcpyHostToDevice));
                        
                        // Enforce SLA: kernel must complete within 5 seconds
                        KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
                        
                        float dot_result = 0.0f;
                        if (cublasSdot(blas.get(), n, d_a.get(), 1, d_b.get(), 1, &dot_result)
                            == CUBLAS_STATUS_SUCCESS) {
                            THEMIS_GPU_QUERY_ACCEL_SYNC();
                            // Check SLA deadline
                            if (!kernel_guard.checkTimeoutDeadline()) {
                                result.value = static_cast<double>(dot_result);
                                gpu_done     = true;
                            }
                        }
                        // d_a, d_b automatically freed on scope exit via unique_gpu_ptr destructor
                    } catch (const std::exception&) {
                        // Allocation failure or CUDA error → fall through to CPU
                        gpu_done = false;
                    }
                } else if (config_.precision_mode == PrecisionMode::FP16) {
                    // --- FP16: quantise on host, cublasGemmEx (1×n × n×1) with RAII ---
                    // Use CUDA_R_16F inputs with CUDA_R_32F output + FP32 compute
                    // to avoid overflow/saturation that a __half output would cause.
                    std::vector<__half> ha(n), hb(n);
                    for (int i = 0; i < n; ++i) {
                        ha[i] = __float2half(a[i]);
                        hb[i] = __float2half(b[i]);
                    }
                    try {
                        // Allocate GPU memory with unique_gpu_ptr
                        auto d_a = themis::gpu::make_unique_gpu<__half>(n);
                        auto d_b = themis::gpu::make_unique_gpu<__half>(n);
                        auto d_c = themis::gpu::make_unique_gpu<float>(1);
                        
                        // Copy quantized vectors to device with error checking
                        CHECKED_CUDA(cudaMemcpy(d_a.get(), ha.data(), n * sizeof(__half), cudaMemcpyHostToDevice));
                        CHECKED_CUDA(cudaMemcpy(d_b.get(), hb.data(), n * sizeof(__half), cudaMemcpyHostToDevice));
                        
                        // Enforce SLA: kernel must complete within 5 seconds
                        KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
                        
                        const float alpha = 1.0f, beta = 0.0f;
                        // Compute C (1×1) = A (1×n) * B (n×1) with FP32 accumulation
                        if (cublasGemmEx(blas.get(), CUBLAS_OP_N, CUBLAS_OP_N, 1, 1, n, &alpha,
                                         d_b.get(), CUDA_R_16F,
                                         1,          // B: n×1 column-major
                                         d_a.get(), CUDA_R_16F, n, // A: 1×n as n×1 transposed
                                         &beta, d_c.get(), CUDA_R_32F, 1, CUBLAS_COMPUTE_32F,
                                         CUBLAS_GEMM_DEFAULT)
                            == CUBLAS_STATUS_SUCCESS) {
                            float c_host = 0.0f;
                            CHECKED_CUDA(cudaMemcpy(&c_host, d_c.get(), sizeof(float), cudaMemcpyDeviceToHost));
                            THEMIS_GPU_QUERY_ACCEL_SYNC();
                            // Check SLA deadline
                            if (!kernel_guard.checkTimeoutDeadline()) {
                                result.value = static_cast<double>(c_host);
                                gpu_done     = true;
                            }
                        }
                        // d_a, d_b, d_c automatically freed on scope exit via unique_gpu_ptr destructor
                    } catch (const std::exception&) {
                        // Allocation failure or CUDA error → fall through to CPU
                        gpu_done = false;
                    }

                } else {
                    // --- BF16: quantise on host, cublasGemmEx with BF16 types ---
                    std::vector<__nv_bfloat16> ba(n), bb(n);
                    for (int i = 0; i < n; ++i) {
                        ba[i] = __float2bfloat16(a[i]);
                        bb[i] = __float2bfloat16(b[i]);
                    }
                    try {
                        // Allocate GPU memory with unique_gpu_ptr
                        auto d_a = themis::gpu::make_unique_gpu<__nv_bfloat16>(n);
                        auto d_b = themis::gpu::make_unique_gpu<__nv_bfloat16>(n);
                        auto d_c = themis::gpu::make_unique_gpu<float>(1);
                        
                        // Copy quantized vectors to device with error checking
                        CHECKED_CUDA(cudaMemcpy(d_a.get(), ba.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice));
                        CHECKED_CUDA(cudaMemcpy(d_b.get(), bb.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice));
                        
                        // Enforce SLA: kernel must complete within 5 seconds
                        KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
                        
                        const float alpha_f = 1.0f, beta_f = 0.0f;
                        if (cublasGemmEx(blas.get(), CUBLAS_OP_N, CUBLAS_OP_N, 1, 1, n, &alpha_f,
                                         d_b.get(), CUDA_R_16BF, 1, d_a.get(), CUDA_R_16BF, n,
                                         &beta_f, d_c.get(), CUDA_R_32F, 1, CUBLAS_COMPUTE_32F,
                                         CUBLAS_GEMM_DEFAULT)
                            == CUBLAS_STATUS_SUCCESS) {
                            float h_c = 0.0f;
                            CHECKED_CUDA(cudaMemcpy(&h_c, d_c.get(), sizeof(float), cudaMemcpyDeviceToHost));
                            THEMIS_GPU_QUERY_ACCEL_SYNC();
                            // Check SLA deadline
                            if (!kernel_guard.checkTimeoutDeadline()) {
                                result.value = static_cast<double>(h_c);
                                gpu_done     = true;
                            }
                        }
                        // d_a, d_b, d_c automatically freed on scope exit via unique_gpu_ptr destructor
                    } catch (const std::exception&) {
                        // Allocation failure or CUDA error → fall through to CPU
                        gpu_done = false;
                    }
                }
            }
        } catch (const std::bad_alloc &ex) {
            // Memory allocation failure during GPU operation
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("dotProduct GPU path (CUDA): memory allocation failure: {}", ex.what());
            }
        } catch (const std::runtime_error &ex) {
            // cuBLAS or runtime error — fall through to CPU
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->debug("dotProduct GPU path (CUDA): runtime error: {}", ex.what());
            }
        } catch (const std::exception &ex) {
            // Other standard exceptions
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("dotProduct GPU path (CUDA): unexpected exception: {}", ex.what());
            }
        }
        // blas (CublasHandle) destroyed here automatically.

        if (gpu_done) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_dot_products;
            if (config_.precision_mode == PrecisionMode::FP16)
                ++stats_.fp16_ops;
            else if (config_.precision_mode == PrecisionMode::BF16)
                ++stats_.bf16_ops;
            recordOp(a.size(), a.size() * sizeof(float) * 2, true);
            return result;
        }
        result.used_gpu = false;
    }
#endif // THEMIS_ENABLE_CUDA

#ifdef THEMIS_ENABLE_HIP
    if (use_gpu) {
        bool gpu_done = false;
        // HipblasHandle RAII — destroyed automatically on scope exit even if an
        // exception is thrown, preventing hipblasHandle_t leaks.
        themis::acceleration::raii::HipblasHandle blas;
        try {
            if (blas.create()) {
                const int n = static_cast<int>(a.size());

                if (config_.precision_mode == PrecisionMode::FP32) {
                    // --- FP32: hipblasSdot with RAII GPU memory ---
                    // Use unique_gpu_ptr for automatic cleanup on scope exit
                    // CHECKED_HIP wraps all HIP calls for error handling
                    try {
                        auto d_a = themis::gpu::make_unique_gpu<float>(n);
                        auto d_b = themis::gpu::make_unique_gpu<float>(n);
                        
                        // Copy vectors to device with error checking
                        CHECKED_HIP(hipMemcpy(d_a.get(), a.data(), n * sizeof(float), hipMemcpyHostToDevice));
                        CHECKED_HIP(hipMemcpy(d_b.get(), b.data(), n * sizeof(float), hipMemcpyHostToDevice));
                        
                        // Enforce SLA: kernel must complete within 5 seconds
                        KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
                        
                        float dot_result = 0.0f;
                        if (hipblasSdot(blas.get(), n, d_a.get(), 1, d_b.get(), 1, &dot_result)
                            == HIPBLAS_STATUS_SUCCESS) {
                            THEMIS_GPU_QUERY_ACCEL_SYNC();
                            // Check SLA deadline
                            if (!kernel_guard.checkTimeoutDeadline()) {
                                result.value = static_cast<double>(dot_result);
                                gpu_done     = true;
                            }
                        }
                        // d_a, d_b automatically freed on scope exit via unique_gpu_ptr destructor
                    } catch (const std::exception&) {
                        // Allocation failure or HIP error → fall through to CPU
                        gpu_done = false;
                    }

                } else if (config_.precision_mode == PrecisionMode::FP16) {
                    // --- FP16: quantise on host, hipblasGemmEx (1×n × n×1) with RAII ---
                    // Use HIPBLAS_R_16F inputs + HIPBLAS_R_32F output to avoid
                    // overflow/saturation from a hipblasHalf output.
                    std::vector<hipblasHalf> ha(n), hb(n);
                    for (int i = 0; i < n; ++i) {
                        ha[i] = __float2half(a[i]);
                        hb[i] = __float2half(b[i]);
                    }
                    try {
                        // Allocate GPU memory with unique_gpu_ptr
                        auto d_a = themis::gpu::make_unique_gpu<hipblasHalf>(n);
                        auto d_b = themis::gpu::make_unique_gpu<hipblasHalf>(n);
                        auto d_c = themis::gpu::make_unique_gpu<float>(1);
                        
                        // Copy quantized vectors to device with error checking
                        CHECKED_HIP(hipMemcpy(d_a.get(), ha.data(), n * sizeof(hipblasHalf), hipMemcpyHostToDevice));
                        CHECKED_HIP(hipMemcpy(d_b.get(), hb.data(), n * sizeof(hipblasHalf), hipMemcpyHostToDevice));
                        
                        // Enforce SLA: kernel must complete within 5 seconds
                        KernelSLAGuard kernel_guard(kGpuDispatchTimeout);
                        
                        const float alpha = 1.0f, beta = 0.0f;
                        if (hipblasGemmEx(blas.get(), HIPBLAS_OP_N, HIPBLAS_OP_N, 1, 1, n, &alpha,
                                          d_b.get(), HIPBLAS_R_16F, 1, d_a.get(), HIPBLAS_R_16F, n,
                                          &beta, d_c.get(), HIPBLAS_R_32F, 1, HIPBLAS_COMPUTE_32F,
                                          HIPBLAS_GEMM_DEFAULT)
                            == HIPBLAS_STATUS_SUCCESS) {
                            float c_host = 0.0f;
                            CHECKED_HIP(hipMemcpy(&c_host, d_c.get(), sizeof(float), hipMemcpyDeviceToHost));
                            THEMIS_GPU_QUERY_ACCEL_SYNC();
                            // Check SLA deadline
                            if (!kernel_guard.checkTimeoutDeadline()) {
                                result.value = static_cast<double>(c_host);
                                gpu_done     = true;
                            }
                        }
                        // d_a, d_b, d_c automatically freed on scope exit via unique_gpu_ptr destructor
                    } catch (const std::exception&) {
                        // Allocation failure or HIP error → fall through to CPU
                        gpu_done = false;
                    }

                } else {
                    // BF16 on ROCm: fall through to CPU path (hipblasGemmEx
                    // BF16 support varies by ROCm version; the CPU BF16
                    // simulation in the fallback path is used instead).
                    gpu_done = false;
                }
            }
        } catch (const std::bad_alloc &ex) {
            // Memory allocation failure during GPU operation
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("dotProduct GPU path (HIP): memory allocation failure: {}", ex.what());
            }
        } catch (const std::runtime_error &ex) {
            // hipBLAS or runtime error — fall through to CPU
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->debug("dotProduct GPU path (HIP): runtime error: {}", ex.what());
            }
        } catch (const std::exception &ex) {
            // Other standard exceptions
            gpu_done = false;
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("dotProduct GPU path (HIP): unexpected exception: {}", ex.what());
            }
        }
        // blas (HipblasHandle) destroyed here automatically.

        if (gpu_done) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_dot_products;
            if (config_.precision_mode == PrecisionMode::FP16)
                ++stats_.fp16_ops;
            else if (config_.precision_mode == PrecisionMode::BF16)
                ++stats_.bf16_ops;
            recordOp(a.size(), a.size() * sizeof(float) * 2, true);
            return result;
        }
        result.used_gpu = false;
    }
#endif // THEMIS_ENABLE_HIP

    // CPU simulation path (also used as fallback when no GPU is present):

    double sum = 0.0;
    switch (config_.precision_mode) {
        case PrecisionMode::FP16:
            for (size_t i = 0; i < a.size(); ++i) {
                sum += static_cast<double>(quantise_fp16(a[i]) * quantise_fp16(b[i]));
            }
            break;
        case PrecisionMode::BF16:
            for (size_t i = 0; i < a.size(); ++i) {
                sum += static_cast<double>(quantise_bf16(a[i]) * quantise_bf16(b[i]));
            }
            break;
        case PrecisionMode::FP32:
        default:
            for (size_t i = 0; i < a.size(); ++i) {
                sum += static_cast<double>(a[i]) * static_cast<double>(b[i]);
            }
            break;
    }
    result.value = sum;

    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_dot_products;
    if (config_.precision_mode == PrecisionMode::FP16) {
        ++stats_.fp16_ops;
    } else if (config_.precision_mode == PrecisionMode::BF16) {
        ++stats_.bf16_ops;
    }
    recordOp(a.size(), a.size() * sizeof(float) * 2, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// annSearch  (GPU-accelerated ANN via cuVS/RAFT; CPU brute-force fallback)
// ---------------------------------------------------------------------------

GPUQueryAccelerator::AnnResult GPUQueryAccelerator::annSearch(const std::vector<float> &queries, size_t numQueries,
                                                              size_t dim, const std::vector<float> &database,
                                                              size_t numVectors, size_t k, bool useL2) {
    AnnResult result;

    // Validate inputs --------------------------------------------------------
    if (dim == 0 || k == 0 || numQueries == 0 || numVectors == 0 || queries.size() != numQueries * dim
        || database.size() != numVectors * dim) {
        std::lock_guard<std::mutex> lk(mutex_);
        ++stats_.total_ann_searches;
        recordOp(0, 0, false);
        return result;
    }

    result.used_gpu = false; // set true only if GPU path executes successfully

    // Graph cache check — key on (numQueries * dim) as row count and pack
    // k + useL2 flag into the parameter hash.  Salt constants ensure that
    // L2 and inner-product shapes with the same k are kept as distinct entries.
    static constexpr uint64_t kAnnL2Salt = 0xABCDEF01ULL; // salt for L2 metric
    static constexpr uint64_t kAnnIPSalt = 0x12345678ULL; // salt for inner-product metric
    if (graph_cache_enabled_) {
        uint64_t param   = static_cast<uint64_t>(k) ^ (useL2 ? kAnnL2Salt : kAnnIPSalt);
        QueryShape shape = makeShape(QueryShape::OpType::ANN_SEARCH, numQueries * dim, param);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

#ifdef THEMIS_ENABLE_CUDA
    const bool use_gpu = shouldUseGPU(numVectors);
    // -------------------------------------------------------------------------
    // GPU path — cuVS/RAFT IVF-Flat ANN search.
    //
    // Activated when THEMIS_ENABLE_CUDA is defined and the database is large
    // enough to exceed the GPU dispatch threshold (shouldUseGPU returns true).
    // Falls through to the CPU brute-force path below on any failure.
    // -------------------------------------------------------------------------
    if (use_gpu) {
        bool gpu_done = false;
#ifdef THEMIS_ENABLE_CUVS
        try {
            raft::device_resources handle;
 
            // 1. Copy database to device
            auto db_dev = raft::make_device_matrix<float>(handle, numVectors, dim);
            CHECKED_CUDA(cudaMemcpy(db_dev.data_handle(), database.data(), numVectors * dim * sizeof(float),
                                    cudaMemcpyHostToDevice));
            // Note: cudaMemcpy above handles the transfer; raft::copy is redundant and removed.
 
            // 2. Build IVF-Flat index with SLA enforcement
            KernelSLAGuard build_guard(std::chrono::seconds(5));
            cuvs::neighbors::ivf_flat::index_params idx_params;
            idx_params.metric
                = useL2 ? cuvs::distance::DistanceType::L2Unexpanded : cuvs::distance::DistanceType::InnerProduct;
            auto index = cuvs::neighbors::ivf_flat::build(handle, idx_params, db_dev.view());
             
            if (build_guard.checkTimeoutDeadline()) {
                // Timeout during index build; fall through to CPU path
                gpu_done = false;
            } else {
                // 3. Copy queries to device and run search
                auto q_dev = raft::make_device_matrix<float>(handle, numQueries, dim);
                CHECKED_CUDA(cudaMemcpy(q_dev.data_handle(), queries.data(), numQueries * dim * sizeof(float),
                                        cudaMemcpyHostToDevice));
                // Note: cudaMemcpy above handles the transfer; raft::copy is redundant and removed.
                auto neighbors_dev = raft::make_device_matrix<uint32_t>(handle, numQueries, k);
                auto distances_dev = raft::make_device_matrix<float>(handle, numQueries, k);
                 
                KernelSLAGuard search_guard(std::chrono::seconds(5));
                cuvs::neighbors::ivf_flat::search_params search_params;
                cuvs::neighbors::ivf_flat::search(handle, search_params, index, q_dev.view(), neighbors_dev.view(),
                                                  distances_dev.view());
                 
                if (!search_guard.checkTimeoutDeadline()) {
                    // 4. Copy results back to host and populate AnnResult
                    std::vector<uint32_t> neighbor_idx(numQueries * k);
                    std::vector<float> host_distances(numQueries * k);
                    CHECKED_CUDA(cudaMemcpy(neighbor_idx.data(), neighbors_dev.data_handle(), 
                                            numQueries * k * sizeof(uint32_t), cudaMemcpyDeviceToHost));
                    CHECKED_CUDA(cudaMemcpy(host_distances.data(), distances_dev.data_handle(), 
                                            numQueries * k * sizeof(float), cudaMemcpyDeviceToHost));
                    // Note: Synchronize stream to ensure host-side copies complete
                    handle.sync_stream();
 
                    result.results.resize(numQueries);
                    for (size_t qi = 0; qi < numQueries; ++qi) {
                        result.results[qi].resize(k);
                        for (size_t ni = 0; ni < k; ++ni) {
                            result.results[qi][ni].index    = neighbor_idx[qi * k + ni];
                            result.results[qi][ni].distance = host_distances[qi * k + ni];
                        }
                    }
                    gpu_done = true;
                }
            }
        } catch (const std::exception& e) {
            // cudaMalloc failure or cuVS error — fall through to CPU path.
            gpu_done = false;
        }
#endif // THEMIS_ENABLE_CUVS

        if (gpu_done) {
            result.used_gpu = true;
            uint64_t bytes  = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_ann_searches;
            recordOp(numVectors, bytes, true);
            return result;
        }
        // GPU path did not complete — fall through to CPU brute-force below.
    }
#endif // THEMIS_ENABLE_CUDA

    // CPU brute-force exact k-NN (used when GPU unavailable or below threshold):

    const size_t actual_k = std::min(k, numVectors);
    result.results.resize(numQueries);

    for (size_t qi = 0; qi < numQueries; ++qi) {
        const float *q = queries.data() + qi * dim;

        // Compute distances from this query to all database vectors
        // using a max-heap of size k to track the k-nearest so far.
        // Heap element: (distance, index)
        using Pair = std::pair<float, size_t>;
        std::vector<Pair> heap;
        heap.reserve(actual_k + 1);

        for (size_t vi = 0; vi < numVectors; ++vi) {
            const float *v = database.data() + vi * dim;

            float dist = 0.0f;
            if (useL2) {
                for (size_t d = 0; d < dim; ++d) {
                    float diff = q[d] - v[d];
                    dist += diff * diff;
                }
            } else {
                // Negative inner product used as a distance (lower = more similar).
                // For unit-normalized vectors this equals cosine distance, but works
                // for unnormalized vectors as well: the result may be negative when
                // dot(q, v) > 0 (high similarity).  Callers performing maximum inner
                // product search (MIPS) should normalize their vectors beforehand to
                // obtain the standard cosine-distance interpretation.
                float dot = 0.0f;
                for (size_t d = 0; d < dim; ++d) {
                    dot += q[d] * v[d];
                }
                dist = -dot;
            }

            if (heap.size() < actual_k) {
                heap.emplace_back(dist, vi);
                if (heap.size() == actual_k) {
                    std::make_heap(heap.begin(), heap.end());
                }
            } else if (dist < heap.front().first) {
                std::pop_heap(heap.begin(), heap.end());
                heap.back() = {dist, vi};
                std::push_heap(heap.begin(), heap.end());
            }
        }

        // Sort heap ascending by distance
        std::sort_heap(heap.begin(), heap.end());

        result.results[qi].resize(heap.size());
        for (size_t ni = 0; ni < heap.size(); ++ni) {
            result.results[qi][ni].index    = heap[ni].second;
            result.results[qi][ni].distance = heap[ni].first;
        }
    }

    uint64_t bytes = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_ann_searches;
    recordOp(numVectors, bytes, result.used_gpu);

    return result;
}

GPUQueryAccelerator::Stats GPUQueryAccelerator::getStats() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return stats_;
}

void GPUQueryAccelerator::resetStats() {
    std::lock_guard<std::mutex> lk(mutex_);
    stats_ = Stats{};
}

} // namespace gpu
} // namespace themis

#ifdef THEMIS_GPU_QUERY_ACCEL_SYNC
#undef THEMIS_GPU_QUERY_ACCEL_SYNC
#endif
