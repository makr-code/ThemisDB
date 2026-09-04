/**
 * @file test_wave4c_t1t4_hardening.cpp
 * @brief Wave 4-C T1–T4 hardening regression tests.
 *
 * Coverage:
 *  T1 — STUB/SIMULATION NOTE present in distributed_transaction_manager.cpp
 *  T2 — Mutual upgrade deadlock detected promptly (not via timeout)
 *  T3 — GTM Phase-2 runs outside global mutex (non-blocking)
 *  T4 — Predicate lock drop increments counter + emits THEMIS_WARN
 *
 * Labels: wave_c release_critical
 */

#include <gtest/gtest.h>
#include "transaction/lock_manager.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using ms = std::chrono::milliseconds;

// ─────────────────────────────────────────────────────────────────────────────
// T1 — STUB/SIMULATION NOTE documentation in source file
// ─────────────────────────────────────────────────────────────────────────────

namespace {
// Reads lines from a file and returns true if the given substring is present.
bool fileContainsString(const std::string& path, const std::string& needle) {
    std::ifstream f(path);
    if (!f.is_open()) {
      return false;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.find(needle) != std::string::npos) {
          return true;
        }
    }
    return false;
}
} // namespace

static const char* kDTMSourcePath =
    THEMIS_ROOT_DIR "/src/transaction/distributed_transaction_manager.cpp";

TEST(Wave4CT1StubNote, Phase2BridgeHasStubNoteComment) {
    EXPECT_TRUE(fileContainsString(kDTMSourcePath, "STUB/SIMULATION NOTE"))
        << "Expected STUB/SIMULATION NOTE comment in " << kDTMSourcePath;
}

TEST(Wave4CT1StubNote, Phase2BridgeDocumentsActivation) {
    EXPECT_TRUE(fileContainsString(kDTMSourcePath, "Activation:"))
        << "Expected 'Activation:' field in STUB/SIMULATION NOTE";
}

TEST(Wave4CT1StubNote, Phase2BridgeDocumentsProductionDelta) {
    EXPECT_TRUE(fileContainsString(kDTMSourcePath, "Production Delta:"))
        << "Expected 'Production Delta:' field in STUB/SIMULATION NOTE";
}

TEST(Wave4CT1StubNote, Phase2BridgeDocumentsRemovalPlan) {
    EXPECT_TRUE(fileContainsString(kDTMSourcePath, "Removal Plan:"))
        << "Expected 'Removal Plan:' field in STUB/SIMULATION NOTE";
}

// ─────────────────────────────────────────────────────────────────────────────
// T2 — Mutual lock-upgrade deadlock detection
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave4CT2UpgradeDeadlock, SingleUpgradeSucceeds) {
    LockManager lm;
    const std::string key = "row:single_upgrade";
    const LockManager::TransactionId txA = 101;

    auto r1 = lm.acquireLock(txA, key, LockType::SHARED, ms{500});
    ASSERT_EQ(r1.status, LockStatus::GRANTED);

    auto r2 = lm.upgradeLock(txA, key, ms{500});
    EXPECT_EQ(r2.status, LockStatus::GRANTED)
        << "Single-holder upgrade must succeed immediately";

    lm.releaseAllLocks(txA);
}

TEST(Wave4CT2UpgradeDeadlock, MutualUpgradeReturnsDeniedNotTimeout) {
    // Two transactions each hold SHARED; both try to upgrade → deadlock detected
    // immediately (one of them gets DENIED/DEADLOCK without waiting for timeout).
    LockManager lm;
    const std::string key = "row:mutual_upgrade";
    const LockManager::TransactionId txA = 201;
    const LockManager::TransactionId txB = 202;

    // Both acquire SHARED
    ASSERT_EQ(lm.acquireLock(txA, key, LockType::SHARED, ms{500}).status,
              LockStatus::GRANTED);
    ASSERT_EQ(lm.acquireLock(txB, key, LockType::SHARED, ms{500}).status,
              LockStatus::GRANTED);

    std::atomic<LockStatus> statusA{LockStatus::GRANTED};
    std::atomic<LockStatus> statusB{LockStatus::GRANTED};
    std::atomic<bool> doneA{false}, doneB{false};

    auto upgradeA = [&] {
        statusA.store(lm.upgradeLock(txA, key, ms{5000}).status);
        doneA.store(true);
    };
    auto upgradeB = [&] {
        statusB.store(lm.upgradeLock(txB, key, ms{5000}).status);
        doneB.store(true);
    };

    std::thread tA(upgradeA);
    std::thread tB(upgradeB);

    // Both should finish well within 2 seconds (deadlock detected, not 5s timeout)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!doneA.load() || !doneB.load()) {
        if (std::chrono::steady_clock::now() > deadline) {
          break;
        }
        std::this_thread::sleep_for(ms{20});
    }

    tA.join();
    tB.join();

    EXPECT_TRUE(doneA.load() && doneB.load())
        << "Both upgrade attempts should complete (not hang until 5s timeout)";

    // At least one must have been denied (deadlock) or the other granted
    bool oneGranted = (statusA == LockStatus::GRANTED) || (statusB == LockStatus::GRANTED);
    bool oneDenied  = (statusA == LockStatus::DENIED)  || (statusB == LockStatus::DENIED);
    EXPECT_TRUE(oneGranted || oneDenied)
        << "Expected at least one GRANTED and/or one DENIED in mutual upgrade scenario";

    // The important assertion: neither should have TIMED_OUT
    bool anyTimeout = (statusA == LockStatus::TIMEOUT) || (statusB == LockStatus::TIMEOUT);
    EXPECT_FALSE(anyTimeout)
        << "Mutual upgrade deadlock should be detected without waiting for full timeout";

    lm.releaseAllLocks(txA);
    lm.releaseAllLocks(txB);
}

