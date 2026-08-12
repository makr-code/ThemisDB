/// @file test_tenant_transaction_namespace.cpp
/// @brief Unit tests for the per-tenant transaction isolation namespace feature.
///
/// Verifies that:
///   1. beginTransaction(tenant_id, isolation) creates a tenant-scoped transaction.
///   2. getTenantId() returns the correct tenant ID.
///   3. Entity keys are stored under a tenant-specific prefix, so data written
///      by tenant-A is invisible to tenant-B (and vice versa).
///   4. Per-tenant statistics are tracked (total_begun / committed / aborted).
///   5. getActiveTenantTransactionCount() and listTenantTransactionIds() report
///      only the transactions belonging to the queried tenant.
///   6. abortTenantTransactions() rolls back all active transactions for a tenant
///      and leaves other tenants' transactions untouched.
///   7. An empty tenant_id falls through to the global (non-tenant) namespace.

#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <filesystem>
#include <string>

using namespace themis;

// ── Test Fixture ─────────────────────────────────────────────────────────────

class TenantTxnTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() / "themis_tenant_txn_test_db").string();
        std::filesystem::remove_all(db_path_);
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.enable_statistics = false;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        sec_idx_   = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<GraphIndexManager>(*db_);
        vec_idx_   = std::make_unique<VectorIndexManager>(*db_);
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
        std::filesystem::remove_all(db_path_);
    }

    static BaseEntity makeEntity(const std::string& pk,
                                  const std::string& value = "v") {
        BaseEntity e;
        e.setPrimaryKey(pk);
        e.setField("data", value);
        return e;
    }

    std::string                            db_path_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sec_idx_;
    std::unique_ptr<GraphIndexManager>     graph_idx_;
    std::unique_ptr<VectorIndexManager>    vec_idx_;
    std::unique_ptr<TransactionManager>    mgr_;
};

// ── getTenantId ───────────────────────────────────────────────────────────────

TEST_F(TenantTxnTest, GetTenantId_NoTenant_ReturnsEmpty) {
    auto id  = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);
    EXPECT_TRUE(txn->getTenantId().empty());
    mgr_->rollbackTransaction(id);
}

TEST_F(TenantTxnTest, GetTenantId_WithTenant_ReturnsCorrectId) {
    auto id  = mgr_->beginTransaction("acme-corp");
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);
    EXPECT_EQ(txn->getTenantId(), "acme-corp");
    mgr_->rollbackTransaction(id);
}

TEST_F(TenantTxnTest, EmptyTenantId_FallsThroughToGlobalNamespace) {
    // An empty tenant_id must behave identically to beginTransaction().
    auto id  = mgr_->beginTransaction("", IsolationLevel::ReadCommitted);
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);
    EXPECT_TRUE(txn->getTenantId().empty());
    mgr_->rollbackTransaction(id);
}

// ── Key namespace isolation ───────────────────────────────────────────────────

TEST_F(TenantTxnTest, TenantADataNotVisibleToTenantB) {
    // Write an entity for tenant-A.
    {
        auto id  = mgr_->beginTransaction("tenant-a");
        auto txn = mgr_->getTransaction(id);
        ASSERT_NE(txn, nullptr);
        ASSERT_TRUE(txn->putEntity("users", makeEntity("user1", "alice")).ok);
        ASSERT_TRUE(mgr_->commitTransaction(id).ok);
    }

    // A raw (global-namespace) read under the un-prefixed key must not find it.
    auto raw_key_no_prefix = std::string("entity:users:user1");
    auto raw = db_->get(raw_key_no_prefix);
    EXPECT_FALSE(raw.has_value())
        << "Tenant-A entity must not be visible under the global key prefix";

    // A different tenant must not see the entity through its own transaction.
    {
        auto id  = mgr_->beginTransaction("tenant-b");
        auto txn = mgr_->getTransaction(id);
        ASSERT_NE(txn, nullptr);

        // The tenant-B transaction looks up the same logical key but under the
        // "tenant:tenant-b:" prefix, which was never written → not found.
        auto tenant_b_key = std::string("tenant:tenant-b:entity:users:user1");
        auto found = db_->get(tenant_b_key);
        EXPECT_FALSE(found.has_value())
            << "Tenant-B must not see Tenant-A's entity";

        mgr_->rollbackTransaction(id);
    }
}

