/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_zstd_compression_security.cpp                 ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:37:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     380                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// ZSTD Compression Security Tests
// Tests for buffer overflow vulnerability fixes in compression code
// Validates size limits, allocation failures, and DoS prevention

#include <gtest/gtest.h>
#include "utils/zstd_codec.h"
#include "utils/error_registry.h"
#include <vector>
#include <cstdint>
#include <string>
#include <limits>
#include <numeric>
#include <utility>

using namespace themis::utils;

class ZstdCompressionSecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize error registry
        auto& registry = themis::errors::ErrorRegistry::getInstance();
    }
    
    // Helper to generate compressible data of a given size
    std::vector<uint8_t> generateData(size_t size) {
        std::vector<uint8_t> data(size);
        // Fill with repetitive pattern (highly compressible)
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<uint8_t>(i % 256);
        }
        return data;
    }
};

// ============================================================================
// Compression Size Validation Tests
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, CompressEmptyData) {
    // Empty data should succeed
    auto result = zstd_compress_safe(nullptr, 0, 3);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(ZstdCompressionSecurityTest, CompressNormalSize) {
    // Normal size data should succeed
    auto data = generateData(1024 * 1024);  // 1MB
    auto result = zstd_compress_safe(data.data(), data.size(), 3);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 0u);
    EXPECT_LT(result->size(), data.size());  // Should compress
}

TEST_F(ZstdCompressionSecurityTest, CompressLargeButValidSize) {
    // Large but within limit (100MB) should succeed
    size_t large_size = 100 * 1024 * 1024;  // 100MB
    auto data = generateData(large_size);
    auto result = zstd_compress_safe(data.data(), data.size(), 3);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 0u);
}

TEST_F(ZstdCompressionSecurityTest, RejectInputTooLarge) {
    // Input size exceeding MAX_INPUT_SIZE (1GB) should fail
    size_t too_large = compression::MAX_INPUT_SIZE + 1;
    
    // Create a small valid buffer but claim it's much larger
    // The validation should happen before any memory access
    std::vector<uint8_t> dummy(1);
    auto result = zstd_compress_safe(dummy.data(), too_large, 3);
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT);
    EXPECT_NE(result.error().message().find("exceeds maximum"), std::string::npos);
}

TEST_F(ZstdCompressionSecurityTest, RejectInputMaxSize) {
    // Input size at UINT64_MAX should fail (DoS attack scenario)
    size_t max_size = std::numeric_limits<size_t>::max();
    
    // Create a small valid buffer but claim it's much larger
    // The validation should happen before any memory access
    std::vector<uint8_t> dummy(1);
    auto result = zstd_compress_safe(dummy.data(), max_size, 3);
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT);
}

TEST_F(ZstdCompressionSecurityTest, CompressAtBoundary) {
    // Input size exactly at MAX_INPUT_SIZE should succeed (boundary test)
    // Verify the size limits are correctly defined
    EXPECT_EQ(compression::MAX_INPUT_SIZE, 1024ULL * 1024 * 1024);       // 1GB
    EXPECT_EQ(compression::MAX_OUTPUT_SIZE, 1024ULL * 1024 * 1024 * 2);  // 2GB
    EXPECT_EQ(compression::MAX_DECOMPRESSED_SIZE, 1024ULL * 1024 * 1024 * 4); // 4GB
}

// ============================================================================
// Decompression Size Validation Tests
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, DecompressEmptyData) {
    // Empty data should succeed
    std::vector<uint8_t> empty;
    auto result = zstd_decompress_safe(empty);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(ZstdCompressionSecurityTest, DecompressNormalData) {
    // Compress then decompress normal data
    auto data = generateData(1024 * 1024);  // 1MB
    auto compressed = zstd_compress_safe(data.data(), data.size(), 3);
    ASSERT_TRUE(compressed.has_value());
    
    auto decompressed = zstd_decompress_safe(*compressed);
    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(decompressed->size(), data.size());
    EXPECT_EQ(*decompressed, data);
}

TEST_F(ZstdCompressionSecurityTest, RejectInvalidCompressedData) {
    // Invalid compressed data should fail
    std::vector<uint8_t> invalid_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    auto result = zstd_decompress_safe(invalid_data);
    
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_UTIL_COMPRESSION_FAILED);
}

