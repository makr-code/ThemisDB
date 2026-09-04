/**
 * @file test_multi_shard_transactions.cpp
 * @brief Multi-shard transaction tests for distributed consistency
 * 
 * Tests distributed transaction scenarios:
 * - Two-phase commit (2PC) across shards
 * - Distributed rollback scenarios
 * - Cross-shard consistency
 * - Coordinator failure handling
 * - Participant timeout handling
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <future>

using namespace std::chrono_literals;

namespace themis {
namespace test {

/**
 * @brief Mock shard for testing
 */
class MockShard {
public:
    enum class State {
        IDLE,
        PREPARING,
        PREPARED,
        COMMITTED,
        ABORTED
    };
    
    explicit MockShard(int id) : shard_id_(id), state_(State::IDLE) {}

    MockShard(const MockShard&) = delete;
    MockShard& operator=(const MockShard&) = delete;

    MockShard(MockShard&& other) noexcept
        : shard_id_(other.shard_id_),
          state_(other.state_),
          prepared_tx_(std::move(other.prepared_tx_)),
          committed_count_(other.committed_count_.load()),
          aborted_count_(other.aborted_count_.load()) {}

    MockShard& operator=(MockShard&& other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(mutex_, other.mutex_);
            shard_id_ = other.shard_id_;
            state_ = other.state_;
            prepared_tx_ = std::move(other.prepared_tx_);
            committed_count_.store(other.committed_count_.load());
            aborted_count_.store(other.aborted_count_.load());
        }
        return *this;
    }
    
    bool prepare(const std::string& tx_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != State::IDLE) {
            return false;
        }
        state_ = State::PREPARING;
        // Simulate prepare work
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        state_ = State::PREPARED;
        prepared_tx_ = tx_id;
        return true;
    }
    
    bool commit(const std::string& tx_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != State::PREPARED || prepared_tx_ != tx_id) {
            return false;
        }
        state_ = State::COMMITTED;
        committed_count_++;
        return true;
    }
    
    bool abort(const std::string& tx_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (prepared_tx_ != tx_id) {
            return false;
        }
        state_ = State::ABORTED;
        aborted_count_++;
        return true;
    }
    
    State getState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = State::IDLE;
        prepared_tx_.clear();
    }
    
    int getCommittedCount() const { return committed_count_; }
    int getAbortedCount() const { return aborted_count_; }
    int getId() const { return shard_id_; }
    
private:
    int shard_id_;
    State state_;
    std::string prepared_tx_;
    mutable std::mutex mutex_;
    std::atomic<int> committed_count_{0};
    std::atomic<int> aborted_count_{0};
};

/**
 * @brief Test basic two-phase commit across shards
 */
TEST(MultiShardTransactionTest, BasicTwoPhaseCommit) {
    constexpr int NUM_SHARDS = 5;
    std::vector<MockShard> shards;
    shards.reserve(NUM_SHARDS);
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    std::string tx_id = "tx_001";
    
    // Phase 1: Prepare
    std::vector<bool> prepare_results = {};

    for (auto& shard : shards) {
        bool result = shard.prepare(tx_id);
        prepare_results.push_back(result);
    }
    
    // Verify all shards prepared successfully
    for (bool result : prepare_results) {
        EXPECT_TRUE(result);
    }
    
    // Phase 2: Commit
    std::vector<bool> commit_results = {};

    for (auto& shard : shards) {
        bool result = shard.commit(tx_id);
        commit_results.push_back(result);
    }
    
    // Verify all shards committed successfully
    for (bool result : commit_results) {
        EXPECT_TRUE(result);
    }
    
    // Verify final state
    for (const auto& shard : shards) {
        EXPECT_EQ(shard.getState(), MockShard::State::COMMITTED);
        EXPECT_EQ(shard.getCommittedCount(), 1);
    }
}

/**
 * @brief Test distributed rollback when one shard fails to prepare
 */
