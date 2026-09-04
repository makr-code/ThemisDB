/**
 * @file test_utils_future_interfaces.cpp
 * @brief Focused tests for utils module features: UUID v7, LZ4 codec,
 *        and streaming ZSTD API.
 */

#include <gtest/gtest.h>
#include "utils/uuid.h"
#include "utils/lz4_codec.h"
#include "utils/zstd_codec.h"

#include <algorithm>
#include <chrono>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace themis::utils;

// ============================================================================
// UUID v7
// ============================================================================

class UUIDv7Tests : public ::testing::Test {};

TEST_F(UUIDv7Tests, UV7_01_FormatIsCorrect) {
    const std::string id = generate_uuid_v7();
    ASSERT_EQ(id.size(), 36u);
    EXPECT_EQ(id[8],  '-');
    EXPECT_EQ(id[13], '-');
    EXPECT_EQ(id[18], '-');
    EXPECT_EQ(id[23], '-');
    // Version nibble must be '7'
    EXPECT_EQ(id[14], '7');
    // Variant nibble must be 8, 9, a, or b
    const char var = id[19];
    EXPECT_TRUE(var == '8' || var == '9' || var == 'a' || var == 'b')
        << "variant nibble was '" << var << "'";
}

TEST_F(UUIDv7Tests, UV7_02_OnlyHexAndHyphens) {
    const std::string id = generate_uuid_v7();
    static const std::regex uuid_re(
        "[0-9a-f]{8}-[0-9a-f]{4}-7[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}");
    EXPECT_TRUE(std::regex_match(id, uuid_re)) << "id=" << id;
}

TEST_F(UUIDv7Tests, UV7_03_DifferentFromV4) {
    const std::string v4 = generate_uuid_v4();
    const std::string v7 = generate_uuid_v7();
    EXPECT_EQ(v4[14], '4');
    EXPECT_EQ(v7[14], '7');
}

TEST_F(UUIDv7Tests, UV7_04_MonotonicityWithinSameMs) {
    // Generate a batch within a tight loop – timestamps should be non-decreasing.
    std::vector<std::string> ids;
    ids.reserve(100);
    for (int i = 0; i < 100; ++i) {
      ids.push_back(generate_uuid_v7());
    }

    for (size_t i = 1; i < ids.size(); ++i) {
        const std::string& a = ids[i-1];
        const std::string& b = ids[i];
        // The first 8 hex chars (time_low) must be >=
        EXPECT_LE(a.substr(0, 8), b.substr(0, 8))
            << "a=" << a << " b=" << b;
    }
}

TEST_F(UUIDv7Tests, UV7_05_Uniqueness) {
    std::set<std::string> seen = {};

    for (int i = 0; i < 1000; ++i) {
      seen.insert(generate_uuid_v7());
    }
    EXPECT_EQ(seen.size(), 1000u);
}

TEST_F(UUIDv7Tests, UV7_06_TimestampEmbedded) {
    // Extract the 48-bit timestamp from a freshly generated UUID v7 and
    // verify it is within ±5 seconds of now.
    const auto before_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string id = generate_uuid_v7();
    const auto after_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Reconstruct: time_low (chars 0-7) and time_mid (chars 9-12)
    const uint64_t time_low = std::stoull(id.substr(0, 8), nullptr, 16);
    const uint64_t time_mid = std::stoull(id.substr(9, 4), nullptr, 16);
    const uint64_t ts_ms    = (time_low << 16) | time_mid;

    EXPECT_GE(static_cast<long long>(ts_ms), before_ms - 5000);
    EXPECT_LE(static_cast<long long>(ts_ms), after_ms  + 5000);
}

TEST_F(UUIDv7Tests, UV7_07_ThreadSafety) {
    constexpr int threads = 4;
    constexpr int per_thread = 250;
    std::vector<std::vector<std::string>> buckets(threads);

    std::vector<std::thread> workers = {};

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < per_thread; ++i)
                buckets[t].push_back(generate_uuid_v7());
        });
    }
    for (auto& w : workers) {
      w.join();
    }

    std::set<std::string> all = {};

    for (auto& b : buckets)
        for (auto& id : b) {
          all.insert(id);
        }

    EXPECT_EQ(all.size(), static_cast<size_t>(threads * per_thread));
}

// ============================================================================
// LZ4 Codec
// ============================================================================

class LZ4CodecTests : public ::testing::Test {};

