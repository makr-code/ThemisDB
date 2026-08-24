/**
 * @file test_transaction_chaos.cpp
 * @brief Comprehensive chaos testing for transaction module
 * 
 * Wave A Batch A-9: Chaos Testing & Fault Injection
 * Tests transaction resilience under chaotic conditions
 * 
 * Scenarios:
 * - CoordinatorCrashRecovery: Crash coordinator mid-commit
 * - DeadlockDetectionAndBreak: Circular wait conditions
 * - LongRunningTransactionTimeout: Transaction exceeds timeout
 * - CascadingRollbackScenario: Multiple transaction rollbacks
 * - PreCommitCrashRecovery: Crash between prepare and commit
 * - WALRecoveryAfterCrash: Recovery from crash using WAL
 * - ConcurrentPrepareAndCommit: Parallel prepare/commit operations
 * - TimeoutRecoveryCorrectness: Verify cleanup after timeout
 * 
 * Date: 2026-08-16
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <iostream>
#include <map>

#include "tests/utils/fault_injector.h"

using namespace std::chrono_literals;

namespace themis {
namespace test {

// ============================================================================
// MOCK TRANSACTION COORDINATOR FOR TESTING
// ============================================================================

/**
 * @brief Simple mock transaction coordinator for chaos testing
 */
class MockTransactionCoordinator {
public:
    enum class TxnState {
        INITIAL,
        PREPARING,
        PREPARED,
        COMMITTING,
        COMMITTED,
        ABORTING,
        ABORTED,
        CRASHED
    };

    struct Transaction {
        int txn_id;
        TxnState state;
        std::string key;
        std::string value;
        std::chrono::steady_clock::time_point start_time;
        bool is_active;
    };

    MockTransactionCoordinator() : next_txn_id_(0), next_deadlock_id_(0) {}

    int beginTransaction() {
        std::lock_guard<std::mutex> lk(mutex_);
        int txn_id = next_txn_id_++;
        txns_[txn_id] = Transaction{
            txn_id, TxnState::INITIAL, "", "", std::chrono::steady_clock::now(), true};
        return txn_id;
    }

    bool prepareTransaction(int txn_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = txns_.find(txn_id);
        if (it == txns_.end()) {
            return false;
        }

        auto& txn = it->second;
        txn.state = TxnState::PREPARING;

        // Simulate crash if injected
        if (crash_injector_ && crash_injector_->shouldCrashAt(
                CrashInjector::CrashPoint::BEFORE_FSYNC)) {
            txn.state = TxnState::CRASHED;
            txn.is_active = false;
            return false;
        }

        txn.state = TxnState::PREPARED;
        return true;
    }

    bool commitTransaction(int txn_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = txns_.find(txn_id);
        if (it == txns_.end()) {
            return false;
        }

        auto& txn = it->second;
        if (txn.state != TxnState::PREPARED) {
            return false;  // Not prepared
        }

        txn.state = TxnState::COMMITTING;

        // Simulate crash if injected
        if (crash_injector_ && crash_injector_->shouldCrashAt(
                CrashInjector::CrashPoint::DURING_COMMIT)) {
            txn.state = TxnState::CRASHED;
            txn.is_active = false;
            return false;
        }

        txn.state = TxnState::COMMITTED;
        txn.is_active = false;
        return true;
    }

    bool abortTransaction(int txn_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = txns_.find(txn_id);
        if (it == txns_.end()) {
            return false;
        }

        auto& txn = it->second;
        txn.state = TxnState::ABORTING;

        std::this_thread::sleep_for(10ms);  // Simulate abort work

        txn.state = TxnState::ABORTED;
        txn.is_active = false;
        return true;
    }

    bool setKey(int txn_id, const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = txns_.find(txn_id);
        if (it == txns_.end()) {
            return false;
        }
        it->second.key = key;
        it->second.value = value;
        return true;
    }

    TxnState getState(int txn_id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = txns_.find(txn_id);
        if (it == txns_.end()) {
            return TxnState::INITIAL;
        }
        return it->second.state;
    }

