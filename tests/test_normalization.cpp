/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_normalization.cpp                             ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 19:00:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Test: German umlaut/ß normalization for fulltext indexes

#include <gtest/gtest.h>
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>

namespace fs = std::filesystem;
using namespace themis;

class NormalizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = "test_norm_db";
        if (fs::exists(dbPath_)) {
            fs::remove_all(dbPath_);
        }
        RocksDBWrapper::Config cfg;
        cfg.db_path = dbPath_;
        db_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        idx_ = std::make_shared<SecondaryIndexManager>(*db_);
    }

    void TearDown() override {
        idx_.reset();
        db_.reset();
        if (fs::exists(dbPath_)) {
            fs::remove_all(dbPath_);
        }
    }

    std::string dbPath_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<SecondaryIndexManager> idx_;
};

TEST_F(NormalizationTest, GermanUmlautsEnabled) {
    SecondaryIndexManager::FulltextConfig config;
    config.stemming_enabled = true;
    config.language = "de";
    config.stopwords_enabled = false;
    config.normalize_umlauts = true;

    auto st = idx_->createFulltextIndex("docs", "text", config);
    ASSERT_TRUE(st.ok);

    BaseEntity d1("d1");
    d1.setField("text", "er läuft sehr schnell");
    idx_->put("docs", d1);

    // Query without umlaut should still match
    {
        auto [status, results] = idx_->scanFulltext("docs", "text", "lauft");
        ASSERT_TRUE(status.ok) << status.message;
        ASSERT_EQ(results.size(), 1u);
        EXPECT_EQ(results[0], "d1");
    }

    // Original umlaut query should also match (normalized during query)
    {
        auto [status, results] = idx_->scanFulltext("docs", "text", "l\u00e4uft");
        ASSERT_TRUE(status.ok) << status.message;
        ASSERT_EQ(results.size(), 1u);
        EXPECT_EQ(results[0], "d1");
    }
}

TEST_F(NormalizationTest, GermanUmlautsDisabled) {
    SecondaryIndexManager::FulltextConfig config;
    config.stemming_enabled = false;
    config.language = "de";
    config.stopwords_enabled = false;
    config.normalize_umlauts = false;

    auto st = idx_->createFulltextIndex("docs", "text", config);
    ASSERT_TRUE(st.ok);

    BaseEntity d1("d1");
    d1.setField("text", "er läuft sehr schnell");
    idx_->put("docs", d1);

    // Without normalization, querying "lauft" should not match
    auto [status, results] = idx_->scanFulltext("docs", "text", "lauft");
    ASSERT_TRUE(status.ok) << status.message;
    EXPECT_EQ(results.size(), 0u);
}