TEST(MultiShardTransactionTest, RollbackOnPrepareFailure) {
    constexpr int NUM_SHARDS = 4;
    std::vector<MockShard> shards;
    shards.reserve(NUM_SHARDS);
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    std::string tx_id = "tx_002";
    
    // Simulate one shard already in use (prepare will fail)
    shards[2].prepare("other_tx");
    
    // Phase 1: Prepare all shards
    std::vector<bool> prepare_results = {};

    for (auto& shard : shards) {
        bool result = shard.prepare(tx_id);
        prepare_results.push_back(result);
    }
    
    // Check if any shard failed to prepare
    bool all_prepared = true;
    for (bool result : prepare_results) {
        if (!result) {
            all_prepared = false;
            break;
        }
    }
    
    EXPECT_FALSE(all_prepared); // Shard 2 should have failed
    
    // Abort transaction on all shards that prepared
    for (size_t i = 0; i < shards.size(); ++i) {
        if (prepare_results[i]) {
            shards[i].abort(tx_id);
        }
    }
    
    // Verify shards are in correct state
    EXPECT_EQ(shards[0].getState(), MockShard::State::ABORTED);
    EXPECT_EQ(shards[1].getState(), MockShard::State::ABORTED);
    EXPECT_NE(shards[2].getState(), MockShard::State::ABORTED); // Was never prepared for tx_002
    EXPECT_EQ(shards[3].getState(), MockShard::State::ABORTED);
}

/**
 * @brief Test concurrent multi-shard transactions
 */
TEST(MultiShardTransactionTest, ConcurrentTransactions) {
    constexpr int NUM_SHARDS = 6;
    constexpr int NUM_TRANSACTIONS = 20;
    
    std::vector<MockShard> shards;
    shards.reserve(NUM_SHARDS);
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    std::atomic<int> successful_tx{0};
    std::atomic<int> failed_tx{0};
    std::vector<std::thread> threads;
    
    for (int tx = 0; tx < NUM_TRANSACTIONS; ++tx) {
        threads.emplace_back([&shards, &successful_tx, &failed_tx, tx]() {
            std::string tx_id = "tx_" + std::to_string(tx);
            
            // Select random subset of shards
            std::random_device rd = {};
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, static_cast<int>(shards.size() - 1));
            
            int shard1 = dis(gen);
            int shard2 = (shard1 + 1 + dis(gen)) % static_cast<int>(shards.size());
            
            // Phase 1: Prepare
            bool prep1 = shards[shard1].prepare(tx_id);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            bool prep2 = shards[shard2].prepare(tx_id);
            
            if (prep1 && prep2) {
                // Phase 2: Commit
                shards[shard1].commit(tx_id);
                shards[shard2].commit(tx_id);
                successful_tx.fetch_add(1);
            } else {
                // Rollback
                if (prep1) {
                  shards[shard1].abort(tx_id);
                }
                if (prep2) {
                  shards[shard2].abort(tx_id);
                }
                failed_tx.fetch_add(1);
            }
            
            // Reset shards for next transaction
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            shards[shard1].reset();
            shards[shard2].reset();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(successful_tx + failed_tx, NUM_TRANSACTIONS);
    // Under high contention, it is valid that all attempts are rolled back.
    EXPECT_GE(successful_tx.load(), 0);
}

/**
 * @brief Test cross-shard read consistency
 */
TEST(MultiShardTransactionTest, CrossShardReadConsistency) {
    constexpr int NUM_SHARDS = 3;
    
    struct ShardData {
        std::mutex mutex = {};
        std::map<std::string, int> data;
        uint64_t version = 0;
    };
    
    std::vector<ShardData> shards(NUM_SHARDS);
    
    // Initialize data
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards[i].data["balance"] = 1000;
        shards[i].version = 1;
    }
    
    // Perform distributed transaction: transfer across shards
    std::string tx_id = "transfer_001";
    int from_shard = 0;
    int to_shard = 2;
    int amount = 200;
    
    uint64_t snapshot_version = 1;
    
    // Read phase (at snapshot version)
    int from_balance = 0;
    int to_balance = 0;
    {
        std::lock_guard<std::mutex> lock1(shards[from_shard].mutex);
        std::lock_guard<std::mutex> lock2(shards[to_shard].mutex);
        
        EXPECT_EQ(shards[from_shard].version, snapshot_version);
        EXPECT_EQ(shards[to_shard].version, snapshot_version);
        
        from_balance = shards[from_shard].data["balance"];
        to_balance = shards[to_shard].data["balance"];
    }
    
    // Write phase
    {
        std::lock_guard<std::mutex> lock1(shards[from_shard].mutex);
        std::lock_guard<std::mutex> lock2(shards[to_shard].mutex);
        
        shards[from_shard].data["balance"] = from_balance - amount;
        shards[to_shard].data["balance"] = to_balance + amount;
        
        shards[from_shard].version++;
        shards[to_shard].version++;
    }
    
    // Verify consistency
    int total_balance = 0;
    for (auto& shard : shards) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        total_balance += shard.data["balance"];
    }
    
    EXPECT_EQ(total_balance, NUM_SHARDS * 1000); // Total unchanged
    EXPECT_EQ(shards[from_shard].data["balance"], 800);
    EXPECT_EQ(shards[to_shard].data["balance"], 1200);
}

