#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "transaction/lock_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <filesystem>
#include <string>

using namespace themis;

// ── Test Fixture ─────────────────────────────────────────────────────────────

class OccTest : public ::testing::Test {
protected:
    static constexpr const char* DB_PATH = "/tmp/themis_occ_test_db";

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
        db_.reset();
        std::filesystem::remove_all(DB_PATH);
    }

    // Create a minimal BaseEntity with a given primary key
    static BaseEntity makeEntity(const std::string& pk, const std::string& name = "") {
        BaseEntity e;
        e.setPrimaryKey(pk);
        e.setField("name", name.empty() ? pk : name);
        return e;
    }

    std::unique_ptr<RocksDBWrapper>         db_;
    std::unique_ptr<SecondaryIndexManager>  sec_idx_;
    std::unique_ptr<GraphIndexManager>      graph_idx_;
    std::unique_ptr<VectorIndexManager>     vec_idx_;
    std::unique_ptr<TransactionManager>     mgr_;
};

// ── getEntityVersion ─────────────────────────────────────────────────────────

TEST_F(OccTest, GetVersionNonExistentIsZero) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);

    auto ver = txn->getEntityVersion("users", "u1");
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(*ver, 0u);

    mgr_->rollbackTransaction(id);
}

TEST_F(OccTest, GetVersionAfterOptimisticPutIsOne) {
    // Insert via optimisticPut with expected_version=0
    auto id1 = mgr_->beginTransaction();
    auto txn1 = mgr_->getTransaction(id1);
    ASSERT_NE(txn1, nullptr);
    auto st = txn1->optimisticPut("users", makeEntity("u1"), 0);
    EXPECT_TRUE(st.ok) << st.message;
    mgr_->commitTransaction(id1);

    // Now read the version in a new transaction
    auto id2 = mgr_->beginTransaction();
    auto txn2 = mgr_->getTransaction(id2);
    auto ver = txn2->getEntityVersion("users", "u1");
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(*ver, 1u);
    mgr_->rollbackTransaction(id2);
}

// ── optimisticPut – new entity ────────────────────────────────────────────────

TEST_F(OccTest, OptimisticPutCreateNewEntity) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    auto st = txn->optimisticPut("products", makeEntity("p42", "Widget"), 0);
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_EQ(mgr_->commitTransaction(id).ok, true);
}

TEST_F(OccTest, OptimisticPutCreateFailsIfAlreadyExists) {
    // First insert
    auto id1 = mgr_->beginTransaction();
    mgr_->getTransaction(id1)->optimisticPut("products", makeEntity("p1"), 0);
    mgr_->commitTransaction(id1);

    // Second insert with expected_version=0 must fail
    auto id2 = mgr_->beginTransaction();
    auto txn2 = mgr_->getTransaction(id2);
    auto st = txn2->optimisticPut("products", makeEntity("p1"), 0);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("OCC entity already exists"), std::string::npos) << st.message;
    mgr_->rollbackTransaction(id2);
}

// ── optimisticPut – update entity ────────────────────────────────────────────

TEST_F(OccTest, OptimisticPutUpdateWithCorrectVersion) {
    // Insert v1
    {
        auto id = mgr_->beginTransaction();
        mgr_->getTransaction(id)->optimisticPut("users", makeEntity("u99", "Alice"), 0);
        mgr_->commitTransaction(id);
    }
    // Update to v2
    {
        auto id = mgr_->beginTransaction();
        auto st = mgr_->getTransaction(id)->optimisticPut("users", makeEntity("u99", "Alice Updated"), 1);
        EXPECT_TRUE(st.ok) << st.message;
        mgr_->commitTransaction(id);
    }
    // Check version is now 2
    {
        auto id = mgr_->beginTransaction();
        auto ver = mgr_->getTransaction(id)->getEntityVersion("users", "u99");
        ASSERT_TRUE(ver.has_value());
        EXPECT_EQ(*ver, 2u);
        mgr_->rollbackTransaction(id);
    }
}

TEST_F(OccTest, OptimisticPutUpdateFailsOnVersionConflict) {
    // Insert entity at version 1
    {
        auto id = mgr_->beginTransaction();
        mgr_->getTransaction(id)->optimisticPut("users", makeEntity("u5"), 0);
        mgr_->commitTransaction(id);
    }
    // Try to update with a stale non-zero expected_version (expected=5, actual=1)
    // This exercises the "OCC version conflict" code path (expected_version != current_version
    // where both are non-zero).
    auto id = mgr_->beginTransaction();
    auto st = mgr_->getTransaction(id)->optimisticPut("users", makeEntity("u5", "Bob"), 5);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("OCC version conflict"), std::string::npos) << st.message;
    EXPECT_NE(st.message.find("expected=5"), std::string::npos) << st.message;
    EXPECT_NE(st.message.find("actual=1"),   std::string::npos) << st.message;
    mgr_->rollbackTransaction(id);
}

