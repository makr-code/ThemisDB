#include <gtest/gtest.h>
#include "storage/distributed_transaction_manager.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace themis::storage;

// ─────────────────────────────────────────────────────────────────────────────
// In-process mock shard participant for unit tests
// ─────────────────────────────────────────────────────────────────────────────

class MockShardParticipant : public IDistributedShardParticipant {
public:
    enum class PreparePolicy { ALWAYS_COMMIT, ALWAYS_ABORT, THROW };

    explicit MockShardParticipant(PreparePolicy policy = PreparePolicy::ALWAYS_COMMIT)
        : policy_(policy) {}

    bool prepare(const std::string& txn_id,
                 const std::vector<DistributedOperation>& ops) override {
        if (policy_ == PreparePolicy::THROW) {
            throw std::runtime_error("mock prepare failure");
        }
        std::lock_guard<std::mutex> lk(mu_);
        last_prepare_txn_ = txn_id;
        prepared_ops_[txn_id] = ops;
        ++prepare_count_;
        return policy_ == PreparePolicy::ALWAYS_COMMIT;
    }

    void commit(const std::string& txn_id) override {
        std::lock_guard<std::mutex> lk(mu_);
        last_commit_txn_ = txn_id;
        // Apply buffered ops to storage
        auto it = prepared_ops_.find(txn_id);
        if (it != prepared_ops_.end()) {
            for (auto& op : it->second) {
                if (op.type == DistributedOperation::Type::PUT) {
                    store_[op.key] = op.value;
                } else {
                    store_.erase(op.key);
                }
            }
            prepared_ops_.erase(it);
        }
        ++commit_count_;
    }

    void abort(const std::string& txn_id) override {
        std::lock_guard<std::mutex> lk(mu_);
        last_abort_txn_ = txn_id;
        prepared_ops_.erase(txn_id);
        ++abort_count_;
    }

    std::optional<std::string> get(const std::string& key) override {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it != store_.end()) {
          return it->second;
        }
        return std::nullopt;
    }

    // Test helpers
    int prepareCount() const { return prepare_count_.load(); }
    int commitCount()  const { return commit_count_.load(); }
    int abortCount()   const { return abort_count_.load(); }

    std::string lastPrepareTxn() const {
        std::lock_guard<std::mutex> lk(mu_);
        return last_prepare_txn_;
    }
    std::string lastCommitTxn() const {
        std::lock_guard<std::mutex> lk(mu_);
        return last_commit_txn_;
    }
    std::string lastAbortTxn() const {
        std::lock_guard<std::mutex> lk(mu_);
        return last_abort_txn_;
    }
    std::optional<std::string> stored(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it != store_.end()) {
          return it->second;
        }
        return std::nullopt;
    }

private:
    PreparePolicy policy_;
    mutable std::mutex mu_;
    std::string last_prepare_txn_;
    std::string last_commit_txn_;
    std::string last_abort_txn_;
    std::atomic<int> prepare_count_{0};
    std::atomic<int> commit_count_{0};
    std::atomic<int> abort_count_{0};
    std::map<std::string, std::vector<DistributedOperation>> prepared_ops_;
    std::map<std::string, std::string> store_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class DistributedTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        shard1 = std::make_unique<MockShardParticipant>();
        shard2 = std::make_unique<MockShardParticipant>();
        shard3 = std::make_unique<MockShardParticipant>();

        mgr = std::make_unique<DistributedTransactionManager>();
        mgr->registerShard("shard1", shard1.get());
        mgr->registerShard("shard2", shard2.get());
        mgr->registerShard("shard3", shard3.get());
    }

    std::unique_ptr<MockShardParticipant> shard1;
    std::unique_ptr<MockShardParticipant> shard2;
    std::unique_ptr<MockShardParticipant> shard3;
    std::unique_ptr<DistributedTransactionManager> mgr;
};

// ============================================================================
// Basic Transaction Tests
// ============================================================================

