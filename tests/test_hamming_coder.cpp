/**
 * ThemisDB HammingCoder Tests
 *
 * Test suite for the HammingCoder erasure coder (RAID-2 / Hamming shard-level
 * error correction). Tests cover:
 *
 *   HC_01 — Encode produces (data_shards + parity_shards) equally-sized chunks
 *   HC_02 — Round-trip encode → decode with no failures recovers original data
 *   HC_03 — Decode with all data shards present (no parity needed)
 *   HC_04 — Single data-shard failure recovered via Hamming parity
 *   HC_05 — Single parity-shard failure recovered (recomputed from data)
 *   HC_06 — Multiple non-overlapping data-shard failures (iterative repair)
 *   HC_07 — All parity shards missing but all data shards present
 *   HC_08 — Input not a multiple of data_shards (padding / truncation correct)
 *   HC_09 — Single-byte input
 *   HC_10 — Large blob (1 MB) round-trip
 *   HC_11 — Hamming(7,4): canonical 4 data / 3 parity shards
 *   HC_12 — Invalid argument: empty data throws
 *   HC_13 — Too many missing data shards throws
 *   HC_14 — factory ErasureCoder::create(HAMMING) returns HammingCoder
 *   HC_15 — Encode parity_shards=1 (single XOR parity over all data shards)
 *   HC_16 — Parity coverage correctness: verify each parity bit assignment
 */

#include <gtest/gtest.h>

#include "sharding/redundancy_strategy.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::sharding;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint8_t> makeData(std::size_t size, uint8_t seed = 0xA5) {
    std::vector<uint8_t> d(size);
    for (std::size_t i = 0; i < size; ++i)
        d[i] = static_cast<uint8_t>((seed + i) & 0xFFu);
    return d;
}

// Encode then build available_chunks + missing_indices by dropping specific
// shards (by absolute index in the chunk vector returned by encode).
struct DropResult {
    std::map<uint32_t, std::vector<uint8_t>> available;
    std::vector<uint32_t>                    missing;
};

static DropResult dropShards(
    const std::vector<std::vector<uint8_t>>& chunks,
    const std::vector<uint32_t>&             drop_indices)
{
    DropResult r;
    const auto isDropped = [&](uint32_t idx) {
        return std::find(drop_indices.begin(), drop_indices.end(), idx) !=
               drop_indices.end();
    };
    for (uint32_t i = 0; i < static_cast<uint32_t>(chunks.size()); ++i) {
        if (isDropped(i))
            r.missing.push_back(i);
        else
            r.available[i] = chunks[i];
    }
    return r;
}

// Strip trailing zero-padding from a recovered blob given the original size.
static std::vector<uint8_t> stripPadding(
    const std::vector<uint8_t>& recovered,
    std::size_t                 original_size)
{
    if (recovered.size() <= original_size)
        return recovered = {};
    return std::vector<uint8_t>(recovered.begin(),
                                recovered.begin() +
                                static_cast<std::ptrdiff_t>(original_size));
}

// ---------------------------------------------------------------------------
// Test Fixture
// ---------------------------------------------------------------------------

class HammingCoderTest : public ::testing::Test {
protected:
    HammingCoder coder_;
};

// ---------------------------------------------------------------------------
// HC_01 — Chunk count and size
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_01_ChunkCountAndSize) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(400);
    auto chunks = coder_.encode(data, k, r);

    ASSERT_EQ(chunks.size(), k + r);

    // All chunks must have the same size = ceil(400 / 4) = 100
    const std::size_t expected_shard_size = (data.size() + k - 1) / k;
    for (const auto& chunk : chunks)
        EXPECT_EQ(chunk.size(), expected_shard_size);
}

// ---------------------------------------------------------------------------
// HC_02 — Round-trip: no failures
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_02_RoundTripNoFailures) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(397); // not a multiple of k
    auto chunks = coder_.encode(data, k, r);

    auto dr = dropShards(chunks, {});
    auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
    auto recovered = stripPadding(recovered_raw, data.size());

    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_03 — Decode with all data shards (no parity in available_chunks)
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_03_DecodeDataOnlyNoMissing) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(200);
    auto chunks = coder_.encode(data, k, r);

    // Only pass data shards
    std::map<uint32_t, std::vector<uint8_t>> avail;
    for (uint32_t i = 0; i < k; ++i)
        avail[i] = chunks[i];

    auto recovered_raw = coder_.decode(avail, {}, k, r);
    auto recovered = stripPadding(recovered_raw, data.size());

    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_04 — Single data-shard failure
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_04_SingleDataShardFailure) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(400);
    auto chunks = coder_.encode(data, k, r);

    for (uint32_t drop = 0; drop < k; ++drop) {
        auto dr = dropShards(chunks, {drop});
        auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
        auto recovered = stripPadding(recovered_raw, data.size());
        EXPECT_EQ(recovered, data)
            << "Failed to recover with data shard " << drop << " missing";
    }
}

