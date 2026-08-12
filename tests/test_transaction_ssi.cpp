/**
 * @file test_transaction_ssi.cpp
 * @brief Focused tests for Serializable Snapshot Isolation (SSI)
 *
 * Acceptance criteria covered:
 *   AC-1  Predicate lock tracking for range queries
 *   AC-2  Read-write conflict detection
 *   AC-3  Write-write conflict detection
 *   AC-4  Automatic serialization failure detection
 *   AC-5  Transaction retry with exponential backoff
 *   AC-6  SIREAD locks for reads that may cause conflicts
 *   AC-7  Commit-time validation of read/write sets
 *   AC-8  IsolationLevel::SerializableSnapshot alias
 *   AC-9  SSIConfig: setSSIConfig() / getSSIConfig()
 *   AC-10 SSIConfig: max_predicate_locks enforcement
 *   AC-11 SSIConfig: enable_predicate_locking = false disables SSI
 *   AC-12 detectConflicts() returns conflict list for SERIALIZABLE txn
 *   AC-13 detectConflicts() returns empty for non-SERIALIZABLE txn
 *   AC-14 Write-skew prevention: two SERIALIZABLE txns reading and writing
 *          the same range are detected as conflicting
 *   AC-15 Predicate locks released on commit
 *   AC-16 Predicate locks released on rollback
 */

#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "transaction/lock_manager.h"
#include "transaction/isolation_level.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/transaction_retry_manager.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <filesystem>
#include <string>
#include <atomic>
#include <stdexcept>
#include <thread>

using namespace themis;
using namespace themisdb::storage;

// ── Test Fixture ─────────────────────────────────────────────────────────────

class SSITest : public ::testing::Test {
protected:
    static constexpr const char* DB_PATH = "/tmp/themis_ssi_test_db";

    void SetUp() override {
        std::filesystem::remove_all(DB_PATH);
        RocksDBWrapper::Config cfg;
        cfg.db_path = DB_PATH;
        cfg.enable_statistics = false;
        db_       = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        sec_idx_  = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_= std::make_unique<GraphIndexManager>(*db_);
        vec_idx_  = std::make_unique<VectorIndexManager>(*db_);
        mgr_      = std::make_unique<TransactionManager>(
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

    static BaseEntity makeEntity(const std::string& pk,
                                 const std::string& value = "") {
        BaseEntity e;
        e.setPrimaryKey(pk);
        e.setField("value", value.empty() ? pk : value);
        return e;
    }

    std::unique_ptr<RocksDBWrapper>         db_;
    std::unique_ptr<SecondaryIndexManager>  sec_idx_;
    std::unique_ptr<GraphIndexManager>      graph_idx_;
    std::unique_ptr<VectorIndexManager>     vec_idx_;
    std::unique_ptr<TransactionManager>     mgr_;
};

// ── AC-8: IsolationLevel::SerializableSnapshot alias ─────────────────────────

TEST_F(SSITest, SerializableSnapshotAliasEqualsSerializable) {
    // SerializableSnapshot and SERIALIZABLE must have the same numeric value.
    EXPECT_EQ(static_cast<int>(IsolationLevel::SerializableSnapshot),
              static_cast<int>(IsolationLevel::SERIALIZABLE));
}

TEST_F(SSITest, BeginTransactionWithSerializableSnapshotAlias) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::SerializableSnapshot);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);
    EXPECT_EQ(txn->getIsolationLevel(), IsolationLevel::SERIALIZABLE);
    mgr_->rollbackTransaction(txn_id);
}

// ── AC-9: SSIConfig setSSIConfig / getSSIConfig ───────────────────────────────

TEST_F(SSITest, SSIConfigDefaults) {
    auto cfg = mgr_->getSSIConfig();
    EXPECT_TRUE(cfg.enable_predicate_locking);
    EXPECT_EQ(cfg.max_predicate_locks, 10000u);
    EXPECT_EQ(cfg.conflict_detection_interval.count(), 100);
}

