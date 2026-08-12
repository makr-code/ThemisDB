#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <filesystem>
#include <string>
#include <vector>

using namespace themis;

// ── Test Fixture ─────────────────────────────────────────────────────────────

class BulkTransactionTest : public ::testing::Test {
protected:
    static constexpr const char* DB_PATH = "/tmp/themis_bulk_txn_test_db";

    void SetUp() override {
        std::filesystem::remove_all(DB_PATH);
        RocksDBWrapper::Config cfg;
        cfg.db_path = DB_PATH;
        cfg.enable_statistics = false;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        sec_idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<GraphIndexManager>(*db_);
        vec_idx_ = std::make_unique<VectorIndexManager>(*db_);
        mgr_ = std::make_unique<TransactionManager>(
            *db_, *sec_idx_, *graph_idx_, *vec_idx_);
    }

    void TearDown() override {
        mgr_.reset();
        vec_idx_.reset();
        graph_idx_.reset();
        sec_idx_.reset();
        db_->close();
        db_.reset();
        std::filesystem::remove_all(DB_PATH);
    }

    static BaseEntity makeEntity(const std::string& pk,
                                 const std::string& name = "",
                                 const std::string& city = "") {
        BaseEntity e;
        e.setPrimaryKey(pk);
        e.setField("name", name.empty() ? pk : name);
        if (!city.empty()) e.setField("city", city);
        return e;
    }

    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sec_idx_;
    std::unique_ptr<GraphIndexManager>     graph_idx_;
    std::unique_ptr<VectorIndexManager>    vec_idx_;
    std::unique_ptr<TransactionManager>    mgr_;
};

// ── bulkPutEntities ───────────────────────────────────────────────────────────

TEST_F(BulkTransactionTest, BulkPutEmptyVectorIsNoOp) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    std::vector<BaseEntity> empty;
    auto st = txn->bulkPutEntities("users", empty);
    EXPECT_TRUE(st.ok) << st.message;

    mgr_->commitTransaction(id);
}

TEST_F(BulkTransactionTest, BulkPutSingleEntity) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    auto st = txn->bulkPutEntities("users", {makeEntity("u1", "Alice")});
    EXPECT_TRUE(st.ok) << st.message;

    auto commit_st = mgr_->commitTransaction(id);
    EXPECT_TRUE(commit_st.ok) << commit_st.message;
}

TEST_F(BulkTransactionTest, BulkPutMultipleEntitiesCommit) {
    sec_idx_->createIndex("users", "city");

    std::vector<BaseEntity> batch;
    for (int i = 0; i < 5; ++i) {
        batch.push_back(makeEntity("u" + std::to_string(i), "User" + std::to_string(i), "Berlin"));
    }

    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    auto st = txn->bulkPutEntities("users", batch);
    EXPECT_TRUE(st.ok) << st.message;

    auto commit_st = mgr_->commitTransaction(id);
    EXPECT_TRUE(commit_st.ok) << commit_st.message;

    // Verify all entities are visible via secondary index
    auto [scan_st, keys] = sec_idx_->scanKeysEqual("users", "city", "Berlin");
    EXPECT_TRUE(scan_st.ok);
    EXPECT_EQ(keys.size(), 5u);
}

TEST_F(BulkTransactionTest, BulkPutRollbackLeavesNoData) {
    sec_idx_->createIndex("users", "city");

    std::vector<BaseEntity> batch;
    batch.push_back(makeEntity("u1", "Alice", "Paris"));
    batch.push_back(makeEntity("u2", "Bob", "Paris"));

    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    auto st = txn->bulkPutEntities("users", batch);
    EXPECT_TRUE(st.ok) << st.message;

    mgr_->rollbackTransaction(id);

    // After rollback nothing should be visible
    auto [scan_st, keys] = sec_idx_->scanKeysEqual("users", "city", "Paris");
    EXPECT_TRUE(scan_st.ok);
    EXPECT_TRUE(keys.empty());
}

TEST_F(BulkTransactionTest, BulkPutRejectsEmptyPrimaryKey) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    BaseEntity bad;
    // no primary key set
    auto st = txn->bulkPutEntities("users", {bad});
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("empty primary key"), std::string::npos);

    mgr_->rollbackTransaction(id);
}

TEST_F(BulkTransactionTest, BulkPutFailsOnFinishedTransaction) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    mgr_->commitTransaction(id);

    auto st = txn->bulkPutEntities("users", {makeEntity("u1")});
    EXPECT_FALSE(st.ok);
}

