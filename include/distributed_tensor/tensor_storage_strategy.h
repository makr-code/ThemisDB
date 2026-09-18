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

enum class QuantizationLevel : uint8_t {
    F32    = 0, ///< 32-bit IEEE 754 float (no quantization).
    F16    = 1, ///< 16-bit IEEE 754 half-precision float.
    BF16   = 2, ///< 16-bit Brain Float (truncated mantissa).
    INT8   = 3, ///< 8-bit signed integer with affine scale+zero-point.
    INT4   = 4, ///< 4-bit signed integer, nibble-packed.
    BINARY = 5, ///< 1-bit binary representation.
};

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

struct QuantizationConstraints {
    double max_l2_error_relative = 0.0;

    uint64_t memory_budget_bytes = 0;

    uint64_t num_params = 0;

    bool has_calibration_data = false;

    bool hw_avx512_bf16 = false;

    bool is_adapter = false;
};

// ---------------------------------------------------------------------------
// QuantizationAssessment — output of the assessor
// ---------------------------------------------------------------------------

struct QuantizationAssessment {
    QuantizationLevel recommended_level = QuantizationLevel::F32;

    double compression_ratio = 1.0;

    double estimated_l2_error = 0.0;

    std::string rationale;

    std::vector<std::string> warnings;
};

// ---------------------------------------------------------------------------
// QuantizationAssessor
// ---------------------------------------------------------------------------

class QuantizationAssessor {
public:
    [[nodiscard]] static QuantizationAssessment assess(
        const QuantizationConstraints& constraints);

    [[nodiscard]] static bool isFeasible(
        QuantizationLevel level,
        const QuantizationConstraints& constraints) noexcept;

private:
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

[[nodiscard]] const char* mmapErrorMessage(MmapError err) noexcept;

// ---------------------------------------------------------------------------
// MmapRegion — RAII handle to a mapped file region
// ---------------------------------------------------------------------------

class MmapRegion {
public:
    MmapRegion() noexcept = default;
    ~MmapRegion() noexcept;

    MmapRegion(const MmapRegion&)            = delete;
    MmapRegion& operator=(const MmapRegion&) = delete;

    MmapRegion(MmapRegion&& other) noexcept;
    MmapRegion& operator=(MmapRegion&& other) noexcept;

    [[nodiscard]] const std::byte* data() const noexcept { return data_; }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    [[nodiscard]] bool isOpen() const noexcept { return data_ != nullptr; }

    template <typename T>
    [[nodiscard]] std::span<const T> as_span() const noexcept {
        if (data_ == nullptr || size_ < sizeof(T)) {
            return {};
        }
        return {reinterpret_cast<const T*>(data_), size_ / sizeof(T)};
    }

    /**
     * @brief Close.
     * @note Exception safety: noexcept.
     */
    void close() noexcept;

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

class MmapLoader {
public:
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

    [[nodiscard]] std::pair<MmapRegion, MmapError> open(
        std::string_view file_path,
        bool             lock_pages = false) const noexcept;

    [[nodiscard]] MmapError advise(
        const MmapRegion& region,
        AccessPattern     pattern) noexcept;
};

// ---------------------------------------------------------------------------
// ZeroCopyAccessor — non-owning view over a MmapRegion
// ---------------------------------------------------------------------------

template <typename T>
class ZeroCopyAccessor {
public:
    explicit ZeroCopyAccessor(const MmapRegion& region) noexcept
        : span_(region.as_span<T>()) {}

    [[nodiscard]] std::size_t size() const noexcept { return span_.size(); }

    [[nodiscard]] bool empty() const noexcept { return span_.empty(); }

    [[nodiscard]] const T& operator[](std::size_t idx) const noexcept {
        return span_[idx];
    }

    [[nodiscard]] auto begin() const noexcept { return span_.begin(); }

    [[nodiscard]] auto end() const noexcept { return span_.end(); }

    [[nodiscard]] std::span<const T> span() const noexcept { return span_; }

private:
    std::span<const T> span_;
};

// ---------------------------------------------------------------------------
// LoadMechanism — preferred load path
// ---------------------------------------------------------------------------

enum class LoadMechanism : uint8_t {
    MMAP_ZERO_COPY = 0,

    MMAP_PREFAULT  = 1,

    BUFFERED_READ  = 2,

    DIRECT_IO      = 3,
};

// ---------------------------------------------------------------------------
// StorageStrategyRecommendation
// ---------------------------------------------------------------------------

struct StorageStrategyRecommendation {
    QuantizationLevel quantization_level = QuantizationLevel::F32;

    LoadMechanism load_mechanism = LoadMechanism::MMAP_ZERO_COPY;

    uint64_t estimated_size_bytes = 0;

    double compression_ratio = 1.0;

    double estimated_l2_error = 0.0;

    std::string summary;

    std::vector<std::string> caveats;
};

// ---------------------------------------------------------------------------
// StorageStrategyAssessor — top-level entry point
// ---------------------------------------------------------------------------

class StorageStrategyAssessor {
public:
    struct Config {
        QuantizationConstraints quant;

        bool storage_on_nvme = true;

        bool multi_consumer = false;

        bool os_supports_mmap = true;

        bool can_lock_pages = false;
    };

    [[nodiscard]] static StorageStrategyRecommendation assess(
        const Config& config);
};

} // namespace distributed_tensor
} // namespace themis
