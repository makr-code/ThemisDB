/**
 * @file test_provenance_store.cpp
 * @brief Regression tests for Persistent Provenance Store (GAP-4.1).
 *
 * Validates RocksDB-backed provenance storage with:
 * - ACID record persistence
 * - Efficient range queries (query_id, time range)
 * - Provenance chain ordering and completeness
 */

#include "observability/provenance_store.h"

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>

namespace themis { namespace observability { namespace test { 

namespace fs = std::filesystem;

class ProvenanceStoreTest : public ::testing::Test {
protected:
    [[nodiscard]] std::string testDbPath() const {
        const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        return std::string("test_provenance_store_db_") +
               (test_info ? test_info->name() : "default");
    }

    void SetUp() override {
        const auto db_path = testDbPath();
        if (fs::exists(db_path)) {
            std::error_code ec = {};
            fs::remove_all(db_path, ec);
        }
    }

    void TearDown() override {
        const auto db_path = testDbPath();
        if (fs::exists(db_path)) {
            std::error_code ec = {};
            fs::remove_all(db_path, ec);
        }
    }

    [[nodiscard]] ProvenanceStepRecord makeRecord(const std::string& query_id,
                                                   int step,
                                                   int64_t timestamp_ms) {
        ProvenanceStepRecord rec;
        rec.query_id                  = query_id;
        rec.step_number               = step;
        rec.layer_name                = "test_layer";
        rec.timestamp_ms              = timestamp_ms;
        rec.correlation_id            = "corr-" + std::to_string(step);
        rec.source_layer              = "test_source";
        rec.input_vector_hash         = "hash-" + std::to_string(step);
        rec.num_candidates            = 10 + step;
        rec.num_selected              = 5 + step;
        rec.shard_id                  = "shard-0";
        rec.backend_name              = "test_backend";
        rec.routing_reason_code       = "TEST_ROUTING";
        rec.fallback_mode             = "NONE";
        rec.confidence_policy_version = "1.0";
        rec.decision_duration_us      = 100 + step;
        return rec;
    }
};

TEST_F(ProvenanceStoreTest, StoreAndRetrieveSingleRecord) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    const auto rec = makeRecord("query-001", 0, 1000);
    ASSERT_TRUE(store.storeRecord("query-001", 0, rec));

    const auto retrieved = store.getRecord("query-001", 0);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->query_id, "query-001");
    EXPECT_EQ(retrieved->step_number, 0);
    EXPECT_EQ(retrieved->timestamp_ms, 1000);
    EXPECT_EQ(retrieved->layer_name, "test_layer");
}

TEST_F(ProvenanceStoreTest, RetrieveNonexistentRecord) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    const auto retrieved = store.getRecord("nonexistent", 0);
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(ProvenanceStoreTest, StoreAndQueryProvenanceChain) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    const std::string query_id = "query-chain-001";
    for (int step = 0; step < 5; ++step) {
        const auto rec = makeRecord(query_id, step, 1000 + step * 100);
        ASSERT_TRUE(store.storeRecord(query_id, step, rec));
    }

    const auto chain = store.getProvenanceChain(query_id);
    ASSERT_EQ(chain.size(), 5u);

    for (size_t i = 0; i < chain.size(); ++i) {
        EXPECT_EQ(chain[i].step_number, static_cast<int>(i));
        EXPECT_EQ(chain[i].timestamp_ms, 1000 + static_cast<int64_t>(i) * 100);
    }
}

TEST_F(ProvenanceStoreTest, ChainOrderingByStepNumber) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    const std::string query_id = "query-order-001";
    // Store in reverse order.
    for (int step = 4; step >= 0; --step) {
        const auto rec = makeRecord(query_id, step, 1000 + step * 100);
        ASSERT_TRUE(store.storeRecord(query_id, step, rec));
    }

    // Verify chain is sorted by step_number.
    const auto chain = store.getProvenanceChain(query_id);
    ASSERT_EQ(chain.size(), 5u);
    for (size_t i = 0; i < chain.size(); ++i) {
        EXPECT_EQ(chain[i].step_number, static_cast<int>(i));
    }
}

TEST_F(ProvenanceStoreTest, QueryByTimeRange) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    // Store records from two queries with different timestamps.
    const auto rec1 = makeRecord("query-001", 0, 1000);
    const auto rec2 = makeRecord("query-001", 1, 2000);
    const auto rec3 = makeRecord("query-002", 0, 1500);

    ASSERT_TRUE(store.storeRecord("query-001", 0, rec1));
    ASSERT_TRUE(store.storeRecord("query-001", 1, rec2));
    ASSERT_TRUE(store.storeRecord("query-002", 0, rec3));

    // Query range [1200, 1800].
    const auto range = store.getRecordsByTimeRange(1200, 1800);
    ASSERT_GE(range.size(), 1u);
    EXPECT_EQ(range[0].query_id, "query-002");
    EXPECT_EQ(range[0].timestamp_ms, 1500);
}

