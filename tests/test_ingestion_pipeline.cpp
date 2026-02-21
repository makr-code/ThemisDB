/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_ingestion_pipeline.cpp                        ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:18:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     109                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

using themis::RocksDBWrapper;

namespace {

std::string makeTempPath() {
    auto tmp = std::filesystem::temp_directory_path();
    auto path = tmp / "themis_ingestion_test";
    std::filesystem::remove_all(path);
    return path.string();
}

TEST(IngestionPipelineTest, SingleIngestAndReadback) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempPath();
    cfg.enable_wal = true;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());

    const int kCount = 1000;
    for (int i = 0; i < kCount; ++i) {
        std::string key = "doc_" + std::to_string(i);
        std::string value = "payload_" + std::to_string(i);
        ASSERT_TRUE(db.put(key, value));
    }

    for (int i = 0; i < kCount; ++i) {
        std::string value;
        std::string key = "doc_" + std::to_string(i);
        ASSERT_TRUE(db.get(key, value));
        ASSERT_EQ(value, "payload_" + std::to_string(i));
    }

    db.close();
    std::filesystem::remove_all(cfg.db_path);
}

TEST(IngestionPipelineTest, BatchIngestThroughputGuardrail) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempPath();
    cfg.enable_wal = true;
    cfg.enable_high_parallel_tuning = true;
    cfg.max_background_jobs = 4;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());

    const int kBatch = 5000;
    auto start = std::chrono::steady_clock::now();

    std::vector<std::string> keys;
    keys.reserve(kBatch);

    for (int i = 0; i < kBatch; ++i) {
        std::string key = "batch_doc_" + std::to_string(i);
        std::string value(256, 'x'); // 256B payload
        ASSERT_TRUE(db.put(key, value));
        keys.push_back(key);
    }

    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Guardrail: inserting 5k small docs should complete quickly on CI hardware.
    ASSERT_LT(duration_ms, 2000) << "Ingestion slower than expected";

    // Spot-check a few keys
    for (int i = 0; i < 10; ++i) {
        std::string val;
        ASSERT_TRUE(db.get(keys[i], val));
        ASSERT_EQ(val.size(), 256u);
    }

    db.close();
    std::filesystem::remove_all(cfg.db_path);
}

} // namespace
