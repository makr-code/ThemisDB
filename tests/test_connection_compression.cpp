/// @file test_connection_compression.cpp
/// @brief Unit and round-trip tests for LZ4 and Zstd connection-level compression.
///
/// Tests cover:
/// - LZ4 compression / decompression round-trip
/// - Zstd compression / decompression round-trip
/// - Minimum payload size threshold (no-op on small data)
/// - Corrupted input handling (empty/truncated vectors)
/// - Zstd level variants
/// - Wire format: 4-byte original-size prefix

#include <gtest/gtest.h>
#include "network/connection_compression.h"

#include <numeric>
#include <string>

using namespace themis::network;

// =============================================================================
// Helper: build a compressible payload of the given size
// =============================================================================
static std::vector<uint8_t> makePayload(size_t size, uint8_t pattern = 0xAB)
{
    std::vector<uint8_t> data(size, pattern);
    // Vary bytes slightly so it resembles real data
    for (size_t i = 0; i < size; ++i)
        data[i] = static_cast<uint8_t>((pattern + i) & 0xFF);
    return data;
}

// =============================================================================
// LZ4 Tests
// =============================================================================

TEST(ConnectionCompressionLZ4, RoundTripBasic) {
    const auto original = makePayload(1024);
    const auto compressed = compressLZ4(original, 256);
    ASSERT_FALSE(compressed.empty()) << "LZ4 compression should succeed for 1 KiB input";

    const auto decompressed = decompressLZ4(compressed);
    ASSERT_EQ(decompressed.size(), original.size());
    EXPECT_EQ(decompressed, original);
}

TEST(ConnectionCompressionLZ4, RoundTripLargePayload) {
    const auto original = makePayload(64 * 1024); // 64 KiB
    const auto compressed = compressLZ4(original, 256);
    ASSERT_FALSE(compressed.empty());

    const auto decompressed = decompressLZ4(compressed);
    EXPECT_EQ(decompressed, original);
}

TEST(ConnectionCompressionLZ4, SkipsSmallPayload) {
    const auto tiny = makePayload(100);
    const auto compressed = compressLZ4(tiny, 256); // below threshold
    EXPECT_TRUE(compressed.empty()) << "Should skip compression for payloads below min_size";
}

TEST(ConnectionCompressionLZ4, ExactlyAtThreshold) {
    const auto data = makePayload(256);
    // Exactly at threshold: compression should be attempted
    const auto compressed = compressLZ4(data, 256);
    // Only check that we get a valid round-trip if compression succeeded
    if (!compressed.empty()) {
        const auto decompressed = decompressLZ4(compressed);
        EXPECT_EQ(decompressed, data);
    }
}

TEST(ConnectionCompressionLZ4, WireFormatHasOriginalSizePrefix) {
    const auto original = makePayload(512);
    const auto compressed = compressLZ4(original, 0 /* no threshold */);
    ASSERT_GE(compressed.size(), 4u);

    // First 4 bytes = original size as uint32_t LE
    uint32_t prefix = 0;
    std::memcpy(&prefix, compressed.data(), 4);
    EXPECT_EQ(prefix, static_cast<uint32_t>(original.size()));
}

TEST(ConnectionCompressionLZ4, DecompressEmptyInput) {
    const auto result = decompressLZ4({});
    EXPECT_TRUE(result.empty());
}

TEST(ConnectionCompressionLZ4, DecompressTruncatedInput) {
    std::vector<uint8_t> truncated = {0x00, 0x02}; // only 2 bytes, less than 4-byte prefix
    EXPECT_TRUE(decompressLZ4(truncated).empty());
}

TEST(ConnectionCompressionLZ4, DecompressCorruptedPayload) {
    // 4-byte header claiming large size, followed by garbage
    std::vector<uint8_t> corrupt(12, 0xFF);
    corrupt[0] = 0x00; corrupt[1] = 0x04; corrupt[2] = 0x00; corrupt[3] = 0x00; // 1 KiB
    // Remaining bytes are invalid compressed data
    EXPECT_TRUE(decompressLZ4(corrupt).empty());
}

// =============================================================================
// Zstd Tests
// =============================================================================