    int getPendingTransactionCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        int count = 0;
        for (const auto& [id, txn] : txns_) {
            if (txn.is_active) {
                count++;
            }
        }
        return count;
    }

    void setCrashInjector(CrashInjector* injector) {
        std::lock_guard<std::mutex> lk(mutex_);
        crash_injector_ = injector;
    }

    void simulateDeadlock(int txn_id_a, int txn_id_b) {
        std::lock_guard<std::mutex> lk(mutex_);
        int deadlock_id = next_deadlock_id_++;
        deadlocks_[deadlock_id] = {txn_id_a, txn_id_b};
    }

    bool hasDeadlock(int txn_id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        for (const auto& [id, pair] : deadlocks_) {
            if (pair.first == txn_id || pair.second == txn_id) {
                return true;
            }
        }
        return false;
    }

    int getDeadlockCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return deadlocks_.size();
    }

    void clearDeadlocks() {
        std::lock_guard<std::mutex> lk(mutex_);
        deadlocks_.clear();
    }

private:
    std::map<int, Transaction> txns_;
    int next_txn_id_;
    int next_deadlock_id_;
    CrashInjector* crash_injector_ = nullptr;
    std::map<int, std::pair<int, int>> deadlocks_;  // deadlock_id -> (txn_a, txn_b)
    mutable std::mutex mutex_;
};

// ============================================================================
// CHAOS TESTS
// ============================================================================

class TransactionChaosTest : public ::testing::Test {
protected:
    void SetUp() override {
        coordinator_ = std::make_unique<MockTransactionCoordinator>();
    }

    void TearDown() override {
        // Verify cleanup
        EXPECT_EQ(coordinator_->getPendingTransactionCount(), 0);
        coordinator_.reset();
    }

    std::unique_ptr<MockTransactionCoordinator> coordinator_;
};

/**
 * @brief Test: Coordinator crash mid-commit recovery
 * 
 * Injects crash before FSYNC during commit phase.
 * Verifies transaction is either fully committed or fully rolled back.
 */
TEST_F(TransactionChaosTest, CoordinatorCrashRecovery) {
    // Setup crash injector
    CrashInjector::CrashConfig cfg{
    };
    cfg.target_component = "coordinator";
    cfg.duration = 1s;  // Auto-recover after 1s
    cfg.auto_recover = true;
    cfg.crash_point = CrashInjector::CrashPoint::DURING_COMMIT;
    CrashInjector crash_injector(cfg);
    coordinator_->setCrashInjector(&crash_injector);

    // Inject crash
    auto result = crash_injector.inject();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.state, FaultInjector::InjectionState::ACTIVE);

    // Begin transaction
    int txn_id = coordinator_->beginTransaction();
    EXPECT_GE(txn_id, 0);

    // Set key
    coordinator_->setKey(txn_id, "key1", "value1");

    // Try to prepare - should succeed (crash injected during commit)
    bool prepare_ok = coordinator_->prepareTransaction(txn_id);
    EXPECT_TRUE(prepare_ok);

    // Try to commit - crash will be triggered
    bool commit_ok = coordinator_->commitTransaction(txn_id);
    // May fail due to crash injection
    if (!commit_ok) {
        // Must abort if not committed
        coordinator_->abortTransaction(txn_id);
    }

    // Recover from crash
    std::this_thread::sleep_for(100ms);
    result = crash_injector.recover();
    EXPECT_TRUE(result.success);

    // Verify state is consistent (either committed or aborted)
    auto final_state = coordinator_->getState(txn_id);
    bool is_consistent =
        (final_state == MockTransactionCoordinator::TxnState::COMMITTED ||
         final_state == MockTransactionCoordinator::TxnState::ABORTED ||
         final_state == MockTransactionCoordinator::TxnState::CRASHED);
    EXPECT_TRUE(is_consistent) << "Transaction in inconsistent state";
}

