// Focused tests for Index Compression (v1.7.0 — Issue #176)
//
// Acceptance criteria covered:
//   AC-1  Delta Encoding      – store differences between adjacent keys
//   AC-2  Prefix Compression  – share common key prefixes
//   AC-3  Bloom Filters       – reduce false lookups
//   AC-4  Dictionary Encoding – map frequent strings to small integers
//   AC-5  Run-Length Encoding – compress repeated values
//
// Also tests the SecondaryIndexManager::Config integration.

#include "index/index_compression.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::index;
using themis::SecondaryIndexManager;

// ============================================================================
// Test fixture
// ============================================================================

class IndexCompressionFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    /// Build a sorted list of typical secondary-index keys.
    static std::vector<std::string> makeIndexKeys(
        std::string_view table,
        std::string_view column,
        std::string_view value,
        const std::vector<std::string>& pks)
    {
        std::vector<std::string> keys = {};

        keys.reserve(pks.size());
        for (const auto& pk : pks) {
            keys.push_back(
                std::string("idx:") + std::string(table) + ":" +
                std::string(column) + ":" +
                std::string(value) + ":" + pk);
        }
        return keys;
    }
};

// ============================================================================
// AC-1: Delta Encoding
// ============================================================================

TEST_F(IndexCompressionFocusedTests, DeltaEncoder_EncodeDecodeRoundtrip) {
    std::vector<int64_t> values = {100, 101, 102, 103, 104};
    auto block = DeltaEncoder::encode(values);

    EXPECT_EQ(block.base, 100);
    ASSERT_EQ(block.deltas.size(), 4u);
    for (auto d : block.deltas) {
      EXPECT_EQ(d, 1);
    }

    auto decoded = DeltaEncoder::decode(block);
    EXPECT_EQ(decoded, values);
}

TEST_F(IndexCompressionFocusedTests, DeltaEncoder_NonUniformDeltas) {
    std::vector<int64_t> values = {0, 5, 12, 20, 100};
    auto block = DeltaEncoder::encode(values);

    EXPECT_EQ(block.base, 0);
    ASSERT_EQ(block.deltas.size(), 4u);
    EXPECT_EQ(block.deltas[0], 5);
    EXPECT_EQ(block.deltas[1], 7);
    EXPECT_EQ(block.deltas[2], 8);
    EXPECT_EQ(block.deltas[3], 80);

    auto decoded = DeltaEncoder::decode(block);
    EXPECT_EQ(decoded, values);
}

TEST_F(IndexCompressionFocusedTests, DeltaEncoder_SingleValue) {
    std::vector<int64_t> values = {42};
    auto block = DeltaEncoder::encode(values);
    EXPECT_EQ(block.base, 42);
    EXPECT_TRUE(block.deltas.empty());

    auto decoded = DeltaEncoder::decode(block);
    EXPECT_EQ(decoded, values);
}

