#include <gtest/gtest.h>
#include "transaction/lock_manager.h"
#include "transaction/isolation_level.h"
#include <string>

using namespace themis;

// ── PredicateLock API ─────────────────────────────────────────────────────────

TEST(PredicateLock, AcquireSucceeds) {
    LockManager lm;
    EXPECT_TRUE(lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z"));
    EXPECT_EQ(lm.getPredicateLockCount(1), 1u);
}

TEST(PredicateLock, MultipleRangesForSameTransaction) {
    LockManager lm;
    EXPECT_TRUE(lm.acquirePredicateLock(1, "entity:users:a", "entity:users:m"));
    EXPECT_TRUE(lm.acquirePredicateLock(1, "entity:orders:100", "entity:orders:200"));
    EXPECT_EQ(lm.getPredicateLockCount(1), 2u);
}

TEST(PredicateLock, ReleaseRemovesAllLocksForTransaction) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z");
    lm.acquirePredicateLock(1, "entity:orders:a", "entity:orders:z");
    lm.releasePredicateLocks(1);
    EXPECT_EQ(lm.getPredicateLockCount(1), 0u);
}

TEST(PredicateLock, ReleaseDoesNotAffectOtherTransactions) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z");
    lm.acquirePredicateLock(2, "entity:users:a", "entity:users:z");
    lm.releasePredicateLocks(1);
    EXPECT_EQ(lm.getPredicateLockCount(1), 0u);
    EXPECT_EQ(lm.getPredicateLockCount(2), 1u);
}

TEST(PredicateLock, ReleaseNonExistentIsNoop) {
    LockManager lm;
    // Should not throw or crash
    EXPECT_NO_THROW(lm.releasePredicateLocks(99));
    EXPECT_EQ(lm.getPredicateLockCount(99), 0u);
}

// ── checkPredicateConflict ────────────────────────────────────────────────────

TEST(PredicateLockConflict, KeyInsideRangeConflicts) {
    LockManager lm;
    // Txn 1 holds a predicate lock on the range
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z");
    // Txn 2 writing a key inside that range → conflict
    auto holder = lm.checkPredicateConflict(2, "entity:users:m");
    EXPECT_EQ(holder, 1u);
}

TEST(PredicateLockConflict, KeyOutsideRangeNoConflict) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:m");
    // Key beyond upper bound – no conflict
    auto holder = lm.checkPredicateConflict(2, "entity:users:z");
    EXPECT_EQ(holder, 0u);
}

TEST(PredicateLockConflict, KeyAtLowerBoundConflicts) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:alice", "entity:users:zara");
    auto holder = lm.checkPredicateConflict(2, "entity:users:alice");
    EXPECT_EQ(holder, 1u);
}

TEST(PredicateLockConflict, KeyAtUpperBoundConflicts) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:alice", "entity:users:zara");
    auto holder = lm.checkPredicateConflict(2, "entity:users:zara");
    EXPECT_EQ(holder, 1u);
}

TEST(PredicateLockConflict, SameTransactionNoConflict) {
    LockManager lm;
    // Txn 1 holds a predicate lock and writes inside its own range → no conflict
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z");
    auto holder = lm.checkPredicateConflict(1, "entity:users:m");
    EXPECT_EQ(holder, 0u);
}

TEST(PredicateLockConflict, NoLocksNoConflict) {
    LockManager lm;
    auto holder = lm.checkPredicateConflict(2, "entity:users:m");
    EXPECT_EQ(holder, 0u);
}

TEST(PredicateLockConflict, AfterReleaseNoConflict) {
    LockManager lm;
    lm.acquirePredicateLock(1, "entity:users:a", "entity:users:z");
    lm.releasePredicateLocks(1);
    auto holder = lm.checkPredicateConflict(2, "entity:users:m");
    EXPECT_EQ(holder, 0u);
}

TEST(PredicateLockConflict, SingleKeyPredicateConflicts) {
    LockManager lm;
    // Single-key predicate (start == end)
    lm.acquirePredicateLock(1, "entity:users:alice", "entity:users:alice");
    EXPECT_EQ(lm.checkPredicateConflict(2, "entity:users:alice"), 1u);
    EXPECT_EQ(lm.checkPredicateConflict(2, "entity:users:bob"),   0u);
}

TEST(PredicateLockConflict, MultipleHoldersFirstConflictReturned) {
    LockManager lm;
    lm.acquirePredicateLock(2, "entity:users:a", "entity:users:z");
    lm.acquirePredicateLock(3, "entity:users:a", "entity:users:z");
    // Txn 4 writes inside the range: should get one of the conflicting holders
    auto holder = lm.checkPredicateConflict(4, "entity:users:m");
    EXPECT_TRUE(holder == 2u || holder == 3u);
}

// ── SSI / IsolationLevel integration ─────────────────────────────────────────

TEST(SSI, SerializableIsolationLevelIsHighest) {
    // Sanity check: SERIALIZABLE > REPEATABLE_READ > READ_COMMITTED
    EXPECT_GT(static_cast<int>(IsolationLevel::SERIALIZABLE),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
}

TEST(SSI, PredicateLockCountStartsAtZero) {
    LockManager lm;
    EXPECT_EQ(lm.getPredicateLockCount(42), 0u);
}

TEST(SSI, ConcurrentPredicateLocksNonOverlappingRanges) {
    LockManager lm;
    // Two transactions with non-overlapping ranges: no conflict
    lm.acquirePredicateLock(1, "entity:orders:100", "entity:orders:199");
    lm.acquirePredicateLock(2, "entity:orders:200", "entity:orders:299");
    EXPECT_EQ(lm.checkPredicateConflict(2, "entity:orders:150"), 1u); // 2 writes into txn1's range
    EXPECT_EQ(lm.checkPredicateConflict(1, "entity:orders:250"), 2u); // 1 writes into txn2's range
    EXPECT_EQ(lm.checkPredicateConflict(3, "entity:orders:150"), 1u); // 3rd txn conflicts with txn1
    EXPECT_EQ(lm.checkPredicateConflict(3, "entity:orders:300"), 0u); // outside both ranges
}