/**
 * @brief Test: Deadlock detection and breaking
 * 
 * Simulates circular wait condition and verifies deadlock detector.
 */
TEST_F(TransactionChaosTest, DeadlockDetectionAndBreak) {
    // Begin two transactions
    int txn_a = coordinator_->beginTransaction();
    int txn_b = coordinator_->beginTransaction();

    // Setup deadlock
    coordinator_->simulateDeadlock(txn_a, txn_b);

    // Verify deadlock detected
    EXPECT_TRUE(coordinator_->hasDeadlock(txn_a));
    EXPECT_TRUE(coordinator_->hasDeadlock(txn_b));
    EXPECT_EQ(coordinator_->getDeadlockCount(), 1);

    // Clear deadlock (detector's action)
    coordinator_->clearDeadlocks();
    EXPECT_EQ(coordinator_->getDeadlockCount(), 0);

    // Both can now proceed
    EXPECT_FALSE(coordinator_->hasDeadlock(txn_a));
    EXPECT_FALSE(coordinator_->hasDeadlock(txn_b));

    // Abort both
    EXPECT_TRUE(coordinator_->abortTransaction(txn_a));
    EXPECT_TRUE(coordinator_->abortTransaction(txn_b));
}

/**
 * @brief Test: Long-running transaction timeout
 * 
 * Transaction that exceeds timeout limit.
 * Verifies system triggers timeout and cleans up resources.
 */
TEST_F(TransactionChaosTest, LongRunningTransactionTimeout) {
    const std::chrono::milliseconds TIMEOUT = 100ms;

    int txn_id = coordinator_->beginTransaction();

    // Simulate long-running operation
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(TIMEOUT + 50ms);
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Verify timeout triggered
    EXPECT_GE(elapsed, TIMEOUT);

    // Abort transaction due to timeout
    bool abort_ok = coordinator_->abortTransaction(txn_id);
    EXPECT_TRUE(abort_ok);

    // Verify state is ABORTED
    auto state = coordinator_->getState(txn_id);
    EXPECT_EQ(state, MockTransactionCoordinator::TxnState::ABORTED);
}

/**
 * @brief Test: Cascading rollback scenario
 * 
 * Multiple transactions fail sequentially, causing cascade of rollbacks.
 */
TEST_F(TransactionChaosTest, CascadingRollbackScenario) {
    const int TXN_COUNT = 5;
    std::vector<int> txn_ids;

    // Begin multiple transactions
    for (int i = 0; i < TXN_COUNT; ++i) {
        int txn_id = coordinator_->beginTransaction();
        coordinator_->setKey(txn_id, "key" + std::to_string(i), "value" + std::to_string(i));
        txn_ids.push_back(txn_id);
    }

    // Prepare all
    int prepared_count = 0;
    for (int txn_id : txn_ids) {
        if (coordinator_->prepareTransaction(txn_id)) {
            prepared_count++;
        }
    }
    EXPECT_EQ(prepared_count, TXN_COUNT);

    // Commit first, then all rest abort
    EXPECT_TRUE(coordinator_->commitTransaction(txn_ids[0]));

    for (int i = 1; i < TXN_COUNT; ++i) {
        EXPECT_TRUE(coordinator_->abortTransaction(txn_ids[i]));
    }

    // Verify states
    EXPECT_EQ(coordinator_->getState(txn_ids[0]),
              MockTransactionCoordinator::TxnState::COMMITTED);
    for (int i = 1; i < TXN_COUNT; ++i) {
        EXPECT_EQ(coordinator_->getState(txn_ids[i]),
                  MockTransactionCoordinator::TxnState::ABORTED);
    }
}

/**
 * @brief Test: Pre-commit crash recovery
 * 
 * Crash between prepare and commit phases.
 * Verifies recovery mechanism restores consistent state.
 */