// ---------------------------------------------------------------------------
// HC_05 — Single parity-shard failure
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_05_SingleParityShardFailure) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(400);
    auto chunks = coder_.encode(data, k, r);

    for (uint32_t drop = k; drop < k + r; ++drop) {
        auto dr = dropShards(chunks, {drop});
        auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
        auto recovered = stripPadding(recovered_raw, data.size());
        EXPECT_EQ(recovered, data)
            << "Failed with parity shard " << drop << " missing";
    }
}

// ---------------------------------------------------------------------------
// HC_06 — Multiple non-overlapping data-shard failures (iterative repair)
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_06_MultipleNonOverlappingFailures) {
    // With 4 data shards and 3 parity shards we can recover shards 0 and 1
    // because parity 0 (bit 0) covers shard 0 alone if we process iteratively.
    // The test checks that iterative Hamming repair works where coverage allows.
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(400);
    auto chunks = coder_.encode(data, k, r);

    // Drop data shard 0 (covered by parities 0) and data shard 3 (covered by parities 0,1)
    // Parity 0 covers shards whose (j+1) has bit 0 set: shards 0 (j+1=1), 2 (j+1=3) — bit0 set
    // With shard 2 present, parity 0 can recover shard 0.
    // Then parity 1 covers shards whose (j+1) has bit 1 set: shards 1(j+1=2),2(j+1=3),3(j+1=4 bit1=0 no)
    // Actually let me just test one known recoverable combination: drop shards 0 and 3.
    // Shard 0 (j+1=1): bit 0 set → covered by parity 0
    // Shard 3 (j+1=4): bit 2 set → covered by parity 2
    // Parities 0 and 2 cover different shards (no overlap for missing), so iterative repair works.
    auto dr = dropShards(chunks, {0u, 3u});
    ASSERT_EQ(dr.missing.size(), 2u);

    auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
    auto recovered = stripPadding(recovered_raw, data.size());
    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_07 — All parity shards missing, all data present
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_07_AllParityMissingDataPresent) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(400);
    auto chunks = coder_.encode(data, k, r);

    std::vector<uint32_t> drop = {};

    for (uint32_t i = k; i < k + r; ++i) {
      drop.push_back(i);
    }
    auto dr = dropShards(chunks, drop);

    auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
    auto recovered = stripPadding(recovered_raw, data.size());
    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_08 — Non-multiple input size (padding correctness)
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_08_NonMultipleInputSize) {
    for (uint32_t k = 2; k <= 6; ++k) {
        for (std::size_t sz : {1u, 3u, 7u, 13u, 100u, 333u}) {
            auto data = makeData(sz, static_cast<uint8_t>(k * 7));
            auto chunks = coder_.encode(data, k, /*parity=*/2);

            // Drop first data shard to exercise decode path
            auto dr = dropShards(chunks, {0u});
            auto recovered_raw = coder_.decode(dr.available, dr.missing, k, 2u);
            auto recovered = stripPadding(recovered_raw, data.size());
            EXPECT_EQ(recovered, data)
                << "k=" << k << " sz=" << sz;
        }
    }
}

