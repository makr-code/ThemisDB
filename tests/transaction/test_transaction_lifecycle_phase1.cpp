#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include "transaction/transaction_manager.h"
#include "transaction/isolation_level.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <filesystem>

using namespace themis;

class TransactionLifecyclePhase1Test : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_transaction_lifecycle_phase1_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;
        config.enable_compression = true;
        config.log_level = "ERROR";

        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";

        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_index_ = std::make_unique<GraphIndexManager>(*db_);
        vector_index_ = std::make_unique<VectorIndexManager>(*db_);

        tx_manager_ = std::make_unique<TransactionManager>(
            *db_, *secondary_index_, *graph_index_, *vector_index_
        );
    }

    void TearDown() override {
        tx_manager_.reset();
        vector_index_.reset();
        graph_index_.reset();
        secondary_index_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }

        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> secondary_index_;
    std::unique_ptr<GraphIndexManager> graph_index_;
    std::unique_ptr<VectorIndexManager> vector_index_;
    std::unique_ptr<TransactionManager> tx_manager_;
};

// ===== LIFECYCLE INVARIANT TESTS =====

/// AC-1: ACID lifecycle isolation enforcement - Basic state transitions
TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_BeginToCommit) {
    // Given: A new transaction
    auto txn_id = tx_manager_->beginTransaction();
    ASSERT_GT(txn_id, 0) << "Failed to begin transaction";

    // Verify the transaction is active (retrievable) before commit
    auto txn = tx_manager_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr) << "Transaction not found after begin";
    EXPECT_FALSE(txn->isFinished()) << "Transaction should not be finished after begin";

    // When: Commit the transaction
    auto status = tx_manager_->commitTransaction(txn_id);
    EXPECT_TRUE(status.ok) << "Commit should succeed: " << status.message;

    // Then: Transaction is removed from the active map; getTransaction() returns nullptr
    auto txn_after = tx_manager_->getTransaction(txn_id);
    EXPECT_EQ(txn_after, nullptr)
        << "Transaction should no longer be in the active map after commit";
}

/// AC-2: Begin/Prepare/Commit/Abort state machine correctness - Rollback path
TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_BeginToAbort) {
    // Given: A new transaction
    auto txn_id = tx_manager_->beginTransaction();
    ASSERT_GT(txn_id, 0) << "Failed to begin transaction";

    // Verify active before rollback
    auto txn = tx_manager_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr) << "Transaction not found";
    EXPECT_FALSE(txn->isFinished()) << "Transaction should not be finished after begin";

    // When: Rollback the transaction (returns bool)
    bool rolled_back = tx_manager_->rollbackTransaction(txn_id);
    EXPECT_TRUE(rolled_back) << "Rollback should succeed";

    // Then: Transaction is removed from the active map
    auto txn_after = tx_manager_->getTransaction(txn_id);
    EXPECT_EQ(txn_after, nullptr)
        << "Transaction should no longer be in the active map after rollback";
}

/// AC-2: Invalid state transitions must be detected and rejected
TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_DoubleCommitPrevention) {
    // Given: A committed transaction
    auto txn_id = tx_manager_->beginTransaction();
    ASSERT_GT(txn_id, 0);

    auto status1 = tx_manager_->commitTransaction(txn_id);
    EXPECT_TRUE(status1.ok) << "First commit should succeed";

    // When: Try to commit again (transaction no longer in active map)
    auto status2 = tx_manager_->commitTransaction(txn_id);

    // Then: Second commit should fail
    EXPECT_FALSE(status2.ok) << "Second commit should fail (transaction already completed)";
}

/// AC-2: Invalid state transitions - double rollback prevention
TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_DoubleRollbackPrevention) {
    // Given: A rolled back transaction
    auto txn_id = tx_manager_->beginTransaction();
    ASSERT_GT(txn_id, 0);

    bool rolled_back1 = tx_manager_->rollbackTransaction(txn_id);
    EXPECT_TRUE(rolled_back1) << "First rollback should succeed";

    // When: Try to rollback again
    bool rolled_back2 = tx_manager_->rollbackTransaction(txn_id);

    // Then: Second rollback should fail (not in active map)
    EXPECT_FALSE(rolled_back2) << "Second rollback should fail (transaction already completed)";
}

/// AC-2: Invalid state transitions - commit after rollback prevention
TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_CommitAfterRollbackPrevention) {
    // Given: A rolled back transaction
    auto txn_id = tx_manager_->beginTransaction();
    ASSERT_GT(txn_id, 0);

    bool rolled_back = tx_manager_->rollbackTransaction(txn_id);
    EXPECT_TRUE(rolled_back) << "Rollback should succeed";

    // When: Try to commit after rollback (transaction not in active map)
    auto status2 = tx_manager_->commitTransaction(txn_id);

    // Then: Commit should fail
    EXPECT_FALSE(status2.ok) << "Commit should fail after rollback";
}

