// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_timeseries_contract_hardening_focused.cpp
 * @brief Phase 4 timeseries contract-hardening focused test suite (TSCH-01..TSCH-16).
 *
 * Verifies the normative contracts defined in
 * include/timeseries/timeseries_api_contract.h using deterministic, mock-I/O
 * test cases.  All tests use kTimeseriesContractSeed = 42.
 *
 * ## Test families
 *
 * ### TSCH-01..04 — Write contract model
 *   TSCH-01  In-order timestamps are accepted with a positive window
 *   TSCH-02  Timestamp below the positive-window cutoff is rejected
 *   TSCH-03  Zero timestamp is not rejected by sign
 *   TSCH-04  Duplicate timestamp is accepted by the watermark comparison
 *
 * ### TSCH-05..08 — Range-query contract
 *   TSCH-05  Inclusive bounds: [start, end] returns all points in range
 *   TSCH-06  Empty range → empty result, not error
 *   TSCH-07  Series-not-found → SERIES_NOT_FOUND
 *   TSCH-08  Points at exact boundary are included
 *
 * ### TSCH-09..12 — Gorilla compression contract
 *   TSCH-09  Gorilla round-trip preserves arbitrary float64 exactly
 *   TSCH-10  NaN is preserved through encode/decode (bit-identical)
 *   TSCH-11  +Inf is preserved through encode/decode
 *   TSCH-12  -Inf is preserved through encode/decode
 *
 * ### TSCH-13..16 — Downsampling contract
 *   TSCH-13  Output bucket count is deterministic for given resolution
 *   TSCH-14  Empty input → empty output
 *   TSCH-15  Single point passes through unchanged
 *   TSCH-16  Zero resolution → DOWNSAMPLING_RESOLUTION_INVALID
 *
 * @see include/timeseries/timeseries_api_contract.h
 * @see src/timeseries/ROADMAP.md — Phase 4 items
 */

#include <gtest/gtest.h>

#include "timeseries/timeseries_api_contract.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <vector>