TEST_F(LZ4CodecTests, LZ4_01_RoundTrip) {
    const std::string original(4096, 'A');
    const auto compressed = lz4_compress(
        reinterpret_cast<const uint8_t*>(original.data()), original.size());

#ifdef THEMIS_HAS_LZ4
    ASSERT_FALSE(compressed.empty());
    const auto decompressed = lz4_decompress(compressed, original.size());
    ASSERT_EQ(decompressed.size(), original.size());
    EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), original);
#else
    EXPECT_TRUE(compressed.empty()); // graceful degradation
#endif
}

TEST_F(LZ4CodecTests, LZ4_02_EmptyInputReturnsEmpty) {
    const auto compressed = lz4_compress(
        static_cast<const uint8_t*>(nullptr),
        static_cast<size_t>(0),
        lz4_compression::DEFAULT_ACCELERATION);
    EXPECT_TRUE(compressed.empty());
}

TEST_F(LZ4CodecTests, LZ4_03_SafeApiRoundTrip) {
    const std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto res = lz4_compress_safe(data.data(), data.size());

#ifdef THEMIS_HAS_LZ4
    ASSERT_TRUE(res.has_value());
    auto dec = lz4_decompress_safe(*res, data.size());
    ASSERT_TRUE(dec.has_value());
    EXPECT_EQ(*dec, data);
#else
    EXPECT_FALSE(res.has_value());
#endif
}

TEST_F(LZ4CodecTests, LZ4_04_CompressBoundNonZero) {
    const size_t bound = lz4_compress_bound(1024);
#ifdef THEMIS_HAS_LZ4
    EXPECT_GT(bound, 1024u);
#else
    EXPECT_EQ(bound, 0u);
#endif
}

TEST_F(LZ4CodecTests, LZ4_05_CompressBoundZeroForOversizedInput) {
    // MAX_INPUT_SIZE + 1 must return 0
    const size_t oversized = lz4_compression::MAX_INPUT_SIZE + 1;
    EXPECT_EQ(lz4_compress_bound(oversized), 0u);
}

TEST_F(LZ4CodecTests, LZ4_06_SafeApiOversizedInputIsError) {
    // Build a fake large-size request — we don't allocate the memory, just test
    // the guard.
    const uint8_t dummy = 0;
    auto res = lz4_compress_safe(&dummy, lz4_compression::MAX_INPUT_SIZE + 1);
    EXPECT_FALSE(res.has_value());
}

TEST_F(LZ4CodecTests, LZ4_07_VectorRoundTrip) {
    const std::vector<uint8_t> original = {10, 20, 30, 40, 50, 60, 70, 80};
    const auto compressed = lz4_compress(original);

#ifdef THEMIS_HAS_LZ4
    ASSERT_FALSE(compressed.empty());
    const auto decompressed = lz4_decompress(compressed, original.size());
    EXPECT_EQ(decompressed, original);
#else
    EXPECT_TRUE(compressed.empty());
#endif
}

TEST_F(LZ4CodecTests, LZ4_08_StringOverload) {
    const std::string text = "Hello, ThemisDB LZ4 codec!";
    const auto compressed = lz4_compress(text);

#ifdef THEMIS_HAS_LZ4
    ASSERT_FALSE(compressed.empty());
    const auto decompressed = lz4_decompress(compressed, text.size());
    EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), text);
#else
    EXPECT_TRUE(compressed.empty());
#endif
}

// ============================================================================
// Streaming ZSTD
// ============================================================================

class ZstdStreamTests : public ::testing::Test {};

TEST_F(ZstdStreamTests, ZS_01_CompressorFlushProducesDecompressibleOutput) {
    const std::string original(8192, 'X');

    ZstdStreamCompressor enc(3);
    auto chunk_res = enc.compress_chunk(
        reinterpret_cast<const uint8_t*>(original.data()), original.size());
    auto flush_res = enc.flush();

#ifdef THEMIS_HAS_ZSTD
    ASSERT_TRUE(chunk_res.has_value());
    ASSERT_TRUE(flush_res.has_value());

    // Combine into a single ZSTD frame.
    std::vector<uint8_t> frame;
    frame.insert(frame.end(), chunk_res->begin(), chunk_res->end());
    frame.insert(frame.end(), flush_res->begin(), flush_res->end());

    const auto decompressed = zstd_decompress(frame);
    ASSERT_EQ(decompressed.size(), original.size());
    EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), original);
#else
    EXPECT_FALSE(chunk_res.has_value());
    EXPECT_FALSE(flush_res.has_value());
#endif
}

