// Phase 3: Hypertable – Production Features Tests

#include <gtest/gtest.h>
#include "timeseries/hypertable.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <memory>
#include <string>

using namespace themis;
namespace fs = std::filesystem;

static std::string makeHyperTempPath(const std::string& tag) {
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_hyper_" + tag + "_" + std::to_string(ns))).string();
}

struct HypertableFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;

    void SetUp() override {
        db_path = makeHyperTempPath("ht");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed at " << db_path;
    }

    void TearDown() override {
        db.reset();
        fs::remove_all(db_path);
    }

    Hypertable::Config makeConfig(const std::string& table = "metrics",
                                   int64_t chunk_secs = 86400,
                                   int32_t retention_days = 30) {
        Hypertable::Config cfg;
        cfg.table_name            = table;
        cfg.chunk_interval_seconds = chunk_secs;
        cfg.retention_days        = retention_days;
        cfg.auto_create_chunks    = true;
        cfg.compress_old_chunks   = false;
        return cfg;
    }
};

// ===== Construction =====

TEST_F(HypertableFixture, ConstructsSuccessfully) {
    EXPECT_NO_THROW({ Hypertable ht(db.get(), makeConfig()); });
}

TEST_F(HypertableFixture, NullDbThrows) {
    EXPECT_THROW({ Hypertable ht(nullptr, makeConfig()); }, std::runtime_error);
}

// ===== Single Insert & Config =====

TEST_F(HypertableFixture, GetConfig) {
    Hypertable ht(db.get(), makeConfig("test_table", 3600, 7));
    EXPECT_EQ(ht.getConfig().table_name, "test_table");
    EXPECT_EQ(ht.getConfig().chunk_interval_seconds, 3600);
    EXPECT_EQ(ht.getConfig().retention_days, 7);
}

TEST_F(HypertableFixture, InsertSinglePoint) {
    Hypertable ht(db.get(), makeConfig());
    bool ok = ht.insert(1700000000LL, R"({"value": 42.0})");
    EXPECT_TRUE(ok);
}

TEST_F(HypertableFixture, InsertMultiplePoints) {
    Hypertable ht(db.get(), makeConfig());
    int64_t base = 1700000000LL;
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(ht.insert(base + i * 3600, R"({"value": 1.0})"));
    }
}

TEST_F(HypertableFixture, InsertPointsAcrossDays) {
    Hypertable ht(db.get(), makeConfig("cross_day", 86400));
    // Day 1 and Day 2
    EXPECT_TRUE(ht.insert(1700000000LL, R"({"day": 1})"));
    EXPECT_TRUE(ht.insert(1700000000LL + 86401, R"({"day": 2})"));
}

// ===== Batch Insert =====

TEST_F(HypertableFixture, BatchInsertSucceeds) {
    Hypertable ht(db.get(), makeConfig());
    std::vector<std::pair<int64_t, std::string>> batch;
    int64_t base = 1700000000LL;
    for (int i = 0; i < 5; ++i) {
        batch.push_back({base + i * 1000, R"({"v": )" + std::to_string(i) + "}"});
    }
    EXPECT_TRUE(ht.insertBatch(batch));
}

TEST_F(HypertableFixture, BatchInsertEmptyBatchSucceeds) {
    Hypertable ht(db.get(), makeConfig());
    EXPECT_TRUE(ht.insertBatch({}));
}

TEST_F(HypertableFixture, BatchInsertAcrossChunks) {
    Hypertable ht(db.get(), makeConfig("batch_cross", 3600));  // 1-hour chunks
    std::vector<std::pair<int64_t, std::string>> batch = {
        {1700000000LL,        R"({"h": 0})"},
        {1700000000LL + 3601, R"({"h": 1})"},
        {1700000000LL + 7201, R"({"h": 2})"},
    };
    EXPECT_TRUE(ht.insertBatch(batch));
}

// ===== Chunk Management =====

TEST_F(HypertableFixture, ListChunksAfterInsert) {
    Hypertable ht(db.get(), makeConfig("chunk_test", 3600));
    int64_t base = 1700000000LL;
    ht.insert(base, R"({"v": 1})");
    ht.insert(base + 7200, R"({"v": 2})");
    auto chunks = ht.listChunks();
    // Should have at least 1 chunk
    EXPECT_GE(chunks.size(), 1u);
}

TEST_F(HypertableFixture, ListChunksEmptyTable) {
    Hypertable ht(db.get(), makeConfig());
    auto chunks = ht.listChunks();
    // Empty or 0 chunks for empty table
    EXPECT_EQ(chunks.size(), 0u);
}

// ===== Statistics =====