TEST_F(TenantTxnTest, TwoTenantsCanStoreIndependentEntitiesForSameKey) {
    // Both tenant-A and tenant-B write "record:1" – they should not conflict.
    auto id_a = mgr_->beginTransaction("tenant-a");
    auto id_b = mgr_->beginTransaction("tenant-b");

    auto txn_a = mgr_->getTransaction(id_a);
    auto txn_b = mgr_->getTransaction(id_b);
    ASSERT_NE(txn_a, nullptr);
    ASSERT_NE(txn_b, nullptr);

    EXPECT_TRUE(txn_a->putEntity("records", makeEntity("record:1", "from-a")).ok);
    EXPECT_TRUE(txn_b->putEntity("records", makeEntity("record:1", "from-b")).ok);

    // Both commits must succeed (no write-write conflict because the namespaced
    // keys are distinct: "tenant:tenant-a:entity:records:record:1" vs
    // "tenant:tenant-b:entity:records:record:1").
    EXPECT_TRUE(mgr_->commitTransaction(id_a).ok);
    EXPECT_TRUE(mgr_->commitTransaction(id_b).ok);

    // Verify that the tenant-A key was physically written with the correct prefix.
    auto key_a = std::string("tenant:tenant-a:entity:records:record:1");
    EXPECT_TRUE(db_->get(key_a).has_value())
        << "Tenant-A entity should be stored under the tenant-A prefix";

    auto key_b = std::string("tenant:tenant-b:entity:records:record:1");
    EXPECT_TRUE(db_->get(key_b).has_value())
        << "Tenant-B entity should be stored under the tenant-B prefix";
}

// ── Per-tenant statistics ─────────────────────────────────────────────────────

TEST_F(TenantTxnTest, PerTenantStats_AfterBeginCommitRollback) {
    // Start two transactions for "stats-tenant", commit one, rollback the other.
    auto id1 = mgr_->beginTransaction("stats-tenant");
    auto id2 = mgr_->beginTransaction("stats-tenant");
    mgr_->commitTransaction(id1);
    mgr_->rollbackTransaction(id2);

    auto stats = mgr_->getTenantTransactionStats("stats-tenant");
    EXPECT_EQ(stats.tenant_id,       "stats-tenant");
    EXPECT_EQ(stats.total_begun,     2u);
    EXPECT_EQ(stats.total_committed, 1u);
    EXPECT_EQ(stats.total_aborted,   1u);
    EXPECT_EQ(stats.active_count,    0u);
}

TEST_F(TenantTxnTest, PerTenantStats_UnknownTenantReturnsZeroes) {
    auto stats = mgr_->getTenantTransactionStats("nonexistent-tenant");
    EXPECT_EQ(stats.tenant_id,       "nonexistent-tenant");
    EXPECT_EQ(stats.total_begun,     0u);
    EXPECT_EQ(stats.total_committed, 0u);
    EXPECT_EQ(stats.total_aborted,   0u);
    EXPECT_EQ(stats.active_count,    0u);
}

TEST_F(TenantTxnTest, GetAllTenantStats_ReturnsAllTenants) {
    mgr_->beginTransaction("alpha");
    auto id2 = mgr_->beginTransaction("beta");
    mgr_->commitTransaction(id2);

    auto all = mgr_->getAllTenantTransactionStats();
    ASSERT_GE(all.size(), 2u);

    bool found_alpha = false, found_beta = false;
    for (const auto& s : all) {
        if (s.tenant_id == "alpha") { found_alpha = true; EXPECT_EQ(s.total_begun, 1u); }
        if (s.tenant_id == "beta")  { found_beta  = true; EXPECT_EQ(s.total_begun, 1u); }
    }
    EXPECT_TRUE(found_alpha);
    EXPECT_TRUE(found_beta);

    // Cleanup: rollback the still-open alpha transaction
    auto ids = mgr_->listTenantTransactionIds("alpha");
    for (auto tid : ids) mgr_->rollbackTransaction(tid);
}