TEST_F(SSITest, SetSSIConfigUpdatesValues) {
    TransactionManager::SSIConfig cfg;
    cfg.enable_predicate_locking      = true;
    cfg.max_predicate_locks           = 500;
    cfg.conflict_detection_interval   = std::chrono::milliseconds{200};
    mgr_->setSSIConfig(cfg);

    auto read_back = mgr_->getSSIConfig();
    EXPECT_TRUE(read_back.enable_predicate_locking);
    EXPECT_EQ(read_back.max_predicate_locks, 500u);
    EXPECT_EQ(read_back.conflict_detection_interval.count(), 200);
}

// ── AC-1 / AC-6: Predicate lock (SIREAD) tracking ────────────────────────────

TEST_F(SSITest, TrackPredicateRead_Serializable_Succeeds) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    auto st = txn->trackPredicateRead("key:a", "key:z");
    EXPECT_TRUE(st.ok) << st.message;

    EXPECT_EQ(mgr_->getLockManager().getPredicateLockCount(txn_id), 1u);
    mgr_->rollbackTransaction(txn_id);
}

TEST_F(SSITest, TrackPredicateRead_NonSerializable_IsNoOp) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::Snapshot);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    auto st = txn->trackPredicateRead("key:a", "key:z");
    EXPECT_TRUE(st.ok) << st.message;

    // No predicate lock should have been recorded for a non-SERIALIZABLE txn.
    EXPECT_EQ(mgr_->getLockManager().getPredicateLockCount(txn_id), 0u);
    mgr_->rollbackTransaction(txn_id);
}

TEST_F(SSITest, TrackPredicateRead_MultipleRanges) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    EXPECT_TRUE(txn->trackPredicateRead("a", "e").ok);
    EXPECT_TRUE(txn->trackPredicateRead("f", "m").ok);
    EXPECT_TRUE(txn->trackPredicateRead("n", "z").ok);

    EXPECT_EQ(mgr_->getLockManager().getPredicateLockCount(txn_id), 3u);
    mgr_->rollbackTransaction(txn_id);
}

TEST_F(SSITest, TrackPredicateRead_FinishedTransaction_ReturnsError) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    mgr_->rollbackTransaction(txn_id);

    // Transaction is now finished – trackPredicateRead must fail.
    auto st = txn->trackPredicateRead("key:a", "key:z");
    EXPECT_FALSE(st.ok);
}

// ── AC-2: Read-write conflict detection ──────────────────────────────────────

TEST_F(SSITest, ReadWriteConflict_DetectedOnWrite) {
    // T1 acquires a SIREAD predicate lock on [key:a, key:z].
    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    EXPECT_TRUE(t1->trackPredicateRead("entity:accounts:key:a",
                                       "entity:accounts:key:z").ok);

    // T2 (SERIALIZABLE) attempts to write key:b, which falls in T1's range.
    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);

    auto st = t2->putEntity("accounts", makeEntity("key:b", "value_b"));
    // The write should fail with a serialization error because key:b is inside
    // T1's predicate range [entity:accounts:key:a, entity:accounts:key:z].
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("serialization failure"), std::string::npos)
        << "Expected serialization failure, got: " << st.message;

    mgr_->rollbackTransaction(t2_id);
    mgr_->rollbackTransaction(t1_id);
}

TEST_F(SSITest, ReadWriteConflict_NotDetected_WhenKeyOutsideRange) {
    // T1 holds a predicate lock on [entity:accounts:key:a, entity:accounts:key:e].
    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    EXPECT_TRUE(t1->trackPredicateRead("entity:accounts:key:a",
                                       "entity:accounts:key:e").ok);

    // T2 writes key:z which is outside T1's predicate range – no conflict.
    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);

    auto st = t2->putEntity("accounts", makeEntity("key:z", "value_z"));
    EXPECT_TRUE(st.ok) << "Unexpected conflict for key outside predicate range: "
                       << st.message;

    mgr_->rollbackTransaction(t2_id);
    mgr_->rollbackTransaction(t1_id);
}

// ── AC-3: Write-write conflict detection ─────────────────────────────────────