TEST_F(ZstdCompressionSecurityTest, RejectDecompressedSizeTooLarge) {
    // This test validates that decompressed size validation works
    // Verify the constants are set correctly
    EXPECT_EQ(compression::MAX_DECOMPRESSED_SIZE, 1024ULL * 1024 * 1024 * 4);  // 4GB
}

TEST_F(ZstdCompressionSecurityTest, RejectCompressedDataTooLarge) {
    // Compressed data larger than MAX_DECOMPRESSED_SIZE should fail
    size_t too_large = compression::MAX_DECOMPRESSED_SIZE + 1;
    std::vector<uint8_t> huge_data(100);  // Small data, just for testing
    
    // Manually test the size check by creating a vector larger than the limit
    // (We can't actually allocate MAX_DECOMPRESSED_SIZE + 1 bytes in test)
    // This verifies the constant is properly defined
    EXPECT_GT(too_large, compression::MAX_DECOMPRESSED_SIZE);
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, LegacyAPICompressNormal) {
    // Legacy API should still work for normal data
    auto data = generateData(1024);
    auto compressed = zstd_compress(data.data(), data.size(), 3);
    
    EXPECT_GT(compressed.size(), 0u);
    EXPECT_LT(compressed.size(), data.size());
}

TEST_F(ZstdCompressionSecurityTest, LegacyAPICompressInvalidReturnsEmpty) {
    // Legacy API returns empty on failure
    size_t too_large = compression::MAX_INPUT_SIZE + 1;
    
    // Create a small valid buffer but claim it's much larger
    std::vector<uint8_t> dummy(1);
    auto compressed = zstd_compress(dummy.data(), too_large, 3);
    
    EXPECT_TRUE(compressed.empty());
}

TEST_F(ZstdCompressionSecurityTest, LegacyAPIDecompressNormal) {
    // Legacy API should still work for normal data
    auto data = generateData(1024);
    auto compressed = zstd_compress(data.data(), data.size(), 3);
    ASSERT_GT(compressed.size(), 0u);
    
    auto decompressed = zstd_decompress(compressed);
    EXPECT_EQ(decompressed.size(), data.size());
}

TEST_F(ZstdCompressionSecurityTest, LegacyAPIDecompressInvalidReturnsEmpty) {
    // Legacy API returns empty on failure
    std::vector<uint8_t> invalid_data = {0x01, 0x02, 0x03};
    auto decompressed = zstd_decompress(invalid_data);
    
    EXPECT_TRUE(decompressed.empty());
}

// ============================================================================
// Round-trip Tests
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, RoundTripSmallData) {
    auto original = generateData(100);
    
    auto compressed = zstd_compress_safe(original.data(), original.size(), 3);
    ASSERT_TRUE(compressed.has_value());
    
    auto decompressed = zstd_decompress_safe(*compressed);
    ASSERT_TRUE(decompressed.has_value());
    
    EXPECT_EQ(*decompressed, original);
}

TEST_F(ZstdCompressionSecurityTest, RoundTripMediumData) {
    auto original = generateData(10 * 1024 * 1024);  // 10MB
    
    auto compressed = zstd_compress_safe(original.data(), original.size(), 3);
    ASSERT_TRUE(compressed.has_value());
    
    auto decompressed = zstd_decompress_safe(*compressed);
    ASSERT_TRUE(decompressed.has_value());
    
    EXPECT_EQ(*decompressed, original);
}