// ── Active count and ID listing ───────────────────────────────────────────────

TEST_F(TenantTxnTest, ActiveCount_OnlyCountsTenantTransactions) {
    auto id_a1 = mgr_->beginTransaction("count-tenant");
    auto id_a2 = mgr_->beginTransaction("count-tenant");
    auto id_b  = mgr_->beginTransaction("other-tenant");

    EXPECT_EQ(mgr_->getActiveTenantTransactionCount("count-tenant"), 2u);
    EXPECT_EQ(mgr_->getActiveTenantTransactionCount("other-tenant"), 1u);

    mgr_->rollbackTransaction(id_a1);
    EXPECT_EQ(mgr_->getActiveTenantTransactionCount("count-tenant"), 1u);

    mgr_->rollbackTransaction(id_a2);
    mgr_->rollbackTransaction(id_b);
    EXPECT_EQ(mgr_->getActiveTenantTransactionCount("count-tenant"), 0u);
}

TEST_F(TenantTxnTest, ListTenantTransactionIds_ReturnsCorrectIds) {
    auto id1 = mgr_->beginTransaction("list-tenant");
    auto id2 = mgr_->beginTransaction("list-tenant");
    auto id3 = mgr_->beginTransaction("other-tenant");

    auto ids = mgr_->listTenantTransactionIds("list-tenant");
    ASSERT_EQ(ids.size(), 2u);

    // Both expected IDs must appear in the result.
    bool has_id1 = std::find(ids.begin(), ids.end(), id1) != ids.end();
    bool has_id2 = std::find(ids.begin(), ids.end(), id2) != ids.end();
    EXPECT_TRUE(has_id1);
    EXPECT_TRUE(has_id2);

    // The third transaction (different tenant) must not appear.
    bool has_id3 = std::find(ids.begin(), ids.end(), id3) != ids.end();
    EXPECT_FALSE(has_id3);

    mgr_->rollbackTransaction(id1);
    mgr_->rollbackTransaction(id2);
    mgr_->rollbackTransaction(id3);
}

// ── abortTenantTransactions ───────────────────────────────────────────────────

TEST_F(TenantTxnTest, AbortTenantTransactions_AbortsAllForTenant) {
    auto id1 = mgr_->beginTransaction("abort-tenant");
    auto id2 = mgr_->beginTransaction("abort-tenant");
    auto id3 = mgr_->beginTransaction("safe-tenant");
    static_cast<void>(id1);
    static_cast<void>(id2);

    size_t aborted = mgr_->abortTenantTransactions("abort-tenant");
    EXPECT_EQ(aborted, 2u);

    // The aborted transactions must no longer be active.
    EXPECT_EQ(mgr_->getActiveTenantTransactionCount("abort-tenant"), 0u);

    // The safe-tenant transaction must still be active.
    EXPECT_EQ(mgr_->getActiveTenantTransactionCount("safe-tenant"), 1u);

    mgr_->rollbackTransaction(id3);
}

TEST_F(TenantTxnTest, AbortTenantTransactions_ReturnsZeroWhenNoActiveTransactions) {
    // Ensure calling abort on a tenant with no active transactions is safe.
    size_t aborted = mgr_->abortTenantTransactions("phantom-tenant");
    EXPECT_EQ(aborted, 0u);
}

// ── IsolationLevel parameter ─────────────────────────────────────────────────

TEST_F(TenantTxnTest, TenantTransaction_RespectsIsolationLevel) {
    auto id = mgr_->beginTransaction("iso-tenant", IsolationLevel::SERIALIZABLE);
    auto txn = mgr_->getTransaction(id);
    ASSERT_NE(txn, nullptr);
    EXPECT_EQ(txn->getIsolationLevel(), IsolationLevel::SERIALIZABLE);
    EXPECT_EQ(txn->getTenantId(), "iso-tenant");
    mgr_->rollbackTransaction(id);
}