namespace themis {
namespace timeseries {
namespace test {

/// Canonical PRNG seed for all TSCH tests.
static constexpr uint64_t kTimeseriesContractSeed = 42;

// ============================================================================
// Mock helpers
// ============================================================================

struct TimePoint {
    std::int64_t timestamp = {};
    double       value = {};
};

/// Models TSStore's window comparison; concrete storage behavior is tested separately.
static bool mockWritePoint(std::int64_t watermark,
                           std::int64_t new_ts,
                           std::int64_t window_ms) {
    return window_ms <= 0 || new_ts >= watermark - window_ms;
}

/// Range query: returns all points in [start, end] inclusive.
static std::vector<TimePoint> mockRangeQuery(
        const std::vector<TimePoint>& series,
        std::int64_t start,
        std::int64_t end,
        bool series_exists = true) {
    if (!series_exists) return {}; // caller checks SERIES_NOT_FOUND separately
    std::vector<TimePoint> result = {};

    for (const auto& p : series) {
        if (p.timestamp >= start && p.timestamp <= end) {
            result.push_back(p);
        }
    }
    return result;
}

static std::optional<TimeseriesErrorCode> mockSeriesCheck(bool exists) {
    if (!exists) {
      return TimeseriesErrorCode::SERIES_NOT_FOUND;
    }
    return std::nullopt;
}

/// Gorilla encode: stores the raw IEEE 754 bits (lossless mock).
static std::uint64_t mockGorillaEncode(double v) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    return bits;
}

/// Gorilla decode: restores the raw IEEE 754 bits.
static double mockGorillaDecode(std::uint64_t bits) {
    double v = 0;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
}

/// Downsampling: groups points into buckets of resolution_ns width.
static std::optional<TimeseriesErrorCode> mockDownsample(
        const std::vector<TimePoint>& input,
        std::int64_t resolution_ns,
        std::vector<double>& out_buckets) {
    if (resolution_ns <= 0) {
      return TimeseriesErrorCode::DOWNSAMPLING_RESOLUTION_INVALID;
    }
    if (input.empty()) {
        out_buckets.clear();
        return std::nullopt;
    }
    std::map<std::int64_t, std::pair<double, int>> buckets;
    for (const auto& p : input) {
        std::int64_t bucket = (p.timestamp / resolution_ns);
        auto& [sum, cnt] = buckets[bucket];
        sum += p.value;
        ++cnt;
    }
    out_buckets.clear();
    for (const auto& [bk, sc] : buckets) {
        out_buckets.push_back(sc.first / sc.second); // mean
    }
    return std::nullopt;
}

// ============================================================================
// TSCH-01..04 — Write contract tests
// ============================================================================

/// TSCH-01: In-order timestamps are accepted with a positive late-arrival window.
TEST(TimeseriesContractHardening, TSCH01_InOrderTimestampsAccepted) {
    constexpr std::int64_t window_ms = 1000LL;
    std::int64_t watermark = 1000LL;
    for (std::int64_t ts = 2000LL; ts <= 5000LL; ts += 1000LL) {
        EXPECT_TRUE(mockWritePoint(watermark, ts, window_ms)) << "ts=" << ts;
        watermark = ts;
    }
}

/// TSCH-02: A timestamp below watermark-window is rejected.
TEST(TimeseriesContractHardening, TSCH02_BeyondWindowTimestampRejected) {
    EXPECT_FALSE(mockWritePoint(5000LL, 3999LL, 1000LL));
}

/// TSCH-03: Zero is not rejected just because of its sign.
TEST(TimeseriesContractHardening, TSCH03_ZeroTimestampAccepted) {
    EXPECT_TRUE(mockWritePoint(0LL, 0LL, 1000LL));
}

/// TSCH-04: Equality with the watermark passes the late-arrival comparison.
TEST(TimeseriesContractHardening, TSCH04_DuplicateTimestampAccepted) {
    EXPECT_TRUE(mockWritePoint(1000LL, 1000LL, 1000LL));
}

// ============================================================================
// TSCH-05..08 — Range-query contract tests
// ============================================================================

/// TSCH-05: Range query [start, end] returns all points with ts in range.
TEST(TimeseriesContractHardening, TSCH05_InclusiveBoundsQuery) {
    std::vector<TimePoint> series = {
        {1000LL, 1.0}, {2000LL, 2.0}, {3000LL, 3.0},
        {4000LL, 4.0}, {5000LL, 5.0},
    };
    auto result = mockRangeQuery(series, 2000LL, 4000LL);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].timestamp, 2000LL);
    EXPECT_EQ(result[2].timestamp, 4000LL);
}

/// TSCH-06: Empty range (no points in [start, end]) → empty result, no error.
TEST(TimeseriesContractHardening, TSCH06_EmptyRangeIsNotError) {
    std::vector<TimePoint> series = {{1000LL, 1.0}, {2000LL, 2.0}};
    auto result = mockRangeQuery(series, 5000LL, 9000LL);
    EXPECT_TRUE(result.empty());
}

/// TSCH-07: Non-existent series → SERIES_NOT_FOUND.
TEST(TimeseriesContractHardening, TSCH07_SeriesNotFoundError) {
    auto err = mockSeriesCheck(/*exists=*/false);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, TimeseriesErrorCode::SERIES_NOT_FOUND);
    EXPECT_TRUE(isLifecycleError(*err));
}

/// TSCH-08: Points at exact start and end boundaries are included.
TEST(TimeseriesContractHardening, TSCH08_BoundaryPointsIncluded) {
    std::vector<TimePoint> series = {
        {999LL, 0.0}, {1000LL, 1.0}, {2000LL, 2.0}, {2001LL, 3.0},
    };
    auto result = mockRangeQuery(series, 1000LL, 2000LL);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result.front().timestamp, 1000LL);
    EXPECT_EQ(result.back().timestamp,  2000LL);
}

// ============================================================================
// TSCH-09..12 — Gorilla compression contract tests
// ============================================================================

/// TSCH-09: Gorilla round-trip preserves arbitrary IEEE 754 float64.
TEST(TimeseriesContractHardening, TSCH09_GorillaRoundTripArbitrary) {
    std::mt19937_64 rng(kTimeseriesContractSeed);
    std::uniform_real_distribution<double> dist(-1e18, 1e18);

    for (int i = 0; i < 1000; ++i) {
        double original = dist(rng);
        auto bits   = mockGorillaEncode(original);
        double decoded  = mockGorillaDecode(bits);
        // Bit-identical round-trip
        std::uint64_t orig_bits = {};
        std::memcpy(&orig_bits, &original, sizeof(orig_bits));
        std::uint64_t dec_bits = {};
        std::memcpy(&dec_bits, &decoded,   sizeof(dec_bits));
        EXPECT_EQ(orig_bits, dec_bits) << "Round-trip failed for value=" << original;
    }
}

