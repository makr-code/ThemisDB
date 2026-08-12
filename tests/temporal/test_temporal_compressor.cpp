/**
 * Tests for TemporalCompressor
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/temporal_compressor.h"
#include "temporal/system_versioned_table.h"

using namespace themisdb::temporal;

class TemporalCompressorTest : public ::testing::Test {
protected:
    TemporalCompressor compressor;
};

// ── algorithmName ─────────────────────────────────────────────────────────────

TEST_F(TemporalCompressorTest, AlgorithmName_Delta) {
    EXPECT_EQ(TemporalCompressor::algorithmName(CompressionAlgorithm::DELTA), "DELTA");
}

TEST_F(TemporalCompressorTest, AlgorithmName_Zstd) {
    EXPECT_EQ(TemporalCompressor::algorithmName(CompressionAlgorithm::ZSTD), "ZSTD");
}

TEST_F(TemporalCompressorTest, AlgorithmName_Gorilla) {
    EXPECT_EQ(TemporalCompressor::algorithmName(CompressionAlgorithm::GORILLA), "GORILLA");
}

TEST_F(TemporalCompressorTest, AlgorithmName_Dictionary) {
    EXPECT_EQ(TemporalCompressor::algorithmName(CompressionAlgorithm::DICTIONARY), "DICTIONARY");
}

// ── compressHistory on empty table ────────────────────────────────────────────

TEST_F(TemporalCompressorTest, CompressHistory_EmptyTable_ReturnsZeroStats) {
    auto table = SystemVersionedTable::createVersionedTable("empty_tbl", {});
    CompressionConfig cfg;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_EQ(stats.versions_processed, 0u);
    EXPECT_EQ(stats.versions_compressed, 0u);
}

// ── decompress on plain (non-compressed) document ─────────────────────────────

TEST_F(TemporalCompressorTest, Decompress_NonCompressedDoc_ReturnsSame) {
    nlohmann::json doc = {{"name", "Alice"}, {"age", 30}};
    auto result = TemporalCompressor::decompress(doc);
    EXPECT_EQ(result, doc);
}

// ── compressHistory with DELTA algorithm ──────────────────────────────────────

TEST_F(TemporalCompressorTest, CompressHistory_DeltaAlgo_ProcessesVersions) {
    auto table = SystemVersionedTable::createVersionedTable("delta_tbl", {});
    table.insert("key1", {{"name", "Alice"}, {"age", 30}});
    table.upsert("key1", {{"name", "Alice"}, {"age", 31}});
    table.upsert("key1", {{"name", "Alice"}, {"age", 32}});

    CompressionConfig cfg;
    cfg.algorithm = CompressionAlgorithm::DELTA;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_GT(stats.versions_compressed, 0u);
}

// ── compressHistory with ZSTD algorithm ───────────────────────────────────────

TEST_F(TemporalCompressorTest, CompressHistory_ZstdAlgo_ProcessesVersions) {
    auto table = SystemVersionedTable::createVersionedTable("zstd_tbl", {});
    table.insert("key1", {{"value", "hello world"}, {"count", 1}});
    table.upsert("key1", {{"value", "hello world updated"}, {"count", 2}});
    table.upsert("key1", {{"value", "hello world again"}, {"count", 3}});

    CompressionConfig cfg;
    cfg.algorithm = CompressionAlgorithm::ZSTD;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_GT(stats.versions_compressed, 0u);
}

// ── compressHistory skips recent versions when delay applies ──────────────────

TEST_F(TemporalCompressorTest, CompressHistory_SkipsRecentVersions) {
    auto table = SystemVersionedTable::createVersionedTable("recent_tbl", {});
    table.insert("key1", {{"name", "Bob"}, {"score", 100}});
    // Second upsert makes the first version historical (non-current),
    // so the delay check can fire on it.
    table.upsert("key1", {{"name", "Bob"}, {"score", 101}});

    CompressionConfig cfg;
    cfg.algorithm = CompressionAlgorithm::DELTA;
    cfg.compress_immediately = false;
    cfg.delay_before_compression = std::chrono::seconds(24 * 3600); // 24h

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_GT(stats.versions_skipped, 0u);
}

// ── Additional edge-case tests ────────────────────────────────────────────────

TEST_F(TemporalCompressorTest, CompressHistory_DictionaryAlgo_ProcessesVersions) {
    auto table = SystemVersionedTable::createVersionedTable("dict_tbl", {});
    table.insert("key1", {{"status", "active"},   {"region", "EU"}});
    table.upsert("key1", {{"status", "inactive"}, {"region", "EU"}});
    table.upsert("key1", {{"status", "active"},   {"region", "US"}});

    CompressionConfig cfg;
    cfg.algorithm = CompressionAlgorithm::DICTIONARY;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_GT(stats.versions_compressed, 0u);
}

TEST_F(TemporalCompressorTest, CompressHistory_CompressImmediately_DoesNotSkip) {
    auto table = SystemVersionedTable::createVersionedTable("imm_tbl", {});
    table.insert("key1", {{"name", "Charlie"}, {"value", 42}});

    CompressionConfig cfg;
    cfg.algorithm = CompressionAlgorithm::DELTA;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_EQ(stats.versions_skipped, 0u);
}

// ── LZ4 algorithm tests (TC-LZ4-01 … TC-LZ4-05) ─────────────────────────────

TEST_F(TemporalCompressorTest, TCLZ4_01_AlgorithmName) {
    EXPECT_EQ(TemporalCompressor::algorithmName(CompressionAlgorithm::LZ4), "LZ4");
}

TEST_F(TemporalCompressorTest, TCLZ4_02_CompressHistoryLZ4ProcessesVersions) {
    auto table = SystemVersionedTable::createVersionedTable("lz4_tbl", {});
    table.insert("key1", {{"x", 1}, {"y", "hello"}});
    table.upsert("key1", {{"x", 2}, {"y", "world"}});
    table.upsert("key1", {{"x", 3}, {"y", "foo"}});

    CompressionConfig cfg;
    cfg.algorithm            = CompressionAlgorithm::LZ4;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_GT(stats.versions_compressed, 0u);
    EXPECT_EQ(stats.errors.size(), 0u);
}

TEST_F(TemporalCompressorTest, TCLZ4_03_DecompressRoundTrip) {
    auto table = SystemVersionedTable::createVersionedTable("lz4_rt", {});
    const nlohmann::json payload = {{"sensor", "temp"}, {"val", 99.7}, {"tag", "abc"}};
    table.insert("k1", payload);
    // Create a closed historical version so replaceHistoricalPayload can update it.
    table.upsert("k1", {{"sensor", "temp"}, {"val", 100.1}, {"tag", "next"}});

    CompressionConfig cfg;
    cfg.algorithm            = CompressionAlgorithm::LZ4;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    ASSERT_GT(stats.versions_compressed, 0u) << "Nothing was compressed";

    // Retrieve the compressed version and decompress it.
    auto versions = table.getHistory("k1");
    ASSERT_FALSE(versions.empty());

    const nlohmann::json& stored = versions.front().data;
    ASSERT_TRUE(stored.contains("__compressed")) << "Expected compressed payload";
    EXPECT_EQ(stored["__compressed"].get<std::string>(), "lz4");

    const auto decoded = TemporalCompressor::decompress(stored);
    EXPECT_EQ(decoded, payload);
}

TEST_F(TemporalCompressorTest, TCLZ4_04_CompressionRatioPositive) {
    auto table = SystemVersionedTable::createVersionedTable("lz4_ratio", {});
    // Insert repetitive data to get measurable compression.
    for (int i = 0; i < 10; ++i) {
        const auto key = "k" + std::to_string(i);
        table.insert(key,
                     {{"a", "aaaaaaaaaa"}, {"b", "bbbbbbbbbb"}, {"n", i}});
        // Ensure one historical version per key is available for compression.
        table.upsert(key,
                     {{"a", "aaaaaaaaaa"}, {"b", "bbbbbbbbbb"}, {"n", i + 1000}});
    }

    CompressionConfig cfg;
    cfg.algorithm            = CompressionAlgorithm::LZ4;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_GT(stats.versions_compressed, 0u);
    EXPECT_GT(stats.original_size_bytes,  0u);
    EXPECT_GT(stats.compressed_size_bytes, 0u);
}

TEST_F(TemporalCompressorTest, TCLZ4_05_EmptyTableNoError) {
    auto table = SystemVersionedTable::createVersionedTable("lz4_empty", {});

    CompressionConfig cfg;
    cfg.algorithm            = CompressionAlgorithm::LZ4;
    cfg.compress_immediately = true;

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_EQ(stats.versions_compressed, 0u);
    EXPECT_EQ(stats.errors.size(), 0u);
}

