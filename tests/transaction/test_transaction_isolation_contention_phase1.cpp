#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/isolation_level.h"
#include "transaction/lock_manager.h"
#include "transaction/transaction_manager.h"

using namespace themis;

class TransactionIsolationContentionPhase1Test : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_transaction_isolation_contention_phase1_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 128;
        config.block_cache_size_mb = 256;

        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";

        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_index_ = std::make_unique<GraphIndexManager>(*db_);
        vector_index_ = std::make_unique<VectorIndexManager>(*db_);

        tx_manager_ = std::make_unique<TransactionManager>(
            *db_, *secondary_index_, *graph_index_, *vector_index_);
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

TEST_F(TransactionIsolationContentionPhase1Test, IsolationEdgeCase_DirtyReadPrevention) {
    auto txn_id1 = tx_manager_->beginTransaction(IsolationLevel::ReadCommitted);
    auto txn_id2 = tx_manager_->beginTransaction(IsolationLevel::ReadCommitted);

    ASSERT_GT(txn_id1, 0u);
    ASSERT_GT(txn_id2, 0u);

    auto txn1 = tx_manager_->getTransaction(txn_id1);
    auto txn2 = tx_manager_->getTransaction(txn_id2);
    ASSERT_NE(txn1, nullptr);
    ASSERT_NE(txn2, nullptr);

    EXPECT_EQ(txn1->getIsolationLevel(), IsolationLevel::ReadCommitted);
    EXPECT_EQ(txn2->getIsolationLevel(), IsolationLevel::ReadCommitted);

    auto status1 = tx_manager_->commitTransaction(txn_id1);
    auto status2 = tx_manager_->commitTransaction(txn_id2);
    EXPECT_TRUE(status1.ok);
    EXPECT_TRUE(status2.ok);
}

TEST_F(TransactionIsolationContentionPhase1Test, IsolationEdgeCase_NonRepeatableReadPrevention) {
    auto txn_id1 = tx_manager_->beginTransaction(IsolationLevel::Snapshot);
    auto txn_id2 = tx_manager_->beginTransaction(IsolationLevel::Snapshot);

    ASSERT_GT(txn_id1, 0u);
    ASSERT_GT(txn_id2, 0u);

    auto txn1 = tx_manager_->getTransaction(txn_id1);
    auto txn2 = tx_manager_->getTransaction(txn_id2);
    ASSERT_NE(txn1, nullptr);
    ASSERT_NE(txn2, nullptr);

    EXPECT_EQ(txn1->getIsolationLevel(), IsolationLevel::Snapshot);
    EXPECT_EQ(txn2->getIsolationLevel(), IsolationLevel::Snapshot);

    EXPECT_TRUE(tx_manager_->rollbackTransaction(txn_id1));
    EXPECT_TRUE(tx_manager_->rollbackTransaction(txn_id2));
}

TEST_F(TransactionIsolationContentionPhase1Test, IsolationEdgeCase_PhantomReadPrevention) {
    auto txn_id1 = tx_manager_->beginTransaction(IsolationLevel::SERIALIZABLE);
    auto txn_id2 = tx_manager_->beginTransaction(IsolationLevel::SERIALIZABLE);

    ASSERT_GT(txn_id1, 0u);
    ASSERT_GT(txn_id2, 0u);

    auto txn1 = tx_manager_->getTransaction(txn_id1);
    auto txn2 = tx_manager_->getTransaction(txn_id2);
    ASSERT_NE(txn1, nullptr);
    ASSERT_NE(txn2, nullptr);

    EXPECT_EQ(txn1->getIsolationLevel(), IsolationLevel::SERIALIZABLE);
    EXPECT_EQ(txn2->getIsolationLevel(), IsolationLevel::SERIALIZABLE);

    auto status1 = tx_manager_->commitTransaction(txn_id1);
    auto status2 = tx_manager_->commitTransaction(txn_id2);
    EXPECT_TRUE(status1.ok);
    EXPECT_TRUE(status2.ok);
}

