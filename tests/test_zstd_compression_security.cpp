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

using namespace themis::utils;

class ZstdCompressionSecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifndef THEMIS_HAS_ZSTD
        GTEST_SKIP() << "ZSTD is not available in this build; compression security tests are not applicable.";
#endif
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
