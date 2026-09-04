// Tests for LockManager – Phase 1: Lock Management & Isolation
// Validates Read/Write/Intent locks, 2PL enforcement, lock escalation,
// timeout behaviour, and upgrade path.

#include <gtest/gtest.h>
#include "transaction/lock_manager.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

using namespace themis;

class LockManagerTest : public ::testing::Test {
protected:
    LockManager lm;
};

// ---------------------------------------------------------------------------
// Basic shared-lock tests
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, AcquireSharedLock) {
    auto result = lm.acquireLock(1, "key:A", LockType::SHARED);
    EXPECT_EQ(result.status, LockStatus::GRANTED);
    EXPECT_TRUE(lm.holdsLock(1, "key:A", LockType::SHARED));
}

TEST_F(LockManagerTest, MultipleReadersAllowed) {
    EXPECT_EQ(lm.acquireLock(1, "row:1", LockType::SHARED).status, LockStatus::GRANTED);
    EXPECT_EQ(lm.acquireLock(2, "row:1", LockType::SHARED).status, LockStatus::GRANTED);
    EXPECT_EQ(lm.acquireLock(3, "row:1", LockType::SHARED).status, LockStatus::GRANTED);
}

// ---------------------------------------------------------------------------
// Exclusive lock tests
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, AcquireExclusiveLock) {
    auto result = lm.acquireLock(1, "key:B", LockType::EXCLUSIVE);
    EXPECT_EQ(result.status, LockStatus::GRANTED);
    EXPECT_TRUE(lm.holdsLock(1, "key:B", LockType::EXCLUSIVE));
}

TEST_F(LockManagerTest, ExclusiveBlocksOtherWriters) {
    // T1 holds exclusive
    EXPECT_EQ(lm.acquireLock(1, "row:X", LockType::EXCLUSIVE).status, LockStatus::GRANTED);

    // T2 requesting exclusive should time out quickly
    auto result = lm.acquireLock(2, "row:X", LockType::EXCLUSIVE,
                                  std::chrono::milliseconds(50));
    EXPECT_EQ(result.status, LockStatus::TIMEOUT);
}

TEST_F(LockManagerTest, ExclusiveBlocksReaders) {
    EXPECT_EQ(lm.acquireLock(1, "row:Y", LockType::EXCLUSIVE).status, LockStatus::GRANTED);

    auto result = lm.acquireLock(2, "row:Y", LockType::SHARED,
                                  std::chrono::milliseconds(50));
    EXPECT_EQ(result.status, LockStatus::TIMEOUT);
}

// ---------------------------------------------------------------------------
// Intent lock tests
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, IntentSharedCompatibleWithIntentShared) {
    EXPECT_EQ(lm.acquireLock(1, "table:T", LockType::INTENT_SHARED).status, LockStatus::GRANTED);
    EXPECT_EQ(lm.acquireLock(2, "table:T", LockType::INTENT_SHARED).status, LockStatus::GRANTED);
}

TEST_F(LockManagerTest, IntentExclusiveIncompatibleWithExclusive) {
    EXPECT_EQ(lm.acquireLock(1, "table:T", LockType::EXCLUSIVE).status, LockStatus::GRANTED);

    auto result = lm.acquireLock(2, "table:T", LockType::INTENT_EXCLUSIVE,
                                  std::chrono::milliseconds(50));
    EXPECT_EQ(result.status, LockStatus::TIMEOUT);
}

// ---------------------------------------------------------------------------
// Release tests
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, ReleaseLock) {
    lm.acquireLock(1, "key:R", LockType::SHARED);
    EXPECT_TRUE(lm.releaseLock(1, "key:R"));
    EXPECT_FALSE(lm.holdsLock(1, "key:R", LockType::SHARED));
}

TEST_F(LockManagerTest, ReleaseAllLocks) {
    lm.acquireLock(1, "key:1", LockType::SHARED);
    lm.acquireLock(1, "key:2", LockType::EXCLUSIVE);
    lm.acquireLock(1, "key:3", LockType::INTENT_SHARED);

    lm.releaseAllLocks(1);

    EXPECT_FALSE(lm.holdsLock(1, "key:1", LockType::SHARED));
    EXPECT_FALSE(lm.holdsLock(1, "key:2", LockType::EXCLUSIVE));
    EXPECT_FALSE(lm.holdsLock(1, "key:3", LockType::INTENT_SHARED));
}

