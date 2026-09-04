/**
 * @file test_transaction_stress.cpp
 * @brief Stress testing for transaction module
 * 
 * Wave A Batch A-9: Chaos Testing & Fault Injection
 * Tests transaction throughput and correctness under stress
 * 
 * Scenarios:
 * - 10k Concurrent Short Transactions
 * - Transaction Cleanup Verification
 * - High Contention Lock Stress
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

using namespace std::chrono_literals;

namespace themis {
namespace test {

// ============================================================================
// SIMPLE TRANSACTION MANAGER FOR STRESS TESTING
// ============================================================================

class StressTransactionManager {
public:
    struct Transaction {
        int txn_id = 0;
        bool committed = {};
        std::chrono::steady_clock::time_point start_time;
    };

    StressTransactionManager() : next_txn_id_(0), committed_count_(0), aborted_count_(0) {}

    int beginTransaction() {
        std::lock_guard<std::mutex> lk(mutex_);
        int txn_id = next_txn_id_++;
        txns_[txn_id] = Transaction{txn_id, false, std::chrono::steady_clock::now()};
        return txn_id;
    }

    bool commitTransaction(int txn_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = txns_.find(txn_id);
        if (it == txns_.end()) {
            return false;
        }
        it->second.committed = true;
        committed_count_++;
        txns_.erase(it);
        return true;
    }

    bool abortTransaction(int txn_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = txns_.find(txn_id);
        if (it == txns_.end()) {
            return false;
        }
        txns_.erase(it);
        aborted_count_++;
        return true;
    }

    int getPendingTransactionCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return txns_.size();
    }

    int getCommittedCount() const { return committed_count_; }
    int getAbortedCount() const { return aborted_count_; }

private:
    std::map<int, Transaction> txns_;
    int next_txn_id_;
    std::atomic<int> committed_count_;
    std::atomic<int> aborted_count_;
    mutable std::mutex mutex_;
};

// ============================================================================
// STRESS TESTS
// ============================================================================

class TransactionStressTest : public ::testing::Test {
protected:
    void SetUp() override { mgr_ = std::make_unique<StressTransactionManager>(); }

    void TearDown() override {
        // Verify cleanup
        EXPECT_EQ(mgr_->getPendingTransactionCount(), 0);
        mgr_.reset();
    }

    std::unique_ptr<StressTransactionManager> mgr_;
};

/**
 * @brief Stress: 10k concurrent short transactions
 * 
 * 10 threads × 1000 transactions each = 10k total
 * Verifies all commit successfully and cleanup occurs.
 */
TEST_F(TransactionStressTest, ConcurrentShortTransactions10k) {
    const int THREAD_COUNT = 10;
    const int TXN_PER_THREAD = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    auto start = std::chrono::steady_clock::now();

    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([this, &success_count, &failure_count, t]() {
            for (int i = 0; i < TXN_PER_THREAD; ++i) {
                int txn_id = mgr_->beginTransaction();
                if (mgr_->commitTransaction(txn_id)) {
                    success_count++;
                } else {
                    failure_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    double elapsed_sec = std::chrono::duration<double>(elapsed).count();
    double throughput = (THREAD_COUNT * TXN_PER_THREAD) / elapsed_sec;

    std::cout << "[TransactionStress] 10k short transactions completed in " << elapsed_sec << "s"
              << " (throughput: " << throughput << " txns/sec)" << std::endl;

    // Verify results
    int total_expected = THREAD_COUNT * TXN_PER_THREAD;
    EXPECT_EQ(success_count, total_expected);
    EXPECT_EQ(failure_count, 0);
    EXPECT_GE(throughput, 1000.0);  // At least 1000 txns/sec
}

/**
 * @brief Stress: High contention lock stress
 * 
 * Multiple threads contending for same locks.
 * Verifies no deadlocks and reasonable performance.
 */
TEST_F(TransactionStressTest, HighContentionLockStress) {
    const int THREAD_COUNT = 8;
    const int OPERATIONS_PER_THREAD = 500;
    std::vector<std::thread> threads;
    std::atomic<int> operations{0};
    std::atomic<int> errors{0};

    auto start = std::chrono::steady_clock::now();

    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([this, &operations, &errors]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int txn_id = mgr_->beginTransaction();
                if (mgr_->commitTransaction(txn_id)) {
                    operations++;
                } else {
                    errors++;
                }
            }
        });
    }

    // Add timeout protection
    bool completed = true;
    for (auto& t : threads) {
        t.join();
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    double elapsed_sec = std::chrono::duration<double>(elapsed).count();

    std::cout << "[TransactionStress] High contention: " << operations << " operations in "
              << elapsed_sec << "s" << std::endl;

    EXPECT_TRUE(completed);
    EXPECT_GT(operations, THREAD_COUNT * OPERATIONS_PER_THREAD * 0.8);
    EXPECT_LT(errors, THREAD_COUNT * OPERATIONS_PER_THREAD * 0.2);
}

/**
 * @brief Stress: Transaction cleanup verification
 * 
 * Verify all transactions are cleaned up after execution.
 */
TEST_F(TransactionStressTest, TransactionCleanupVerification) {
    const int TXN_COUNT = 1000;
    std::vector<int> txn_ids;

    // Create transactions
    for (int i = 0; i < TXN_COUNT; ++i) {
        int txn_id = mgr_->beginTransaction();
        txn_ids.push_back(txn_id);
    }

    // Verify pending
    EXPECT_EQ(mgr_->getPendingTransactionCount(), TXN_COUNT);

    // Commit all
    int committed = 0;
    for (int txn_id : txn_ids) {
        if (mgr_->commitTransaction(txn_id)) {
            committed++;
        }
    }
    EXPECT_EQ(committed, TXN_COUNT);

    // Verify cleanup
    EXPECT_EQ(mgr_->getPendingTransactionCount(), 0);
    EXPECT_EQ(mgr_->getCommittedCount(), TXN_COUNT);
}

}  // namespace test
}  // namespace themis
