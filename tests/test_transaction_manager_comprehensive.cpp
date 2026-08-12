#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "transaction/lock_manager.h"
#include <string>

using namespace themis;

// ---------------------------------------------------------------------------
// IsolationLevel enum tests
// ---------------------------------------------------------------------------

TEST(TransactionManagerComprehensive, IsolationLevelEnumValues) {
    EXPECT_EQ(static_cast<int>(IsolationLevel::READ_UNCOMMITTED), 0);
    EXPECT_EQ(static_cast<int>(IsolationLevel::READ_COMMITTED),   1);
    EXPECT_EQ(static_cast<int>(IsolationLevel::REPEATABLE_READ),  3);
    EXPECT_EQ(static_cast<int>(IsolationLevel::SERIALIZABLE),     4);
}

TEST(TransactionManagerComprehensive, LegacyIsolationLevelAliases) {
    // ReadCommitted == READ_COMMITTED
    EXPECT_EQ(static_cast<int>(IsolationLevel::ReadCommitted),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    // Snapshot == REPEATABLE_READ
    EXPECT_EQ(static_cast<int>(IsolationLevel::Snapshot),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
}

// ---------------------------------------------------------------------------
// LockManager integration (exposed via getLockManager())
// ---------------------------------------------------------------------------

TEST(TransactionManagerComprehensive, LockManagerAccessible) {
    // LockManager should be accessible without creating a full DB setup
    LockManager lm;

    auto r1 = lm.acquireLock(1, "row:1", LockType::SHARED);
    EXPECT_EQ(r1.status, LockStatus::GRANTED);

    auto r2 = lm.acquireLock(2, "row:1", LockType::SHARED);
    EXPECT_EQ(r2.status, LockStatus::GRANTED);

    // Exclusive should block while both shared holders exist
    auto r3 = lm.acquireLock(3, "row:1", LockType::EXCLUSIVE,
                              std::chrono::milliseconds(20));
    EXPECT_EQ(r3.status, LockStatus::TIMEOUT);

    lm.releaseAllLocks(1);
    lm.releaseAllLocks(2);

    // Now exclusive should succeed
    auto r4 = lm.acquireLock(3, "row:1", LockType::EXCLUSIVE);
    EXPECT_EQ(r4.status, LockStatus::GRANTED);
}

TEST(TransactionManagerComprehensive, LockManagerTwoPL) {
    LockManager lm;

    lm.acquireLock(10, "row:A", LockType::SHARED);
    EXPECT_FALSE(lm.isInShrinkingPhase(10));

    lm.beginShrinkingPhase(10);
    EXPECT_TRUE(lm.isInShrinkingPhase(10));

    // In shrinking phase no new locks allowed
    auto res = lm.acquireLock(10, "row:B", LockType::SHARED,
                               std::chrono::milliseconds(10));
    EXPECT_EQ(res.status, LockStatus::DENIED);
}

TEST(TransactionManagerComprehensive, LockManagerStatistics) {
    LockManager lm;
    lm.acquireLock(1, "k1", LockType::EXCLUSIVE);
    lm.acquireLock(2, "k2", LockType::SHARED);
    lm.acquireLock(3, "k2", LockType::EXCLUSIVE, std::chrono::milliseconds(10)); // timeout

    auto stats = lm.getStats();
    EXPECT_GE(stats.total_acquired,  2u);
    EXPECT_GE(stats.total_timeouts,  1u);
    EXPECT_EQ(stats.current_held,    2u);
}

// ---------------------------------------------------------------------------
// Predicate locking / SSI integration tests (via LockManager)
// ---------------------------------------------------------------------------

TEST(TransactionManagerComprehensive, SerializableIsolationLevelValue) {
    // Value 4 is part of the public ABI (persisted in RocksDB isolation-level fields).
    // This is intentionally an exact-value regression guard. See IsolationLevel docs.
    EXPECT_EQ(static_cast<int>(IsolationLevel::SERIALIZABLE), 4);
    EXPECT_GT(static_cast<int>(IsolationLevel::SERIALIZABLE),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
}

TEST(TransactionManagerComprehensive, PredicateLockAcquireAndCount) {
    LockManager lm;
    EXPECT_TRUE(lm.acquirePredicateLock(1, "entity:users:alice", "entity:users:zara"));
    EXPECT_EQ(lm.getPredicateLockCount(1), 1u);
    EXPECT_EQ(lm.getPredicateLockCount(2), 0u); // other txn unaffected
}

TEST(TransactionManagerComprehensive, PredicateLockNoConflictSameTransaction) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z");
    // Writing within own predicate range must NOT trigger a conflict
    EXPECT_EQ(lm.checkPredicateConflict(1, "entity:users:m"), 0u);
}

TEST(TransactionManagerComprehensive, PredicateLockConflictDetected) {
    LockManager lm;
    // T1 holds a predicate lock covering the range
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z");
    // T2 writing into that range must see a conflict with T1
    auto holder = lm.checkPredicateConflict(2, "entity:users:bob");
    EXPECT_EQ(holder, 1u);
}

TEST(TransactionManagerComprehensive, PredicateLockNoConflictOutsideRange) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:m");
    // Key outside the upper bound → no conflict
    EXPECT_EQ(lm.checkPredicateConflict(2, "entity:users:z"), 0u);
}

TEST(TransactionManagerComprehensive, PredicateLockReleasedOnCommit) {
    // Simulate acquire → release (as called by Transaction::commit)
    LockManager lm;
    lm.acquirePredicateLock(5, "entity:orders:100", "entity:orders:999");
    EXPECT_EQ(lm.getPredicateLockCount(5), 1u);

    lm.releasePredicateLocks(5);
    EXPECT_EQ(lm.getPredicateLockCount(5), 0u);
    // After release the key must not conflict any more
    EXPECT_EQ(lm.checkPredicateConflict(6, "entity:orders:500"), 0u);
}

TEST(TransactionManagerComprehensive, PredicateLockMultipleTransactions) {
    LockManager lm;
    // T1 and T2 hold non-overlapping predicate ranges
    lm.acquirePredicateLock(1, "entity:accounts:100", "entity:accounts:199");
    lm.acquirePredicateLock(2, "entity:accounts:200", "entity:accounts:299");

    // T3 writing into T1's range conflicts with T1, not T2
    EXPECT_EQ(lm.checkPredicateConflict(3, "entity:accounts:150"), 1u);
    // T3 writing into T2's range conflicts with T2, not T1
    EXPECT_EQ(lm.checkPredicateConflict(3, "entity:accounts:250"), 2u);
    // T3 writing outside both ranges has no conflict
    EXPECT_EQ(lm.checkPredicateConflict(3, "entity:accounts:300"), 0u);
}

TEST(TransactionManagerComprehensive, NonSerializableIsolationNoPredicateTracking) {
    // Transactions with non-SERIALIZABLE isolation levels must not acquire
    // predicate locks (trackPredicateRead is a no-op for those levels).
    // Simulate the LockManager state: a non-SERIALIZABLE txn only uses regular
    // locks, so getPredicateLockCount must remain 0.
    LockManager lm;

    // Simulate READ_COMMITTED txn (txn_id = 10): acquires regular read lock, no predicate lock
    lm.acquireLock(10, "entity:users:alice", LockType::SHARED);
    EXPECT_EQ(lm.getPredicateLockCount(10), 0u);

    // Simulate REPEATABLE_READ txn (txn_id = 11): same — no predicate lock
    lm.acquireLock(11, "entity:users:bob", LockType::SHARED);
    EXPECT_EQ(lm.getPredicateLockCount(11), 0u);

    // Only SERIALIZABLE txn (txn_id = 12) acquires a predicate lock
    lm.acquirePredicateLock(12, "entity:users:a", "entity:users:z");
    EXPECT_EQ(lm.getPredicateLockCount(12), 1u);
    // Confirm non-serializable txns still have 0 predicate locks
    EXPECT_EQ(lm.getPredicateLockCount(10), 0u);
    EXPECT_EQ(lm.getPredicateLockCount(11), 0u);
}

// ---------------------------------------------------------------------------
// TransactionExplain – unit tests (no DB required)
// ---------------------------------------------------------------------------

// Verify that getLocksHeld() – the data source for explain() – reports exactly
// the locks acquired by a transaction.
TEST(TransactionManagerComprehensive, ExplainLocksHeldViaLockManager) {
    LockManager lm;

    lm.acquireLock(42, "entity:orders:100", LockType::SHARED);
    lm.acquireLock(42, "entity:orders:200", LockType::EXCLUSIVE);

    auto held = lm.getLocksHeld(42);
    ASSERT_EQ(held.size(), 2u);

    bool found_shared    = false;
    bool found_exclusive = false;
    for (const auto& [key, lt] : held) {
        if (key == "entity:orders:100" && lt == LockType::SHARED)    found_shared    = true;
        if (key == "entity:orders:200" && lt == LockType::EXCLUSIVE) found_exclusive = true;
    }
    EXPECT_TRUE(found_shared);
    EXPECT_TRUE(found_exclusive);
}

// Verify that after releaseAllLocks the lock list is empty (as would be reported
// in an explain() call on a finished transaction).
TEST(TransactionManagerComprehensive, ExplainLocksEmptyAfterRelease) {
    LockManager lm;

    lm.acquireLock(7, "entity:products:p1", LockType::EXCLUSIVE);
    EXPECT_EQ(lm.getLocksHeld(7).size(), 1u);

    lm.releaseAllLocks(7);
    EXPECT_EQ(lm.getLocksHeld(7).size(), 0u);
}

// ExplainResult field defaults are sane.
TEST(TransactionManagerComprehensive, ExplainResultDefaultValues) {
    TransactionManager::Transaction::ExplainResult r;
    EXPECT_EQ(r.txn_id, 0u);
    EXPECT_TRUE(r.isolation_level.empty());
    EXPECT_EQ(r.duration_ms, 0u);
    EXPECT_FALSE(r.is_finished);
    EXPECT_TRUE(r.locks_held.empty());
    EXPECT_TRUE(r.write_set.empty());
}

// ExplainLockEntry and ExplainWriteEntry can be constructed and compared.
TEST(TransactionManagerComprehensive, ExplainEntryFields) {
    TransactionManager::Transaction::ExplainLockEntry lock{"entity:users:alice", "SHARED"};
    EXPECT_EQ(lock.key,       "entity:users:alice");
    EXPECT_EQ(lock.lock_type, "SHARED");

    TransactionManager::Transaction::ExplainWriteEntry write{"entity:users:bob", "put"};
    EXPECT_EQ(write.key,       "entity:users:bob");
    EXPECT_EQ(write.operation, "put");
}

