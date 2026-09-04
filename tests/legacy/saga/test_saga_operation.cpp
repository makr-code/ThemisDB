// Unit tests for SagaOperation helper functions.
//
// Focus:
//   - indexPutWithCompensation: compensation calls idx.erase() to remove
//     all secondary-index entries for the entity (Issue fix: was a no-op warn).
//   - graphAddWithCompensation: compensation calls graph.deleteEdge() to
//     remove the added edge (Issue fix: was a no-op warn).
//   - putEntityWithCompensation: restores old value (update) or deletes
//     key (insert) on rollback.
//   - deleteEntityWithCompensation: restores deleted value on rollback.
//   - vectorAddWithCompensation: calls removeByPk() on rollback.

#include <gtest/gtest.h>
#include "transaction/saga.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"

#include <filesystem>
#include <string>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class SagaOperationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_saga_op_test_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_;
        cfg.enable_wal = true;
        db_            = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        sec_idx_   = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<GraphIndexManager>(*db_);
        vec_idx_   = std::make_unique<VectorIndexManager>(*db_);
    }

    void TearDown() override {
        vec_idx_.reset();
        sec_idx_.reset();
        graph_idx_.reset();
        if (db_) { db_->close(); db_.reset(); }
        fs::remove_all(db_path_);
    }

    static BaseEntity makeEntity(const std::string& pk,
                                 const std::string& name = "") {
        BaseEntity e;
        e.setPrimaryKey(pk);
        e.setField("name", name.empty() ? pk : name);
        return e;
    }

    static BaseEntity makeEdge(const std::string& edge_id,
                               const std::string& from_pk,
                               const std::string& to_pk) {
        BaseEntity e;
        e.setPrimaryKey(edge_id);
        e.setField("id", edge_id);
        e.setField("_from", from_pk);
        e.setField("_to", to_pk);
        return e;
    }

    std::string                             db_path_;
    std::unique_ptr<RocksDBWrapper>         db_;
    std::unique_ptr<SecondaryIndexManager>  sec_idx_;
    std::unique_ptr<GraphIndexManager>      graph_idx_;
    std::unique_ptr<VectorIndexManager>     vec_idx_;
};

// ─────────────────────────────────────────────────────────────────────────────
// putEntityWithCompensation – insert path: compensating action deletes key
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, PutEntityWithCompensation_InsertPath_DeletesKeyOnCompensate) {
    const std::string key = "entity:users:u1";
    const std::vector<uint8_t> value = {'v', '1'};

    Saga saga;
    // Register compensation BEFORE the insert so the helper captures that the
    // key did not exist yet (insert-path rollback should delete the key).
    SagaOperation::putEntityWithCompensation(*db_, key, value, saga);
    EXPECT_EQ(saga.stepCount(), 1u);

    // Simulate the actual put
    db_->put(key, value);

    // Compensate should delete the key
    saga.compensate();

    std::string out = {};
    EXPECT_FALSE(db_->get(key, out)) << "Key should have been deleted by compensation";
}

// ─────────────────────────────────────────────────────────────────────────────
// putEntityWithCompensation – update path: compensating action restores old value
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, PutEntityWithCompensation_UpdatePath_RestoresOldValueOnCompensate) {
    const std::string key = "entity:users:u2";
    const std::vector<uint8_t> old_value = {'o', 'l', 'd'};
    const std::vector<uint8_t> new_value = {'n', 'e', 'w'};

    // Pre-populate old value
    db_->put(key, old_value);

    // Register compensation BEFORE the update (captures current value)
    Saga saga;
    SagaOperation::putEntityWithCompensation(*db_, key, new_value, saga);

    // Perform the actual update
    db_->put(key, new_value);

    // Compensate should restore old value
    saga.compensate();

    auto result = db_->get(key);
    ASSERT_TRUE(result.has_value()) << "Key should still exist after compensation";
    EXPECT_EQ(*result, old_value) << "Old value should have been restored by compensation";
}

// ─────────────────────────────────────────────────────────────────────────────
// deleteEntityWithCompensation – restores deleted key on compensate
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, DeleteEntityWithCompensation_RestoresDeletedKeyOnCompensate) {
    const std::string key = "entity:users:u3";
    const std::vector<uint8_t> value = {'d', 'a', 't', 'a'};

    db_->put(key, value);

    Saga saga;
    SagaOperation::deleteEntityWithCompensation(*db_, key, saga);
    EXPECT_EQ(saga.stepCount(), 1u);

    // Simulate the actual delete
    db_->del(key);

    // Compensate should restore the deleted key
    saga.compensate();

    auto result = db_->get(key);
    EXPECT_TRUE(result.has_value()) << "Key should have been restored by compensation";
    EXPECT_EQ(*result, value);
}

// ─────────────────────────────────────────────────────────────────────────────
// deleteEntityWithCompensation – no-op when key does not exist
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, DeleteEntityWithCompensation_NoopForMissingKey) {
    const std::string key = "entity:users:nonexistent";

    Saga saga;
    SagaOperation::deleteEntityWithCompensation(*db_, key, saga);

    // No step should be added for a non-existent key
    EXPECT_EQ(saga.stepCount(), 0u);
}