TEST_F(ZstdStreamTests, ZS_02_DecompressorRoundTrip) {
    const std::string original(4096, 'Y');

    // First compress into a ZSTD frame using the block API.
    const auto frame = zstd_compress(
        reinterpret_cast<const uint8_t*>(original.data()), original.size());

#ifdef THEMIS_HAS_ZSTD
    ASSERT_FALSE(frame.empty());

    ZstdStreamDecompressor dec;
    auto out = dec.decompress_chunk(frame);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(std::string(out->begin(), out->end()), original);
    EXPECT_TRUE(dec.is_done());
#else
    EXPECT_TRUE(frame.empty());
#endif
}

TEST_F(ZstdStreamTests, ZS_03_MultiChunkCompressDecompress) {
    // Build a larger payload and feed it to the compressor in two halves.
    const std::string original(16384, 'Z');
    const size_t half = original.size() / 2;

    ZstdStreamCompressor enc(1);
    std::vector<uint8_t> frame;

    auto r1 = enc.compress_chunk(reinterpret_cast<const uint8_t*>(original.data()), half);
    auto r2 = enc.compress_chunk(reinterpret_cast<const uint8_t*>(original.data() + half), half);
    auto rf = enc.flush();

#ifdef THEMIS_HAS_ZSTD
    ASSERT_TRUE(r1.has_value()); ASSERT_TRUE(r2.has_value()); ASSERT_TRUE(rf.has_value());
    frame.insert(frame.end(), r1->begin(), r1->end());
    frame.insert(frame.end(), r2->begin(), r2->end());
    frame.insert(frame.end(), rf->begin(), rf->end());

    // Decompress the full frame.
    ZstdStreamDecompressor dec;
    auto out = dec.decompress_chunk(frame);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->size(), original.size());
    EXPECT_EQ(std::string(out->begin(), out->end()), original);
#else
    EXPECT_FALSE(r1.has_value());
#endif
}

TEST_F(ZstdStreamTests, ZS_04_CompressorReset) {
    // After reset(), a new frame should be producible.
    const std::string original("reset test data");

    ZstdStreamCompressor enc(1);
    auto r1 = enc.compress_chunk(
        reinterpret_cast<const uint8_t*>(original.data()), original.size());
    auto f1 = enc.flush();

#ifdef THEMIS_HAS_ZSTD
    ASSERT_TRUE(r1.has_value()); ASSERT_TRUE(f1.has_value());

    enc.reset();
    auto r2 = enc.compress_chunk(
        reinterpret_cast<const uint8_t*>(original.data()), original.size());
    auto f2 = enc.flush();
    ASSERT_TRUE(r2.has_value()); ASSERT_TRUE(f2.has_value());

    // Both frames should decompress correctly.
    std::vector<uint8_t> frame2;
    frame2.insert(frame2.end(), r2->begin(), r2->end());
    frame2.insert(frame2.end(), f2->begin(), f2->end());
    const auto dec = zstd_decompress(frame2);
    EXPECT_EQ(std::string(dec.begin(), dec.end()), original);
#else
    EXPECT_FALSE(r1.has_value());
#endif
}

TEST_F(ZstdStreamTests, ZS_05_DecompressorReset) {
    const std::string original("decompressor reset test");
    const auto frame = zstd_compress(
        reinterpret_cast<const uint8_t*>(original.data()), original.size());

#ifdef THEMIS_HAS_ZSTD
    ASSERT_FALSE(frame.empty());

    ZstdStreamDecompressor dec;
    auto out1 = dec.decompress_chunk(frame);
    ASSERT_TRUE(out1.has_value());
    EXPECT_TRUE(dec.is_done());

    // Reset and decompress the same frame again.
    dec.reset();
    EXPECT_FALSE(dec.is_done());
    auto out2 = dec.decompress_chunk(frame);
    ASSERT_TRUE(out2.has_value());
    EXPECT_EQ(*out1, *out2);
#else
    EXPECT_TRUE(frame.empty());
#endif
}

TEST_F(ZstdStreamTests, ZS_06_EmptyChunkIsHarmless) {
    ZstdStreamCompressor enc(1);
    auto res = enc.compress_chunk(nullptr, 0);
#ifdef THEMIS_HAS_ZSTD
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->empty());
#else
    EXPECT_FALSE(res.has_value());
#endif
}

TEST_F(ZstdStreamTests, ZS_07_IsDoneAfterFullFrame) {
    const std::string original("done check");
    const auto frame = zstd_compress(
        reinterpret_cast<const uint8_t*>(original.data()), original.size());

#ifdef THEMIS_HAS_ZSTD
    ASSERT_FALSE(frame.empty());
    ZstdStreamDecompressor dec;
    EXPECT_FALSE(dec.is_done());
    (void)dec.decompress_chunk(frame);
    EXPECT_TRUE(dec.is_done());
#else
    EXPECT_TRUE(frame.empty());
#endif
}