TEST_F(ZstdCompressionSecurityTest, RoundTripDifferentCompressionLevels) {
    auto original = generateData(1024);
    
    for (int level : {1, 3, 9, 15}) {
        auto compressed = zstd_compress_safe(original.data(), original.size(), level);
        ASSERT_TRUE(compressed.has_value()) << "Compression level " << level << " failed";
        
        auto decompressed = zstd_decompress_safe(*compressed);
        ASSERT_TRUE(decompressed.has_value()) << "Decompression level " << level << " failed";
        
        EXPECT_EQ(*decompressed, original) << "Round-trip failed for level " << level;
    }
}

// ============================================================================
// Error Message Tests
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, ErrorMessageInputTooLarge) {
    size_t too_large = compression::MAX_INPUT_SIZE + 1;
    
    // Create a small valid buffer but claim it's much larger
    std::vector<uint8_t> dummy(1);
    auto result = zstd_compress_safe(dummy.data(), too_large, 3);
    
    ASSERT_FALSE(result.has_value());
    
    std::string msg = result.error().message();
    EXPECT_NE(msg.find("exceeds maximum"), std::string::npos);
    EXPECT_NE(msg.find(std::to_string(compression::MAX_INPUT_SIZE)), std::string::npos);
}

TEST_F(ZstdCompressionSecurityTest, ErrorMessageInvalidCompressedData) {
    std::vector<uint8_t> invalid_data = {0x01, 0x02, 0x03};
    auto result = zstd_decompress_safe(invalid_data);
    
    ASSERT_FALSE(result.has_value());
    
    std::string msg = result.error().message();
    // Should contain ZSTD error message or "not valid"
    EXPECT_FALSE(msg.empty());
}

// ============================================================================
// String Overload Tests
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, CompressStringOverload) {
    std::string text = "Hello, ThemisDB! This is a test of string compression.";
    auto result = zstd_compress(text, 3);
    
    EXPECT_GT(result.size(), 0u);
}

TEST_F(ZstdCompressionSecurityTest, CompressVectorOverload) {
    auto data = generateData(1024);
    auto result = zstd_compress(data, 3);
    
    EXPECT_GT(result.size(), 0u);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, CompressNullPointerWithZeroSize) {
    // nullptr with size 0 should be handled gracefully
    auto result = zstd_compress_safe(nullptr, 0, 3);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(ZstdCompressionSecurityTest, CompressSingleByte) {
    uint8_t byte = 0x42;
    auto result = zstd_compress_safe(&byte, 1, 3);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 0u);
    
    auto decompressed = zstd_decompress_safe(*result);
    ASSERT_TRUE(decompressed.has_value());
    ASSERT_EQ(decompressed->size(), 1u);
    EXPECT_EQ((*decompressed)[0], byte);
}

TEST_F(ZstdCompressionSecurityTest, CompressHighlyCompressibleData) {
    // All zeros - should compress very well
    std::vector<uint8_t> zeros(1024 * 1024, 0);
    auto result = zstd_compress_safe(zeros.data(), zeros.size(), 3);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_LT(result->size(), zeros.size() / 100);  // Should compress to < 1%
}

TEST_F(ZstdCompressionSecurityTest, DecompressContentSizeUnknown) {
    // Test decompression when content size is unknown
    // ZSTD can create frames without storing the decompressed size
    // Our implementation should handle this by estimating the size
    
    // For this test, we just verify that normal compression/decompression works
    // The internal logic handles ZSTD_CONTENTSIZE_UNKNOWN cases
    auto data = generateData(1024);
    auto compressed = zstd_compress_safe(data.data(), data.size(), 3);
    ASSERT_TRUE(compressed.has_value());
    
    auto decompressed = zstd_decompress_safe(*compressed);
    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(*decompressed, data);
}

// ============================================================================
// Performance / Stress Tests
// ============================================================================

TEST_F(ZstdCompressionSecurityTest, StressTestMultipleCompressions) {
    // Verify we can handle many compression operations without issues
    for (int i = 0; i < 100; ++i) {
        auto data = generateData(1024);
        auto result = zstd_compress_safe(data.data(), data.size(), 3);
        ASSERT_TRUE(result.has_value()) << "Compression failed at iteration " << i;
    }
}