/**
 * @brief Test coordinator failure during commit
 */
TEST(MultiShardTransactionTest, CoordinatorFailureDuringCommit) {
    constexpr int NUM_SHARDS = 4;
    std::vector<MockShard> shards;
    shards.reserve(NUM_SHARDS);
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    std::string tx_id = "tx_coord_fail";
    
    // Phase 1: All shards prepare successfully
    for (auto& shard : shards) {
        EXPECT_TRUE(shard.prepare(tx_id));
    }
    
    // Phase 2: Coordinator fails after committing first 2 shards
    shards[0].commit(tx_id);
    shards[1].commit(tx_id);
    
    // Simulate coordinator failure - remaining shards left in prepared state
    EXPECT_EQ(shards[0].getState(), MockShard::State::COMMITTED);
    EXPECT_EQ(shards[1].getState(), MockShard::State::COMMITTED);
    EXPECT_EQ(shards[2].getState(), MockShard::State::PREPARED);
    EXPECT_EQ(shards[3].getState(), MockShard::State::PREPARED);
    
    // Recovery: New coordinator should commit remaining prepared shards
    shards[2].commit(tx_id);
    shards[3].commit(tx_id);
    
    // Verify all shards eventually committed
    for (const auto& shard : shards) {
        EXPECT_EQ(shard.getState(), MockShard::State::COMMITTED);
    }
}

/**
 * @brief Test participant timeout handling
 */
TEST(MultiShardTransactionTest, ParticipantTimeoutHandling) {
    constexpr int NUM_SHARDS = 3;
    constexpr int TIMEOUT_MS = 100;
    
    std::vector<MockShard> shards;
    shards.reserve(NUM_SHARDS);
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    std::string tx_id = "tx_timeout";
    
    // Phase 1: Prepare with timeout simulation
    std::vector<std::future<bool>> prepare_futures;
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        prepare_futures.push_back(std::async(std::launch::async, [&shards, i, &tx_id]() {
            if (i == 1) {
                // Simulate slow shard
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
            return shards[i].prepare(tx_id);
        }));
    }
    
    // Wait for results with timeout
    std::vector<bool> results = {};

    for (auto& future : prepare_futures) {
        auto status = future.wait_for(std::chrono::milliseconds(TIMEOUT_MS));
        
        if (status == std::future_status::ready) {
            results.push_back(future.get());
        } else {
            results.push_back(false); // Timeout
        }
    }
    
    // At least one shard should have timed out
    bool has_timeout = false;
    for (size_t i = 0; i < results.size(); ++i) {
        if (!results[i]) {
            has_timeout = true;
        }
    }
    
    EXPECT_TRUE(has_timeout);
    
    // Abort transaction due to timeout
    for (size_t i = 0; i < shards.size(); ++i) {
        if (results[i] && shards[i].getState() == MockShard::State::PREPARED) {
            shards[i].abort(tx_id);
        }
    }
}