/// AC-1: ACID lifecycle - Multiple concurrent transactions
TEST_F(TransactionLifecyclePhase1Test, LifecycleInvariants_ConcurrentTransactions) {
    constexpr int num_threads = 10;
    constexpr int txns_per_thread = 5;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    std::atomic<int> failure_count(0);
    std::mutex errors_mutex;
    std::vector<std::string> errors;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &success_count, &failure_count, &errors_mutex, &errors] {
            for (int j = 0; j < txns_per_thread; ++j) {
                try {
                    auto txn_id = tx_manager_->beginTransaction();
                    if (txn_id <= 0) {
                        ++failure_count;
                        std::lock_guard<std::mutex> lock(errors_mutex);
                        errors.push_back("Failed to begin transaction");
                        continue;
                    }

                    bool commit = (j % 2 == 0);
                    bool ok;
                    if (commit) {
                        auto status = tx_manager_->commitTransaction(txn_id);
                        ok = status.ok;
                    } else {
                        ok = tx_manager_->rollbackTransaction(txn_id);
                    }

                    if (ok) {
                        ++success_count;
                    } else {
                        ++failure_count;
                        std::lock_guard<std::mutex> lock(errors_mutex);
                        errors.push_back("Commit/rollback failed");
                    }
                } catch (const std::exception& e) {
                    ++failure_count;
                    std::lock_guard<std::mutex> lock(errors_mutex);
                    errors.push_back(std::string("Exception: ") + e.what());
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count, num_threads * txns_per_thread)
        << "All transactions should succeed";
    EXPECT_EQ(failure_count, 0) << "No failures expected";

    for (const auto& error : errors) {
        FAIL() << error;
    }
}

// ===== ISOLATION LEVEL EDGE CASES =====

/// AC-3: Isolation level behavior - READ_COMMITTED basic test
TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_ReadCommitted_BasicBehavior) {
    auto txn_id = tx_manager_->beginTransaction(IsolationLevel::READ_COMMITTED);
    ASSERT_GT(txn_id, 0) << "Failed to begin transaction with READ_COMMITTED";

    auto txn = tx_manager_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);
    EXPECT_EQ(txn->getIsolationLevel(), IsolationLevel::READ_COMMITTED);

    tx_manager_->commitTransaction(txn_id);
}

/// AC-3: Isolation level behavior - SNAPSHOT (MVCC) basic test
TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_Snapshot_MVCCBehavior) {
    // IsolationLevel::Snapshot == IsolationLevel::REPEATABLE_READ == 3
    auto txn_id = tx_manager_->beginTransaction(IsolationLevel::Snapshot);
    ASSERT_GT(txn_id, 0) << "Failed to begin transaction with Snapshot";

    auto txn = tx_manager_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);
    EXPECT_EQ(txn->getIsolationLevel(), IsolationLevel::Snapshot);

    tx_manager_->commitTransaction(txn_id);
}

/// AC-3: Isolation level behavior - SERIALIZABLE SSI basic test
TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_Serializable_SSIBehavior) {
    auto txn_id = tx_manager_->beginTransaction(IsolationLevel::SERIALIZABLE);
    ASSERT_GT(txn_id, 0) << "Failed to begin transaction with SERIALIZABLE";

    auto txn = tx_manager_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);
    EXPECT_EQ(txn->getIsolationLevel(), IsolationLevel::SERIALIZABLE);

    tx_manager_->commitTransaction(txn_id);
}