// ---------------------------------------------------------------------------
// HC_09 — Single-byte input
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_09_SingleByteInput) {
    constexpr uint32_t k = 2, r = 2;
    std::vector<uint8_t> data = {0xBE};
    auto chunks = coder_.encode(data, k, r);
    ASSERT_EQ(chunks.size(), k + r);

    // Drop first data shard
    auto dr = dropShards(chunks, {0u});
    auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
    auto recovered = stripPadding(recovered_raw, data.size());
    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_10 — Large blob round-trip (1 MB)
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_10_LargeBlobRoundTrip) {
    constexpr uint32_t k = 8, r = 4;
    auto data = makeData(1024u * 1024u, 0x37);
    auto chunks = coder_.encode(data, k, r);
    ASSERT_EQ(chunks.size(), k + r);

    // Drop two independent data shards
    auto dr = dropShards(chunks, {2u, 5u});
    auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
    auto recovered = stripPadding(recovered_raw, data.size());
    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_11 — Canonical Hamming(7,4): 4 data / 3 parity shards
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_11_Hamming74Canonical) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(128);
    auto chunks = coder_.encode(data, k, r);
    ASSERT_EQ(chunks.size(), 7u);

    // Verify parity[0] = XOR of data shards covered by bit-0 of (j+1):
    // j=0 (j+1=1, bit0=1), j=2 (j+1=3, bit0=1)
    const std::size_t shard_size = chunks[0].size();
    std::vector<uint8_t> expected_p0(shard_size, 0);
    for (uint32_t j : {0u, 2u})
        for (std::size_t b = 0; b < shard_size; ++b)
            expected_p0[b] ^= chunks[j][b];
    EXPECT_EQ(chunks[k + 0], expected_p0);

    // Verify parity[1] = XOR of j=1 (j+1=2, bit1=1), j=2 (j+1=3, bit1=1)
    std::vector<uint8_t> expected_p1(shard_size, 0);
    for (uint32_t j : {1u, 2u})
        for (std::size_t b = 0; b < shard_size; ++b)
            expected_p1[b] ^= chunks[j][b];
    EXPECT_EQ(chunks[k + 1], expected_p1);

    // Verify parity[2] = XOR of j=3 (j+1=4, bit2=1)
    std::vector<uint8_t> expected_p2(shard_size, 0);
    for (uint32_t j : {3u})
        for (std::size_t b = 0; b < shard_size; ++b)
            expected_p2[b] ^= chunks[j][b];
    EXPECT_EQ(chunks[k + 2], expected_p2);

    // Full round-trip with one failure each possible
    for (uint32_t drop = 0; drop < k + r; ++drop) {
        auto dr = dropShards(chunks, {drop});
        auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
        auto recovered = stripPadding(recovered_raw, data.size());
        EXPECT_EQ(recovered, data) << "Drop shard " << drop;
    }
}

// ---------------------------------------------------------------------------
// HC_12 — Invalid: empty data throws
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_12_EmptyDataThrows) {
    EXPECT_THROW(
        coder_.encode({}, 4u, 3u),
        std::invalid_argument);
}

// ---------------------------------------------------------------------------
// HC_13 — Too many missing data shards throws
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_13_TooManyMissingThrows) {
    constexpr uint32_t k = 4, r = 3;
    auto data = makeData(400);
    auto chunks = coder_.encode(data, k, r);

    // Drop ALL data shards — impossible to recover
    std::vector<uint32_t> drop_all_data = {0u, 1u, 2u, 3u};
    auto dr = dropShards(chunks, drop_all_data);

    EXPECT_THROW(
        coder_.decode(dr.available, dr.missing, k, r),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// HC_14 — Factory creates HammingCoder
// ---------------------------------------------------------------------------
TEST(HammingCoderFactoryTest, HC_14_FactoryCreatesHammingCoder) {
    auto coder = ErasureCoder::create(ErasureCodingAlgorithm::HAMMING);
    ASSERT_NE(coder, nullptr);

    // Verify it actually works
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
    auto chunks = coder->encode(data, 4u, 3u);
    EXPECT_EQ(chunks.size(), 7u);

    auto dr = dropShards(chunks, {1u});
    auto recovered_raw = coder->decode(dr.available, dr.missing, 4u, 3u);
    auto recovered = stripPadding(recovered_raw, data.size());
    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_15 — Single parity shard (XOR of subset of data shards)
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_15_SingleParityShard) {
    constexpr uint32_t k = 4, r = 1;
    auto data = makeData(400);
    auto chunks = coder_.encode(data, k, r);
    ASSERT_EQ(chunks.size(), k + r);

    // With only one parity shard (bit 0), it covers only shards j where bit 0
    // of (j+1) is set: shards 0 (j+1=1) and 2 (j+1=3).  Shards 1 and 3 are
    // not covered, so single-failure recovery is not possible for all shards.
    // This test verifies correct encoding and no-failure round-trip only.
    auto dr = dropShards(chunks, {});
    auto recovered_raw = coder_.decode(dr.available, dr.missing, k, r);
    auto recovered = stripPadding(recovered_raw, data.size());
    EXPECT_EQ(recovered, data);
}

// ---------------------------------------------------------------------------
// HC_16 — Parity coverage: explicit verification of bit-mask assignment
// ---------------------------------------------------------------------------
TEST_F(HammingCoderTest, HC_16_ParityCoverageVerification) {
    // For k=6, r=3: verify each parity covers the correct data shards.
    constexpr uint32_t k = 6, r = 3;
    auto data = makeData(600, 0x11);
    auto chunks = coder_.encode(data, k, r);

    const std::size_t shard_size = chunks[0].size();

    for (uint32_t p = 0; p < r; ++p) {
        std::vector<uint8_t> expected(shard_size, 0);
        for (uint32_t j = 0; j < k; ++j) {
            if (((j + 1u) >> p) & 1u) {
                for (std::size_t b = 0; b < shard_size; ++b)
                    expected[b] ^= chunks[j][b];
            }
        }
        EXPECT_EQ(chunks[k + p], expected)
            << "Parity shard " << p << " has wrong coverage";
    }
}