TEST(ConnectionCompressionZstd, RoundTripBasic) {
    const auto original = makePayload(1024);
    const auto compressed = compressZstd(original, 256, 3);
    ASSERT_FALSE(compressed.empty()) << "Zstd compression should succeed for 1 KiB input";

    const auto decompressed = decompressZstd(compressed);
    ASSERT_EQ(decompressed.size(), original.size());
    EXPECT_EQ(decompressed, original);
}

TEST(ConnectionCompressionZstd, RoundTripLargePayload) {
    const auto original = makePayload(64 * 1024); // 64 KiB
    const auto compressed = compressZstd(original, 256, 3);
    ASSERT_FALSE(compressed.empty());

    const auto decompressed = decompressZstd(compressed);
    EXPECT_EQ(decompressed, original);
}

TEST(ConnectionCompressionZstd, SkipsSmallPayload) {
    const auto tiny = makePayload(100);
    const auto compressed = compressZstd(tiny, 256, 3); // below threshold
    EXPECT_TRUE(compressed.empty()) << "Should skip compression for payloads below min_size";
}

TEST(ConnectionCompressionZstd, LevelVariants) {
    const auto original = makePayload(2048);
    for (int level : {1, 3, 6, 9}) {
        const auto compressed = compressZstd(original, 0, level);
        ASSERT_FALSE(compressed.empty()) << "Level " << level << " should succeed";
        const auto decompressed = decompressZstd(compressed);
        EXPECT_EQ(decompressed, original) << "Level " << level << " round-trip failed";
    }
}

TEST(ConnectionCompressionZstd, WireFormatHasOriginalSizePrefix) {
    const auto original = makePayload(512);
    const auto compressed = compressZstd(original, 0 /* no threshold */, 3);
    ASSERT_GE(compressed.size(), 4u);

    uint32_t prefix = 0;
    std::memcpy(&prefix, compressed.data(), 4);
    EXPECT_EQ(prefix, static_cast<uint32_t>(original.size()));
}

TEST(ConnectionCompressionZstd, DecompressEmptyInput) {
    EXPECT_TRUE(decompressZstd({}).empty());
}

TEST(ConnectionCompressionZstd, DecompressTruncatedInput) {
    std::vector<uint8_t> truncated = {0x00, 0x02};
    EXPECT_TRUE(decompressZstd(truncated).empty());
}

TEST(ConnectionCompressionZstd, DecompressCorruptedPayload) {
    std::vector<uint8_t> corrupt(12, 0xFF);
    corrupt[0] = 0x00; corrupt[1] = 0x04; corrupt[2] = 0x00; corrupt[3] = 0x00; // 1 KiB
    EXPECT_TRUE(decompressZstd(corrupt).empty());
}

// =============================================================================
// Cross-codec correctness: LZ4 ≠ Zstd output
// =============================================================================

TEST(ConnectionCompressionCross, LZ4andZstdProduceDifferentOutput) {
    const auto original = makePayload(1024);
    const auto lz4_out  = compressLZ4(original, 0);
    const auto zstd_out = compressZstd(original, 0, 3);

    ASSERT_FALSE(lz4_out.empty());
    ASSERT_FALSE(zstd_out.empty());
    // Compressed bytes must differ (different codecs, different format)
    EXPECT_NE(lz4_out, zstd_out);
}

TEST(ConnectionCompressionCross, LZ4CompressedCannotBeDecompressedByZstd) {
    const auto original   = makePayload(1024);
    const auto lz4_bytes  = compressLZ4(original, 0);
    ASSERT_FALSE(lz4_bytes.empty());

    // Feeding LZ4 output to Zstd decompressor must fail gracefully
    const auto result = decompressZstd(lz4_bytes);
    EXPECT_TRUE(result.empty());
}

TEST(ConnectionCompressionCross, ZstdCompressedCannotBeDecompressedByLZ4) {
    const auto original    = makePayload(1024);
    const auto zstd_bytes  = compressZstd(original, 0, 3);
    ASSERT_FALSE(zstd_bytes.empty());

    const auto result = decompressLZ4(zstd_bytes);
    // The result might be non-empty (LZ4 may not detect corruption), but
    // if it decompresses to anything it must NOT match the original.
    if (!result.empty()) {
        EXPECT_NE(result, original);
    }
}
