// Tests for GorillaSIMDDecoder — vectorised Gorilla chunk decoder (Issue #117)
//
// Verifies that the SIMD decoder produces output byte-for-byte identical to
// GorillaDecoder for all input shapes, and exercises the runtime CPU dispatch.

#include <gtest/gtest.h>
#include "timeseries/gorilla_simd.h"
#include "timeseries/gorilla.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

using namespace themis;

// ────────────────────────────────────────────────────────────────────────────
// Helpers
// ────────────────────────────────────────────────────────────────────────────

static std::vector<uint8_t> encode(const std::vector<std::pair<int64_t, double>>& pts) {
    GorillaEncoder enc;
    for (auto& [ts, v] : pts) enc.add(ts, v);
    return enc.finish();
}

static std::vector<std::pair<int64_t, double>> decode_scalar(
        const std::vector<uint8_t>& data) {
    GorillaDecoder dec(data);
    std::vector<std::pair<int64_t, double>> out;
    while (auto p = dec.next()) out.push_back(*p);
    return out;
}

static std::vector<std::pair<int64_t, double>> decode_simd(
        const std::vector<uint8_t>& data) {
    GorillaSIMDDecoder dec(data);
    std::vector<std::pair<int64_t, double>> out;
    dec.decodeAll(out);
    return out;
}

// Strict equality check: same timestamp and bit-identical double value.
static void expect_eq(
        const std::vector<std::pair<int64_t, double>>& expected,
        const std::vector<std::pair<int64_t, double>>& actual,
        const char* context) {
    ASSERT_EQ(expected.size(), actual.size()) << context;
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i].first, actual[i].first)
            << context << " ts mismatch at index " << i;
        // Use memcmp for bit-identical comparison (handles NaN, ±0, etc.)
        uint64_t e_bits = 0, a_bits = 0;
        std::memcpy(&e_bits, &expected[i].second, 8);
        std::memcpy(&a_bits, &actual[i].second,   8);
        EXPECT_EQ(e_bits, a_bits)
            << context << " value mismatch at index " << i
            << " (expected " << expected[i].second
            << " actual "   << actual[i].second   << ")";
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Test fixture
// ────────────────────────────────────────────────────────────────────────────

class GorillaSIMDTest : public ::testing::Test {};

// ── Runtime feature detection ────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, RuntimeDetectionDoesNotCrash) {
    // gorilla_simd_has_avx2() / has_neon() must never throw or crash.
    bool avx2 = gorilla_simd_has_avx2();
    bool neon  = gorilla_simd_has_neon();
    (void)avx2;
    (void)neon;
    // At most one of {avx2, neon} can be true on any single platform.
    EXPECT_FALSE(avx2 && neon);
}

// ── Empty input ──────────────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, EmptyInputReturnsZeroPoints) {
    std::vector<uint8_t> empty;
    GorillaSIMDDecoder dec(empty);
    std::vector<std::pair<int64_t, double>> out;
    size_t n = dec.decodeAll(out);
    EXPECT_EQ(n, 0u);
    EXPECT_TRUE(out.empty());
    EXPECT_FALSE(dec.hasError());
    EXPECT_EQ(dec.decodedCount(), 0u);
}

TEST_F(GorillaSIMDTest, EmptyEncoderOutputDecodes) {
    GorillaEncoder enc;
    auto bytes = enc.finish();
    GorillaSIMDDecoder dec(bytes);
    std::vector<std::pair<int64_t, double>> out;
    dec.decodeAll(out);
    EXPECT_TRUE(out.empty());
}

// ── Single point ─────────────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, SinglePointRoundTrip) {
    std::vector<std::pair<int64_t, double>> pts = {{1700000000000LL, 42.0}};
    auto bytes = encode(pts);
    auto simd  = decode_simd(bytes);
    auto ref   = decode_scalar(bytes);
    expect_eq(ref, simd, "SinglePoint");
}

