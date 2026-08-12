// Phase 1: Gorilla Codec – Error Recovery & Validation Tests
// Validates GorillaDecoder handles corrupt/truncated data gracefully

#include <gtest/gtest.h>
#include "timeseries/gorilla.h"
#include <cmath>
#include <limits>
#include <vector>
#include <random>
#include <algorithm>

using namespace themis;

class GorillaErrorRecoveryTest : public ::testing::Test {
protected:
    std::vector<std::pair<int64_t, double>> makeSeriesN(int n,
                                                         int64_t t0 = 1700000000000LL,
                                                         int64_t step_ms = 1000) {
        std::vector<std::pair<int64_t, double>> s;
        s.reserve(n);
        for (int i = 0; i < n; ++i) {
            s.push_back({t0 + i * step_ms, std::sin(i * 0.05)});
        }
        return s;
    }

    std::vector<uint8_t> encode(const std::vector<std::pair<int64_t, double>>& s) {
        GorillaEncoder enc;
        for (auto& p : s) enc.add(p.first, p.second);
        return enc.finish();
    }
};

// ===== Empty / Minimal Input =====

TEST_F(GorillaErrorRecoveryTest, EmptyBytesReturnsNullopt) {
    std::vector<uint8_t> empty;
    GorillaDecoder dec(empty);
    EXPECT_FALSE(dec.next().has_value());
    EXPECT_FALSE(dec.hasError());
    EXPECT_EQ(dec.decodedCount(), 0u);
}

TEST_F(GorillaErrorRecoveryTest, SingleByteReturnsNullopt) {
    std::vector<uint8_t> data = {0x01};
    GorillaDecoder dec(data);
    EXPECT_FALSE(dec.next().has_value());
}

TEST_F(GorillaErrorRecoveryTest, SinglePointRoundtrip) {
    GorillaEncoder enc;
    enc.add(1700000000000LL, 3.14159);
    auto bytes = enc.finish();
    GorillaDecoder dec(bytes);
    auto p = dec.next();
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->first, 1700000000000LL);
    EXPECT_NEAR(p->second, 3.14159, 1e-12);
    EXPECT_EQ(dec.decodedCount(), 1u);
    EXPECT_FALSE(dec.next().has_value());
}

// ===== Truncated Data Recovery =====

TEST_F(GorillaErrorRecoveryTest, TruncatedAfterFirstPoint) {
    auto series = makeSeriesN(20);
    auto bytes = encode(series);
    // Keep only first ~10 bytes (enough for 1 point header + first timestamp/value)
    bytes.resize(std::min(bytes.size(), size_t(12)));
    GorillaDecoder dec(bytes);
    // Should get at least partial result without crash
    int count = 0;
    while (auto p = dec.next()) { ++count; }
    EXPECT_GE(count, 0);
}

TEST_F(GorillaErrorRecoveryTest, TruncatedAtHalf) {
    auto series = makeSeriesN(100);
    auto bytes = encode(series);
    bytes.resize(bytes.size() / 2);
    GorillaDecoder dec(bytes);
    int count = 0;
    while (auto p = dec.next()) {
        ++count;
    }
    // Truncated data yields fewer points than original
    EXPECT_LT(count, 100);
    EXPECT_EQ(dec.decodedCount(), static_cast<size_t>(count));
}

TEST_F(GorillaErrorRecoveryTest, TruncatedToOneByteAfterValid) {
    auto series = makeSeriesN(5);
    auto bytes = encode(series);
    // Truncate to leave only first few bytes beyond the header
    if (bytes.size() > 15) bytes.resize(15);
    GorillaDecoder dec(bytes);
    int count = 0;
    while (auto p = dec.next()) ++count;
    EXPECT_GE(count, 0);
}

// ===== Corrupted Data =====

TEST_F(GorillaErrorRecoveryTest, CorruptedMiddleBytesNocrash) {
    auto series = makeSeriesN(50);
    auto bytes = encode(series);
    // Corrupt middle bytes
    for (size_t i = bytes.size() / 3; i < 2 * bytes.size() / 3; ++i) {
        bytes[i] ^= 0xFF;
    }
    GorillaDecoder dec(bytes);
    int count = 0;
    // Must not crash regardless of data corruption
    while (auto p = dec.next()) { ++count; }
    // Some valid points may be decoded, some may not
    EXPECT_GE(count, 0);
}

TEST_F(GorillaErrorRecoveryTest, AllZeroesNoCrash) {
    std::vector<uint8_t> zeros(64, 0x00);
    GorillaDecoder dec(zeros);
    int count = 0;
    while (auto p = dec.next()) { ++count; if (count > 200) break; }
    // Should terminate eventually (no infinite loop)
    EXPECT_LT(count, 200);
}

TEST_F(GorillaErrorRecoveryTest, AllOnesByteNoCrash) {
    std::vector<uint8_t> ones(64, 0xFF);
    GorillaDecoder dec(ones);
    int count = 0;
    while (auto p = dec.next()) { ++count; if (count > 200) break; }
    EXPECT_LT(count, 200);
}

// ===== hasError() validation =====

TEST_F(GorillaErrorRecoveryTest, HasErrorFalseForCleanDecode) {
    auto series = makeSeriesN(10);
    auto bytes = encode(series);
    GorillaDecoder dec(bytes);
    while (dec.next().has_value()) {}
    EXPECT_FALSE(dec.hasError());
    EXPECT_EQ(dec.decodedCount(), 10u);
}