TEST_F(ProvenanceStoreTest, TimeRangeIncludesEndpoints) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    const auto rec1 = makeRecord("query-001", 0, 1000);
    const auto rec2 = makeRecord("query-001", 1, 2000);
    const auto rec3 = makeRecord("query-001", 2, 3000);

    ASSERT_TRUE(store.storeRecord("query-001", 0, rec1));
    ASSERT_TRUE(store.storeRecord("query-001", 1, rec2));
    ASSERT_TRUE(store.storeRecord("query-001", 2, rec3));

    // Query range [1000, 2000] should include both endpoints.
    const auto range = store.getRecordsByTimeRange(1000, 2000);
    ASSERT_GE(range.size(), 2u);
    EXPECT_EQ(range[0].timestamp_ms, 1000);
    EXPECT_EQ(range[1].timestamp_ms, 2000);
}

TEST_F(ProvenanceStoreTest, ListQueryIds) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    // Store records for multiple queries.
    ASSERT_TRUE(store.storeRecord("query-001", 0, makeRecord("query-001", 0, 1000)));
    ASSERT_TRUE(store.storeRecord("query-002", 0, makeRecord("query-002", 0, 1000)));
    ASSERT_TRUE(store.storeRecord("query-003", 0, makeRecord("query-003", 0, 1000)));

    const auto query_ids = store.listQueryIds();
    ASSERT_EQ(query_ids.size(), 3u);
    EXPECT_TRUE(std::find(query_ids.begin(), query_ids.end(), "query-001") != query_ids.end());
    EXPECT_TRUE(std::find(query_ids.begin(), query_ids.end(), "query-002") != query_ids.end());
    EXPECT_TRUE(std::find(query_ids.begin(), query_ids.end(), "query-003") != query_ids.end());
}

TEST_F(ProvenanceStoreTest, DeleteQuery) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    const std::string query_id = "query-delete-001";
    for (int step = 0; step < 3; ++step) {
        ASSERT_TRUE(store.storeRecord(query_id, step, makeRecord(query_id, step, 1000 + step * 100)));
    }

    // Verify records exist.
    ASSERT_EQ(store.getProvenanceChain(query_id).size(), 3u);

    // Delete the query.
    ASSERT_TRUE(store.deleteQuery(query_id));

    // Verify records are gone.
    EXPECT_EQ(store.getProvenanceChain(query_id).size(), 0u);
}

TEST_F(ProvenanceStoreTest, FlushWrites) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    const auto rec = makeRecord("query-flush-001", 0, 1000);
    ASSERT_TRUE(store.storeRecord("query-flush-001", 0, rec));

    // Flush to ensure durability.
    ASSERT_TRUE(store.flush());

    // Records should still be retrievable after flush.
    const auto retrieved = store.getRecord("query-flush-001", 0);
    ASSERT_TRUE(retrieved.has_value());
}

TEST_F(ProvenanceStoreTest, MultipleQueriesIndependent) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    RocksDBProvenanceStore store(cfg);

    // Store chains for two independent queries.
    for (int q = 1; q <= 2; ++q) {
        const auto query_id = "query-multi-" + std::to_string(q);
        for (int step = 0; step < 3; ++step) {
            const auto rec = makeRecord(query_id, step, 1000 + q * 1000 + step * 100);
            ASSERT_TRUE(store.storeRecord(query_id, step, rec));
        }
    }

    // Verify independence.
    const auto chain1 = store.getProvenanceChain("query-multi-1");
    const auto chain2 = store.getProvenanceChain("query-multi-2");

    ASSERT_EQ(chain1.size(), 3u);
    ASSERT_EQ(chain2.size(), 3u);
    EXPECT_EQ(chain1[0].query_id, "query-multi-1");
    EXPECT_EQ(chain2[0].query_id, "query-multi-2");
}

TEST_F(ProvenanceStoreTest, RetentionMaxRecordsKeepsMostRecentRecords) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    cfg.retention_max_records = 2;
    RocksDBProvenanceStore store(cfg);

    ASSERT_TRUE(store.storeRecord("query-ret", 0, makeRecord("query-ret", 0, 1000)));
    ASSERT_TRUE(store.storeRecord("query-ret", 1, makeRecord("query-ret", 1, 2000)));
    ASSERT_TRUE(store.storeRecord("query-ret", 2, makeRecord("query-ret", 2, 3000)));

    const auto chain = store.getProvenanceChain("query-ret");
    ASSERT_EQ(chain.size(), 2u);
    EXPECT_EQ(chain[0].step_number, 1);
    EXPECT_EQ(chain[1].step_number, 2);
}

TEST_F(ProvenanceStoreTest, RetentionMaxAgePrunesOldRecordsOnWrite) {
    RocksDBProvenanceStore::Config cfg;
    cfg.db_path = testDbPath();
    cfg.retention_max_age_ms = 1000;
    RocksDBProvenanceStore store(cfg);

    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    ASSERT_TRUE(store.storeRecord("query-age", 0, makeRecord("query-age", 0, now_ms - 6000)));
    ASSERT_TRUE(store.storeRecord("query-age", 1, makeRecord("query-age", 1, now_ms - 100)));

    const auto chain = store.getProvenanceChain("query-age");
    ASSERT_EQ(chain.size(), 1u);
    EXPECT_EQ(chain[0].step_number, 1);
}
} } } // namespace themis::observability::test