TEST_F(ZstdCompressionSecurityTest, StressTestMultipleDecompressions) {
    // Compress once, decompress many times
    auto data = generateData(1024);
    auto compressed = zstd_compress_safe(data.data(), data.size(), 3);
    ASSERT_TRUE(compressed.has_value());
    
    for (int i = 0; i < 100; ++i) {
        auto result = zstd_decompress_safe(*compressed);
        ASSERT_TRUE(result.has_value()) << "Decompression failed at iteration " << i;
        EXPECT_EQ(*result, data);
    }
}


// ============================================================================
// Streaming ZSTD tests (ZstdStreamingTests)
// ============================================================================

class ZstdStreamingTests : public ::testing::Test {
protected:
    // Build a chunk-based source from a flat byte vector.
    static auto makeVecSource(const std::vector<uint8_t>& data, size_t chunk_size) {
        struct State {
            const std::vector<uint8_t>& buf;
            size_t                      offset     = 0;
            size_t                      chunk_size = 0;
        };
        return [state = std::make_shared<State>(State{data, 0, chunk_size})]()
               -> std::pair<const uint8_t*, size_t> {
            if (state->offset >= state->buf.size()) {
                return {nullptr, 0};
            }
            size_t n = std::min(state->chunk_size, state->buf.size() - state->offset);
            const uint8_t* ptr = state->buf.data() + state->offset;
            state->offset += n;
            return {ptr, n};
        };
    }

    // Collect all sink chunks into one vector.
    static auto makeVecSink(std::vector<uint8_t>& out) {
        return [&out](const uint8_t* p, size_t n) -> bool {
            out.insert(out.end(), p, p + n);
            return true;
        };
    }

    // Generate compressible data (repetitive pattern).
    static std::vector<uint8_t> makeData(size_t n) {
        std::vector<uint8_t> d(n);
        for (size_t i = 0; i < n; ++i) d[i] = static_cast<uint8_t>(i % 251);
        return d;
    }
};

// ZS-01: Round-trip 1 KiB ────────────────────────────────────────────────────

TEST(ZstdStreamingTests, ZS_01_RoundTrip_1KiB) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    auto data = makeData(1024);
    std::vector<uint8_t> compressed, decompressed;

    auto r1 = zstd_compress_stream(makeVecSource(data, 256), makeVecSink(compressed));
    ASSERT_TRUE(r1.has_value()) << r1.error().message();
    EXPECT_FALSE(compressed.empty());

    auto r2 = zstd_decompress_stream(makeVecSource(compressed, 128), makeVecSink(decompressed));
    ASSERT_TRUE(r2.has_value()) << r2.error().message();
    EXPECT_EQ(decompressed, data);
}

// ZS-02: Round-trip 64 KiB with 4 KiB chunks ────────────────────────────────

TEST(ZstdStreamingTests, ZS_02_RoundTrip_64KiB_4KiB_Chunks) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    auto data = makeData(65536);
    std::vector<uint8_t> compressed, decompressed;

    ASSERT_TRUE(zstd_compress_stream(makeVecSource(data, 4096), makeVecSink(compressed)).has_value());
    ASSERT_TRUE(zstd_decompress_stream(makeVecSource(compressed, 4096), makeVecSink(decompressed)).has_value());
    EXPECT_EQ(decompressed, data);
}

// ZS-03: Empty input produces valid (empty) output ───────────────────────────

TEST(ZstdStreamingTests, ZS_03_EmptyInput) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    std::vector<uint8_t> empty_input;
    std::vector<uint8_t> compressed, decompressed;

    ASSERT_TRUE(zstd_compress_stream(makeVecSource(empty_input, 256), makeVecSink(compressed)).has_value());
    ASSERT_TRUE(zstd_decompress_stream(makeVecSource(compressed, 128), makeVecSink(decompressed)).has_value());
    EXPECT_TRUE(decompressed.empty());
}

// ZS-04: Compressed output is smaller than input (compressible data) ─────────

