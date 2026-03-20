/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_temporal_compressor.cpp                       ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:22:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    auto table = SystemVersionedTable::createVersionedTable("empty_tbl");
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
    auto table = SystemVersionedTable::createVersionedTable("delta_tbl");
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
    auto table = SystemVersionedTable::createVersionedTable("zstd_tbl");
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
    auto table = SystemVersionedTable::createVersionedTable("recent_tbl");
    table.insert("key1", {{"name", "Bob"}, {"score", 100}});

    CompressionConfig cfg;
    cfg.algorithm = CompressionAlgorithm::DELTA;
    cfg.compress_immediately = false;
    cfg.delay_before_compression = std::chrono::seconds(24 * 3600); // 24h

    auto stats = compressor.compressHistory(table, {0, kMaxTimestamp}, cfg);
    EXPECT_GT(stats.versions_skipped, 0u);
}
