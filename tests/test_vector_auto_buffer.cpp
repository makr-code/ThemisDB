/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_vector_auto_buffer.cpp                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:57:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     175                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_vector_auto_buffer.cpp
 * @brief Unit tests for VectorAutoBuffer – vector index auto-batching buffer
 */

#include <gtest/gtest.h>
#include "index/vector_auto_buffer.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <chrono>
#include <filesystem>
#include <memory>

namespace themis {
namespace {

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_vec_buf_" + tag + "_" + std::to_string(ns))).string();
}

struct VectorAutoBufferFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<VectorIndexManager> vim;

    void SetUp() override {
        db_path = makeTempPath("test");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;
        vim = std::make_unique<VectorIndexManager>(*db);
    }

    void TearDown() override {
        vim.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    // Build a tiny BaseEntity with a 4-dim embedding
    static BaseEntity makeEntity(const std::string& pk, float base_val = 1.0f) {
        BaseEntity e(pk);
        std::vector<float> emb = {base_val, base_val + 0.1f,
                                  base_val + 0.2f, base_val + 0.3f};
        e.setField("embedding", emb);
        return e;
    }
};

TEST_F(VectorAutoBufferFixture, ConstructsWithoutThrowing) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush = false;
    EXPECT_NO_THROW({ VectorAutoBuffer buf(vim.get(), cfg); });
}

TEST_F(VectorAutoBufferFixture, NullIndexManagerThrows) {
    EXPECT_THROW(VectorAutoBuffer(nullptr), std::invalid_argument);
}

TEST_F(VectorAutoBufferFixture, AddEntityReturnsOK) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush = false;
    VectorAutoBuffer buf(vim.get(), cfg);

    auto status = buf.add(makeEntity("pk_001", 1.0f));
    EXPECT_TRUE(status.ok) << status.message;
}

TEST_F(VectorAutoBufferFixture, RemoveEntityReturnsOK) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush = false;
    VectorAutoBuffer buf(vim.get(), cfg);

    // Add then remove
    buf.add(makeEntity("pk_del", 2.0f));
    auto status = buf.remove("pk_del");
    EXPECT_TRUE(status.ok) << status.message;
}

TEST_F(VectorAutoBufferFixture, ManualFlushDecrementsBuffer) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush           = false;
    cfg.max_vectors_per_buffer = 1000;
    VectorAutoBuffer buf(vim.get(), cfg);

    for (int i = 0; i < 5; ++i) {
        buf.add(makeEntity("pk_f" + std::to_string(i), static_cast<float>(i)));
    }
    size_t flushed = buf.flush();
    EXPECT_GE(flushed, 5u);

    auto stats = buf.getStats();
    EXPECT_GE(stats.vectors_flushed.load(), 5u);
    EXPECT_GE(stats.flush_count.load(), 1u);
}

TEST_F(VectorAutoBufferFixture, SizeTriggeredAutoFlush) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush            = false;
    cfg.max_vectors_per_buffer = 2;
    cfg.flush_batch_size       = 2;
    VectorAutoBuffer buf(vim.get(), cfg);

    for (int i = 0; i < 3; ++i) {
        buf.add(makeEntity("pk_s" + std::to_string(i), static_cast<float>(i)));
    }
    buf.flush();
    EXPECT_GE(buf.getStats().vectors_buffered.load(), 3u);
}

TEST_F(VectorAutoBufferFixture, IsRunningReflectsStartStop) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush    = true;
    cfg.flush_interval = std::chrono::milliseconds(10000);
    VectorAutoBuffer buf(vim.get(), cfg);

    EXPECT_FALSE(buf.isRunning());
    buf.start();
    EXPECT_TRUE(buf.isRunning());
    buf.stop();
    EXPECT_FALSE(buf.isRunning());
}

TEST_F(VectorAutoBufferFixture, GetStatsDefaultsAreZero) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush = false;
    VectorAutoBuffer buf(vim.get(), cfg);
    auto stats = buf.getStats();
    EXPECT_EQ(stats.vectors_buffered.load(), 0u);
    EXPECT_EQ(stats.vectors_flushed.load(), 0u);
    EXPECT_EQ(stats.flush_count.load(), 0u);
}

TEST_F(VectorAutoBufferFixture, ConfigCanBeUpdated) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush            = false;
    cfg.max_vectors_per_buffer = 500;
    VectorAutoBuffer buf(vim.get(), cfg);

    VectorAutoBufferConfig new_cfg = cfg;
    new_cfg.max_vectors_per_buffer = 50;
    EXPECT_NO_THROW(buf.setConfig(new_cfg));
    EXPECT_EQ(buf.getConfig().max_vectors_per_buffer, 50u);
}

}  // namespace
}  // namespace themis