TEST_F(GorillaErrorRecoveryTest, DecodedCountMatchesSeries) {
    for (int n : {1, 5, 10, 50, 200}) {
        auto series = makeSeriesN(n);
        auto bytes = encode(series);
        GorillaDecoder dec(bytes);
        while (dec.next().has_value()) {}
        EXPECT_EQ(dec.decodedCount(), static_cast<size_t>(n)) << "n=" << n;
    }
}

// ===== Re-use encoder after finish =====

TEST_F(GorillaErrorRecoveryTest, EncoderFinishReturnsConsistentBytes) {
    GorillaEncoder enc;
    enc.add(1700000000000LL, 1.0);
    enc.add(1700000001000LL, 2.0);
    auto b1 = enc.finish();
    // finish() should flush partial bits; result must be decodable
    GorillaDecoder dec(b1);
    auto p1 = dec.next();
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(p1->first, 1700000000000LL);
}

// ===== IEEE-754 Special Values =====

TEST_F(GorillaErrorRecoveryTest, NaNRoundtrip) {
    GorillaEncoder enc;
    enc.add(1700000000000LL, std::numeric_limits<double>::quiet_NaN());
    auto bytes = enc.finish();
    GorillaDecoder dec(bytes);
    auto p = dec.next();
    ASSERT_TRUE(p.has_value());
    EXPECT_TRUE(std::isnan(p->second));
}

TEST_F(GorillaErrorRecoveryTest, InfinityRoundtrip) {
    GorillaEncoder enc;
    enc.add(1700000000000LL, std::numeric_limits<double>::infinity());
    enc.add(1700000001000LL, -std::numeric_limits<double>::infinity());
    auto bytes = enc.finish();
    GorillaDecoder dec(bytes);
    auto p1 = dec.next();
    auto p2 = dec.next();
    ASSERT_TRUE(p1.has_value());
    ASSERT_TRUE(p2.has_value());
    EXPECT_TRUE(std::isinf(p1->second) && p1->second > 0);
    EXPECT_TRUE(std::isinf(p2->second) && p2->second < 0);
}

TEST_F(GorillaErrorRecoveryTest, MinMaxDoubleRoundtrip) {
    GorillaEncoder enc;
    enc.add(1700000000000LL, std::numeric_limits<double>::max());
    enc.add(1700000001000LL, std::numeric_limits<double>::min());
    enc.add(1700000002000LL, std::numeric_limits<double>::lowest());
    auto bytes = enc.finish();
    GorillaDecoder dec(bytes);
    auto p1 = dec.next();
    auto p2 = dec.next();
    auto p3 = dec.next();
    ASSERT_TRUE(p1.has_value());
    ASSERT_TRUE(p2.has_value());
    ASSERT_TRUE(p3.has_value());
    EXPECT_DOUBLE_EQ(p1->second, std::numeric_limits<double>::max());
    EXPECT_DOUBLE_EQ(p2->second, std::numeric_limits<double>::min());
    EXPECT_DOUBLE_EQ(p3->second, std::numeric_limits<double>::lowest());
}

// ===== Timestamp edge cases =====

TEST_F(GorillaErrorRecoveryTest, NegativeTimestampRoundtrip) {
    GorillaEncoder enc;
    enc.add(-1000000LL, 1.0);
    enc.add(-500000LL, 2.0);
    enc.add(0LL, 3.0);
    auto bytes = enc.finish();
    GorillaDecoder dec(bytes);
    auto p1 = dec.next();
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(p1->first, -1000000LL);
    auto p2 = dec.next();
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(p2->first, -500000LL);
    auto p3 = dec.next();
    ASSERT_TRUE(p3.has_value());
    EXPECT_EQ(p3->first, 0LL);
}

TEST_F(GorillaErrorRecoveryTest, IrregularTimestepsRoundtrip) {
    // Non-uniform deltas (delta-of-delta should handle variable steps)
    std::vector<std::pair<int64_t, double>> series = {
        {1000, 1.0}, {1001, 2.0}, {1010, 3.0}, {1100, 4.0}, {2000, 5.0}
    };
    GorillaEncoder enc;
    for (auto& p : series) enc.add(p.first, p.second);
    auto bytes = enc.finish();
    GorillaDecoder dec(bytes);
    for (auto& expected : series) {
        auto got = dec.next();
        ASSERT_TRUE(got.has_value());
        EXPECT_EQ(got->first, expected.first);
        EXPECT_DOUBLE_EQ(got->second, expected.second);
    }
    EXPECT_FALSE(dec.next().has_value());
}

// ===== Compression ratio validation =====

TEST_F(GorillaErrorRecoveryTest, CompressionRatioConstantTimesteps) {
    // Constant step = delta-of-delta always 0 → great timestamp compression
    auto series = makeSeriesN(1000);
    auto bytes = encode(series);
    size_t raw = series.size() * (sizeof(int64_t) + sizeof(double));
    EXPECT_LT(bytes.size(), raw * 0.7) << "Expected at least 30% compression";
}

TEST_F(GorillaErrorRecoveryTest, CompressionRatioConstantValues) {
    // Constant value → XOR is 0 every time → great value compression
    std::vector<std::pair<int64_t, double>> series;
    for (int i = 0; i < 1000; ++i) series.push_back({1700000000000LL + i * 1000, 42.0});
    auto bytes = encode(series);
    size_t raw = series.size() * (sizeof(int64_t) + sizeof(double));
    EXPECT_LT(bytes.size(), raw * 0.3) << "Expected at least 70% compression for constant values";
}