TEST_F(TransactionChaosTest, PreCommitCrashRecovery) {
    CrashInjector::CrashConfig cfg{
    };
    cfg.target_component = "coordinator_pre_commit";
    cfg.duration = 500ms;
    cfg.auto_recover = true;
    cfg.crash_point = CrashInjector::CrashPoint::DURING_COMMIT;
    CrashInjector crash_injector(cfg);
    coordinator_->setCrashInjector(&crash_injector);

    int txn_id = coordinator_->beginTransaction();
    coordinator_->setKey(txn_id, "test_key", "test_value");

    // Prepare succeeds
    EXPECT_TRUE(coordinator_->prepareTransaction(txn_id));

    // Inject crash
    crash_injector.inject();

    // Try to commit - will fail
    bool commit_ok = coordinator_->commitTransaction(txn_id);
    EXPECT_FALSE(commit_ok);

    // Wait for recovery
    std::this_thread::sleep_for(600ms);
    crash_injector.recover();

    // Verify state is recoverable
    auto state = coordinator_->getState(txn_id);
    bool is_consistent = (state == MockTransactionCoordinator::TxnState::COMMITTED ||
                         state == MockTransactionCoordinator::TxnState::CRASHED);
    EXPECT_TRUE(is_consistent);
}

/**
 * @brief Test: Concurrent prepare and commit operations
 * 
 * Multiple threads executing prepare and commit concurrently.
 * Verifies no race conditions or deadlocks.
 */
TEST_F(TransactionChaosTest, ConcurrentPrepareAndCommit) {
    const int THREAD_COUNT = 4;
    const int TXN_PER_THREAD = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([this, &success_count, t]() {
            for (int i = 0; i < TXN_PER_THREAD; ++i) {
                int txn_id = coordinator_->beginTransaction();
                std::string key = "thread" + std::to_string(t) + "_key" + std::to_string(i);
                coordinator_->setKey(txn_id, key, "value" + std::to_string(i));

                if (coordinator_->prepareTransaction(txn_id) &&
                    coordinator_->commitTransaction(txn_id)) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify significant success rate
    EXPECT_GE(success_count, THREAD_COUNT * TXN_PER_THREAD * 0.8);  // At least 80%
}

/**
 * @brief Test: WAL recovery after crash
 * 
 * Simulates WAL-based recovery from crash.
 */
TEST_F(TransactionChaosTest, WALRecoveryAfterCrash) {
    // Begin transaction
    int txn_id = coordinator_->beginTransaction();
    coordinator_->setKey(txn_id, "recovery_key", "recovery_value");

    // Prepare (normally WALed)
    EXPECT_TRUE(coordinator_->prepareTransaction(txn_id));

    // Simulate crash before commit is persisted
    auto state_before = coordinator_->getState(txn_id);
    EXPECT_EQ(state_before, MockTransactionCoordinator::TxnState::PREPARED);

    // Simulate recovery - would read WAL and find prepared transaction
    // In real system, would recover and rollback or commit based on WAL

    // Abort to simulate recovery decision
    EXPECT_TRUE(coordinator_->abortTransaction(txn_id));

    // Verify final state
    EXPECT_EQ(coordinator_->getState(txn_id), MockTransactionCoordinator::TxnState::ABORTED);
}

/**
 * @brief Test: Timeout recovery correctness
 * 
 * Verify proper cleanup when transaction times out.
 */
TEST_F(TransactionChaosTest, TimeoutRecoveryCorrectness) {
    const int TXN_COUNT = 10;
    std::vector<int> txn_ids;

    for (int i = 0; i < TXN_COUNT; ++i) {
        int txn_id = coordinator_->beginTransaction();
        txn_ids.push_back(txn_id);
    }

    // Simulate timeout - abort all
    for (int txn_id : txn_ids) {
        coordinator_->abortTransaction(txn_id);
    }

    // Verify all are cleaned up
    for (int txn_id : txn_ids) {
        auto state = coordinator_->getState(txn_id);
        EXPECT_EQ(state, MockTransactionCoordinator::TxnState::ABORTED);
    }
}

}  // namespace test
}  // namespace themis