TEST_F(SSITest, WriteWriteConflict_DetectedAtCommitTime) {
    // Both transactions write the same key – one must fail at commit.
    const std::string pk = "shared_key";

    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);

    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);

    // Both write the same entity. Depending on implementation details,
    // conflict may be detected eagerly on putEntity() or later at commit.
    auto put1 = t1->putEntity("table", makeEntity(pk, "value_from_t1"));
    auto put2 = t2->putEntity("table", makeEntity(pk, "value_from_t2"));

    if (!put1.ok || !put2.ok) {
        const bool has_conflict_msg =
            (!put1.ok && (put1.message.find("serialization failure") != std::string::npos ||
                          put1.message.find("conflict") != std::string::npos)) ||
            (!put2.ok && (put2.message.find("serialization failure") != std::string::npos ||
                          put2.message.find("conflict") != std::string::npos));
        EXPECT_TRUE(has_conflict_msg);
        mgr_->rollbackTransaction(t1_id);
        mgr_->rollbackTransaction(t2_id);
        return;
    }

    // If both writes were accepted, conflict must be enforced at commit.
    auto st1 = mgr_->commitTransaction(t1_id);
    auto st2 = mgr_->commitTransaction(t2_id);
    EXPECT_TRUE(st1.ok || st2.ok) << "At least one transaction should commit";
    EXPECT_TRUE(!st1.ok || !st2.ok) << "At least one transaction should fail";
    if (!st1.ok) {
        EXPECT_TRUE(st1.message.find("serialization failure") != std::string::npos ||
                    st1.message.find("conflict") != std::string::npos)
            << "Expected conflict message, got: " << st1.message;
    }
    if (!st2.ok) {
        EXPECT_TRUE(st2.message.find("serialization failure") != std::string::npos ||
                    st2.message.find("conflict") != std::string::npos)
            << "Expected conflict message, got: " << st2.message;
    }
}

// ── AC-4 / AC-7: Automatic serialization failure + commit-time validation ────

TEST_F(SSITest, SerializationFailureIsReturnedAsErrorStatus) {
    // T1 reads predicate range; T2 writes inside that range and gets aborted.
    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    EXPECT_TRUE(t1->trackPredicateRead("entity:items:a", "entity:items:z").ok);

    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);

    auto st = t2->putEntity("items", makeEntity("b", "b_val"));
    ASSERT_FALSE(st.ok);
    // The message must indicate a serialization failure.
    EXPECT_NE(st.message.find("serialization failure"), std::string::npos);

    mgr_->rollbackTransaction(t2_id);
    mgr_->rollbackTransaction(t1_id);
}

// ── AC-5: Transaction retry with exponential backoff ─────────────────────────

TEST_F(SSITest, SSI_ErrorIsClassifiedAsRetryable) {
    // "serialization failure: write conflicts..." must be classified as
    // WRITE_CONFLICT so that TransactionRetryManager will retry it.
    const std::string ssi_error =
        "serialization failure: write conflicts with predicate lock held by txn 42, "
        "transaction must be retried";
    EXPECT_EQ(TransactionRetryManager::classifyError(ssi_error),
              ErrorType::WRITE_CONFLICT);
}

TEST_F(SSITest, RetryManager_RetriesOnSerializationError) {
    TransactionRetryConfig cfg;
    cfg.max_attempts         = 5;
    cfg.base_delay_ms        = 0;
    cfg.max_delay_ms         = 0;
    cfg.enable_jitter        = false;
    cfg.max_total_timeout_ms = 60000;
    cfg.enable_circuit_breaker = false;
    cfg.backoff_strategy     = BackoffStrategy::EXPONENTIAL;

    TransactionRetryManager retry_mgr(cfg);

    std::atomic<int> attempt_count{0};
    int result = retry_mgr.executeWithRetry([&]() -> int {
        int n = ++attempt_count;
        if (n < 3) {
            throw std::runtime_error(
                "serialization failure: write conflicts with predicate lock "
                "held by txn 1, transaction must be retried");
        }
        return n;
    }, "ssi_op");

    EXPECT_EQ(result, 3);
    EXPECT_EQ(attempt_count.load(), 3);
    auto stats = retry_mgr.getStatistics();
    EXPECT_EQ(stats.successful_operations.load(), 1u);
    EXPECT_GT(stats.total_retry_attempts.load(), 0u);
}