TEST_F(OccTest, OptimisticPutVersionConflictShowsExpectedAndActual) {
    // Create entity (version 1)
    {
        auto id = mgr_->beginTransaction();
        mgr_->getTransaction(id)->optimisticPut("users", makeEntity("u10"), 0);
        mgr_->commitTransaction(id);
    }
    // Update it to version 2
    {
        auto id = mgr_->beginTransaction();
        mgr_->getTransaction(id)->optimisticPut("users", makeEntity("u10", "updated"), 1);
        mgr_->commitTransaction(id);
    }
    // Try with expected=1 (actual is 2)
    auto id = mgr_->beginTransaction();
    auto st = mgr_->getTransaction(id)->optimisticPut("users", makeEntity("u10", "stale"), 1);
    EXPECT_FALSE(st.ok);
    // Message should contain expected and actual version numbers
    EXPECT_NE(st.message.find("expected=1"), std::string::npos) << st.message;
    EXPECT_NE(st.message.find("actual=2"),   std::string::npos) << st.message;
    mgr_->rollbackTransaction(id);
}

// ── optimisticErase ───────────────────────────────────────────────────────────

TEST_F(OccTest, OptimisticEraseSuccess) {
    // Create entity
    {
        auto id = mgr_->beginTransaction();
        mgr_->getTransaction(id)->optimisticPut("items", makeEntity("i1"), 0);
        mgr_->commitTransaction(id);
    }
    // Erase with correct version
    {
        auto id = mgr_->beginTransaction();
        auto st = mgr_->getTransaction(id)->optimisticErase("items", "i1", 1);
        EXPECT_TRUE(st.ok) << st.message;
        mgr_->commitTransaction(id);
    }
    // Version should now be 0 (entity gone)
    {
        auto id = mgr_->beginTransaction();
        auto ver = mgr_->getTransaction(id)->getEntityVersion("items", "i1");
        ASSERT_TRUE(ver.has_value());
        EXPECT_EQ(*ver, 0u);
        mgr_->rollbackTransaction(id);
    }
}

TEST_F(OccTest, OptimisticEraseFailsWhenEntityNotFound) {
    auto id = mgr_->beginTransaction();
    auto st = mgr_->getTransaction(id)->optimisticErase("items", "nonexistent", 1);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("OCC entity not found"), std::string::npos) << st.message;
    mgr_->rollbackTransaction(id);
}

TEST_F(OccTest, OptimisticEraseFailsOnVersionConflict) {
    // Create entity (version 1)
    {
        auto id = mgr_->beginTransaction();
        mgr_->getTransaction(id)->optimisticPut("items", makeEntity("i2"), 0);
        mgr_->commitTransaction(id);
    }
    // Try erase with wrong version (0 instead of 1)
    auto id = mgr_->beginTransaction();
    auto st = mgr_->getTransaction(id)->optimisticErase("items", "i2", 0);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("OCC version conflict"), std::string::npos) << st.message;
    mgr_->rollbackTransaction(id);
}

// ── OCC retry pattern ─────────────────────────────────────────────────────────

TEST_F(OccTest, ReadVersionThenOptimisticPutPattern) {
    // Insert initial entity
    {
        auto id = mgr_->beginTransaction();
        mgr_->getTransaction(id)->optimisticPut("counters", makeEntity("c1", "0"), 0);
        mgr_->commitTransaction(id);
    }

    // Read-modify-write pattern (typical OCC usage)
    bool committed = false;
    for (int attempt = 0; attempt < 3 && !committed; ++attempt) {
        auto id = mgr_->beginTransaction();
        auto txn = mgr_->getTransaction(id);

        auto ver = txn->getEntityVersion("counters", "c1");
        ASSERT_TRUE(ver.has_value());

        auto st = txn->optimisticPut("counters", makeEntity("c1", "1"), *ver);
        if (st.ok) {
            auto commit_st = mgr_->commitTransaction(id);
            committed = commit_st.ok;
        } else {
            mgr_->rollbackTransaction(id);
        }
    }
    EXPECT_TRUE(committed);
}

// ── Isolation level interaction ───────────────────────────────────────────────

TEST_F(OccTest, OccWorksWithAllIsolationLevels) {
    for (auto iso : {IsolationLevel::READ_COMMITTED,
                     IsolationLevel::REPEATABLE_READ,
                     IsolationLevel::SERIALIZABLE}) {
        std::string pk = "iso_test_" + std::to_string(static_cast<int>(iso));

        auto id = mgr_->beginTransaction(iso);
        auto txn = mgr_->getTransaction(id);
        ASSERT_NE(txn, nullptr);

        // getEntityVersion
        auto ver = txn->getEntityVersion("iso_table", pk);
        ASSERT_TRUE(ver.has_value());
        EXPECT_EQ(*ver, 0u);

        // optimisticPut
        auto st = txn->optimisticPut("iso_table", makeEntity(pk), 0);
        EXPECT_TRUE(st.ok) << "isolation=" << static_cast<int>(iso) << " err=" << st.message;

        mgr_->commitTransaction(id);
    }
}

// ── getEntityVersion returns nullopt when transaction is finished ─────────────

TEST_F(OccTest, GetVersionReturnsNulloptWhenFinished) {
    auto id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    mgr_->rollbackTransaction(id);

    // Transaction is finished — mvcc_txn_ is no longer active
    auto ver = txn->getEntityVersion("users", "u1");
    EXPECT_FALSE(ver.has_value());
}