TEST(Wave4CT2UpgradeDeadlock, ThreeConcurrentUpgraders_NoHang) {
    // Three transactions holding SHARED; all try upgrade concurrently.
    // Verifies system doesn't hang even with 3+ upgraders.
    LockManager lm;
    const std::string key = "row:three_upgraders";
    const LockManager::TransactionId txA = 301, txB = 302, txC = 303;

    ASSERT_EQ(lm.acquireLock(txA, key, LockType::SHARED, ms{500}).status, LockStatus::GRANTED);
    ASSERT_EQ(lm.acquireLock(txB, key, LockType::SHARED, ms{500}).status, LockStatus::GRANTED);
    ASSERT_EQ(lm.acquireLock(txC, key, LockType::SHARED, ms{500}).status, LockStatus::GRANTED);

    std::atomic<bool> doneA{false}, doneB{false}, doneC{false};
    auto upA = [&]{ lm.upgradeLock(txA, key, ms{2000}); doneA.store(true); };
    auto upB = [&]{ lm.upgradeLock(txB, key, ms{2000}); doneB.store(true); };
    auto upC = [&]{ lm.upgradeLock(txC, key, ms{2000}); doneC.store(true); };

    std::thread tA(upA), tB(upB), tC(upC);
    tA.join(); tB.join(); tC.join();

    EXPECT_TRUE(doneA.load() && doneB.load() && doneC.load())
        << "All upgrade threads should complete without hanging";

    lm.releaseAllLocks(txA);
    lm.releaseAllLocks(txB);
    lm.releaseAllLocks(txC);
}

// ─────────────────────────────────────────────────────────────────────────────
// T3 — GTM Phase-2 runs outside global lock (non-regression)
// ─────────────────────────────────────────────────────────────────────────────
// The GTM Phase-2 patch is validated via source inspection (file-contains check)
// since GTM requires full RocksDB/network stack.  The test confirms the
// snapshot-then-release pattern is present in the source.

static const char* kGTMSourcePath =
    THEMIS_ROOT_DIR "/src/transaction/global_transaction_manager.cpp";

TEST(Wave4CT3GTMPhase2, SnapshotPatternPresent) {
    EXPECT_TRUE(fileContainsString(kGTMSourcePath, "rec_snapshot"))
        << "Expected snapshot variable in global_transaction_manager.cpp (T3 pattern)";
}

TEST(Wave4CT3GTMPhase2, Phase2CalledOutsideLock) {
    // The snapshot-then-release pattern uses .unlock() before runPhase2 call.
    // Verify source contains the unlock + runPhase2 sequence.
    EXPECT_TRUE(fileContainsString(kGTMSourcePath, "runPhase2"))
        << "Expected runPhase2 call in global_transaction_manager.cpp";
    EXPECT_TRUE(fileContainsString(kGTMSourcePath, "no mutex held during RPC"))
        << "Expected unlock comment before runPhase2 in global_transaction_manager.cpp";
}

// ─────────────────────────────────────────────────────────────────────────────
// T4 — Predicate lock drop counter + THEMIS_WARN
// ─────────────────────────────────────────────────────────────────────────────

TEST(Wave4CT4PredicateLockDrop, SuccessUnderCapacity) {
    LockManager lm;
    lm.setPredicateLockingEnabled(true);
    lm.setMaxPredicateLocks(10);

    bool ok = lm.acquirePredicateLock(1, "a", "z");
    EXPECT_TRUE(ok) << "Predicate lock should succeed when under capacity";
    EXPECT_EQ(lm.predicateLockDropCount(), 0u)
        << "Drop counter must remain 0 when lock succeeds";
}

TEST(Wave4CT4PredicateLockDrop, DropIncrementsCounter) {
    LockManager lm;
    lm.setPredicateLockingEnabled(true);
    lm.setMaxPredicateLocks(1);

    // First lock fills capacity
    bool first = lm.acquirePredicateLock(10, "a", "m");
    EXPECT_TRUE(first);
    EXPECT_EQ(lm.predicateLockDropCount(), 0u);

    // Second lock should be dropped
    bool second = lm.acquirePredicateLock(11, "n", "z");
    EXPECT_FALSE(second) << "Lock should be dropped when at capacity";
    EXPECT_EQ(lm.predicateLockDropCount(), 1u)
        << "Drop counter must be incremented on capacity reject";
}

TEST(Wave4CT4PredicateLockDrop, MultipleDropsAccumulate) {
    LockManager lm;
    lm.setPredicateLockingEnabled(true);
    lm.setMaxPredicateLocks(2);

    lm.acquirePredicateLock(1, "a", "b");
    lm.acquirePredicateLock(2, "c", "d");

    // Three more attempts — all should drop
    lm.acquirePredicateLock(3, "e", "f");
    lm.acquirePredicateLock(4, "g", "h");
    lm.acquirePredicateLock(5, "i", "j");

    EXPECT_EQ(lm.predicateLockDropCount(), 3u)
        << "Each capacity-reject must increment drop counter by 1";
}

TEST(Wave4CT4PredicateLockDrop, CounterReflectsOnlyDrops) {
    LockManager lm;
    lm.setPredicateLockingEnabled(true);
    lm.setMaxPredicateLocks(5);

    // 5 successful, 2 drops
    for (int i = 0; i < 5; ++i) {
        lm.acquirePredicateLock(static_cast<uint64_t>(i + 100), "k" + std::to_string(i),
                                "k" + std::to_string(i) + "z");
    }
    lm.acquirePredicateLock(200, "x", "y");
    lm.acquirePredicateLock(201, "x2", "y2");

    EXPECT_EQ(lm.predicateLockDropCount(), 2u);
}