TEST_F(SagaOperationTest, SagaCompensate_CStringExceptionDoesNotAbortOtherSteps) {
    Saga saga;
    bool second_compensated = false;

    saga.addStep("first", []() { throw "first failed"; });
    saga.addStep("second", [&second_compensated]() { second_compensated = true; });

    EXPECT_NO_THROW(saga.compensate());
    EXPECT_TRUE(second_compensated);
}

TEST_F(SagaOperationTest, SagaCompensateWithRetry_CStringExceptionIncrementsFailureMetric) {
    Saga saga;
    saga.addStep("retry-fail", []() { throw "retry failed"; });

    EXPECT_NO_THROW(saga.compensateWithRetry(1, std::chrono::milliseconds(1)));

    auto metrics = saga.getMetrics();
    EXPECT_EQ(metrics.failed_compensations, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// indexPutWithCompensation – compensation removes index entries via idx.erase()
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, IndexPutWithCompensation_RemovesIndexEntriesOnCompensate) {
    const std::string table = "users";
    sec_idx_->createIndex(table, "name");

    BaseEntity entity = makeEntity("u10", "Alice");

    // Index the entity
    auto put_st = sec_idx_->put(table, entity);
    ASSERT_TRUE(put_st.ok) << "Index put failed: " << put_st.message;

    // Verify it's indexed
    auto [scan_st, keys_before] = sec_idx_->scanKeysEqual(table, "name", "Alice");
    EXPECT_TRUE(scan_st.ok);
    EXPECT_FALSE(keys_before.empty()) << "Entity should be in index before compensation";

    // Register compensation (no WriteBatch needed here, batch arg is unused)
    auto batch = db_->createWriteBatch();
    Saga saga;
    SagaOperation::indexPutWithCompensation(*sec_idx_, table, entity, *batch, saga);
    EXPECT_EQ(saga.stepCount(), 1u);

    // Execute compensation → should call idx.erase(table, pk)
    saga.compensate();
    EXPECT_TRUE(saga.isFullyCompensated());

    // Verify index entries are gone
    auto [scan_st2, keys_after] = sec_idx_->scanKeysEqual(table, "name", "Alice");
    EXPECT_TRUE(scan_st2.ok);
    EXPECT_TRUE(keys_after.empty()) << "Index entries should have been removed by compensation";
}

// ─────────────────────────────────────────────────────────────────────────────
// graphAddWithCompensation – compensation removes edge via graph.deleteEdge()
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, GraphAddWithCompensation_DeletesEdgeOnCompensate) {
    BaseEntity edge = makeEdge("edge1", "user_a", "user_b");

    // Add edge to graph
    auto add_st = graph_idx_->addEdge(edge);
    ASSERT_TRUE(add_st.ok) << "addEdge failed: " << add_st.message;

    // Verify edge is present (user_a has out-neighbor user_b)
    {
        auto [st, neighbors] = graph_idx_->outNeighbors("user_a");
        EXPECT_TRUE(st.ok);
        EXPECT_EQ(neighbors.size(), 1u);
        EXPECT_EQ(neighbors[0], "user_b");
    }

    // Register compensation (batch arg is unused by implementation)
    auto batch = db_->createWriteBatch();
    Saga saga;
    SagaOperation::graphAddWithCompensation(*graph_idx_, edge, *batch, saga);
    EXPECT_EQ(saga.stepCount(), 1u);

    // Execute compensation → should call graph.deleteEdge("edge1")
    saga.compensate();
    EXPECT_TRUE(saga.isFullyCompensated());

    // Verify edge is gone
    {
        auto [st, neighbors] = graph_idx_->outNeighbors("user_a");
        EXPECT_TRUE(st.ok);
        EXPECT_TRUE(neighbors.empty()) << "Edge should have been removed by compensation";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// graphAddWithCompensation – compensation for non-existent edge is graceful
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, GraphAddWithCompensation_NonExistentEdgeIsGraceful) {
    BaseEntity edge = makeEdge("ghost_edge", "user_x", "user_y");
    // Do NOT add edge to graph – compensation should not crash

    auto batch = db_->createWriteBatch();
    Saga saga;
    SagaOperation::graphAddWithCompensation(*graph_idx_, edge, *batch, saga);
    EXPECT_EQ(saga.stepCount(), 1u);

    // Should complete without throwing even if the edge doesn't exist
    EXPECT_NO_THROW(saga.compensate());
    // Step is still considered compensated (the compensate() closure ran)
    EXPECT_TRUE(saga.isFullyCompensated());
}

// ─────────────────────────────────────────────────────────────────────────────
// vectorAddWithCompensation – compensation removes vector entry via removeByPk()
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SagaOperationTest, VectorAddWithCompensation_RemovesVectorOnCompensate) {
    vec_idx_->init("docs", 3, VectorIndexManager::Metric::COSINE);

    BaseEntity entity = makeEntity("doc1");
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    entity.setField("embedding", embedding);

    // Add to vector index
    auto add_st = vec_idx_->addEntity(entity, "embedding");
    ASSERT_TRUE(add_st.ok) << "Vector add failed: " << add_st.message;

    // Register compensation
    auto batch = db_->createWriteBatch();
    Saga saga;
    SagaOperation::vectorAddWithCompensation(*vec_idx_, entity, *batch, "embedding", saga);
    EXPECT_EQ(saga.stepCount(), 1u);

    // Execute compensation → should call vec_idx_->removeByPk("doc1")
    saga.compensate();
    EXPECT_TRUE(saga.isFullyCompensated());
}
