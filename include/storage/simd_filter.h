/**
 * @file simd_filter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include "storage/columnar_format.h"

namespace themis {
namespace storage {

// ============================================================================
// Filter operation enum (equality and range predicates)
// ============================================================================

enum class FilterOp : uint8_t {
    EQ = 0,   ///< equal
    NE = 1,   ///< not equal
    LT = 2,   ///< less than
    LE = 3,   ///< less than or equal
    GT = 4,   ///< greater than
    GE = 5,   ///< greater than or equal
};

// ============================================================================
// Runtime SIMD capability detection
// ============================================================================

/// Available SIMD levels (ascending capability order).
enum class SIMDLevel : uint8_t {
    SCALAR  = 0,
    SSE4    = 1,
    NEON    = 2,   ///< ARM NEON / AArch64 (4×i32, 2×i64, 4×f32, 2×f64 per iter)
    AVX2    = 3,
    AVX512  = 4,
};

/// Detect the best available SIMD level at runtime.
/// Returns NEON on AArch64, SSE4/AVX2/AVX512 on x86 when supported,
/// SCALAR otherwise.  Result is memoised after the first call.
SIMDLevel detectSIMDLevel() noexcept;

// ============================================================================
// SIMD batch filter kernels
// ============================================================================

/// Filter a raw int32 data array.
/// Writes the indices of matching rows into @p out_indices (appended).
/// Returns the number of matching rows written.
///
/// Uses AVX2 (8-way) or scalar path depending on detectSIMDLevel().
/// Graceful fallback: scalar path is always correct.
///
/// @param data        Pointer to contiguous int32 values.
/// @param n           Number of elements.
/// @param op          Comparison operator.
/// @param threshold   Value to compare against.
/// @param out_indices Output vector receiving matching row indices.
/// @returns           Number of matching rows appended.
size_t simd_filter_int32(const int32_t* data,
                         size_t n,
                         FilterOp op,
                         int32_t threshold,
                         std::vector<uint32_t>& out_indices);

/// Filter a raw int64 data array.
/// Same contract as simd_filter_int32.
size_t simd_filter_int64(const int64_t* data,
                         size_t n,
                         FilterOp op,
                         int64_t threshold,
                         std::vector<uint32_t>& out_indices);

/// Filter a raw float (float32) data array.
/// Same contract as simd_filter_int32.
size_t simd_filter_float(const float* data,
                         size_t n,
                         FilterOp op,
                         float threshold,
                         std::vector<uint32_t>& out_indices);

/// Filter a raw double (float64) data array.
/// Same contract as simd_filter_int32.
size_t simd_filter_double(const double* data,
                          size_t n,
                          FilterOp op,
                          double threshold,
                          std::vector<uint32_t>& out_indices);

// ============================================================================
// SIMDColumnFilter – integrates with ColumnSegment / ColumnarFormatManager
// ============================================================================

/// Statistics collected during a vectorized scan.
struct SIMDFilterStats {
    SIMDLevel simd_level    = SIMDLevel::SCALAR;
    size_t    rows_processed = 0;
    size_t    rows_matched   = 0;
    double    elapsed_us     = 0.0;  ///< wall-clock microseconds for last scan
};

/// Predicate for a single-column filter.
struct ColumnPredicate {
    FilterOp  op;
    ColumnType column_type;
    union Value {
        int32_t  i32;
        int64_t  i64;
        float    f32;
        double   f64;
    } threshold;
};

/// High-level SIMD-accelerated column filter.
///
/// Usage:
/// @code
///     SIMDColumnFilter filter;
///     ColumnPredicate pred{FilterOp::GT, ColumnType::INT64, {.i64 = 42}};
///
///     auto result = filter.scan(segment, pred);
///     // result contains row indices satisfying pred
///     auto stats = filter.lastStats();
/// @endcode
class SIMDColumnFilter {
public:
    SIMDColumnFilter() = default;

    /// Scan a decoded ColumnSegment and return the indices of rows satisfying
    /// the predicate.  The segment must already be decoded (i.e., rawData()
    /// is populated with the column's native type data).
    ///
    /// @param segment   Decoded ColumnSegment.
    /// @param predicate Predicate to evaluate (must match segment column type).
    /// @returns         Sorted list of matching row indices (0-based).
    ///                  Returns an empty vector if the zone-map eliminates the
    ///                  entire segment.
    std::vector<uint32_t> scan(const ColumnSegment& segment,
                               const ColumnPredicate& predicate);

    /// Batch-scan multiple segments for the same predicate (e.g., multi-chunk
    /// column).  Offsets each segment's indices by its start row.
    ///
    /// @param segments     Ordered list of decoded ColumnSegments.
    /// @param predicate    Predicate to evaluate.
    /// @returns            Global row indices (ascending) of matching rows.
    std::vector<uint32_t> scanBatch(const std::vector<ColumnSegment>& segments,
                                    const ColumnPredicate& predicate);

    /// Statistics from the most recent scan() or scanBatch() call.
    const SIMDFilterStats& lastStats() const noexcept { return stats_; }

    /// Reset statistics counters.
    void resetStats() noexcept;

private:
    SIMDFilterStats stats_;

    // Zone-map early-out check — returns true if the entire segment can be
    // skipped without evaluating any rows.
    static bool canSkipSegment(const ColumnSegment& segment,
                               const ColumnPredicate& pred) noexcept;
};

} // namespace storage
} // namespace themis