TEST_F(SSITest, RetryManager_ExponentialBackoff_StatsTracked) {
    TransactionRetryConfig cfg;
    cfg.max_attempts         = 3;
    cfg.base_delay_ms        = 0;
    cfg.max_delay_ms         = 0;
    cfg.enable_jitter        = false;
    cfg.max_total_timeout_ms = 60000;
    cfg.enable_circuit_breaker = false;
    cfg.backoff_strategy     = BackoffStrategy::EXPONENTIAL;
    cfg.backoff_multiplier   = 2.0;

    TransactionRetryManager retry_mgr(cfg);

    std::atomic<int> calls{0};
    EXPECT_THROW({
        retry_mgr.executeWithRetry([&]() -> int {
            ++calls;
            throw std::runtime_error(
                "serialization failure: write conflicts with predicate lock "
                "held by txn 1, transaction must be retried");
        }, "always_fail");
    }, std::runtime_error);

    EXPECT_EQ(calls.load(), 3);
    auto stats = retry_mgr.getStatistics();
    EXPECT_EQ(stats.failed_operations.load(), 1u);
    EXPECT_EQ(stats.total_retry_attempts.load(), 2u); // 3 calls - 1 initial = 2 retries
}

// ── AC-10: max_predicate_locks enforcement ───────────────────────────────────

TEST_F(SSITest, MaxPredicateLocks_Enforced) {
    TransactionManager::SSIConfig cfg;
    cfg.enable_predicate_locking = true;
    cfg.max_predicate_locks      = 2;
    mgr_->setSSIConfig(cfg);

    auto txn_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    EXPECT_TRUE(txn->trackPredicateRead("a", "b").ok);
    EXPECT_TRUE(txn->trackPredicateRead("c", "d").ok);
    // Third acquire exceeds the global limit of 2 – silently dropped.
    EXPECT_TRUE(txn->trackPredicateRead("e", "f").ok);

    // Only 2 locks should be recorded (limit was reached before the 3rd).
    EXPECT_LE(mgr_->getLockManager().getPredicateLockCount(txn_id), 2u);

    mgr_->rollbackTransaction(txn_id);
}

// ── AC-11: enable_predicate_locking = false disables SSI ────────────────────

TEST_F(SSITest, DisablePredicateLocking_NoConflictDetected) {
    TransactionManager::SSIConfig cfg;
    cfg.enable_predicate_locking = false;
    mgr_->setSSIConfig(cfg);

    // T1 acquires a predicate lock (which should be ignored).
    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    // trackPredicateRead is a no-op when predicate locking is disabled.
    EXPECT_TRUE(t1->trackPredicateRead("entity:items:a", "entity:items:z").ok);

    // T2 writes inside T1's range – should NOT conflict because locking is off.
    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);

    auto st = t2->putEntity("items", makeEntity("b", "no_conflict"));
    EXPECT_TRUE(st.ok) << "Unexpected conflict when predicate locking is disabled: "
                       << st.message;

    mgr_->rollbackTransaction(t2_id);
    mgr_->rollbackTransaction(t1_id);

    // Re-enable for subsequent tests.
    TransactionManager::SSIConfig re_enable;
    re_enable.enable_predicate_locking = true;
    mgr_->setSSIConfig(re_enable);
}

// ── AC-12: detectConflicts returns conflicts for SERIALIZABLE txn ─────────────

TEST_F(SSITest, DetectConflicts_ReturnsEmptyWhenNoConflict) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);
    EXPECT_TRUE(txn->trackPredicateRead("myrange_start", "myrange_end").ok);

    auto conflicts = mgr_->detectConflicts(txn_id);
    EXPECT_TRUE(conflicts.empty());

    mgr_->rollbackTransaction(txn_id);
}

// ── AC-13: detectConflicts returns empty for non-SERIALIZABLE txn ─────────────

