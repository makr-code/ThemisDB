/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_transaction_manager_comprehensive.cpp         ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     107                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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