TEST_F(IndexCompressionFocusedTests, DeltaEncoder_LargeSequence) {
    std::vector<int64_t> values(1000);
    std::iota(values.begin(), values.end(), 10'000LL);
    auto block = DeltaEncoder::encode(values);

    EXPECT_EQ(block.base, 10'000);
    for (auto d : block.deltas) {
      EXPECT_EQ(d, 1);
    }

    auto decoded = DeltaEncoder::decode(block);
    EXPECT_EQ(decoded, values);
}

// ============================================================================
// AC-2: Prefix Compression
// ============================================================================

TEST_F(IndexCompressionFocusedTests, PrefixCompressor_CommonPrefixGrouped) {
    auto keys = makeIndexKeys("users", "country", "USA", {"pk1", "pk2", "pk3"});
    std::sort(keys.begin(), keys.end());

    auto blocks = PrefixCompressor::compress(keys, 4);

    // All three keys share "idx:users:country:USA:" — should be one block
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_FALSE(blocks[0].prefix.empty());
    EXPECT_EQ(blocks[0].suffixes.size(), 3u);

    auto decompressed = PrefixCompressor::decompress(blocks);
    std::sort(decompressed.begin(), decompressed.end());
    std::vector<std::string> expected_keys = keys;
    std::sort(expected_keys.begin(), expected_keys.end());
    EXPECT_EQ(decompressed, expected_keys);
}

TEST_F(IndexCompressionFocusedTests, PrefixCompressor_ByteSavings) {
    auto keys = makeIndexKeys("users", "country", "USA", {"pk1", "pk2", "pk3"});
    std::sort(keys.begin(), keys.end());
    auto blocks = PrefixCompressor::compress(keys, 4);

    size_t saved = 0;
    for (const auto& b : blocks) {
      saved += b.savedBytes();
    }
    // "idx:users:country:USA:" is 23 chars, saved for 2 extra keys => 46 bytes
    EXPECT_GT(saved, 0u);
}

TEST_F(IndexCompressionFocusedTests, PrefixCompressor_NoPrefixNoGrouping) {
    std::vector<std::string> keys = {"apple", "banana", "cherry"};
    auto blocks = PrefixCompressor::compress(keys, 4);

    // No keys share ≥4 chars prefix → each in its own block
    auto decompressed = PrefixCompressor::decompress(blocks);
    std::sort(decompressed.begin(), decompressed.end());
    std::vector<std::string> sorted_keys = keys;
    std::sort(sorted_keys.begin(), sorted_keys.end());
    EXPECT_EQ(decompressed, sorted_keys);
}

TEST_F(IndexCompressionFocusedTests, PrefixCompressor_MixedPrefixes) {
    std::vector<std::string> keys = {
        "idx:users:country:DEU:pk1",
        "idx:users:country:DEU:pk2",
        "idx:users:country:USA:pk1",
        "idx:users:country:USA:pk2",
        "idx:users:country:USA:pk3",
    };
    std::sort(keys.begin(), keys.end());

    auto blocks = PrefixCompressor::compress(keys, 4);
    auto decompressed = PrefixCompressor::decompress(blocks);
    std::sort(decompressed.begin(), decompressed.end());
    std::vector<std::string> sorted_keys = keys;
    std::sort(sorted_keys.begin(), sorted_keys.end());
    EXPECT_EQ(decompressed, sorted_keys);
}

TEST_F(IndexCompressionFocusedTests, PrefixBlock_DecompressReconstructsFullKeys) {
    PrefixBlock block;
    block.prefix   = "idx:users:country:USA:";
    block.suffixes = {"pk1", "pk2", "pk3"};

    auto keys = block.decompress();
    ASSERT_EQ(keys.size(), 3u);
    EXPECT_EQ(keys[0], "idx:users:country:USA:pk1");
    EXPECT_EQ(keys[1], "idx:users:country:USA:pk2");
    EXPECT_EQ(keys[2], "idx:users:country:USA:pk3");
}

// ============================================================================
// AC-3: Bloom Filters
// ============================================================================

TEST_F(IndexCompressionFocusedTests, BloomFilter_InsertedKeyMightContain) {
    BloomFilter bf(1000, 0.01);
    bf.insert("idx:users:country:USA:pk42");
    EXPECT_TRUE(bf.mightContain("idx:users:country:USA:pk42"));
}

TEST_F(IndexCompressionFocusedTests, BloomFilter_AbsentKeyReturnsFalse) {
    BloomFilter bf(1000, 0.01);
    bf.insert("idx:users:country:USA:pk1");
    bf.insert("idx:users:country:USA:pk2");

    // Test many strongly distinct keys — with FP rate 1%, expect ≥90% to return false
    size_t true_count = 0;
    size_t total = 200;
    for (size_t i = 0; i < total; ++i) {
        std::string absent = "absent_key_xyzzy_" + std::to_string(i * 997 + 13);
        if (bf.mightContain(absent)) {
          ++true_count;
        }
    }
    // At 1% FP rate, expected false positives ≤ 10%.  Allow 15% for variance.
    double fp_rate = static_cast<double>(true_count) / static_cast<double>(total);
    EXPECT_LE(fp_rate, 0.15)
        << "False positive rate " << fp_rate << " exceeds 15% threshold";
}

TEST_F(IndexCompressionFocusedTests, BloomFilter_ClearResetsState) {
    BloomFilter bf(100, 0.01);
    bf.insert("key1");
    bf.clear();
    // After clear the filter is empty; "key1" might or might not return true
    // because the bit array is zeroed.  It must return false (zeroed bits).
    EXPECT_FALSE(bf.mightContain("key1"));
}

TEST_F(IndexCompressionFocusedTests, BloomFilter_BulkInsertNoFalseNegatives) {
    BloomFilter bf(500, 0.01);
    std::vector<std::string> inserted = {};

    for (int i = 0; i < 500; ++i) {
        std::string key = "idx:tbl:col:val:" + std::to_string(i);
        bf.insert(key);
        inserted.push_back(key);
    }
    for (const auto& key : inserted) {
        EXPECT_TRUE(bf.mightContain(key))
            << "False negative for inserted key: " << key;
    }
}

TEST_F(IndexCompressionFocusedTests, BloomFilter_BitCountAndHashCount) {
    BloomFilter bf(1000, 0.01);
    EXPECT_GT(bf.bitCount(), 0u);
    EXPECT_GT(bf.hashCount(), 0u);
}

// ============================================================================
// AC-4: Dictionary Encoding
// ============================================================================

TEST_F(IndexCompressionFocusedTests, DictionaryCodec_TrainAndEncode) {
    std::vector<std::string> corpus = {
        "USA", "USA", "USA", "DEU", "DEU", "FRA", "FRA", "FRA", "GBR"
    };
    DictionaryCodec codec;
    codec.train(corpus);

    EXPECT_FALSE(codec.empty());
    EXPECT_GT(codec.size(), 0u);

    // Frequent values should be in the dictionary
    uint32_t code_usa = codec.encode("USA");
    EXPECT_NE(code_usa, DictionaryCodec::kMissCode);
    EXPECT_EQ(codec.decode(code_usa), "USA");

    uint32_t code_deu = codec.encode("DEU");
    EXPECT_NE(code_deu, DictionaryCodec::kMissCode);
    EXPECT_EQ(codec.decode(code_deu), "DEU");
}

TEST_F(IndexCompressionFocusedTests, DictionaryCodec_MissCodeForUnknown) {
    DictionaryCodec codec;
    codec.train({"A", "A", "B", "B"});
    EXPECT_EQ(codec.encode("UNKNOWN_XYZ"), DictionaryCodec::kMissCode);
}

TEST_F(IndexCompressionFocusedTests, DictionaryCodec_MinFrequencyFilter) {
    DictionaryCodec::Config cfg;
    cfg.min_frequency = 3;
    DictionaryCodec codec(cfg);

    // "rare" only appears once — should NOT be in dictionary
    std::vector<std::string> corpus = {"common", "common", "common", "rare"};
    codec.train(corpus);

    EXPECT_NE(codec.encode("common"), DictionaryCodec::kMissCode);
    EXPECT_EQ(codec.encode("rare"), DictionaryCodec::kMissCode);
}

TEST_F(IndexCompressionFocusedTests, DictionaryCodec_DecodeUnknownCodeReturnsEmpty) {
    DictionaryCodec codec;
    codec.train({"x", "x"});
    EXPECT_EQ(codec.decode(DictionaryCodec::kMissCode), "");
    EXPECT_EQ(codec.decode(99999u), "");
}

TEST_F(IndexCompressionFocusedTests, DictionaryCodec_MaxDictSize) {
    DictionaryCodec::Config cfg;
    cfg.max_dict_size  = 2;
    cfg.min_frequency  = 1;
    DictionaryCodec codec(cfg);

    std::vector<std::string> corpus = {"a","a","b","b","c","c","d","d"};
    codec.train(corpus);
    EXPECT_LE(codec.size(), 2u);
}

// ============================================================================
// AC-5: Run-Length Encoding
// ============================================================================

TEST_F(IndexCompressionFocusedTests, RunLengthEncoder_BasicRoundtrip) {
    std::vector<std::string> values = {"USA", "USA", "USA", "DEU", "DEU", "FRA"};
    auto block = RunLengthEncoder::encode(values);

    ASSERT_EQ(block.runs.size(), 3u);
    EXPECT_EQ(block.runs[0].value, "USA");
    EXPECT_EQ(block.runs[0].count, 3u);
    EXPECT_EQ(block.runs[1].value, "DEU");
    EXPECT_EQ(block.runs[1].count, 2u);
    EXPECT_EQ(block.runs[2].value, "FRA");
    EXPECT_EQ(block.runs[2].count, 1u);

    auto decoded = RunLengthEncoder::decode(block);
    EXPECT_EQ(decoded, values);
}

TEST_F(IndexCompressionFocusedTests, RunLengthEncoder_AllSame) {
    std::vector<std::string> values(100, "REPEAT");
    auto block = RunLengthEncoder::encode(values);
    ASSERT_EQ(block.runs.size(), 1u);
    EXPECT_EQ(block.runs[0].count, 100u);

    auto decoded = RunLengthEncoder::decode(block);
    EXPECT_EQ(decoded, values);
}

TEST_F(IndexCompressionFocusedTests, RunLengthEncoder_AllDistinct) {
    std::vector<std::string> values = {"a", "b", "c", "d"};
    auto block = RunLengthEncoder::encode(values);
    EXPECT_EQ(block.runs.size(), values.size());

    auto decoded = RunLengthEncoder::decode(block);
    EXPECT_EQ(decoded, values);
}

TEST_F(IndexCompressionFocusedTests, RunLengthEncoder_CompressionRatio) {
    std::vector<std::string> values(50, "SAME_VALUE");
    double ratio = RunLengthEncoder::compressionRatio(values);
    EXPECT_GT(ratio, 1.0) << "Expected compression ratio > 1 for 50 identical values";
}

TEST_F(IndexCompressionFocusedTests, RunLengthBlock_Decompress) {
    RunLengthBlock block;
    block.runs = {{"A", 3}, {"B", 2}};
    auto values = block.decompress();
    std::vector<std::string> expected = {"A", "A", "A", "B", "B"};
    EXPECT_EQ(values, expected);
}

// ============================================================================
// IndexCompressionCodec integration tests
// ============================================================================

TEST_F(IndexCompressionFocusedTests, Codec_DefaultConfigDisabled) {
    IndexCompressionCodec codec;
    EXPECT_FALSE(codec.config().enable_bloom_filter);
    EXPECT_FALSE(codec.config().enable_prefix_compression);
}

TEST_F(IndexCompressionFocusedTests, Codec_CompressDecompressKeysRoundtrip) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_prefix_compression = true;
    cfg.enable_bloom_filter       = true;
    IndexCompressionCodec codec(cfg);

    auto keys = makeIndexKeys("users", "country", "USA", {"pk1", "pk2", "pk3"});
    std::sort(keys.begin(), keys.end());

    auto blocks     = codec.compressKeys(keys);
    auto decompressed = codec.decompressKeys(blocks);
    std::sort(decompressed.begin(), decompressed.end());
    std::vector<std::string> sorted_keys = keys;
    EXPECT_EQ(decompressed, sorted_keys);
}

TEST_F(IndexCompressionFocusedTests, Codec_BloomFilterUpdatedOnCompress) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_bloom_filter = true;
    IndexCompressionCodec codec(cfg);

    auto keys = makeIndexKeys("users", "country", "USA", {"pk1", "pk2"});
    std::sort(keys.begin(), keys.end());
    codec.compressKeys(keys);

    EXPECT_TRUE(codec.bloomMightContain(keys[0]));
    EXPECT_TRUE(codec.bloomMightContain(keys[1]));
}