TEST_F(HypertableFixture, GetStatsAfterInsert) {
    Hypertable ht(db.get(), makeConfig());
    ht.insert(1700000000LL, R"({"v": 1})");
    ht.insert(1700000001LL, R"({"v": 2})");
    auto stats = ht.getStats();
    EXPECT_GE(stats.total_rows, 2u);
    EXPECT_GE(stats.total_chunks, 1u);
}

TEST_F(HypertableFixture, GetStatsEmptyTable) {
    Hypertable ht(db.get(), makeConfig());
    auto stats = ht.getStats();
    EXPECT_EQ(stats.total_rows, 0u);
    EXPECT_EQ(stats.total_chunks, 0u);
}

// ===== Chunk Compression & Retention =====

TEST_F(HypertableFixture, CompressOldChunksDoesNotCrash) {
    Hypertable ht(db.get(), makeConfig());
    ht.insert(1700000000LL, R"({"v": 1})");
    EXPECT_NO_THROW(ht.compressOldChunks());
}

TEST_F(HypertableFixture, DropExpiredChunksDoesNotCrash) {
    Hypertable ht(db.get(), makeConfig("retention_test", 86400, 1));  // 1-day retention
    ht.insert(1700000000LL, R"({"v": 1})");
    EXPECT_NO_THROW(ht.dropExpiredChunks());
}

TEST_F(HypertableFixture, RetentionDaysConfiguration) {
    auto cfg = makeConfig("ret", 86400, 90);
    Hypertable ht(db.get(), cfg);
    EXPECT_EQ(ht.getConfig().retention_days, 90);
}

// ===== Configuration edge cases =====

TEST_F(HypertableFixture, HourlyChunkInterval) {
    auto cfg = makeConfig("hourly", 3600, 7);
    Hypertable ht(db.get(), cfg);
    EXPECT_TRUE(ht.insert(1700000000LL, R"({"v": 1})"));
}

TEST_F(HypertableFixture, ShortChunkInterval) {
    auto cfg = makeConfig("short", 60, 1);  // 1-minute chunks
    Hypertable ht(db.get(), cfg);
    EXPECT_TRUE(ht.insert(1700000000LL, R"({"v": 1})"));
    EXPECT_TRUE(ht.insert(1700000000LL + 61, R"({"v": 2})"));
}

// ===== Chunk Health Metrics & Lifecycle Monitoring =====

TEST_F(HypertableFixture, GetChunkHealthEmptyTable) {
    Hypertable ht(db.get(), makeConfig("health_empty"));
    auto health = ht.getChunkHealth();
    // No data, no chunks tracked → empty health report
    EXPECT_TRUE(health.empty());
}

TEST_F(HypertableFixture, GetChunkHealthAfterInsert) {
    Hypertable ht(db.get(), makeConfig("health_insert"));
    ht.insert(1700000000LL, R"({"v": 1})");
    // Should not throw, report can be empty (depends on listChunks implementation)
    EXPECT_NO_THROW(ht.getChunkHealth());
}

TEST_F(HypertableFixture, ChunkStatusEnumValues) {
    // Verify the enum is usable
    Hypertable::ChunkStatus s = Hypertable::ChunkStatus::Active;
    EXPECT_EQ(s, Hypertable::ChunkStatus::Active);
    s = Hypertable::ChunkStatus::Expired;
    EXPECT_EQ(s, Hypertable::ChunkStatus::Expired);
    s = Hypertable::ChunkStatus::Compressed;
    EXPECT_EQ(s, Hypertable::ChunkStatus::Compressed);
    s = Hypertable::ChunkStatus::Compressible;
    EXPECT_NE(s, Hypertable::ChunkStatus::Frozen);
}

TEST_F(HypertableFixture, ChunkHealthStructFields) {
    Hypertable::ChunkHealth h;
    h.chunk_name    = "test_chunk";
    h.status        = Hypertable::ChunkStatus::Active;
    h.is_healthy    = true;
    h.row_count     = 100;
    h.size_bytes    = 4096;
    h.start_time    = 1700000000LL;
    h.end_time      = 1700086400LL;
    h.status_message = "Active chunk";
    EXPECT_EQ(h.chunk_name, "test_chunk");
    EXPECT_TRUE(h.is_healthy);
    EXPECT_EQ(h.row_count, 100u);
}

TEST_F(HypertableFixture, GetChunkHealthDoesNotCrashWithMultipleInserts) {
    Hypertable ht(db.get(), makeConfig("health_multi", 3600, 30));
    for (int i = 0; i < 5; ++i) {
        ht.insert(1700000000LL + i * 3601, R"({"v": 1})");
    }
    EXPECT_NO_THROW(ht.getChunkHealth());
}