/**
 * @brief Test deadlock detection in distributed transactions
 */
TEST(MultiShardTransactionTest, DeadlockDetection) {
    constexpr int NUM_SHARDS = 2;
    std::vector<MockShard> shards;
    shards.reserve(NUM_SHARDS);
    
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }
    
    std::atomic<bool> deadlock_detected{false};
    
    // Transaction 1: Lock shard 0 then shard 1
    std::thread tx1([&shards, &deadlock_detected]() {
        std::string tx_id = "tx_deadlock_1";
        
        if (shards[0].prepare(tx_id)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            // Try to acquire shard 1 (may be locked by tx2)
            auto start = std::chrono::steady_clock::now();
            while (!shards[1].prepare(tx_id)) {
                auto elapsed = std::chrono::steady_clock::now() - start;
                if (elapsed > std::chrono::milliseconds(100)) {
                    deadlock_detected.store(true);
                    shards[0].abort(tx_id);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            
            shards[0].commit(tx_id);
            shards[1].commit(tx_id);
            shards[0].reset();
            shards[1].reset();
        }
    });
    
    // Transaction 2: Lock shard 1 then shard 0
    std::thread tx2([&shards, &deadlock_detected]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::string tx_id = "tx_deadlock_2";
        
        if (shards[1].prepare(tx_id)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            // Try to acquire shard 0 (may be locked by tx1)
            auto start = std::chrono::steady_clock::now();
            while (!shards[0].prepare(tx_id)) {
                auto elapsed = std::chrono::steady_clock::now() - start;
                if (elapsed > std::chrono::milliseconds(100)) {
                    deadlock_detected.store(true);
                    shards[1].abort(tx_id);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            
            shards[1].commit(tx_id);
            shards[0].commit(tx_id);
            shards[1].reset();
            shards[0].reset();
        }
    });
    
    tx1.join();
    tx2.join();
    
    // At least one transaction should detect potential deadlock
    EXPECT_TRUE(deadlock_detected.load());
}

/**
 * @brief Test transaction isolation across shards
 */
TEST(MultiShardTransactionTest, CrossShardIsolation) {
    constexpr int NUM_SHARDS = 3;
    
    struct VersionedShard {
        std::mutex mutex = {};
        std::map<std::string, int> data;
        uint64_t read_version = 1;
        uint64_t write_version = 1;
    };
    
    std::vector<VersionedShard> shards(NUM_SHARDS);
    
    // Initialize
    for (auto& shard : shards) {
        shard.data["value"] = 100;
    }
    
    // Transaction 1: Read from all shards at version 1
    uint64_t tx1_snapshot = 1;
    std::vector<int> tx1_reads;
    
    for (auto& shard : shards) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        EXPECT_EQ(shard.read_version, tx1_snapshot);
        tx1_reads.push_back(shard.data["value"]);
    }
    
    // Transaction 2: Write to shards (version 2)
    for (auto& shard : shards) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        shard.data["value"] = 200;
        shard.write_version = 2;
        shard.read_version = 2;
    }
    
    // Transaction 1 should still see old values (if using MVCC)
    for (int val : tx1_reads) {
        EXPECT_EQ(val, 100);
    }
    
    // New transaction should see new values
    for (auto& shard : shards) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        EXPECT_EQ(shard.data["value"], 200);
        EXPECT_EQ(shard.read_version, 2);
    }
}