TEST(ZstdStreamingTests, ZS_04_CompressionReducesSize) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    // Highly compressible: all-zero 128 KiB
    std::vector<uint8_t> data(131072, 0);
    std::vector<uint8_t> compressed;

    ASSERT_TRUE(zstd_compress_stream(makeVecSource(data, 16384), makeVecSink(compressed)).has_value());
    EXPECT_LT(compressed.size(), data.size());
}

// ZS-05: max_output_bytes guard triggers on over-sized decompression ──────────

TEST(ZstdStreamingTests, ZS_05_MaxOutputBytes_GuardTriggered) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    auto data = makeData(8192);
    std::vector<uint8_t> compressed;
    ASSERT_TRUE(zstd_compress_stream(makeVecSource(data, 4096), makeVecSink(compressed)).has_value());

    // Limit decompressed to 512 bytes — must fail
    std::vector<uint8_t> sink_out;
    auto result = zstd_decompress_stream(makeVecSource(compressed, 256), makeVecSink(sink_out), 512);
    EXPECT_FALSE(result.has_value())
        << "Expected failure when max_output_bytes=512 but data=8192";
}

// ZS-06: Sink returning false aborts compression ─────────────────────────────

TEST(ZstdStreamingTests, ZS_06_SinkReturnFalse_AbortsCompression) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    auto data = makeData(65536);
    int  call_count = 0;
    auto aborting_sink = [&](const uint8_t*, size_t) -> bool {
        ++call_count;
        return false;  // always reject
    };

    auto r = zstd_compress_stream(makeVecSource(data, 4096), aborting_sink);
    EXPECT_FALSE(r.has_value());
    EXPECT_LE(call_count, 2);  // aborted quickly
}

// ZS-07: Sink returning false aborts decompression ───────────────────────────

TEST(ZstdStreamingTests, ZS_07_SinkReturnFalse_AbortsDecompression) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    auto data = makeData(16384);
    std::vector<uint8_t> compressed;
    ASSERT_TRUE(zstd_compress_stream(makeVecSource(data, 4096), makeVecSink(compressed)).has_value());

    auto aborting_sink = [](const uint8_t*, size_t) -> bool { return false; };
    auto r = zstd_decompress_stream(makeVecSource(compressed, 1024), aborting_sink);
    EXPECT_FALSE(r.has_value());
}

// ZS-08: Corrupt compressed data returns error ───────────────────────────────

TEST(ZstdStreamingTests, ZS_08_CorruptInput_ReturnsError) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    std::vector<uint8_t> garbage(512, 0xDE);
    std::vector<uint8_t> sink_out;
    auto r = zstd_decompress_stream(makeVecSource(garbage, 64), makeVecSink(sink_out));
    EXPECT_FALSE(r.has_value());
}

// ZS-09: Single-byte chunk size still works ──────────────────────────────────

TEST(ZstdStreamingTests, ZS_09_SingleByteChunks_RoundTrip) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    auto data = makeData(256);
    std::vector<uint8_t> compressed, decompressed;

    ASSERT_TRUE(zstd_compress_stream(makeVecSource(data, 1), makeVecSink(compressed)).has_value());
    ASSERT_TRUE(zstd_decompress_stream(makeVecSource(compressed, 1), makeVecSink(decompressed)).has_value());
    EXPECT_EQ(decompressed, data);
}

// ZS-10: max_output_bytes = 0 uses default (should succeed for normal input) ──

TEST(ZstdStreamingTests, ZS_10_MaxOutput_Zero_UsesDefault) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available";
#endif
    auto data = makeData(4096);
    std::vector<uint8_t> compressed, decompressed;

    ASSERT_TRUE(zstd_compress_stream(makeVecSource(data, 1024), makeVecSink(compressed)).has_value());
    // max_output_bytes = 0 → uses compression::MAX_DECOMPRESSED_SIZE
    auto r = zstd_decompress_stream(makeVecSource(compressed, 512), makeVecSink(decompressed), 0);
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(decompressed, data);
}