TEST_F(DistributedTransactionTest, BeginTransactionReturnsValidId) {
    auto tx = mgr->beginDistributedTransaction();
    ASSERT_NE(tx, nullptr);
    EXPECT_FALSE(tx->id().empty());
    // IDs should start with "dtx-"
    EXPECT_EQ(tx->id().substr(0, 4), "dtx-");
}

TEST_F(DistributedTransactionTest, TxnStateIsActiveAfterBegin) {
    auto tx = mgr->beginDistributedTransaction();
    EXPECT_EQ(tx->state(), DistributedTxnState::ACTIVE);
}

TEST_F(DistributedTransactionTest, SingleShardPutAndCommit) {
    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:user:1", "alice");
    EXPECT_EQ(tx->operationCount(), 1u);

    bool ok = tx->commit();
    EXPECT_TRUE(ok);
    EXPECT_EQ(tx->state(), DistributedTxnState::COMMITTED);

    EXPECT_EQ(shard1->prepareCount(), 1);
    EXPECT_EQ(shard1->commitCount(), 1);
    EXPECT_EQ(shard1->stored("user:1"), "alice");
}

TEST_F(DistributedTransactionTest, MultiShardAtomicCommit) {
    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:key1", "value1");
    tx->put("shard2:key2", "value2");
    tx->put("shard3:key3", "value3");

    bool ok = tx->commit();
    EXPECT_TRUE(ok);
    EXPECT_EQ(tx->state(), DistributedTxnState::COMMITTED);

    EXPECT_EQ(shard1->commitCount(), 1);
    EXPECT_EQ(shard2->commitCount(), 1);
    EXPECT_EQ(shard3->commitCount(), 1);

    EXPECT_EQ(shard1->stored("key1"), "value1");
    EXPECT_EQ(shard2->stored("key2"), "value2");
    EXPECT_EQ(shard3->stored("key3"), "value3");
}

TEST_F(DistributedTransactionTest, EmptyTransactionCommitsSuccessfully) {
    auto tx = mgr->beginDistributedTransaction();
    EXPECT_EQ(tx->operationCount(), 0u);
    bool ok = tx->commit();
    EXPECT_TRUE(ok);
    EXPECT_EQ(tx->state(), DistributedTxnState::COMMITTED);
}

// ============================================================================
// Two-Phase Commit Tests
// ============================================================================

TEST_F(DistributedTransactionTest, TwoPhaseCommitSuccessCallsPrepareAndCommit) {
    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:a", "1");
    tx->put("shard2:b", "2");

    bool ok = tx->commit();
    EXPECT_TRUE(ok);
    EXPECT_EQ(shard1->prepareCount(), 1);
    EXPECT_EQ(shard1->commitCount(), 1);
    EXPECT_EQ(shard1->abortCount(),  0);
    EXPECT_EQ(shard2->prepareCount(), 1);
    EXPECT_EQ(shard2->commitCount(), 1);
    EXPECT_EQ(shard2->abortCount(),  0);
}

TEST_F(DistributedTransactionTest, TwoPhaseCommitAbortWhenShardVotesNo) {
    // Replace shard2 with an always-aborting participant
    MockShardParticipant aborting_shard(MockShardParticipant::PreparePolicy::ALWAYS_ABORT);
    mgr->unregisterShard("shard2");
    mgr->registerShard("shard2", &aborting_shard);

    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:x", "hello");
    tx->put("shard2:y", "world");

    bool ok = tx->commit();
    EXPECT_FALSE(ok);
    EXPECT_EQ(tx->state(), DistributedTxnState::ABORTED);

    // shard1 voted COMMIT, so it must receive ABORT
    EXPECT_EQ(shard1->prepareCount(), 1);
    EXPECT_EQ(shard1->abortCount(),   1);
    EXPECT_EQ(shard1->commitCount(),  0);

    // aborting_shard voted ABORT, no abort message is needed for it
    // (it never received a successful prepare)
    EXPECT_EQ(aborting_shard.commitCount(), 0);
}