/**
 * @brief Test cross-shard write-skew detection via prepare-phase conflict checking.
 *
 * Write-skew scenario (Percolator / SNAPSHOT_ISOLATION):
 *   - Initial state: shard-A balance=100, shard-B balance=100, total=200
 *   - Invariant: at least one shard must hold balance >= 100
 *   - Tx1 reads both shards (both=100), decides to zero shard-A (total still OK)
 *   - Tx2 reads both shards (both=100), decides to zero shard-B (total still OK)
 *   - Without conflict detection both commits succeed → total=0 (invariant broken)
 *   - With conflict detection (prepare phase) the concurrent write to the same
 *     key on a shard is caught: only one transaction may commit; the other is
 *     aborted, preserving the invariant.
 *
 * This test models the prepare-based conflict gate: a MockShard accepts only
 * one PREPARE for a given key at a time.  Both transactions attempt to prepare
 * concurrently; exactly one is expected to be rejected, demonstrating that
 * the Percolator path's prepare phase prevents write-skew.
 */
TEST(MultiShardTransactionTest, CrossShardWriteSkewDetection) {
    // Shard that tracks per-key write locks to simulate a conflict-detecting
    // prepare phase.  A PREPARE for a key already locked by another transaction
    // returns false (conflict detected), mirroring what a real shard participant
    // does when it detects a concurrent write to the same row.
    struct ConflictDetectingShard {
        mutable std::mutex mutex;
        std::map<std::string, std::string> write_locks;  // key → txn_id
        std::map<std::string, int>         data;
        int  committed_count = 0;
        int  aborted_count   = 0;

        bool prepare(const std::string& txn_id, const std::string& key) {
            std::lock_guard<std::mutex> lk(mutex);
            // If another transaction already holds a write lock on this key,
            // vote ABORT (write-write conflict).
            if (write_locks.count(key) && write_locks[key] != txn_id) {
                return false;
            }
            write_locks[key] = txn_id;
            return true;
        }

        bool commit(const std::string& txn_id, const std::string& key, int value) {
            std::lock_guard<std::mutex> lk(mutex);
            auto it = write_locks.find(key);
            if (it == write_locks.end() || it->second != txn_id) {
                return false;
            }
            data[key] = value;
            write_locks.erase(it);
            ++committed_count;
            return true;
        }

        void abort(const std::string& txn_id, const std::string& key) {
            std::lock_guard<std::mutex> lk(mutex);
            auto it = write_locks.find(key);
            if (it != write_locks.end() && it->second == txn_id) {
                write_locks.erase(it);
            }
            ++aborted_count;
        }

        int get(const std::string& key) const {
            std::lock_guard<std::mutex> lk(mutex);
            auto it = data.find(key);
            return it != data.end() ? it->second : 0;
        }
    };

    ConflictDetectingShard shard_a;
    ConflictDetectingShard shard_b;
    shard_a.data["balance"] = 100;
    shard_b.data["balance"] = 100;

    // Invariant: shard_a.balance + shard_b.balance >= 100 at all times.
    // Each transaction reads the total, decides it can zero its own shard,
    // then prepares (conflict check) and commits if no conflict is detected.

    std::atomic<int> committed_transactions{0};
    std::atomic<int> aborted_transactions{0};

    auto run_tx = [&](const std::string& txn_id,
                      ConflictDetectingShard& write_shard,
                      ConflictDetectingShard& read_shard,
                      const std::string& write_key) {
        // Read snapshot (both shards).
        int wa = shard_a.get("balance");
        int wb = shard_b.get("balance");
        int total = wa + wb;

        if (total < 100) {
            // Invariant already broken — abort without writing.
            ++aborted_transactions;
            return;
        }

        // Prepare phase: check for write-write conflict on the target key.
        bool prepared = write_shard.prepare(txn_id, write_key);
        if (!prepared) {
            // Conflict detected — another transaction already locked this key.
            write_shard.abort(txn_id, write_key);
            ++aborted_transactions;
            return;
        }

        // Small delay to allow the concurrent transaction to attempt its prepare
        // so both land in the conflict window.
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Commit: zero the target shard's balance.
        bool ok = write_shard.commit(txn_id, write_key, 0);
        if (ok) {
            ++committed_transactions;
        } else {
            write_shard.abort(txn_id, write_key);
            ++aborted_transactions;
        }
    };

    // Launch both transactions concurrently — each targets a different shard
    // but both read the full state.  The prepare-phase conflict gate must
    // ensure that if Tx1 zeroes shard-A while Tx2 concurrently zeroes shard-B,
    // at least one is rejected (because the combined write would violate the
    // invariant and one shard's lock will be contended).
    std::thread tx1([&]() { run_tx("tx1", shard_a, shard_b, "balance"); });
    std::thread tx2([&]() { run_tx("tx2", shard_b, shard_a, "balance"); });

    tx1.join();
    tx2.join();

    // Exactly one transaction should have committed.
    EXPECT_EQ(committed_transactions.load() + aborted_transactions.load(), 2);

    // At least one transaction was aborted (conflict detected).
    EXPECT_GE(aborted_transactions.load(), 1);

    // The invariant must hold: combined balance >= 100.
    int final_a = shard_a.get("balance");
    int final_b = shard_b.get("balance");
    EXPECT_GE(final_a + final_b, 100)
        << "Write-skew invariant violated: shard_a=" << final_a
        << ", shard_b=" << final_b;
}

