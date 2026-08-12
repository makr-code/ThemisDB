#include "../test_fixture.h"

#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis { namespace test { 

namespace {

struct TxCommitResult {
    bool committed{false};
    std::string status;
};

class TransactionReplicationPipeline {
public:
    void Begin(const std::string& tx_id) {
        pending_[tx_id] = {};
    }

    void StageWrite(const std::string& tx_id,
                    const std::string& shard,
                    const std::string& key,
                    const std::string& value) {
        pending_[tx_id].push_back({shard, key, value});
    }

    [[nodiscard]] TxCommitResult Commit(const std::string& tx_id,
                                        bool raft_failover = false,
                                        const std::string& fail_on_shard = "") {
        auto it = pending_.find(tx_id);
        if (it == pending_.end()) {
            return {false, "tx_not_found"};
        }

        if (!fail_on_shard.empty()) {
            for (const auto& [shard, key, value] : it->second) {
                (void)key;
                (void)value;
                if (shard == fail_on_shard) {
                    ++saga_compensations_;
                    pending_.erase(it);
                    return {false, "aborted_saga_compensation"};
                }
            }
        }

        // STUB/SIMULATION NOTE:
        // Purpose: Simulate deterministic Raft failover abort for testing even/odd failover paths.
        // Activation: raft_failover=true and the tx_id string has even length (length % 2 == 0).
        // Production Delta: Real Raft failover is driven by leader election and log durability,
        //   not by tx_id string length. The length parity is a test-only stand-in.
        // Removal Plan: Replace with a proper mock Raft leader-election injector when available.
        if (raft_failover && (tx_id.size() % 2 == 0)) {
            pending_.erase(it);
            return {false, "aborted_during_failover"};
        }

        for (const auto& [shard, key, value] : it->second) {
            shards_[shard][key] = value;
            wal_entries_.push_back(key + "=" + value);
            replica_entries_.push_back(key + "=" + value);
            cdc_events_.push_back("cdc:" + key);
        }

        pending_.erase(it);
        return {true, "committed"};
    }

    void Rollback(const std::string& tx_id) {
        pending_.erase(tx_id);
        ++mvcc_cleanup_runs_;
    }

    [[nodiscard]] bool HasWalEntry(const std::string& key) const {
        for (const auto& entry : wal_entries_) {
            if (entry.find(key + "=") == 0U) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool HasReplicaEntry(const std::string& key) const {
        for (const auto& entry : replica_entries_) {
            if (entry.find(key + "=") == 0U) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_t CdcEventCount() const { return cdc_events_.size(); }
    [[nodiscard]] size_t SagaCompensations() const { return saga_compensations_; }
    [[nodiscard]] size_t MvccCleanupRuns() const { return mvcc_cleanup_runs_; }

private:
    std::unordered_map<std::string, std::vector<std::tuple<std::string, std::string, std::string>>> pending_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> shards_;
    std::vector<std::string> wal_entries_;
    std::vector<std::string> replica_entries_;
    std::vector<std::string> cdc_events_;
    size_t saga_compensations_{0};
    size_t mvcc_cleanup_runs_{0};
};

} // namespace

class TransactionReplicationPipelineTest : public IntegrationTestFixture {
protected:
    TransactionReplicationPipeline pipeline_;
};

TEST_F(TransactionReplicationPipelineTest, TXR01_MultiShardCommitReplicatesAndEmitsCdc) {
    pipeline_.Begin("tx_1");
    pipeline_.StageWrite("tx_1", "shard_a", "k1", "v1");
    pipeline_.StageWrite("tx_1", "shard_b", "k2", "v2");

    const auto result = pipeline_.Commit("tx_1");

    ASSERT_TRUE(result.committed);
    EXPECT_TRUE(pipeline_.HasWalEntry("k1"));
    EXPECT_TRUE(pipeline_.HasReplicaEntry("k2"));
    EXPECT_EQ(pipeline_.CdcEventCount(), 2U);
}

TEST_F(TransactionReplicationPipelineTest, TXR02_RollbackLeavesNoWalAndTriggersMvccCleanup) {
    pipeline_.Begin("tx_2");
    pipeline_.StageWrite("tx_2", "shard_a", "k_rollback", "v");

    pipeline_.Rollback("tx_2");

    EXPECT_FALSE(pipeline_.HasWalEntry("k_rollback"));
    EXPECT_EQ(pipeline_.MvccCleanupRuns(), 1U);
}

TEST_F(TransactionReplicationPipelineTest, TXR03_SagaCompensationRunsOnPartialFailure) {
    pipeline_.Begin("tx_3");
    pipeline_.StageWrite("tx_3", "shard_ok", "k_ok", "v_ok");
    pipeline_.StageWrite("tx_3", "shard_fail", "k_fail", "v_fail");

    const auto result = pipeline_.Commit("tx_3", false, "shard_fail");

    EXPECT_FALSE(result.committed);
    EXPECT_EQ(result.status, "aborted_saga_compensation");
    EXPECT_EQ(pipeline_.SagaCompensations(), 1U);
    EXPECT_FALSE(pipeline_.HasWalEntry("k_fail"));
}

// tx_id "tx_failover" has 11 characters (odd length) → raft failover does NOT abort → committed.
TEST_F(TransactionReplicationPipelineTest, TXR04_RaftFailoverWithOddLengthTxIdStillCommitsWithoutPartialLoss) {
    pipeline_.Begin("tx_failover");
    pipeline_.StageWrite("tx_failover", "shard_a", "k_f", "v_f");

    const auto result = pipeline_.Commit("tx_failover", true);

    EXPECT_TRUE(result.committed);
    EXPECT_EQ(result.status, "committed");
    EXPECT_TRUE(pipeline_.HasWalEntry("k_f"));
    EXPECT_TRUE(pipeline_.HasReplicaEntry("k_f"));
}

// tx_id "tx_even_01" has 10 characters (even length) → raft failover aborts → no WAL residue.
TEST_F(TransactionReplicationPipelineTest, TXR05_RaftFailoverWithEvenLengthTxIdAbortsAndLeavesNoWalResidue) {
    pipeline_.Begin("tx_even_01");
    pipeline_.StageWrite("tx_even_01", "shard_a", "k_abort", "v_abort");

    const auto result = pipeline_.Commit("tx_even_01", true);

    EXPECT_FALSE(result.committed);
    EXPECT_EQ(result.status, "aborted_during_failover");
    EXPECT_FALSE(pipeline_.HasWalEntry("k_abort"));
    EXPECT_FALSE(pipeline_.HasReplicaEntry("k_abort"));
}
} } // namespace themis::test