TEST_F(DistributedTransactionTest, PrepareExceptionTreatedAsAbortVote) {
    MockShardParticipant throwing_shard(MockShardParticipant::PreparePolicy::THROW);
    mgr->unregisterShard("shard2");
    mgr->registerShard("shard2", &throwing_shard);

    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:k", "v");
    tx->put("shard2:k", "v");

    bool ok = tx->commit();
    EXPECT_FALSE(ok);
    EXPECT_EQ(tx->state(), DistributedTxnState::ABORTED);

    // shard1 must have been sent ABORT
    EXPECT_EQ(shard1->abortCount(), 1);
    EXPECT_EQ(shard1->commitCount(), 0);
}

// ============================================================================
// Rollback / Explicit Abort Tests
// ============================================================================

TEST_F(DistributedTransactionTest, ExplicitRollbackAbortsTransaction) {
    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:m", "n");

    tx->rollback();
    EXPECT_EQ(tx->state(), DistributedTxnState::ABORTED);

    // No prepare was sent, so no abort message either
    EXPECT_EQ(shard1->prepareCount(), 0);
    EXPECT_EQ(shard1->abortCount(),   0);
}

TEST_F(DistributedTransactionTest, CommitAfterRollbackReturnsFalse) {
    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:p", "q");

    tx->rollback();
    bool ok = tx->commit();
    EXPECT_FALSE(ok);
}

TEST_F(DistributedTransactionTest, RollbackIsIdempotent) {
    auto tx = mgr->beginDistributedTransaction();
    tx->rollback();
    tx->rollback();  // Second call must not throw or crash
    EXPECT_EQ(tx->state(), DistributedTxnState::ABORTED);
}

// ============================================================================
// Delete operation Tests
// ============================================================================

TEST_F(DistributedTransactionTest, DeleteOperationIsBufferedAndCommitted) {
    // Pre-populate shard via a separate transaction
    {
        auto tx = mgr->beginDistributedTransaction();
        tx->put("shard1:toDelete", "exists");
        tx->commit();
    }
    ASSERT_EQ(shard1->stored("toDelete"), "exists");

    // Now delete it
    auto tx = mgr->beginDistributedTransaction();
    tx->del("shard1:toDelete");
    bool ok = tx->commit();
    EXPECT_TRUE(ok);
    EXPECT_EQ(shard1->stored("toDelete"), std::nullopt);
}

// ============================================================================
// Read operation Tests
// ============================================================================

TEST_F(DistributedTransactionTest, GetReadsFromCorrectShard) {
    // Pre-populate
    {
        auto tx = mgr->beginDistributedTransaction();
        tx->put("shard2:readKey", "readValue");
        tx->commit();
    }

    auto tx = mgr->beginDistributedTransaction();
    auto val = tx->get("shard2:readKey");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "readValue");
}

TEST_F(DistributedTransactionTest, GetMissingKeyReturnsNullopt) {
    // A registered shard with no data for the key must return nullopt.
    auto tx = mgr->beginDistributedTransaction();
    auto val = tx->get("shard1:noSuchKey");
    EXPECT_FALSE(val.has_value());
}

TEST_F(DistributedTransactionTest, GetUnknownShardThrows) {
    // get() on an unregistered shard must throw, consistent with put()/del().
    auto tx = mgr->beginDistributedTransaction();
    EXPECT_THROW(tx->get("unregisteredShard:key"), std::invalid_argument);
}

// ============================================================================
// Manager API Tests
// ============================================================================

TEST_F(DistributedTransactionTest, ShardCountReflectsRegistrations) {
    EXPECT_EQ(mgr->shardCount(), 3u);
    mgr->unregisterShard("shard3");
    EXPECT_EQ(mgr->shardCount(), 2u);
}

TEST_F(DistributedTransactionTest, HasShardReturnsTrueForRegisteredShard) {
    EXPECT_TRUE(mgr->hasShard("shard1"));
    EXPECT_FALSE(mgr->hasShard("unknown_shard"));
}

TEST_F(DistributedTransactionTest, RegisterNullParticipantThrows) {
    EXPECT_THROW(mgr->registerShard("newShard", nullptr), std::invalid_argument);
}