TEST_F(LockManagerTest, ReleaseUnlocksWaiters) {
    // T1 holds exclusive, T2 is blocked
    lm.acquireLock(1, "row:W", LockType::EXCLUSIVE);

    std::atomic<bool> t2_granted{false};
    std::thread t2([&] {
        auto res = lm.acquireLock(2, "row:W", LockType::SHARED,
                                   std::chrono::milliseconds(2000));
        t2_granted = (res.status == LockStatus::GRANTED);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    lm.releaseLock(1, "row:W");
    t2.join();

    EXPECT_TRUE(t2_granted);
}

// ---------------------------------------------------------------------------
// Lock upgrade test
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, UpgradeSharedToExclusive) {
    lm.acquireLock(1, "row:U", LockType::SHARED);
    auto res = lm.upgradeLock(1, "row:U");
    EXPECT_EQ(res.status, LockStatus::GRANTED);
    EXPECT_TRUE(lm.holdsLock(1, "row:U", LockType::EXCLUSIVE));
}

TEST_F(LockManagerTest, UpgradeBlockedByOtherReader) {
    lm.acquireLock(1, "row:V", LockType::SHARED);
    lm.acquireLock(2, "row:V", LockType::SHARED);

    // T1 wants to upgrade; T2 is still holding shared
    auto res = lm.upgradeLock(1, "row:V", std::chrono::milliseconds(100));
    // Should timeout because T2 still holds shared
    EXPECT_EQ(res.status, LockStatus::TIMEOUT);
}

// ---------------------------------------------------------------------------
// Two-Phase Locking enforcement
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, TwoPLShrinkingPhase) {
    lm.acquireLock(1, "row:A", LockType::SHARED);
    lm.beginShrinkingPhase(1);

    // New acquisition denied in shrinking phase
    auto res = lm.acquireLock(1, "row:B", LockType::SHARED,
                               std::chrono::milliseconds(50));
    EXPECT_EQ(res.status, LockStatus::DENIED);
}

TEST_F(LockManagerTest, TwoPLCanReleaseInShrinkingPhase) {
    lm.acquireLock(1, "row:C", LockType::EXCLUSIVE);
    lm.beginShrinkingPhase(1);
    EXPECT_TRUE(lm.releaseLock(1, "row:C"));
}

// ---------------------------------------------------------------------------
// getLocksHeld and statistics
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, GetLocksHeld) {
    lm.acquireLock(5, "key:X", LockType::SHARED);
    lm.acquireLock(5, "key:Y", LockType::EXCLUSIVE);

    auto locks = lm.getLocksHeld(5);
    EXPECT_EQ(locks.size(), 2u);
}

TEST_F(LockManagerTest, Statistics) {
    lm.acquireLock(1, "key:S1", LockType::SHARED);
    lm.acquireLock(1, "key:S2", LockType::EXCLUSIVE);
    lm.releaseLock(1, "key:S1");

    auto stats = lm.getStats();
    EXPECT_GE(stats.total_acquired, 2u);
    EXPECT_GE(stats.total_released, 1u);
}

TEST_F(LockManagerTest, TimeoutCountedInStats) {
    lm.acquireLock(1, "row:T", LockType::EXCLUSIVE);
    lm.acquireLock(2, "row:T", LockType::EXCLUSIVE, std::chrono::milliseconds(20));

    auto stats = lm.getStats();
    EXPECT_GE(stats.total_timeouts, 1u);
}

// ---------------------------------------------------------------------------
// Concurrent correctness
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, ConcurrentExclusiveAccess) {
    const int num_threads = 8;
    std::atomic<int> inside{0};
    std::atomic<int> violations{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            LockManager::TransactionId txn = static_cast<uint64_t>(i + 100);
            auto res = lm.acquireLock(txn, "shared_row", LockType::EXCLUSIVE,
                                      std::chrono::milliseconds(5000));
            if (res.status == LockStatus::GRANTED) {
                int cnt = ++inside;
                if (cnt > 1) {
                  ++violations;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                --inside;
                lm.releaseLock(txn, "shared_row");
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(violations.load(), 0) << "Mutual exclusion violated";
}

TEST_F(LockManagerTest, ConcurrentSharedReadAccess) {
    const int num_readers = 10;
    std::atomic<int> concurrent_readers{0};
    std::atomic<bool> exceeded_single{false};

    // First ensure no exclusive lock holds
    std::vector<std::thread> threads;
    for (int i = 0; i < num_readers; ++i) {
        threads.emplace_back([&, i]() {
            auto res = lm.acquireLock(static_cast<uint64_t>(i + 200), "shared_data",
                                      LockType::SHARED,
                                      std::chrono::milliseconds(2000));
            if (res.status == LockStatus::GRANTED) {
                int cnt = ++concurrent_readers;
                if (cnt > 1) exceeded_single = true; // Multiple readers expected
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                --concurrent_readers;
                lm.releaseLock(static_cast<uint64_t>(i + 200), "shared_data");
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }

    // Multiple readers should have run concurrently
    EXPECT_TRUE(exceeded_single) << "Expected concurrent shared reads";
}

// ---------------------------------------------------------------------------
// Deadlock detector helpers
// ---------------------------------------------------------------------------

TEST_F(LockManagerTest, GetWaitersAndWaitingFor) {
    lm.acquireLock(1, "row:D", LockType::EXCLUSIVE);

    std::thread t([&] {
        lm.acquireLock(2, "row:D", LockType::EXCLUSIVE,
                       std::chrono::milliseconds(500));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto waiters = lm.getWaiters("row:D");
    EXPECT_FALSE(waiters.empty());

    auto waiting_for = lm.getWaitingFor(2);
    EXPECT_FALSE(waiting_for.empty());

    lm.releaseLock(1, "row:D");
    t.join();
}
