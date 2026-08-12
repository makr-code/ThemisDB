#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "transaction/lock_manager.h"
#include <string>

using namespace themis;

// ── Enum correctness ──────────────────────────────────────────────────────────

TEST(IsolationLevelEnum, AllFourLevelsDefined) {
    EXPECT_NE(static_cast<int>(IsolationLevel::READ_UNCOMMITTED),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    EXPECT_NE(static_cast<int>(IsolationLevel::READ_COMMITTED),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
    EXPECT_NE(static_cast<int>(IsolationLevel::REPEATABLE_READ),
              static_cast<int>(IsolationLevel::SERIALIZABLE));
}

TEST(IsolationLevelEnum, LegacyAliasesMatch) {
    EXPECT_EQ(static_cast<int>(IsolationLevel::ReadCommitted),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    EXPECT_EQ(static_cast<int>(IsolationLevel::Snapshot),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
}

TEST(IsolationLevelEnum, StrictnessOrder) {
    EXPECT_LT(static_cast<int>(IsolationLevel::READ_UNCOMMITTED),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    EXPECT_LT(static_cast<int>(IsolationLevel::READ_COMMITTED),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
    EXPECT_LT(static_cast<int>(IsolationLevel::REPEATABLE_READ),
              static_cast<int>(IsolationLevel::SERIALIZABLE));
}

TEST(IsolationLevelEnum, ExactNumericValues) {
    // These values are part of the public ABI: changing them would break
    // persisted isolation-level fields in RocksDB, so they are intentionally
    // tested as a regression guard.
    EXPECT_EQ(static_cast<int>(IsolationLevel::READ_UNCOMMITTED), 0);
    EXPECT_EQ(static_cast<int>(IsolationLevel::READ_COMMITTED),   1);
    EXPECT_EQ(static_cast<int>(IsolationLevel::REPEATABLE_READ),  3);
    EXPECT_EQ(static_cast<int>(IsolationLevel::SERIALIZABLE),     4);
}

TEST(IsolationLevelEnum, ValueTwoIsUnused) {
    // The gap at value 2 is intentional: the legacy Snapshot alias must equal 3
    // (REPEATABLE_READ) for binary compatibility, and value 2 is reserved for
    // a future intermediate level.  See IsolationLevel documentation.
    EXPECT_NE(static_cast<int>(IsolationLevel::READ_COMMITTED),  2);
    EXPECT_NE(static_cast<int>(IsolationLevel::REPEATABLE_READ), 2);
}

TEST(IsolationLevelEnum, ReadUncommittedIsLowest) {
    EXPECT_EQ(static_cast<int>(IsolationLevel::READ_UNCOMMITTED), 0);
}

TEST(IsolationLevelEnum, SerializableIsHighest) {
    EXPECT_GT(static_cast<int>(IsolationLevel::SERIALIZABLE),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
    EXPECT_GT(static_cast<int>(IsolationLevel::SERIALIZABLE),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    EXPECT_GT(static_cast<int>(IsolationLevel::SERIALIZABLE),
              static_cast<int>(IsolationLevel::READ_UNCOMMITTED));
}

// ── IsolationLevel and LockManager interactions ───────────────────────────────

TEST(IsolationLevelComplianceRC, ReadCommittedAllowsNonRepeatableReads) {
    // At READ_COMMITTED multiple readers of the same key can run concurrently.
    LockManager lm;
    auto r1 = lm.acquireLock(1, "row:A", LockType::SHARED);
    auto r2 = lm.acquireLock(2, "row:A", LockType::SHARED);
    EXPECT_EQ(r1.status, LockStatus::GRANTED);
    EXPECT_EQ(r2.status, LockStatus::GRANTED);
}

TEST(IsolationLevelComplianceRC, ReadCommittedWriterBlocksReader) {
    LockManager lm;
    lm.acquireLock(1, "row:B", LockType::EXCLUSIVE);
    auto r = lm.acquireLock(2, "row:B", LockType::SHARED,
                             std::chrono::milliseconds(20));
    EXPECT_EQ(r.status, LockStatus::TIMEOUT);
}

TEST(IsolationLevelComplianceRC, ReadCommittedAfterReleaseReaderSucceeds) {
    LockManager lm;
    lm.acquireLock(1, "row:C", LockType::EXCLUSIVE);
    lm.releaseLock(1, "row:C");
    auto r = lm.acquireLock(2, "row:C", LockType::SHARED);
    EXPECT_EQ(r.status, LockStatus::GRANTED);
}

TEST(IsolationLevelComplianceRR, RepeatableReadHoldsSharedLock) {
    // REPEATABLE_READ: once a row is read the shared lock is kept until
    // end-of-transaction, so a writer must wait.
    LockManager lm;
    lm.acquireLock(1, "row:D", LockType::SHARED);
    // Writer is blocked
    auto w = lm.acquireLock(2, "row:D", LockType::EXCLUSIVE,
                             std::chrono::milliseconds(20));
    EXPECT_EQ(w.status, LockStatus::TIMEOUT);
}

TEST(IsolationLevelComplianceSZ, SerializableExclusivePreventsAllAccess) {
    LockManager lm;
    lm.acquireLock(1, "row:E", LockType::EXCLUSIVE);
    EXPECT_EQ(lm.acquireLock(2, "row:E", LockType::SHARED,
                              std::chrono::milliseconds(20)).status,
              LockStatus::TIMEOUT);
    EXPECT_EQ(lm.acquireLock(2, "row:E", LockType::EXCLUSIVE,
                              std::chrono::milliseconds(20)).status,
              LockStatus::TIMEOUT);
}

TEST(IsolationLevelComplianceSZ, SerializableAfterCommitOtherCanRead) {
    LockManager lm;
    lm.acquireLock(1, "row:F", LockType::EXCLUSIVE);
    // Simulate commit: release all locks
    lm.releaseAllLocks(1);
    auto r = lm.acquireLock(2, "row:F", LockType::SHARED);
    EXPECT_EQ(r.status, LockStatus::GRANTED);
}

// ── Two-Phase Locking isolation ───────────────────────────────────────────────

TEST(IsolationLevel2PL, GrowingPhaseAllowsAcquisition) {
    LockManager lm;
    EXPECT_EQ(lm.acquireLock(1, "a", LockType::SHARED).status, LockStatus::GRANTED);
    EXPECT_EQ(lm.acquireLock(1, "b", LockType::SHARED).status, LockStatus::GRANTED);
    EXPECT_FALSE(lm.isInShrinkingPhase(1));
}

TEST(IsolationLevel2PL, ShrinkingPhaseBlocksAcquisition) {
    LockManager lm;
    lm.acquireLock(1, "a", LockType::SHARED);
    lm.beginShrinkingPhase(1);
    auto r = lm.acquireLock(1, "b", LockType::SHARED,
                             std::chrono::milliseconds(10));
    EXPECT_EQ(r.status, LockStatus::DENIED);
}

TEST(IsolationLevel2PL, ShrinkingPhaseAllowsRelease) {
    LockManager lm;
    lm.acquireLock(1, "a", LockType::EXCLUSIVE);
    lm.beginShrinkingPhase(1);
    EXPECT_TRUE(lm.releaseLock(1, "a"));
}

// ── Intent lock isolation ─────────────────────────────────────────────────────

TEST(IsolationLevelIntent, IntentSharedCompatibleWithShared) {
    LockManager lm;
    EXPECT_EQ(lm.acquireLock(1, "tbl", LockType::INTENT_SHARED).status,
              LockStatus::GRANTED);
    EXPECT_EQ(lm.acquireLock(2, "tbl", LockType::SHARED).status,
              LockStatus::GRANTED);
}

TEST(IsolationLevelIntent, IntentExclusiveIncompatibleWithShared) {
    LockManager lm;
    lm.acquireLock(1, "tbl", LockType::SHARED);
    auto r = lm.acquireLock(2, "tbl", LockType::INTENT_EXCLUSIVE,
                             std::chrono::milliseconds(20));
    EXPECT_EQ(r.status, LockStatus::TIMEOUT);
}

TEST(IsolationLevelIntent, IntentExclusiveCompatibleWithIntentExclusive) {
    LockManager lm;
    EXPECT_EQ(lm.acquireLock(1, "tbl", LockType::INTENT_EXCLUSIVE).status,
              LockStatus::GRANTED);
    EXPECT_EQ(lm.acquireLock(2, "tbl", LockType::INTENT_EXCLUSIVE).status,
              LockStatus::GRANTED);
}

TEST(IsolationLevelIntent, ExclusiveBlocksIntentShared) {
    LockManager lm;
    lm.acquireLock(1, "tbl", LockType::EXCLUSIVE);
    auto r = lm.acquireLock(2, "tbl", LockType::INTENT_SHARED,
                             std::chrono::milliseconds(20));
    EXPECT_EQ(r.status, LockStatus::TIMEOUT);
}