TEST_F(DistributedTransactionTest, RegisterEmptyShardIdThrows) {
    MockShardParticipant p;
    EXPECT_THROW(mgr->registerShard("", &p), std::invalid_argument);
}

TEST_F(DistributedTransactionTest, UnregisterNonExistentShardReturnsFalse) {
    EXPECT_FALSE(mgr->unregisterShard("nonexistent"));
}

TEST_F(DistributedTransactionTest, PutUnknownShardThrows) {
    auto tx = mgr->beginDistributedTransaction();
    EXPECT_THROW(tx->put("unknownShard:key", "value"), std::invalid_argument);
}

TEST_F(DistributedTransactionTest, PutOnNonActiveTransactionThrows) {
    auto tx = mgr->beginDistributedTransaction();
    tx->rollback();
    EXPECT_THROW(tx->put("shard1:k", "v"), std::invalid_argument);
}

TEST_F(DistributedTransactionTest, MultipleTransactionsAreIndependent) {
    auto tx1 = mgr->beginDistributedTransaction();
    auto tx2 = mgr->beginDistributedTransaction();

    EXPECT_NE(tx1->id(), tx2->id());

    tx1->put("shard1:tx1key", "tx1val");
    tx2->put("shard1:tx2key", "tx2val");

    EXPECT_TRUE(tx1->commit());
    EXPECT_TRUE(tx2->commit());

    EXPECT_EQ(shard1->stored("tx1key"), "tx1val");
    EXPECT_EQ(shard1->stored("tx2key"), "tx2val");
}

// ============================================================================
// Participating shards & operation count
// ============================================================================

TEST_F(DistributedTransactionTest, ParticipatingShards) {
    auto tx = mgr->beginDistributedTransaction();
    tx->put("shard1:a", "1");
    tx->put("shard3:b", "2");

    auto shards = tx->participatingShards();
    ASSERT_EQ(shards.size(), 2u);
    EXPECT_NE(std::find(shards.begin(), shards.end(), "shard1"), shards.end());
    EXPECT_NE(std::find(shards.begin(), shards.end(), "shard3"), shards.end());
}

TEST_F(DistributedTransactionTest, OperationCountAccumulates) {
    auto tx = mgr->beginDistributedTransaction();
    EXPECT_EQ(tx->operationCount(), 0u);
    tx->put("shard1:a", "1");
    EXPECT_EQ(tx->operationCount(), 1u);
    tx->put("shard2:b", "2");
    EXPECT_EQ(tx->operationCount(), 2u);
    tx->del("shard1:a");
    EXPECT_EQ(tx->operationCount(), 3u);
}

// ============================================================================
// Concurrent Transactions Tests
// ============================================================================

TEST_F(DistributedTransactionTest, ConcurrentTransactionsAllSucceed) {
    const int num_transactions = 10;
    std::vector<std::thread> threads;
    std::vector<bool> results(num_transactions, false);

    for (int i = 0; i < num_transactions; ++i) {
        threads.emplace_back([this, i, &results]() {
            auto tx = mgr->beginDistributedTransaction();
            tx->put("shard1:key_" + std::to_string(i), "val_" + std::to_string(i));
            tx->put("shard2:key_" + std::to_string(i), "val_" + std::to_string(i));
            results[i] = tx->commit();
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (bool r : results) {
        EXPECT_TRUE(r);
    }
}

// ============================================================================
// Performance smoke test
// ============================================================================

TEST_F(DistributedTransactionTest, HighThroughputSingleShard) {
    const int num_txns = 100;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_txns; ++i) {
        auto tx = mgr->beginDistributedTransaction();
        tx->put("shard1:key_" + std::to_string(i), std::to_string(i));
        ASSERT_TRUE(tx->commit());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Processed " << num_txns << " transactions in " << ms << "ms ("
              << (ms > 0 ? (num_txns * 1000.0 / ms) : 0.0) << " txn/sec)\n";

    EXPECT_EQ(shard1->commitCount(), num_txns);
}