TEST_F(IndexCompressionFocusedTests, Codec_BloomFilterRejectsAbsentKey) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_bloom_filter = true;
    cfg.bloom_expected_elements = 1000;
    cfg.bloom_false_positive_rate = 0.001;
    IndexCompressionCodec codec(cfg);

    codec.bloomInsert("idx:users:country:USA:pk1");
    // A very different key should be rejected
    EXPECT_FALSE(codec.bloomMightContain("this_key_is_definitely_absent_xyz_abc_123"));
}

TEST_F(IndexCompressionFocusedTests, Codec_DictionaryTrainAndEncode) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_dict_encoding = true;
    IndexCompressionCodec codec(cfg);

    std::vector<std::string> sample = {"USA","USA","USA","DEU","DEU","FRA"};
    codec.trainDictionary(sample);
    EXPECT_TRUE(codec.dictionaryReady());

    uint32_t code = codec.encodeValue("USA");
    EXPECT_NE(code, DictionaryCodec::kMissCode);
    EXPECT_EQ(codec.decodeValue(code), "USA");
}

TEST_F(IndexCompressionFocusedTests, Codec_RLECompressDecompress) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_rle = true;
    IndexCompressionCodec codec(cfg);

    std::vector<std::string> values = {"USA","USA","USA","DEU","FRA","FRA"};
    auto block = codec.compressValues(values);
    auto decoded = codec.decompressValues(block);
    EXPECT_EQ(decoded, values);
}