TEST_F(SSITest, DetectConflicts_EmptyForNonSerializable) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::Snapshot);
    auto conflicts = mgr_->detectConflicts(txn_id);
    EXPECT_TRUE(conflicts.empty());
    mgr_->rollbackTransaction(txn_id);
}

TEST_F(SSITest, DetectConflicts_EmptyForUnknownTxn) {
    auto conflicts = mgr_->detectConflicts(99999u);
    EXPECT_TRUE(conflicts.empty());
}

// ── AC-15: Predicate locks released on commit ────────────────────────────────

TEST_F(SSITest, PredicateLocksReleasedOnCommit) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    EXPECT_TRUE(txn->trackPredicateRead("aa", "zz").ok);
    EXPECT_EQ(mgr_->getLockManager().getPredicateLockCount(txn_id), 1u);

    mgr_->commitTransaction(txn_id);

    // After commit the predicate locks must have been released.
    EXPECT_EQ(mgr_->getLockManager().getPredicateLockCount(txn_id), 0u);
}

// ── AC-16: Predicate locks released on rollback ──────────────────────────────

TEST_F(SSITest, PredicateLocksReleasedOnRollback) {
    auto txn_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn    = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    EXPECT_TRUE(txn->trackPredicateRead("a", "m").ok);
    EXPECT_EQ(mgr_->getLockManager().getPredicateLockCount(txn_id), 1u);

    mgr_->rollbackTransaction(txn_id);

    EXPECT_EQ(mgr_->getLockManager().getPredicateLockCount(txn_id), 0u);
}

// ── Write-skew prevention (classic anomaly check) ────────────────────────────

TEST_F(SSITest, WriteSkew_SerializableIsolation_DetectsConflict) {
    // Classic write-skew scenario:
    //   T1: reads account A and B, sees both 100, writes A = 0
    //   T2: reads account A and B, sees both 100, writes B = 0
    //   Invariant: A + B >= 100 (violated if both commit)
    //
    // With SSI / predicate locking, T1 acquires SIREAD locks on A and B.
    // T2 then tries to write B, which is inside T1's predicate range – conflict.
    const std::string a_key = "entity:accounts:acct_A";
    const std::string b_key = "entity:accounts:acct_B";

    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    // T1 reads the entire accounts range.
    EXPECT_TRUE(t1->trackPredicateRead(a_key, b_key).ok);

    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);
    // T2 also reads the entire accounts range.
    EXPECT_TRUE(t2->trackPredicateRead(a_key, b_key).ok);

    // Depending on timing/order, either writer may become the conflicting one.
    auto st_t1 = t1->putEntity("accounts", makeEntity("acct_A", "0"));
    auto st_t2 = t2->putEntity("accounts", makeEntity("acct_B", "0"));

    const bool eager_conflict = !st_t1.ok || !st_t2.ok;
    if (eager_conflict) {
        const bool has_serialization_msg =
            (!st_t1.ok && st_t1.message.find("serialization failure") != std::string::npos) ||
            (!st_t2.ok && st_t2.message.find("serialization failure") != std::string::npos);
        EXPECT_TRUE(has_serialization_msg)
            << "Expected write-skew detection, got t1='" << st_t1.message
            << "' t2='" << st_t2.message << "'";
        mgr_->rollbackTransaction(t1_id);
        mgr_->rollbackTransaction(t2_id);
        return;
    }

    // If both puts were accepted, one commit must fail due to serialization conflict.
    auto c1 = mgr_->commitTransaction(t1_id);
    auto c2 = mgr_->commitTransaction(t2_id);
    EXPECT_TRUE(c1.ok || c2.ok);
    EXPECT_TRUE(!c1.ok || !c2.ok);
    if (!c1.ok) {
        EXPECT_NE(c1.message.find("serialization failure"), std::string::npos)
            << "Expected write-skew detection, got: " << c1.message;
    }
    if (!c2.ok) {
        EXPECT_NE(c2.message.find("serialization failure"), std::string::npos)
            << "Expected write-skew detection, got: " << c2.message;
    }
}

// ── LockManager predicate-lock unit tests ────────────────────────────────────