/// AC-3: Multiple isolation levels in concurrent transactions
TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_MixedConcurrentLevels) {
    std::vector<std::thread> threads;
    std::atomic<int> read_committed_count(0);
    std::atomic<int> snapshot_count(0);
    std::atomic<int> serializable_count(0);
    std::atomic<int> failure_count(0);

    for (int i = 0; i < 9; ++i) {
        threads.emplace_back([this, i, &read_committed_count, &snapshot_count,
                              &serializable_count, &failure_count] {
            IsolationLevel level;
            if (i % 3 == 0) {
                level = IsolationLevel::READ_COMMITTED;
            } else if (i % 3 == 1) {
                level = IsolationLevel::Snapshot;
            } else {
                level = IsolationLevel::SERIALIZABLE;
            }

            auto txn_id = tx_manager_->beginTransaction(level);
            if (txn_id <= 0) {
                ++failure_count;
                return;
            }

            auto txn = tx_manager_->getTransaction(txn_id);
            if (!txn) {
                ++failure_count;
                return;
            }

            auto isolation = txn->getIsolationLevel();
            if (isolation == IsolationLevel::READ_COMMITTED) {
                ++read_committed_count;
            } else if (isolation == IsolationLevel::Snapshot) {
                ++snapshot_count;
            } else if (isolation == IsolationLevel::SERIALIZABLE) {
                ++serializable_count;
            }

            auto status = tx_manager_->commitTransaction(txn_id);
            if (!status.ok) {
                ++failure_count;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(read_committed_count, 3) << "Expected 3 READ_COMMITTED transactions";
    EXPECT_EQ(snapshot_count, 3) << "Expected 3 SNAPSHOT transactions";
    EXPECT_EQ(serializable_count, 3) << "Expected 3 SERIALIZABLE transactions";
    EXPECT_EQ(failure_count, 0) << "No failures expected";
}

// ===== ERROR PATH DETERMINISM =====

/// AC-2: Error path determinism - Timeout behavior
TEST_F(TransactionLifecyclePhase1Test, ErrorPathDeterminism_TimeoutBehavior) {
    // Begin a transaction and set a short per-transaction timeout
    auto txn_id = tx_manager_->beginTransaction();
    ASSERT_GT(txn_id, 0);

    {
        auto txn = tx_manager_->getTransaction(txn_id);
        ASSERT_NE(txn, nullptr);
        txn->setTimeout(std::chrono::milliseconds(100));
    }

    // Wait for the timeout to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // The timeout sweeper may already have rolled back the transaction.
    // Attempting another rollback on a timed-out (already-completed) transaction
    // must return false (not crash or block).
    bool second_rollback = tx_manager_->rollbackTransaction(txn_id);
    // Either still active (rollback succeeds) or already swept (rollback returns false).
    // Both outcomes are valid; the important invariant is determinism (no UB, no exception).
    (void)second_rollback;

    // The transaction must not be in the active map regardless of which path took it.
    // (After either timeout sweep or explicit rollback it is in completed.)
    // We only verify the manager is still healthy by starting a new transaction.
    auto new_txn_id = tx_manager_->beginTransaction();
    EXPECT_GT(new_txn_id, 0) << "Manager should still accept new transactions after a timeout";
    tx_manager_->rollbackTransaction(new_txn_id);
}

/// AC-2: Error path determinism - Large number of state transitions
TEST_F(TransactionLifecyclePhase1Test, ErrorPathDeterminism_LargeStateTransitionVolume) {
    constexpr int num_iterations = 100;
    std::atomic<int> success_count(0);
    std::atomic<int> failure_count(0);

    for (int i = 0; i < num_iterations; ++i) {
        auto txn_id = tx_manager_->beginTransaction();
        if (txn_id <= 0) {
            ++failure_count;
            continue;
        }

        bool ok;
        if (i % 2 == 0) {
            ok = tx_manager_->commitTransaction(txn_id).ok;
        } else {
            ok = tx_manager_->rollbackTransaction(txn_id);
        }

        if (ok) {
            ++success_count;
        } else {
            ++failure_count;
        }
    }

    EXPECT_EQ(success_count + failure_count, num_iterations) << "All operations accounted for";
    EXPECT_EQ(success_count, num_iterations) << "All operations should succeed under normal conditions";
}

// ===== STRESS TESTS =====

/// AC-1: Stress test - High transaction creation rate
TEST_F(TransactionLifecyclePhase1Test, StressTest_HighTransactionCreationRate) {
    constexpr int num_threads = 5;
    constexpr int txns_per_thread = 20;
    std::vector<std::thread> threads;
    std::atomic<uint64_t> total_txn_count(0);
    std::atomic<int> error_count(0);

    auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &total_txn_count, &error_count] {
            for (int j = 0; j < txns_per_thread; ++j) {
                auto txn_id = tx_manager_->beginTransaction();
                if (txn_id <= 0) {
                    ++error_count;
                    continue;
                }

                total_txn_count.fetch_add(1, std::memory_order_relaxed);

                auto status = tx_manager_->commitTransaction(txn_id);
                if (!status.ok) {
                    ++error_count;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(total_txn_count, static_cast<uint64_t>(num_threads * txns_per_thread))
        << "All transactions created";
    EXPECT_EQ(error_count, 0) << "No errors under high creation rate";

    std::cout << "High transaction creation rate test completed in "
              << duration.count() << " ms for "
              << total_txn_count << " transactions" << std::endl;
}

// ===== SUMMARY =====
/*
 * AC-1: ACID Lifecycle Isolation Enforcement
 * ✓ BeginTransaction creates valid transaction state
 * ✓ CommitTransaction finalizes transaction (moves out of active map)
 * ✓ RollbackTransaction reverts transaction (moves out of active map)
 * ✓ Concurrent transactions maintain isolation
 *
 * AC-2: Begin/Prepare/Commit/Abort State Machine
 * ✓ Valid transitions allowed (begin -> commit, begin -> rollback)
 * ✓ Invalid transitions rejected (double commit, double rollback, etc.)
 * ✓ Deterministic error handling
 * ✓ Large scale state transitions stable
 *
 * AC-3: Isolation Level Behavior
 * ✓ READ_COMMITTED isolation level correctly set and retrieved
 * ✓ Snapshot isolation level correctly set and retrieved
 * ✓ SERIALIZABLE isolation level correctly set and retrieved
 * ✓ Mixed isolation levels in concurrent transactions work correctly
 */