TEST_F(GorillaSIMDTest, SinglePointDecodedCount) {
    std::vector<std::pair<int64_t, double>> pts = {{1700000000000LL, 3.14}};
    GorillaSIMDDecoder dec(encode(pts));
    std::vector<std::pair<int64_t, double>> out;
    size_t n = dec.decodeAll(out);
    EXPECT_EQ(n, 1u);
    EXPECT_EQ(dec.decodedCount(), 1u);
}

// ── Two points ───────────────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, TwoPointRoundTrip) {
    std::vector<std::pair<int64_t, double>> pts = {
        {1700000000000LL, 1.0},
        {1700000001000LL, 2.0}
    };
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "TwoPoints");
}

// ── Basic series ─────────────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, BasicSineSeries1k) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 1000; ++i)
        pts.emplace_back(t0 + i * 1000, std::sin(i * 0.01));
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "SineSeries1k");
}

TEST_F(GorillaSIMDTest, MonotonicSeries2k) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 2000; ++i)
        pts.emplace_back(t0 + i * 1000, i * 0.001);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "MonotonicSeries2k");
}

// ── Larger dataset (tests SIMD loop body exhaustively) ───────────────────────

TEST_F(GorillaSIMDTest, LargeDataset10k) {
    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, 1.0);
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t = 1700000000000LL;
    double  v = 100.0;
    for (int i = 0; i < 10000; ++i) {
        v += noise(rng);
        pts.emplace_back(t, v);
        t += 1000;
    }
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "LargeDataset10k");
}

// ── Constant values (XOR = 0 path) ───────────────────────────────────────────

TEST_F(GorillaSIMDTest, ConstantValues) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 200; ++i)
        pts.emplace_back(t0 + i * 1000, 99.9);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "ConstantValues");
}

// ── Alternating values ───────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, AlternatingValues) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 200; ++i)
        pts.emplace_back(t0 + i * 1000, (i % 2 == 0) ? 1.0 : -1.0);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "AlternatingValues");
}

// ── Special floating-point values ───────────────────────────────────────────

TEST_F(GorillaSIMDTest, InfinityValues) {
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<std::pair<int64_t, double>> pts = {
        {1700000000000LL,  inf},
        {1700000001000LL, -inf},
        {1700000002000LL,  inf},
        {1700000003000LL, 42.0},
        {1700000004000LL,  inf},
    };
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "Infinity");
}

TEST_F(GorillaSIMDTest, NaNValues) {
    std::vector<std::pair<int64_t, double>> pts = {
        {1700000000000LL, 1.0},
        {1700000001000LL, std::numeric_limits<double>::quiet_NaN()},
        {1700000002000LL, 2.0},
        {1700000003000LL, std::numeric_limits<double>::signaling_NaN()},
        {1700000004000LL, 3.0},
    };
    auto bytes = encode(pts);
    // For NaN we can't use bit-exact comparison directly via EXPECT_EQ,
    // so we compare via the scalar decoder (both must produce NaN at the same index).
    auto ref  = decode_scalar(bytes);
    auto simd = decode_simd(bytes);
    ASSERT_EQ(ref.size(), simd.size());
    for (size_t i = 0; i < ref.size(); ++i) {
        EXPECT_EQ(ref[i].first, simd[i].first) << "ts mismatch at " << i;
        if (std::isnan(ref[i].second)) {
            EXPECT_TRUE(std::isnan(simd[i].second)) << "NaN not preserved at " << i;
        } else {
            uint64_t rb = 0, sb = 0;
            std::memcpy(&rb, &ref[i].second,  8);
            std::memcpy(&sb, &simd[i].second, 8);
            EXPECT_EQ(rb, sb) << "value mismatch at " << i;
        }
    }
}

TEST_F(GorillaSIMDTest, ZeroValues) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 100; ++i)
        pts.emplace_back(t0 + i * 1000, 0.0);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "ZeroValues");
}

TEST_F(GorillaSIMDTest, NegativeValues) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 50; ++i)
        pts.emplace_back(t0 + i * 1000, -static_cast<double>(i));
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "NegativeValues");
}