TEST_F(IndexCompressionFocusedTests, Codec_DeltaEncodePKs) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_delta_encoding = true;
    IndexCompressionCodec codec(cfg);

    std::vector<int64_t> pks = {1001, 1002, 1003, 1004, 1005};
    auto block   = codec.encodePKs(pks);
    auto decoded = codec.decodePKs(block);
    EXPECT_EQ(decoded, pks);
}

TEST_F(IndexCompressionFocusedTests, Codec_StatsTracked) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_prefix_compression = true;
    cfg.enable_bloom_filter       = true;
    cfg.enable_dict_encoding      = true;
    cfg.enable_rle                = true;
    cfg.enable_delta_encoding     = true;
    IndexCompressionCodec codec(cfg);

    auto keys = makeIndexKeys("orders", "status", "PENDING", {"a","b","c"});
    std::sort(keys.begin(), keys.end());
    codec.compressKeys(keys);

    EXPECT_EQ(codec.stats().keys_compressed, 3u);
    EXPECT_GT(codec.stats().bloom_inserts, 0u);
}

TEST_F(IndexCompressionFocusedTests, Codec_ResetStats) {
    IndexCompressionCodec::Config cfg;
    cfg.enable_bloom_filter = true;
    IndexCompressionCodec codec(cfg);

    codec.bloomInsert("key");
    EXPECT_GT(codec.stats().bloom_inserts, 0u);
    codec.resetStats();
    EXPECT_EQ(codec.stats().bloom_inserts, 0u);
}