/// TSCH-10: NaN is preserved through Gorilla encode/decode (bit-identical).
TEST(TimeseriesContractHardening, TSCH10_NanPreservedGorilla) {
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    auto bits = mockGorillaEncode(nan_val);
    double decoded = mockGorillaDecode(bits);

    std::uint64_t orig_bits, dec_bits;
    std::memcpy(&orig_bits, &nan_val, sizeof(orig_bits));
    std::memcpy(&dec_bits,  &decoded, sizeof(dec_bits));
    EXPECT_EQ(orig_bits, dec_bits);
    EXPECT_TRUE(std::isnan(decoded));
}

/// TSCH-11: +Inf is preserved through Gorilla encode/decode.
TEST(TimeseriesContractHardening, TSCH11_PosInfPreservedGorilla) {
    double inf_val = std::numeric_limits<double>::infinity();
    double decoded = mockGorillaDecode(mockGorillaEncode(inf_val));
    EXPECT_TRUE(std::isinf(decoded));
    EXPECT_GT(decoded, 0.0);
}

/// TSCH-12: -Inf is preserved through Gorilla encode/decode.
TEST(TimeseriesContractHardening, TSCH12_NegInfPreservedGorilla) {
    double neg_inf = -std::numeric_limits<double>::infinity();
    double decoded = mockGorillaDecode(mockGorillaEncode(neg_inf));
    EXPECT_TRUE(std::isinf(decoded));
    EXPECT_LT(decoded, 0.0);
}

// ============================================================================
// TSCH-13..16 — Downsampling contract tests
// ============================================================================

/// TSCH-13: Output bucket count is deterministic for given resolution.
TEST(TimeseriesContractHardening, TSCH13_DownsamplingCountDeterministic) {
    std::vector<TimePoint> series = {};

    for (int i = 0; i < 1000; ++i) {
        series.push_back({static_cast<std::int64_t>(i) * 1'000'000LL, static_cast<double>(i)});
    }
    std::vector<double> buckets1, buckets2;
    mockDownsample(series, 10'000'000LL, buckets1); // 10ms buckets
    mockDownsample(series, 10'000'000LL, buckets2);

    EXPECT_EQ(buckets1.size(), buckets2.size());
    EXPECT_FALSE(buckets1.empty());
}

/// TSCH-14: Empty input → empty output.
TEST(TimeseriesContractHardening, TSCH14_EmptyInputEmptyOutput) {
    std::vector<TimePoint> empty_series;
    std::vector<double> buckets;
    auto err = mockDownsample(empty_series, 1'000'000LL, buckets);
    EXPECT_FALSE(err.has_value());
    EXPECT_TRUE(buckets.empty());
}

/// TSCH-15: Single point passes through unchanged as single bucket.
TEST(TimeseriesContractHardening, TSCH15_SinglePointPassthrough) {
    std::vector<TimePoint> single = {{1000LL, 42.0}};
    std::vector<double> buckets;
    auto err = mockDownsample(single, 1'000'000LL, buckets);
    EXPECT_FALSE(err.has_value());
    ASSERT_EQ(buckets.size(), 1u);
    EXPECT_DOUBLE_EQ(buckets[0], 42.0);
}

/// TSCH-16: Zero resolution → DOWNSAMPLING_RESOLUTION_INVALID.
TEST(TimeseriesContractHardening, TSCH16_ZeroResolutionError) {
    std::vector<TimePoint> series = {{1000LL, 1.0}};
    std::vector<double> buckets;
    auto err = mockDownsample(series, 0LL, buckets);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, TimeseriesErrorCode::DOWNSAMPLING_RESOLUTION_INVALID);
    EXPECT_TRUE(isHardTimeseriesError(*err));
}

} // namespace test
} // namespace timeseries
} // namespace themis