// ── Timestamp edge cases ─────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, LargeTimestampGap) {
    std::vector<std::pair<int64_t, double>> pts = {
        {1000000000000LL, 1.0},
        {1000000001000LL, 2.0},
        {2000000000000LL, 3.0},  // ~11.5 day gap
        {2000000001000LL, 4.0},
    };
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "LargeTimestampGap");
}

TEST_F(GorillaSIMDTest, IrregularIntervals) {
    std::mt19937 rng(7);
    std::uniform_int_distribution<int64_t> jitter(500, 2000);
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t = 1700000000000LL;
    for (int i = 0; i < 500; ++i) {
        pts.emplace_back(t, static_cast<double>(i));
        t += jitter(rng);
    }
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "IrregularIntervals");
}

TEST_F(GorillaSIMDTest, RegularOneSecondIntervals) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 1000; ++i)
        pts.emplace_back(t0 + i * 1000, static_cast<double>(i));
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "RegularIntervals");
}

// ── Precision ────────────────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, DoublePrecisionPreserved) {
    std::vector<std::pair<int64_t, double>> pts = {
        {1700000000000LL, 3.141592653589793},
        {1700000001000LL, 2.718281828459045},
        {1700000002000LL, 1.618033988749895},
        {1700000003000LL, 0.123456789012345},
        {1700000004000LL, 123456789.987654321},
    };
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "Precision");
}

// ── decodedCount / hasError API ──────────────────────────────────────────────

TEST_F(GorillaSIMDTest, DecodedCountMatchesPointCount) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 50; ++i)
        pts.emplace_back(t0 + i * 1000, static_cast<double>(i));
    auto bytes = encode(pts);

    GorillaSIMDDecoder dec(bytes);
    std::vector<std::pair<int64_t, double>> out;
    size_t n = dec.decodeAll(out);
    EXPECT_EQ(n, 50u);
    EXPECT_EQ(dec.decodedCount(), 50u);
    EXPECT_FALSE(dec.hasError());
}

TEST_F(GorillaSIMDTest, TruncatedDataHandledGracefully) {
    // Data truncated to 1 byte forces an incomplete first-point parse.
    // The decoder must not crash, must return 0 points (no partial garbage),
    // and must set the hasError() flag.
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 20; ++i)
        pts.emplace_back(t0 + i * 1000, static_cast<double>(i));
    auto full_bytes = encode(pts);

    // Truncate to a single byte — definitely mid-first-point.
    auto truncated = full_bytes;
    truncated.resize(1);

    GorillaSIMDDecoder dec(truncated);
    std::vector<std::pair<int64_t, double>> out;
    size_t n = dec.decodeAll(out);

    EXPECT_EQ(n, 0u) << "No points should be emitted when first-point parse failed";
    EXPECT_TRUE(out.empty()) << "Output vector must remain empty on total failure";
    EXPECT_TRUE(dec.hasError()) << "Error flag must be set for truncated stream";
}

TEST_F(GorillaSIMDTest, TruncatedDataMidPointSetsErrorFlag) {
    // Encode two points; keep only the first point's bytes plus one extra
    // byte from the second point's ZigZag to guarantee mid-point truncation.
    std::vector<std::pair<int64_t, double>> pts = {
        {1700000000000LL, 1.0},
        {1700000001000LL, 2.0},
        {1700000002000LL, 3.0},
    };
    auto full_bytes = encode(pts);
    // The first point occupies ZigZag(ts) + 64 bits. Truncate to that
    // length plus one byte (into the second point's ZigZag), which means
    // the second point's value bits are missing.
    GorillaEncoder one_pt;
    one_pt.add(pts[0].first, pts[0].second);
    auto first_point_bytes = one_pt.finish();
    // Take first-point bytes + 1 byte more from the full stream:
    //   that 1 extra byte is the start of the second point's ZigZag.
    //   The second point's value is truncated → parse error.
    auto partial = full_bytes;
    partial.resize(first_point_bytes.size() + 1);

    GorillaSIMDDecoder dec(partial);
    std::vector<std::pair<int64_t, double>> out;
    dec.decodeAll(out);
    EXPECT_TRUE(dec.hasError());
}