TEST(LockManagerPredicateTest, AcquireAndCheck) {
    LockManager lm;
    EXPECT_TRUE(lm.acquirePredicateLock(1, "a", "z"));
    EXPECT_EQ(lm.checkPredicateConflict(2, "m"), 1u);  // conflict
    EXPECT_EQ(lm.checkPredicateConflict(1, "m"), 0u);  // self – no conflict
    EXPECT_EQ(lm.checkPredicateConflict(2, "0"), 0u);  // outside range
}

TEST(LockManagerPredicateTest, ReleaseClears) {
    LockManager lm;
    lm.acquirePredicateLock(1, "a", "z");
    lm.releasePredicateLocks(1);
    EXPECT_EQ(lm.checkPredicateConflict(2, "m"), 0u);
    EXPECT_EQ(lm.getPredicateLockCount(1), 0u);
}

TEST(LockManagerPredicateTest, MaxPredicateLocksEnforced) {
    LockManager lm;
    lm.setMaxPredicateLocks(2);
    EXPECT_TRUE(lm.acquirePredicateLock(1, "a", "b"));
    EXPECT_TRUE(lm.acquirePredicateLock(1, "c", "d"));
    // Third call must be rejected (returns false) due to global limit.
    EXPECT_FALSE(lm.acquirePredicateLock(1, "e", "f"));
    EXPECT_EQ(lm.getPredicateLockCount(1), 2u);
}

TEST(LockManagerPredicateTest, MaxPredicateLocksZeroMeansUnlimited) {
    LockManager lm;
    lm.setMaxPredicateLocks(0); // 0 = unlimited
    for (int i = 0; i < 100; ++i) {
        std::string k = std::to_string(i);
        EXPECT_TRUE(lm.acquirePredicateLock(1, k, k));
    }
    EXPECT_EQ(lm.getPredicateLockCount(1), 100u);
}

TEST(LockManagerPredicateTest, SetPredicateLockingDisabled) {
    LockManager lm;
    lm.setPredicateLockingEnabled(false);
    EXPECT_FALSE(lm.acquirePredicateLock(1, "a", "z"));
    EXPECT_EQ(lm.getPredicateLockCount(1), 0u);
    EXPECT_EQ(lm.checkPredicateConflict(2, "m"), 0u);
}

TEST(LockManagerPredicateTest, SetPredicateLockingEnabled_Default) {
    LockManager lm;
    EXPECT_TRUE(lm.isPredicateLockingEnabled());
    lm.setPredicateLockingEnabled(false);
    EXPECT_FALSE(lm.isPredicateLockingEnabled());
    lm.setPredicateLockingEnabled(true);
    EXPECT_TRUE(lm.isPredicateLockingEnabled());
}

TEST(LockManagerPredicateTest, GetMaxPredicateLocks) {
    LockManager lm;
    EXPECT_EQ(lm.getMaxPredicateLocks(), 0u); // default = unlimited
    lm.setMaxPredicateLocks(500);
    EXPECT_EQ(lm.getMaxPredicateLocks(), 500u);
}

// ── SerializationConflict struct completeness ─────────────────────────────────

TEST_F(SSITest, SerializationConflict_FieldsArePopulated) {
    // Verify that SerializationConflict is a proper struct with expected fields.
    TransactionManager::SerializationConflict sc;
    sc.other_txn_id  = 42;
    sc.key           = "entity:t:pk";
    sc.conflict_type = "read-write";
    sc.message       = "conflict description";

    EXPECT_EQ(sc.other_txn_id, 42u);
    EXPECT_EQ(sc.key, "entity:t:pk");
    EXPECT_EQ(sc.conflict_type, "read-write");
    EXPECT_FALSE(sc.message.empty());
}

// ── AC-12 (extended): detectConflicts returns conflicts when ranges overlap ───

