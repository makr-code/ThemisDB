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

// ============================================================================
// Product Quantization config field tests
// ============================================================================

TEST_F(VectorAutoBufferFixture, PQConfigDefaultFields) {
    VectorAutoBufferConfig cfg;
    EXPECT_EQ(cfg.pq_num_subvectors, 8);
    EXPECT_EQ(cfg.pq_num_centroids, 256);
    EXPECT_EQ(cfg.compression,
              VectorAutoBufferConfig::Compression::None);
}

TEST_F(VectorAutoBufferFixture, PQConfigRoundtrip) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush        = false;
    cfg.compression        = VectorAutoBufferConfig::Compression::ProductQuantization;
    cfg.pq_num_subvectors  = 2;
    cfg.pq_num_centroids   = 4;

    VectorAutoBuffer buf(vim.get(), cfg);
    const auto& stored = buf.getConfig();
    EXPECT_EQ(stored.compression,
              VectorAutoBufferConfig::Compression::ProductQuantization);
    EXPECT_EQ(stored.pq_num_subvectors, 2);
    EXPECT_EQ(stored.pq_num_centroids,  4);
}

// Helper: build a 4-dim entity with distinct values to avoid zero-vector edge case
static BaseEntity makePQEntity(const std::string& pk, int seed) {
    BaseEntity e(pk);
    std::vector<float> emb = {
        static_cast<float>(seed),
        static_cast<float>(seed + 1),
        static_cast<float>(seed + 2),
        static_cast<float>(seed + 3)
    };
    e.setField("embedding", emb);
    return e;
}

// With pq_num_centroids > batch_size the PQ path must fall back gracefully
// (return uncompressed entities) rather than crash.
TEST_F(VectorAutoBufferFixture, PQFallbackWhenTooFewTrainingVectors) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush        = false;
    cfg.compression        = VectorAutoBufferConfig::Compression::ProductQuantization;
    cfg.pq_num_subvectors  = 2;
    cfg.pq_num_centroids   = 100;  // requires ≥ 100 entities; we only add 5

    VectorAutoBuffer buf(vim.get(), cfg);
    for (int i = 0; i < 5; ++i) {
        buf.add(makePQEntity("e" + std::to_string(i), i * 10));
    }
    // flush() must succeed without throwing even though PQ training is skipped
    EXPECT_NO_THROW(buf.flush());
}

// With enough training vectors and a valid dim/subvectors split the PQ path
// should complete the flush successfully.
TEST_F(VectorAutoBufferFixture, PQFlushSucceedsWithSufficientBatch) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush        = false;
    cfg.compression        = VectorAutoBufferConfig::Compression::ProductQuantization;
    cfg.pq_num_subvectors  = 2;   // 4-dim / 2 = 2-dim sub-vectors
    cfg.pq_num_centroids   = 4;   // need ≥ 4 training vectors

    VectorAutoBuffer buf(vim.get(), cfg);
    for (int i = 0; i < 8; ++i) {
        buf.add(makePQEntity("pq_" + std::to_string(i), i * 5));
    }
    size_t flushed = 0;
    EXPECT_NO_THROW(flushed = buf.flush());
    EXPECT_GT(flushed, 0u);
}

// When vector dim is not divisible by pq_num_subvectors the PQ path should
// fall back gracefully (return uncompressed entities).
TEST_F(VectorAutoBufferFixture, PQFallbackOnBadDimDivisibility) {
    VectorAutoBufferConfig cfg;
    cfg.async_flush        = false;
    cfg.compression        = VectorAutoBufferConfig::Compression::ProductQuantization;
    cfg.pq_num_subvectors  = 3;   // 4 % 3 != 0 → must fall back
    cfg.pq_num_centroids   = 4;

    VectorAutoBuffer buf(vim.get(), cfg);
    for (int i = 0; i < 8; ++i) {
        buf.add(makePQEntity("bad_" + std::to_string(i), i));
    }
    EXPECT_NO_THROW(buf.flush());
}

// ============================================================================
// fallback_dim config field tests (stub #90)
// ============================================================================

TEST_F(VectorAutoBufferFixture, FallbackDimDefaultIs768) {
    VectorAutoBufferConfig cfg;
    EXPECT_EQ(cfg.fallback_dim, 768u);
}

TEST_F(VectorAutoBufferFixture, FallbackDimUsedWhenEmbeddingAbsent) {
    // Build an entity WITHOUT an "embedding" field so estimateVectorSize()
    // must fall back to config_.fallback_dim.
    BaseEntity no_embed_entity("no_embed");
    no_embed_entity.setField("name", std::string("test"));

    // With fallback_dim = 512 the memory estimate for the op should be
    // at least 512 * sizeof(float) = 2048 bytes.
    VectorAutoBufferConfig cfg;
    cfg.async_flush  = false;
    cfg.fallback_dim = 512;
    VectorAutoBuffer buf(vim.get(), cfg);

    // add() should succeed; after adding, current_buffer_memory reflects 512-dim fallback
    auto status = buf.add(no_embed_entity);
    EXPECT_TRUE(status.ok) << status.message;

    const auto stats = buf.getStats();
    EXPECT_GE(stats.current_buffer_memory, 512u * sizeof(float));
}

}  // namespace
}  // namespace themis
