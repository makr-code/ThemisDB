/**
 * @file test_utils_compression_hardening.cpp
 * @brief Phase 4 hardening tests for zstd_codec and lz4_codec.
 *
 * Coverage targets (Phase 4 gate):
 *  - zstd_compress_safe / zstd_decompress_safe: round-trip correctness
 *  - zstd_decompress_safe: corrupt input returns Err, not UB
 *  - lz4_compress_safe / lz4_decompress_safe: round-trip correctness
 *  - lz4_decompress_safe: corrupt input returns Err
 *  - Concurrent zstd encode/decode safety (N threads)
 *  - Compression error codes in range 9060-9069
 *  - ErrorContext category for compression codes
 */

#include <gtest/gtest.h>

#include "utils/error_contracts.h"
#include "utils/expected.h"
#include "utils/lz4_codec.h"
#include "utils/zstd_codec.h"

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace themis::utils;

// ─────────────────────────────────────────────────────────────────────────────
// CC-01: zstd round-trip
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, ZstdRoundTripCorrectness) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available in this build";
#endif
    const std::string original = "Hello, ThemisDB ZSTD compression round-trip test!";
    auto compressed = zstd_compress_safe(
        reinterpret_cast<const uint8_t*>(original.data()),
        original.size(), 3);
    ASSERT_TRUE(compressed.has_value()) << "zstd_compress_safe failed";

    auto decompressed = zstd_decompress_safe(*compressed);
    ASSERT_TRUE(decompressed.has_value()) << "zstd_decompress_safe failed";

    std::string result(decompressed->begin(), decompressed->end());
    EXPECT_EQ(result, original);
}

// ─────────────────────────────────────────────────────────────────────────────
// CC-02: zstd corrupt input returns Err (not crash / UB)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, ZstdCorruptInputReturnsError) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available in this build";
#endif
    // Craft obviously corrupt zstd frame data
    std::vector<uint8_t> corrupt = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00};
    auto result = zstd_decompress_safe(corrupt);
    EXPECT_FALSE(result.has_value())
        << "Expected Err on corrupt ZSTD input, got Ok";
}

// ─────────────────────────────────────────────────────────────────────────────
// CC-03: zstd empty input compresses to valid (possibly empty) frame
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, ZstdEmptyInputHandled) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available in this build";
#endif
    auto compressed = zstd_compress_safe(nullptr, 0, 3);
    // Should either succeed (empty frame) or return Err – must not crash.
    (void)compressed;
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// CC-04: LZ4 round-trip correctness
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, LZ4RoundTripCorrectness) {
#ifndef THEMIS_HAS_LZ4
    GTEST_SKIP() << "LZ4 not available in this build";
#endif
    const std::string original = "ThemisDB LZ4 compression test data. Test test test.";
    auto compressed = lz4_compress_safe(
        reinterpret_cast<const uint8_t*>(original.data()),
        original.size());
    ASSERT_TRUE(compressed.has_value()) << "lz4_compress_safe failed";

    auto decompressed = lz4_decompress_safe(*compressed, original.size());
    ASSERT_TRUE(decompressed.has_value()) << "lz4_decompress_safe failed";

    std::string result(decompressed->begin(), decompressed->end());
    EXPECT_EQ(result, original);
}

// ─────────────────────────────────────────────────────────────────────────────
// CC-05: LZ4 corrupt input returns Err
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, LZ4CorruptInputReturnsError) {
#ifndef THEMIS_HAS_LZ4
    GTEST_SKIP() << "LZ4 not available in this build";
#endif
    std::vector<uint8_t> corrupt = {0xFF, 0xFF, 0xFF, 0xFF, 0xDE, 0xAD};
    // original_size deliberately mismatched (huge value)
    auto result = lz4_decompress_safe(corrupt, 1024 * 1024);
    EXPECT_FALSE(result.has_value())
        << "Expected Err on corrupt LZ4 input, got Ok";
}

// ─────────────────────────────────────────────────────────────────────────────
// CC-06: Concurrent zstd encode/decode is safe (N=8 threads)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, ZstdConcurrentEncodeDecodeSafe) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available in this build";
#endif
    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> failures{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t, &failures]() {
            std::string payload = "thread-" + std::to_string(t) + "-data-";
            payload.append(128, static_cast<char>('A' + t % 26));

            auto comp = zstd_compress_safe(
                reinterpret_cast<const uint8_t*>(payload.data()),
                payload.size(), 1);
            if (!comp.has_value()) { ++failures; return; }

            auto decomp = zstd_decompress_safe(*comp);
            if (!decomp.has_value()) { ++failures; return; }

            std::string result(decomp->begin(), decomp->end());
            if (result != payload) {
              ++failures;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(failures.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// CC-07: Compression error codes are in range 9060-9069
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, CompressionErrorCodesInRange) {
    using EC = ErrorCode;
    auto check = [](EC code) {
        auto v = static_cast<uint16_t>(code);
        EXPECT_GE(v, uint16_t{9060}) << "code " << v << " below 9060";
        EXPECT_LE(v, uint16_t{9069}) << "code " << v << " above 9069";
    };
    check(EC::COMPRESSION_FAILED);
    check(EC::DECOMPRESSION_FAILED);
    check(EC::COMPRESSION_BUFFER_SMALL);
    check(EC::COMPRESSION_INPUT_INVALID);
    check(EC::COMPRESSION_BOMB_DETECTED);
    check(EC::COMPRESSION_RATIO_EXCEEDED);
}

// ─────────────────────────────────────────────────────────────────────────────
// CC-08: ErrorContext category for compression codes is Compression
// ─────────────────────────────────────────────────────────────────────────────
TEST(CompressionHardening, ErrorContextCategoryIsCompression) {
    for (auto code : {ErrorCode::COMPRESSION_FAILED,
                      ErrorCode::DECOMPRESSION_FAILED,
                      ErrorCode::COMPRESSION_INPUT_INVALID}) {
        auto ctx = makeErrorContext(code, "CC-08-test", "unit test",
                                    ErrorSeverity::Error, false);
        EXPECT_EQ(ctx.category, ErrorCategory::ZstdCodec)
            << "code " << static_cast<uint16_t>(code)
            << " should be Compression category";
    }
}