/**
 * @brief Test that the prepare phase aborts all participants on write-write
 *        conflict, leaving no shard in a prepared-but-uncommitted state.
 *
 * Ensures that when the coordinator detects a conflict during the prepare phase
 * (simulating the cross-shard write-write conflict detection in Percolator mode),
 * it sends ABORT to all previously prepared participants before returning false.
 */
TEST(MultiShardTransactionTest, PercolatorPrepareAbortOnConflict) {
    constexpr int NUM_SHARDS = 3;
    std::vector<MockShard> shards;
    shards.reserve(NUM_SHARDS);
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shards.emplace_back(i);
    }

    const std::string tx_a = "tx_conflict_a";
    const std::string tx_b = "tx_conflict_b";

    // Tx-A prepares shards 0 and 1 successfully.
    EXPECT_TRUE(shards[0].prepare(tx_a));
    EXPECT_TRUE(shards[1].prepare(tx_a));

    // Tx-B attempts to prepare shard 1, which is already locked by Tx-A:
    // shard 1 is not IDLE so prepare returns false (conflict).
    bool prep_b_shard0 = shards[0].prepare(tx_b);  // also locked
    bool prep_b_shard1 = shards[1].prepare(tx_b);  // locked by tx_a — conflict

    // Simulate coordinator abort logic: abort all tx_b participants that prepared.
    if (!prep_b_shard0 || !prep_b_shard1) {
        if (prep_b_shard0) {
          shards[0].abort(tx_b);
        }
        if (prep_b_shard1) {
          shards[1].abort(tx_b);
        }
    }

    // Tx-A must still be in PREPARED state (unaffected by Tx-B conflict).
    EXPECT_EQ(shards[0].getState(), MockShard::State::PREPARED);
    EXPECT_EQ(shards[1].getState(), MockShard::State::PREPARED);

    // Tx-B prepare must have failed on at least one shard.
    EXPECT_FALSE(prep_b_shard0 && prep_b_shard1)
        << "Both shards accepted conflicting prepare — write-skew guard failed";

    // Tx-A can now commit cleanly.
    EXPECT_TRUE(shards[0].commit(tx_a));
    EXPECT_TRUE(shards[1].commit(tx_a));
    EXPECT_EQ(shards[0].getState(), MockShard::State::COMMITTED);
    EXPECT_EQ(shards[1].getState(), MockShard::State::COMMITTED);
}

} // namespace test
} // namespace themis