TEST_F(SSITest, DetectConflicts_ReturnsConflict_WhenRangesOverlap) {
    // T1 holds predicate range ["a", "z"].
    // T2 holds predicate range ["m", "q"] which overlaps T1's range.
    // detectConflicts(T1) should detect the overlap with T2.

    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    EXPECT_TRUE(t1->trackPredicateRead("a", "z").ok);

    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);
    EXPECT_TRUE(t2->trackPredicateRead("m", "q").ok);

    auto conflicts = mgr_->detectConflicts(t1_id);
    ASSERT_FALSE(conflicts.empty()) << "Expected at least one conflict";

    // The conflict should reference T2.
    bool found_t2 = false;
    for (const auto& c : conflicts) {
        if (c.other_txn_id == t2_id) {
            found_t2 = true;
            EXPECT_EQ(c.conflict_type, "read-write");
            EXPECT_FALSE(c.message.empty());
        }
    }
    EXPECT_TRUE(found_t2) << "Expected conflict referencing T2";

    mgr_->rollbackTransaction(t2_id);
    mgr_->rollbackTransaction(t1_id);
}

TEST_F(SSITest, DetectConflicts_EmptyWhenRangesDoNotOverlap) {
    // T1 holds ["a", "e"]; T2 holds ["f", "z"] – no overlap.
    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    EXPECT_TRUE(t1->trackPredicateRead("a", "e").ok);

    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);
    EXPECT_TRUE(t2->trackPredicateRead("f", "z").ok);

    auto conflicts = mgr_->detectConflicts(t1_id);
    EXPECT_TRUE(conflicts.empty())
        << "Expected no conflict for non-overlapping ranges";

    mgr_->rollbackTransaction(t2_id);
    mgr_->rollbackTransaction(t1_id);
}

TEST_F(SSITest, DetectConflicts_SymmetricRanges) {
    // T1 and T2 both hold the same range ["key:0", "key:9"].
    // detectConflicts should detect a conflict from both sides.
    auto t1_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t1    = mgr_->getTransaction(t1_id);
    ASSERT_NE(t1, nullptr);
    EXPECT_TRUE(t1->trackPredicateRead("key:0", "key:9").ok);

    auto t2_id = mgr_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto t2    = mgr_->getTransaction(t2_id);
    ASSERT_NE(t2, nullptr);
    EXPECT_TRUE(t2->trackPredicateRead("key:0", "key:9").ok);

    // Both should see a conflict with the other.
    auto c1 = mgr_->detectConflicts(t1_id);
    auto c2 = mgr_->detectConflicts(t2_id);
    EXPECT_FALSE(c1.empty()) << "T1 should detect conflict with T2";
    EXPECT_FALSE(c2.empty()) << "T2 should detect conflict with T1";

    mgr_->rollbackTransaction(t2_id);
    mgr_->rollbackTransaction(t1_id);
}

// ── LockManager: getPredicateLockRanges ──────────────────────────────────────

TEST(LockManagerPredicateTest, GetPredicateLockRanges_Empty) {
    LockManager lm;
    auto ranges = lm.getPredicateLockRanges(1);
    EXPECT_TRUE(ranges.empty());
}

TEST(LockManagerPredicateTest, GetPredicateLockRanges_SingleLock) {
    LockManager lm;
    lm.acquirePredicateLock(1, "start", "end");
    auto ranges = lm.getPredicateLockRanges(1);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].first,  "start");
    EXPECT_EQ(ranges[0].second, "end");
}

TEST(LockManagerPredicateTest, GetPredicateLockRanges_MultipleOwners) {
    LockManager lm;
    lm.acquirePredicateLock(1, "a", "e");
    lm.acquirePredicateLock(2, "f", "z");
    lm.acquirePredicateLock(1, "g", "h");

    auto ranges1 = lm.getPredicateLockRanges(1);
    auto ranges2 = lm.getPredicateLockRanges(2);

    EXPECT_EQ(ranges1.size(), 2u); // "a"-"e" and "g"-"h"
    EXPECT_EQ(ranges2.size(), 1u); // "f"-"z"
}

TEST(LockManagerPredicateTest, GetPredicateLockRanges_AfterRelease) {
    LockManager lm;
    lm.acquirePredicateLock(1, "x", "z");
    lm.releasePredicateLocks(1);
    auto ranges = lm.getPredicateLockRanges(1);
    EXPECT_TRUE(ranges.empty());
}
