// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file tensor_storage_strategy.h
 * @brief Quantization, mmap, and zero-copy strategy API for tensor and adapter
 *        artifacts (Issue #5443).
 *
 * ## Overview
 *
 * This header defines the public API for evaluating and applying storage
 * strategies for ThemisDB distributed tensor artifacts:
 *
 *   - **QuantizationLevel / QuantizationAssessor** — Determines which numeric
 *     precision is appropriate for a given artifact based on its error budget,
 *     memory constraints, and hardware capabilities.
 *
 *   - **MmapLoader / MmapRegion** — RAII-managed memory-mapped file access.
 *     Loads tensor artifact payloads via `mmap(2)` (or `MapViewOfFile` on
 *     Windows) to avoid redundant kernel-to-userspace copies and support
 *     page-granular eviction by the OS.
 *
 *   - **ZeroCopyAccessor** — Non-owning view over a live MmapRegion that
 *     exposes typed read-only span access for consumers that must never copy
 *     the underlying tensor bytes.
 *
 *   - **StorageStrategyRecommendation** — Aggregated result returned by
 *     `StorageStrategyAssessor::assess()` covering the recommended
 *     quantization level, preferred load mechanism, and any caveats.
 *
 * ## Advisory-Only Invariant
 *
 * All APIs in this header handle **advisory** tensor artifact data.  They
 * MUST NOT be used to override graph-verified query results.
 *
 * ## Platform Notes
 *
 * MmapLoader is functional on POSIX platforms (Linux ≥ 3.14, macOS ≥ 10.9)
 * and Windows (Vista+).  On unsupported platforms `MmapLoader::open()` returns
 * `MmapError::UNSUPPORTED_PLATFORM`.
 *
 * @see src/distributed_tensor/src/tensor_storage_strategy.cc
 * @see src/distributed_tensor/TENSOR_STORAGE_STRATEGY_ASSESSMENT.md
 */

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// QuantizationLevel
// ---------------------------------------------------------------------------

/**
 * @brief Numeric precision levels used for tensor artifact storage.
 *
 * Listed from highest (F32) to lowest (BINARY) precision.  Precision
 * reductions yield proportional savings in artifact size and memory
 * bandwidth, but each level has minimum L2-error-budget requirements that
 * must be satisfied by the assessor before recommending it.
 *
 * | Level  | Bytes/param | Typical use-case                    |
 * |--------|-------------|-------------------------------------|
 * | F32    | 4           | Reference, training checkpoints     |
 * | F16    | 2           | Inference: low-noise workloads      |
 * | BF16   | 2           | Inference: ML accelerators (AVX512) |
 * | INT8   | 1           | Inference: accuracy-tolerant tasks  |
 * | INT4   | 0.5         | Inference: highly memory-constrained|
 * | BINARY | 0.125       | Experimental / extreme compression  |
 */
enum class QuantizationLevel : uint8_t {
    F32    = 0, ///< 32-bit IEEE 754 float (no quantization).
    F16    = 1, ///< 16-bit IEEE 754 half-precision float.
    BF16   = 2, ///< 16-bit Brain Float (truncated mantissa).
    INT8   = 3, ///< 8-bit signed integer with affine scale+zero-point.
    INT4   = 4, ///< 4-bit signed integer, nibble-packed.
    BINARY = 5, ///< 1-bit binary representation.
};

/**
 * @brief Returns the byte width of one parameter at @p level.
 *
 * For sub-byte levels (INT4: 0.5 B, BINARY: 0.125 B) the value is rounded up
 * to the smallest whole byte that holds one parameter.  Callers that need
 * exact packed-buffer sizes must use `packedBytesForParams()` instead.
 *
 * @param level  Target quantization level.
 * @return       Byte width per parameter (always ≥ 1).
 */
[[nodiscard]] constexpr uint8_t bytesPerParam(QuantizationLevel level) noexcept {
    switch (level) {
        case QuantizationLevel::F32:    return 4;
        case QuantizationLevel::F16:    [[fallthrough]];
        case QuantizationLevel::BF16:   return 2;
        case QuantizationLevel::INT8:   return 1;
        case QuantizationLevel::INT4:   return 1; // nibble-packed: 2 params/byte, rounded up
        case QuantizationLevel::BINARY: return 1; // 8 params/byte, rounded up
    }
    return 4; // unreachable; fallback to F32
}

/**
 * @brief Computes the packed buffer size in bytes for @p num_params parameters.
 *
 * Unlike `bytesPerParam()`, this function handles sub-byte levels accurately:
 *  - INT4: `ceil(num_params / 2)` bytes
 *  - BINARY: `ceil(num_params / 8)` bytes
 *
 * @param num_params  Number of scalar parameters.
 * @param level       Quantization level.
 * @return            Minimum buffer size in bytes.
 */
[[nodiscard]] constexpr uint64_t packedBytesForParams(
    uint64_t num_params, QuantizationLevel level) noexcept {
    switch (level) {
        case QuantizationLevel::F32:    return num_params * 4;
        case QuantizationLevel::F16:    [[fallthrough]];
        case QuantizationLevel::BF16:   return num_params * 2;
        case QuantizationLevel::INT8:   return num_params;
        case QuantizationLevel::INT4:   return (num_params + 1) / 2;
        case QuantizationLevel::BINARY: return (num_params + 7) / 8;
    }
    return num_params * 4;
}

// ---------------------------------------------------------------------------
// QuantizationConstraints — inputs to the assessor
// ---------------------------------------------------------------------------

/**
 * @brief Input constraints that guide quantization level selection.
 *
 * All fields have conservative defaults so callers can set only the
 * parameters they care about.
 */
struct QuantizationConstraints {
    /// Maximum acceptable L2 reconstruction error (relative, ∈ [0, 1]).
    /// 0.0 = no error tolerated (forces F32); 1.0 = unlimited.
    double max_l2_error_relative = 0.0;

    /// Available memory budget in bytes for the artifact payload.
    /// 0 = unconstrained.
    uint64_t memory_budget_bytes = 0;

    /// Number of scalar parameters in the artifact.
    /// Required for budget-based quantization selection.
    uint64_t num_params = 0;

    /// True when INT8 calibration data is available (required for INT8/INT4).
    bool has_calibration_data = false;

    /// True when the target hardware supports AVX-512 BF16 instructions.
    bool hw_avx512_bf16 = false;

    /// True when the artifact is an adapter (LoRA, prefix-tuning, etc.).
    /// Adapters tolerate higher compression than base model weights.
    bool is_adapter = false;
};

// ---------------------------------------------------------------------------
// QuantizationAssessment — output of the assessor
// ---------------------------------------------------------------------------

/**
 * @brief Result returned by QuantizationAssessor::assess().
 */
struct QuantizationAssessment {
    /// Recommended quantization level.
    QuantizationLevel recommended_level = QuantizationLevel::F32;

    /// Estimated size reduction factor relative to F32 (e.g., 4.0 = 4× smaller).
    double compression_ratio = 1.0;

    /// Estimated L2 error at the recommended level (relative, ∈ [0, 1]).
    double estimated_l2_error = 0.0;

    /// Human-readable rationale for the recommendation.
    std::string rationale;

    /// Warnings raised during assessment (non-fatal).
    std::vector<std::string> warnings;
};

// ---------------------------------------------------------------------------
// QuantizationAssessor
// ---------------------------------------------------------------------------

/**
 * @brief Stateless assessor that selects the optimal quantization level.
 *
 * The assessor applies a priority-ordered decision tree:
 *
 *  1. If `max_l2_error_relative == 0` → F32 (lossless reference).
 *  2. If memory budget is satisfied by F16/BF16 and error budget allows → F16 or BF16.
 *  3. If `has_calibration_data` and error budget allows INT8 → INT8.
 *  4. If `is_adapter` and error budget allows INT4 → INT4.
 *  5. Fallback: F32.
 *
 * ### Example
 * @code
 * QuantizationConstraints c;
 * c.max_l2_error_relative = 0.01;  // 1 % L2 error budget
 * c.num_params             = 7'000'000'000ULL; // 7B model
 * c.memory_budget_bytes    = 8ULL * 1024 * 1024 * 1024; // 8 GiB
 * c.has_calibration_data   = true;
 *
 * auto result = QuantizationAssessor::assess(c);
 * // result.recommended_level == QuantizationLevel::INT8
 * @endcode
 */
class QuantizationAssessor {
public:
    /**
     * @brief Assess the optimal quantization level for the given constraints.
     *
     * @param constraints  Input constraints (error budget, memory, hardware caps).
     * @return             Assessment with recommended level and rationale.
     */
    [[nodiscard]] static QuantizationAssessment assess(
        const QuantizationConstraints& constraints);

    /**
     * @brief Returns true when @p level is feasible for @p constraints without
     *        violating the L2 error budget or memory budget.
     *
     * @param level        Candidate quantization level.
     * @param constraints  Constraints to check against.
     * @return             true if @p level is within budget.
     */
    [[nodiscard]] static bool isFeasible(
        QuantizationLevel level,
        const QuantizationConstraints& constraints) noexcept;

private:
    /// Typical L2 error introduced by each quantization level.
    /// Values are conservative upper-bound estimates (relative).
    static constexpr double kTypicalL2Error[] = {
        0.0,    // F32 — lossless
        0.001,  // F16 — ~0.1 % typical
        0.002,  // BF16 — ~0.2 % typical (shorter mantissa than F16)
        0.01,   // INT8 — ~1 % typical with calibration
        0.05,   // INT4 — ~5 % typical
        0.20,   // BINARY — ~20 % typical
    };
};

// ---------------------------------------------------------------------------
// MmapError
// ---------------------------------------------------------------------------

/**
 * @brief Error codes returned by MmapLoader operations.
 */
enum class MmapError : uint8_t {
    OK                   = 0, ///< No error.
    FILE_NOT_FOUND       = 1, ///< The artifact file path does not exist.
    PERMISSION_DENIED    = 2, ///< Insufficient permissions to open or map the file.
    FILE_TOO_LARGE       = 3, ///< File size exceeds the platform address-space limit.
    MAPPING_FAILED       = 4, ///< `mmap()`/`MapViewOfFile()` returned an error.
    ALREADY_OPEN         = 5, ///< A mapping is already open; close it first.
    NOT_OPEN             = 6, ///< No mapping is currently open.
    UNSUPPORTED_PLATFORM = 7, ///< Current platform does not support mmap.
    LOCK_FAILED          = 8, ///< `mlock()` failed (non-fatal; mapping still usable).
    IO_ERROR             = 9, ///< Generic I/O error (check errno/GetLastError).
};

/**
 * @brief Returns a human-readable description for @p err.
 *
 * @param err  Error code.
 * @return     Static string description (never nullptr).
 */
[[nodiscard]] const char* mmapErrorMessage(MmapError err) noexcept;

// ---------------------------------------------------------------------------
// MmapRegion — RAII handle to a mapped file region
// ---------------------------------------------------------------------------

/**
 * @brief RAII handle that owns a single memory-mapped file region.
 *
 * On destruction, the mapping is automatically unmapped via `munmap()` (POSIX)
 * or `UnmapViewOfFile()` (Windows).  Move-only: copying a MmapRegion would
 * create aliased mappings and is therefore deleted.
 *
 * ### Zero-copy contract
 *
 * Data returned by `data()` is a direct pointer into the kernel page cache.
 * Callers MUST NOT write through this pointer (the mapping is read-only).
 * Callers MUST NOT hold raw pointers across a `close()` or destruction.
 *
 * @invariant `data() != nullptr` iff `isOpen()`.
 */
class MmapRegion {
public:
    MmapRegion() noexcept = default;
    ~MmapRegion() noexcept;

    MmapRegion(const MmapRegion&)            = delete;
    MmapRegion& operator=(const MmapRegion&) = delete;

    MmapRegion(MmapRegion&& other) noexcept;
    MmapRegion& operator=(MmapRegion&& other) noexcept;

    /**
     * @brief Returns the base address of the mapped region.
     *
     * @return  Non-null pointer when `isOpen()`, nullptr otherwise.
     */
    [[nodiscard]] const std::byte* data() const noexcept { return data_; }

    /**
     * @brief Returns the size of the mapped region in bytes.
     *
     * @return  Mapping size; 0 when not open.
     */
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    /**
     * @brief Returns true when a live mapping is held by this object.
     */
    [[nodiscard]] bool isOpen() const noexcept { return data_ != nullptr; }

    /**
     * @brief Provides a typed read-only view over the entire mapped region.
     *
     * Alignment of the first element is guaranteed only to `alignof(std::byte)`
     * unless the file was created with the alignment of T in mind.
     *
     * @tparam T  Element type.
     * @return    Span over `size()/sizeof(T)` elements; empty when not open.
     */
    template <typename T>
    [[nodiscard]] std::span<const T> as_span() const noexcept {
        if (data_ == nullptr || size_ < sizeof(T)) {
            return {};
        }
        return {reinterpret_cast<const T*>(data_), size_ / sizeof(T)};
    }

    /**
     * @brief Releases the mapping and resets this object to the not-open state.
     *
     * No-op when already closed.
     */
    void close() noexcept;

    /**
     * @brief Returns the file path associated with this mapping (for diagnostics).
     *
     * @return  File path string; empty when not open.
     */
    [[nodiscard]] const std::string& path() const noexcept { return path_; }

private:
    friend class MmapLoader;

    const std::byte* data_ = nullptr;
    std::size_t      size_ = 0;
    std::string      path_;

#if defined(_WIN32)
    void* file_handle_   = nullptr; // HANDLE
    void* mapping_handle_= nullptr; // HANDLE
#else
    int  fd_   = -1;
#endif
};

// ---------------------------------------------------------------------------
// MmapLoader — factory for MmapRegions
// ---------------------------------------------------------------------------

/**
 * @brief Stateless factory that opens memory-mapped regions for tensor artifact
 *        files.
 *
 * Files are always mapped read-only.  `advise()` hints may be issued after
 * mapping to tune kernel prefetch behavior based on expected access pattern.
 *
 * ### Example — map and read as float32 span
 * @code
 * MmapLoader loader;
 * auto [region, err] = loader.open("/data/tensors/embedding.f32");
 * if (err != MmapError::OK) {
 *     log_error(mmapErrorMessage(err));
 *     return;
 * }
 * auto floats = region.as_span<float>();
 * // floats is a zero-copy view into the kernel page cache.
 * @endcode
 *
 * ### mmap vs. read() trade-offs
 *
 * | Aspect          | mmap                                 | read()                   |
 * |-----------------|--------------------------------------|--------------------------|
 * | Memory copy     | Zero kernel→userspace copy          | One copy per read call   |
 * | RSS pressure    | Shares pages with other processes   | Private process pages    |
 * | Random access   | O(1) via page faults                | Seek + read overhead     |
 * | Sequential      | Requires MADV_SEQUENTIAL hint       | Kernel read-ahead OK     |
 * | Locking         | Optional `mlock()` to pin pages     | Automatic for heap alloc |
 * | NVMe latency    | First access: page-fault latency    | Buffered by read-ahead   |
 *
 * For tensor artifacts (sequential, large, multi-consumer), mmap with
 * `MADV_SEQUENTIAL | MADV_WILLNEED` is the preferred loading path.
 */
class MmapLoader {
public:
    /**
     * @brief Access pattern hint passed to `madvise()` / `VirtualAlloc()`.
     */
    enum class AccessPattern {
        SEQUENTIAL, ///< Bytes are read front-to-back (enables kernel prefetch).
        RANDOM,     ///< Access order is unpredictable (disables prefetch).
        WILLNEED,   ///< All bytes will be accessed soon (triggers eager fault-in).
        DONTNEED,   ///< Pages are no longer needed (allow OS eviction).
    };

    MmapLoader() noexcept = default;
    ~MmapLoader()         = default;

    MmapLoader(const MmapLoader&)            = delete;
    MmapLoader& operator=(const MmapLoader&) = delete;
    MmapLoader(MmapLoader&&)                 noexcept = default;
    MmapLoader& operator=(MmapLoader&&)      noexcept = default;

    /**
     * @brief Opens a read-only memory-mapped region for @p file_path.
     *
     * The returned MmapRegion is in the open state on success and closed on
     * any error.  The caller takes ownership of the region.
     *
     * @param file_path   Absolute or relative path to the artifact file.
     * @param lock_pages  If true, attempt `mlock()` to prevent page-out.
     *                    Failure is non-fatal (warnings are available via the
     *                    region's error state); root privileges may be required.
     * @return            Pair of (MmapRegion, MmapError).
     *
     * @throws  Nothing — all errors are returned as MmapError.
     */
    [[nodiscard]] std::pair<MmapRegion, MmapError> open(
        std::string_view file_path,
        bool             lock_pages = false) const noexcept;

    /**
     * @brief Issues an madvise hint on an open region.
     *
     * No-op on platforms that do not support `madvise()`; returns OK in that
     * case (advisory failures are not fatal).
     *
     * @param region   Region to advise (must be open).
     * @param pattern  Access-pattern hint.
     * @return         MmapError::OK on success or NOT_OPEN if not open.
     */
    [[nodiscard]] MmapError advise(
        const MmapRegion& region,
        AccessPattern     pattern) noexcept;
};

// ---------------------------------------------------------------------------
// ZeroCopyAccessor — non-owning view over a MmapRegion
// ---------------------------------------------------------------------------

/**
 * @brief Non-owning, typed accessor for a live MmapRegion.
 *
 * ZeroCopyAccessor holds a non-owning pointer to the underlying MmapRegion.
 * It provides typed element access without copying data.  The caller must
 * ensure the referenced MmapRegion outlives any ZeroCopyAccessor instance.
 *
 * ### Usage
 * @code
 * auto [region, err] = loader.open(path);
 * ZeroCopyAccessor<float> acc(region);
 * float first = acc[0]; // zero-copy read from mmap'd page
 * @endcode
 *
 * @tparam T  Scalar element type (float, int8_t, etc.).
 */
template <typename T>
class ZeroCopyAccessor {
public:
    /**
     * @brief Constructs an accessor over @p region.
     *
     * @param region  Live MmapRegion.  Must outlive this accessor.
     *
     * @pre  region.isOpen() == true
     */
    explicit ZeroCopyAccessor(const MmapRegion& region) noexcept
        : span_(region.as_span<T>()) {}

    /**
     * @brief Returns the number of elements visible through this accessor.
     */
    [[nodiscard]] std::size_t size() const noexcept { return span_.size(); }

    /**
     * @brief Returns true when there are no elements.
     */
    [[nodiscard]] bool empty() const noexcept { return span_.empty(); }

    /**
     * @brief Element access (bounds-checked in debug builds).
     *
     * @param idx  Zero-based element index.
     * @return     Const reference to element at @p idx.
     *
     * @pre  idx < size()
     */
    [[nodiscard]] const T& operator[](std::size_t idx) const noexcept {
        return span_[idx];
    }

    /**
     * @brief Returns an iterator to the first element.
     */
    [[nodiscard]] auto begin() const noexcept { return span_.begin(); }

    /**
     * @brief Returns a past-the-end iterator.
     */
    [[nodiscard]] auto end() const noexcept { return span_.end(); }

    /**
     * @brief Returns the underlying span (read-only).
     */
    [[nodiscard]] std::span<const T> span() const noexcept { return span_; }

private:
    std::span<const T> span_;
};

// ---------------------------------------------------------------------------
// LoadMechanism — preferred load path
// ---------------------------------------------------------------------------

/**
 * @brief Recommended mechanism for loading a tensor artifact into memory.
 */
enum class LoadMechanism : uint8_t {
    /// Memory-mapped file (zero-copy, shared page cache, lazy fault-in).
    MMAP_ZERO_COPY = 0,

    /// Prefaulted mmap with MADV_WILLNEED (eager page-in, for NVMe latency).
    MMAP_PREFAULT  = 1,

    /// Buffered read() into a heap allocation (compatibility fallback).
    BUFFERED_READ  = 2,

    /// Direct I/O (O_DIRECT) — bypasses page cache; requires aligned buffers.
    DIRECT_IO      = 3,
};

// ---------------------------------------------------------------------------
// StorageStrategyRecommendation
// ---------------------------------------------------------------------------

/**
 * @brief Aggregated recommendation returned by StorageStrategyAssessor::assess().
 *
 * Covers the quantization level, load mechanism, and any caveats that the
 * deployment team should review before committing to a strategy.
 */
struct StorageStrategyRecommendation {
    /// Recommended quantization level (precision).
    QuantizationLevel quantization_level = QuantizationLevel::F32;

    /// Recommended file-loading mechanism.
    LoadMechanism load_mechanism = LoadMechanism::MMAP_ZERO_COPY;

    /// Estimated artifact size in bytes at the recommended quantization.
    uint64_t estimated_size_bytes = 0;

    /// Compression ratio relative to F32 baseline (> 1.0 means smaller).
    double compression_ratio = 1.0;

    /// Estimated worst-case L2 reconstruction error (relative).
    double estimated_l2_error = 0.0;

    /// Human-readable summary of the recommendation.
    std::string summary;

    /// Operational caveats (non-fatal notes for ops team).
    std::vector<std::string> caveats;
};

// ---------------------------------------------------------------------------
// StorageStrategyAssessor — top-level entry point
// ---------------------------------------------------------------------------

/**
 * @brief Assesses the optimal storage strategy for a tensor artifact.
 *
 * Combines quantization selection with load-mechanism selection based on:
 *  - Artifact precision and error constraints.
 *  - Available memory and storage medium characteristics.
 *  - OS and hardware capabilities.
 *
 * ### Example
 * @code
 * StorageStrategyAssessor::Config cfg;
 * cfg.quant.max_l2_error_relative = 0.005; // 0.5 % L2 error budget
 * cfg.quant.num_params             = 7'000'000'000ULL;
 * cfg.quant.memory_budget_bytes    = 16ULL * 1024 * 1024 * 1024; // 16 GiB
 * cfg.quant.has_calibration_data   = true;
 * cfg.quant.is_adapter             = false;
 * cfg.storage_on_nvme              = true;
 * cfg.multi_consumer               = true;
 *
 * auto rec = StorageStrategyAssessor::assess(cfg);
 * // rec.quantization_level == QuantizationLevel::INT8
 * // rec.load_mechanism     == LoadMechanism::MMAP_PREFAULT
 * @endcode
 */
class StorageStrategyAssessor {
public:
    /**
     * @brief Configuration for the assessor.
     */
    struct Config {
        /// Quantization constraints.
        QuantizationConstraints quant;

        /// True when the artifact resides on an NVMe SSD (vs. HDD or RAM disk).
        bool storage_on_nvme = true;

        /// True when multiple processes will consume the same artifact concurrently.
        /// Favours mmap (shared page cache) over buffered read (private copies).
        bool multi_consumer = false;

        /// True when the OS supports mmap (set to false to force BUFFERED_READ).
        bool os_supports_mmap = true;

        /// True when mlock is available (root privileges or RLIMIT_MEMLOCK).
        bool can_lock_pages = false;
    };

    /**
     * @brief Produce a storage strategy recommendation for the given config.
     *
     * @param config  Assessor configuration.
     * @return        Recommendation covering quantization, load mechanism, size,
     *                compression ratio, and caveats.
     */
    [[nodiscard]] static StorageStrategyRecommendation assess(
        const Config& config);
};

} // namespace distributed_tensor
} // namespace themis