// ── Output appended (not overwritten) ────────────────────────────────────────

TEST_F(GorillaSIMDTest, DecodeAllAppendsToExistingOutput) {
    std::vector<std::pair<int64_t, double>> pts = {
        {1700000000000LL, 1.0},
        {1700000001000LL, 2.0},
    };
    auto bytes = encode(pts);

    std::vector<std::pair<int64_t, double>> out;
    out.emplace_back(0LL, 0.0);  // pre-existing sentinel

    GorillaSIMDDecoder dec(bytes);
    size_t n = dec.decodeAll(out);

    EXPECT_EQ(n, 2u);
    EXPECT_EQ(out.size(), 3u);         // sentinel + 2 decoded
    EXPECT_EQ(out[0].first, 0LL);      // sentinel untouched
}

// ── Realistic sensor workload ────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, RealisticSensorWorkload) {
    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, 0.1);
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t = 1700000000000LL;
    double temp = 20.0;
    for (int i = 0; i < 5000; ++i) {
        temp += noise(rng);
        pts.emplace_back(t, temp);
        t += 1000;
    }
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "SensorWorkload");

    // Compression sanity check
    size_t raw = pts.size() * 16u;
    EXPECT_LT(bytes.size(), raw) << "Gorilla should compress this data";
}

// ── Non-multiple-of-4 sizes (exercises SIMD tail handling) ───────────────────

TEST_F(GorillaSIMDTest, NonMultipleOf4Size_5Points) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 5; ++i)
        pts.emplace_back(t0 + i * 1000, i * 1.5);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "5Points");
}

TEST_F(GorillaSIMDTest, NonMultipleOf4Size_7Points) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 7; ++i)
        pts.emplace_back(t0 + i * 1000, i * 1.5);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "7Points");
}

TEST_F(GorillaSIMDTest, NonMultipleOf4Size_9Points) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 9; ++i)
        pts.emplace_back(t0 + i * 1000, i * 1.5);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "9Points");
}

TEST_F(GorillaSIMDTest, ExactlyFourPoints) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 4; ++i)
        pts.emplace_back(t0 + i * 1000, i * 1.5);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "4Points");
}

// ── Very large values ────────────────────────────────────────────────────────

TEST_F(GorillaSIMDTest, VeryLargeDoubleValues) {
    std::vector<std::pair<int64_t, double>> pts;
    int64_t t0 = 1700000000000LL;
    for (int i = 1; i <= 50; ++i)
        pts.emplace_back(t0 + i * 1000, std::numeric_limits<double>::max() / i);
    auto bytes = encode(pts);
    expect_eq(decode_scalar(bytes), decode_simd(bytes), "VeryLargeValues");
}

TEST_F(GorillaSIMDTest, ParityPropertyRandomSeries1k) {
    std::mt19937_64 rng(0xC0FFEE1234ULL);
    std::uniform_int_distribution<int> length_dist(1, 256);
    std::uniform_int_distribution<int64_t> dt_dist(1, 5000);
    std::uniform_real_distribution<double> value_dist(-1e6, 1e6);

    for (int series_idx = 0; series_idx < 1000; ++series_idx) {
        const int len = length_dist(rng);
        std::vector<std::pair<int64_t, double>> pts;
        pts.reserve(static_cast<size_t>(len));

        int64_t ts = 1700000000000LL + static_cast<int64_t>(series_idx) * 1000000LL;
        for (int i = 0; i < len; ++i) {
            ts += dt_dist(rng);
            pts.emplace_back(ts, value_dist(rng));
        }

        const auto bytes = encode(pts);
        const auto ref = decode_scalar(bytes);
        const auto simd = decode_simd(bytes);
        expect_eq(ref, simd, "ParityPropertyRandomSeries1k");
    }
}