// ── bulkEraseEntities ─────────────────────────────────────────────────────────

TEST_F(BulkTransactionTest, BulkEraseEmptyVectorIsNoOp) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    std::vector<std::string> empty;
    auto st = txn->bulkEraseEntities("users", empty);
    EXPECT_TRUE(st.ok) << st.message;

    mgr_->commitTransaction(id);
}

TEST_F(BulkTransactionTest, BulkEraseRemovesInsertedEntities) {
    sec_idx_->createIndex("users", "city");

    // Insert entities first
    {
        auto id = mgr_->beginTransaction();
        auto txn = mgr_->getTransaction(id);
        for (int i = 0; i < 3; ++i) {
            txn->putEntity("users", makeEntity("u" + std::to_string(i), "", "London"));
        }
        mgr_->commitTransaction(id);
    }

    // Verify they exist
    {
        auto [scan_st, keys] = sec_idx_->scanKeysEqual("users", "city", "London");
        EXPECT_EQ(keys.size(), 3u);
    }

    // Bulk erase
    {
        auto id = mgr_->beginTransaction();
        auto txn = mgr_->getTransaction(id);

        std::vector<std::string> pks = {"u0", "u1", "u2"};
        auto st = txn->bulkEraseEntities("users", pks);
        EXPECT_TRUE(st.ok) << st.message;

        mgr_->commitTransaction(id);
    }

    // All should be gone
    auto [scan_st, keys] = sec_idx_->scanKeysEqual("users", "city", "London");
    EXPECT_TRUE(scan_st.ok);
    EXPECT_TRUE(keys.empty());
}

TEST_F(BulkTransactionTest, BulkEraseRejectsEmptyPrimaryKey) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    std::vector<std::string> pks = {"u1", ""};
    auto st = txn->bulkEraseEntities("users", pks);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("empty"), std::string::npos);

    mgr_->rollbackTransaction(id);
}

TEST_F(BulkTransactionTest, BulkEraseFailsOnFinishedTransaction) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    mgr_->rollbackTransaction(id);

    std::vector<std::string> pks = {"u1"};
    auto st = txn->bulkEraseEntities("users", pks);
    EXPECT_FALSE(st.ok);
}

// ── Mixed bulk put and erase in the same transaction ─────────────────────────

TEST_F(BulkTransactionTest, BulkPutAndEraseInSameTransaction) {
    // makeEntity stores the third arg under the "city" field
    sec_idx_->createIndex("users", "city");

    // Seed some initial data
    {
        auto id = mgr_->beginTransaction();
        auto txn = mgr_->getTransaction(id);
        txn->putEntity("users", makeEntity("existing1", "Existing1", "active"));
        txn->putEntity("users", makeEntity("existing2", "Existing2", "active"));
        mgr_->commitTransaction(id);
    }

    // In one transaction: bulk insert new records AND bulk erase old ones
    {
        auto id = mgr_->beginTransaction();
        auto txn = mgr_->getTransaction(id);

        std::vector<BaseEntity> inserts = {
            makeEntity("new1", "New1", "pending"),
            makeEntity("new2", "New2", "pending"),
        };
        auto put_st = txn->bulkPutEntities("users", inserts);
        EXPECT_TRUE(put_st.ok) << put_st.message;

        std::vector<std::string> deletes = {"existing1", "existing2"};
        auto erase_st = txn->bulkEraseEntities("users", deletes);
        EXPECT_TRUE(erase_st.ok) << erase_st.message;

        mgr_->commitTransaction(id);
    }

    // New records present
    auto [st_pending, pending_keys] = sec_idx_->scanKeysEqual("users", "city", "pending");
    EXPECT_EQ(pending_keys.size(), 2u);

    // Old records gone
    auto [st_active, active_keys] = sec_idx_->scanKeysEqual("users", "city", "active");
    EXPECT_TRUE(active_keys.empty());
}

// ── Large batch stress test ───────────────────────────────────────────────────

TEST_F(BulkTransactionTest, BulkPutLargeBatchCommit) {
    constexpr size_t N = 200;

    std::vector<BaseEntity> batch;
    batch.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        batch.push_back(makeEntity("key" + std::to_string(i)));
    }

    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    auto st = txn->bulkPutEntities("items", batch);
    EXPECT_TRUE(st.ok) << st.message;

    auto commit_st = mgr_->commitTransaction(id);
    EXPECT_TRUE(commit_st.ok) << commit_st.message;
}