// ============================================================================
// SecondaryIndexManager::Config integration
// ============================================================================

TEST_F(IndexCompressionFocusedTests, SIM_DefaultConstructorDisablesCompression) {
    // SecondaryIndexManager requires a RocksDBWrapper — use a mock path
    // We only test the Config API since constructing a real DB is heavyweight.
    SecondaryIndexManager::Config cfg;
    EXPECT_FALSE(cfg.enable_compression);
    EXPECT_EQ(cfg.compression_algorithm, CompressionAlgorithm::NONE);
    EXPECT_EQ(cfg.compression_level, 3);
}

TEST_F(IndexCompressionFocusedTests, SIM_ConfigFieldsSetCorrectly) {
    SecondaryIndexManager::Config cfg;
    cfg.enable_compression      = true;
    cfg.compression_algorithm   = CompressionAlgorithm::ZSTD;
    cfg.compression_level       = 5;
    cfg.enable_prefix_compression = true;
    cfg.enable_delta_encoding     = true;
    cfg.enable_rle                = true;
    cfg.enable_dict_encoding      = true;
    cfg.enable_bloom_filter       = true;

    EXPECT_TRUE(cfg.enable_compression);
    EXPECT_EQ(cfg.compression_algorithm, CompressionAlgorithm::ZSTD);
    EXPECT_EQ(cfg.compression_level, 5);
    EXPECT_TRUE(cfg.enable_prefix_compression);
    EXPECT_TRUE(cfg.enable_delta_encoding);
    EXPECT_TRUE(cfg.enable_rle);
    EXPECT_TRUE(cfg.enable_dict_encoding);
    EXPECT_TRUE(cfg.enable_bloom_filter);
}

TEST_F(IndexCompressionFocusedTests, CompressionAlgorithm_EnumValues) {
    // Verify all expected algorithm enum values exist
    CompressionAlgorithm none    = CompressionAlgorithm::NONE;
    CompressionAlgorithm lz4     = CompressionAlgorithm::LZ4;
    CompressionAlgorithm zstd    = CompressionAlgorithm::ZSTD;
    CompressionAlgorithm snappy  = CompressionAlgorithm::SNAPPY;

    EXPECT_NE(none,   lz4);
    EXPECT_NE(none,   zstd);
    EXPECT_NE(none,   snappy);
    EXPECT_NE(lz4,    zstd);
    EXPECT_NE(lz4,    snappy);
    EXPECT_NE(zstd,   snappy);
}