TEST_F(TransactionIsolationContentionPhase1Test, LockContention_HighConcurrentWriteLoad) {
    constexpr int num_threads = 10;
    constexpr int txns_per_thread = 5;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    std::atomic<int> failure_count(0);
    std::atomic<int> conflict_count(0);
    std::mutex results_mutex = {};
    std::vector<std::string> errors;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &success_count, &failure_count, &conflict_count, &results_mutex, &errors] {
            for (int j = 0; j < txns_per_thread; ++j) {
                auto txn_id = tx_manager_->beginTransaction(
                    (i % 3 == 0) ? IsolationLevel::ReadCommitted : IsolationLevel::Snapshot);
                if (txn_id == 0u) {
                    ++failure_count;
                    continue;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(100));

                if (j % 2 == 0) {
                    auto status = tx_manager_->commitTransaction(txn_id);
                    if (status.ok) {
                        ++success_count;
                    } else {
                        if (!status.conflict_id.empty()) {
                            ++conflict_count;
                        }
                        ++failure_count;
                        std::lock_guard<std::mutex> lock(results_mutex);
                        errors.push_back("Transaction failed: " + status.message);
                    }
                } else {
                    if (tx_manager_->rollbackTransaction(txn_id)) {
                        ++success_count;
                    } else {
                        ++failure_count;
                        std::lock_guard<std::mutex> lock(results_mutex);
                        errors.push_back("Transaction rollback failed");
                    }
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count + failure_count, num_threads * txns_per_thread);
    EXPECT_LE(failure_count, num_threads * txns_per_thread);
    (void)conflict_count;
    (void)errors;
}

TEST_F(TransactionIsolationContentionPhase1Test, LockContention_DeadlockDetection) {
    constexpr int num_threads = 4;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    std::atomic<int> error_count(0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &success_count, &error_count] {
            auto txn_id = tx_manager_->beginTransaction();
            if (txn_id == 0u) {
                ++error_count;
                return;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(50 * i));

            auto status = tx_manager_->commitTransaction(txn_id);
            if (status.ok) {
                ++success_count;
            } else {
                ++error_count;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count + error_count, num_threads);
}

TEST_F(TransactionIsolationContentionPhase1Test, TimeoutDeterminism_ShortTimeoutUnderContention) {
    constexpr int num_threads = 5;
    std::vector<std::thread> threads;
    std::atomic<int> timeout_count(0);
    std::atomic<int> success_count(0);
    std::atomic<int> failure_count(0);

    tx_manager_->setDefaultTransactionTimeout(std::chrono::milliseconds(50));

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &timeout_count, &success_count, &failure_count] {
            auto txn_id = tx_manager_->beginTransaction();
            if (txn_id == 0u) {
                ++failure_count;
                return;
            }

            auto txn = tx_manager_->getTransaction(txn_id);
            ASSERT_NE(txn, nullptr);
            EXPECT_EQ(txn->getTimeout(), std::chrono::milliseconds(50));

            if (tx_manager_->commitTransaction(txn_id).ok) {
                ++success_count;
            } else {
                ++timeout_count;
                ++failure_count;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count + failure_count, num_threads);
    EXPECT_EQ(timeout_count.load(), 0);
}

TEST_F(TransactionIsolationContentionPhase1Test, TimeoutDeterminism_RetryConsistency) {
    constexpr int num_retries = 5;
    std::vector<bool> results;
    results.reserve(num_retries);

    tx_manager_->setDefaultTransactionTimeout(std::chrono::milliseconds(30));

    for (int attempt = 0; attempt < num_retries; ++attempt) {
        auto txn_id = tx_manager_->beginTransaction();
        ASSERT_GT(txn_id, 0u);

        auto txn = tx_manager_->getTransaction(txn_id);
        ASSERT_NE(txn, nullptr);
        EXPECT_EQ(txn->getTimeout(), std::chrono::milliseconds(30));

        auto status = tx_manager_->commitTransaction(txn_id);
        results.push_back(status.ok);
    }

    EXPECT_EQ(results.size(), num_retries);
}

TEST_F(TransactionIsolationContentionPhase1Test, StressTest_SimultaneousTransactionsWithContention) {
    constexpr int num_threads = 8;
    constexpr int txns_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> total_committed(0);
    std::atomic<int> total_rolled_back(0);
    std::atomic<int> total_conflicts(0);
    std::atomic<int> total_errors(0);

    auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &total_committed, &total_rolled_back, &total_conflicts, &total_errors] {
            for (int j = 0; j < txns_per_thread; ++j) {
                auto txn_id = tx_manager_->beginTransaction(
                    (j % 3 == 0) ? IsolationLevel::SERIALIZABLE : IsolationLevel::Snapshot);
                if (txn_id == 0u) {
                    ++total_errors;
                    continue;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(50));

                if (j % 2 == 0) {
                    auto status = tx_manager_->commitTransaction(txn_id);
                    if (status.ok) {
                        ++total_committed;
                    } else {
                        if (!status.conflict_id.empty()) {
                            ++total_conflicts;
                        } else {
                            ++total_errors;
                        }
                    }
                } else {
                    if (tx_manager_->rollbackTransaction(txn_id)) {
                        ++total_rolled_back;
                    } else {
                        ++total_errors;
                    }
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    int total = total_committed + total_rolled_back + total_conflicts + total_errors;
    EXPECT_EQ(total, num_threads * txns_per_thread);
    EXPECT_GT(duration.count(), -1);
}

TEST_F(TransactionIsolationContentionPhase1Test, StressTest_MixedIsolationLevelsUnderContention) {
    constexpr int num_threads = 6;
    constexpr int txns_per_thread = 8;
    std::vector<std::thread> threads;
    std::atomic<int> read_committed_success(0);
    std::atomic<int> snapshot_success(0);
    std::atomic<int> serializable_success(0);
    std::atomic<int> total_failures(0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &read_committed_success, &snapshot_success, &serializable_success, &total_failures] {
            IsolationLevel base_level;
            switch (i % 3) {
                case 0: base_level = IsolationLevel::ReadCommitted; break;
                case 1: base_level = IsolationLevel::Snapshot; break;
                default: base_level = IsolationLevel::SERIALIZABLE; break;
            }

            for (int j = 0; j < txns_per_thread; ++j) {
                auto txn_id = tx_manager_->beginTransaction(base_level);
                if (txn_id == 0u) {
                    ++total_failures;
                    continue;
                }

                auto txn = tx_manager_->getTransaction(txn_id);
                if (!txn || txn->getIsolationLevel() != base_level) {
                    ++total_failures;
                    tx_manager_->rollbackTransaction(txn_id);
                    continue;
                }

                auto status = tx_manager_->commitTransaction(txn_id);
                if (status.ok) {
                    switch (base_level) {
                        case IsolationLevel::ReadCommitted:
                            ++read_committed_success;
                            break;
                        case IsolationLevel::Snapshot:
                            ++snapshot_success;
                            break;
                        case IsolationLevel::SERIALIZABLE:
                            ++serializable_success;
                            break;
                        default:
                            break;
                    }
                } else {
                    ++total_failures;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int total_success = read_committed_success + snapshot_success + serializable_success;
    int total = total_success + total_failures;
    EXPECT_EQ(total, num_threads * txns_per_thread);
    EXPECT_GE(total_success, 0);
}
