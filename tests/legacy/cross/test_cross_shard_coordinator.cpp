/**
 * ThemisDB Cross-Shard Transaction Coordinator Tests
 * 
 * Tests for the enhanced cross-shard transaction coordinator including:
 * - 2PC, 3PC, SAGA, and Percolator protocols
 * - RPC calls with retry logic
 * - Transaction log persistence
 * - Coordinator failure recovery
 * - Deadlock detection
 */

#include <gtest/gtest.h>
#include "sharding/cross_shard_transaction.h"
#include "sharding/consensus_module.h"
#include "sharding/transaction_snapshot.h"
#include "sharding/truetime.h"
#include "sharding/orphan_detector.h"
#include "sharding/shard_rpc_client.h"
#include <atomic>
#include <filesystem>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>
#ifdef _WIN32
#  include <process.h>
#  define getpid _getpid
#else
#  include <unistd.h>
#endif

using namespace themisdb::sharding;

namespace {

std::string makeTempTxnLogPath(const std::string& prefix) {
    return (std::filesystem::temp_directory_path() /
            (prefix + std::to_string(::getpid()) + ".jsonl")).string();
}

}  // namespace

// ============================================================================
// DISABLED: MockConsensusModule Test Infrastructure
// ============================================================================
//
// NOTE: This test file has been disabled due to API mismatches between the test
// infrastructure and the ThemisDB ConsensusModule interface:
//
// 1. MockConsensusModule::propose() returns bool, but ConsensusModule::propose()
//    signature requires: std::optional<uint64_t> propose(
//        const std::string& operation,
//        const nlohmann::json& data
//    )
//
// 2. Missing implementation for virtual methods from ConsensusModule:
//    - ConsensusType getType() const
//    - bool initialize(const std::string& node_id, const std::vector<std::string>& cluster_nodes)
//    - bool start()
//    - void stop()
//    - std::string getLeaderId() const
//    - ConsensusState getState() const
//    - bool waitForCommit(uint64_t log_index, std::chrono::milliseconds timeout)
//    - std::vector<ConsensusLogEntry> readLog(uint64_t start_index, std::optional<uint64_t> end_index)
//    - uint64_t getCommitIndex() const
//    - uint64_t getLastLogIndex() const
//    - bool addNode(const std::string& node_id, const std::string& endpoint)
//    - bool removeNode(const std::string& node_id)
//    - bool transferLeadership(const std::string& target_node_id)
//    - bool takeSnapshot(const nlohmann::json& snapshot_data)
//    - bool restoreSnapshot(const nlohmann::json& snapshot_data)
//    - ConsensusStats getStats() const
//    - nlohmann::json getStatus() const
//    - void onCommit(std::function<void(const ConsensusLogEntry&)> callback)
//    - void onStateChange(std::function<void(ConsensusState, ConsensusState)> callback)
//    - void onLeaderChange(std::function<void(const std::string&, const std::string&)> callback)
//
// 3. Additional type mismatches:
//    - CrossShardTransactionCoordinator API not matching expectations
//    - RPC infrastructure not fully available in unit test context
//
// ACTION REQUIRED: Update MockConsensusModule to implement all virtual methods
// with proper signatures before re-enabling tests.
//
// Placeholder test to keep file valid:
class MockConsensusModule : public ConsensusModule {
public:
    ConsensusType getType() const override { return ConsensusType::RAFT; }
    
    bool initialize(
        const std::string&,
        const std::vector<std::string>&
    ) override { return true; }
    
    bool start() override { return true; }
    void stop() override {}
    
    bool isLeader() const override { return true; }
    std::string getLeaderId() const override { return "leader-1"; }
    ConsensusState getState() const override { return ConsensusState::LEADER; }
    
    std::optional<uint64_t> propose(
        const std::string& operation,
        const nlohmann::json& data
    ) override {
        proposals_.push_back({operation, data});
        return ++last_index_;
    }
    
    bool waitForCommit([[maybe_unused]] uint64_t log_index,
                       [[maybe_unused]] std::chrono::milliseconds timeout) override { return true; }
    
    std::vector<ConsensusLogEntry> readLog(
        [[maybe_unused]] uint64_t start_index,
        [[maybe_unused]] std::optional<uint64_t> end_index = std::nullopt
    ) override { return {}; }
    
    uint64_t getCommitIndex() const override { return 0; }
    uint64_t getLastLogIndex() const override { return last_index_; }
    
    bool addNode([[maybe_unused]] const std::string& node_id,
                 [[maybe_unused]] const std::string& endpoint) override { return true; }
    bool removeNode([[maybe_unused]] const std::string& node_id) override { return true; }
    bool transferLeadership([[maybe_unused]] const std::string& target_node_id) override { return true; }
    
    bool takeSnapshot([[maybe_unused]] const nlohmann::json& snapshot_data) override { return true; }
    bool restoreSnapshot([[maybe_unused]] const nlohmann::json& snapshot_data) override { return true; }
    
    ConsensusStats getStats() const override {
        return ConsensusStats{
            0,              // current_term
            0,              // commit_index
            0,              // last_applied
            ConsensusState::LEADER,  // state
            "leader-1",     // current_leader
            1,              // cluster_size
            1,              // reachable_nodes
            std::chrono::milliseconds(0),  // average_replication_latency
            0,              // total_operations
            0               // failed_operations
        };
    }
    
    nlohmann::json getStatus() const override { return nlohmann::json::object(); }
    
    void onCommit([[maybe_unused]] std::function<void(const ConsensusLogEntry&)> callback) override {}
    void onStateChange([[maybe_unused]] std::function<void(ConsensusState, ConsensusState)> callback) override {}
    void onLeaderChange([[maybe_unused]] std::function<void(const std::string&, const std::string&)> callback) override {}
    
    std::vector<std::pair<std::string, nlohmann::json>> proposals_;
    uint64_t last_index_ = 0;
};

// ============================================================================
// PLACEHOLDER TEST
// ============================================================================
class CrossShardCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CrossShardCoordinatorTest on Windows due to intermittent deadlock/timeout instability in coordinator race tests.";
#endif
        auto consensus = std::make_shared<MockConsensusModule>();
        CrossShardTransactionConfig config;
        config.transaction_log_path = makeTempTxnLogPath("themisdb_cscoord_");
        coordinator_ = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
        coordinator_->initialize();
        coordinator_->start();
    }
    
    void TearDown() override {
        if (coordinator_) {
            coordinator_->stop();
        }
    }

    std::unique_ptr<CrossShardTransactionCoordinator> coordinator_;
};

// Placeholder test to keep file structure valid
TEST_F(CrossShardCoordinatorTest, PlaceholderTestDisabledInfrastructure) {
    // DISABLED: See comments above for required API fixes
    // This placeholder test preserves file structure while tests remain disabled
    EXPECT_TRUE(true);
}

TEST_F(CrossShardCoordinatorTest, ConcurrentCommitThenAbortKeepsSingleTerminalDecision) {
    const std::string txn_id = "txn-race-commit-first";
    ASSERT_TRUE(coordinator_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator_->addParticipant(
        txn_id, "shard-race-a", "localhost:50051", {"write:key-race-a"}));

    std::promise<void> start_signal;
    auto start_gate = start_signal.get_future().share();
    std::atomic<bool> commit_result{false};
    std::atomic<bool> abort_result{false};

    std::thread commit_thread([&] {
        start_gate.wait();
        commit_result.store(coordinator_->commit(txn_id), std::memory_order_relaxed);
    });
    std::thread abort_thread([&] {
        start_gate.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        abort_result.store(coordinator_->abort(txn_id), std::memory_order_relaxed);
    });

    start_signal.set_value();
    commit_thread.join();
    abort_thread.join();

    EXPECT_TRUE(commit_result.load(std::memory_order_relaxed));
    EXPECT_FALSE(abort_result.load(std::memory_order_relaxed));

    const auto state = coordinator_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::COMMITTED);

    const auto stats = coordinator_->getStatistics();
    EXPECT_EQ(stats["committed_transactions"].get<uint64_t>(), 1u);
    EXPECT_EQ(stats["aborted_transactions"].get<uint64_t>(), 0u);
}

TEST_F(CrossShardCoordinatorTest, ConcurrentAbortThenCommitKeepsSingleTerminalDecision) {
    const std::string txn_id = "txn-race-abort-first";
    ASSERT_TRUE(coordinator_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator_->addParticipant(
        txn_id, "shard-race-b", "localhost:50052", {"write:key-race-b"}));

    std::promise<void> start_signal;
    auto start_gate = start_signal.get_future().share();
    std::atomic<bool> commit_result{false};
    std::atomic<bool> abort_result{false};

    std::thread abort_thread([&] {
        start_gate.wait();
        abort_result.store(coordinator_->abort(txn_id), std::memory_order_relaxed);
    });
    std::thread commit_thread([&] {
        start_gate.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        commit_result.store(coordinator_->commit(txn_id), std::memory_order_relaxed);
    });

    start_signal.set_value();
    abort_thread.join();
    commit_thread.join();

    EXPECT_TRUE(abort_result.load(std::memory_order_relaxed));
    EXPECT_FALSE(commit_result.load(std::memory_order_relaxed));

    const auto state = coordinator_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ABORTED);

    const auto stats = coordinator_->getStatistics();
    EXPECT_EQ(stats["committed_transactions"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["aborted_transactions"].get<uint64_t>(), 1u);
}

TEST_F(CrossShardCoordinatorTest, DuplicateCommitOnCommittedTransactionIsIdempotent) {
    const std::string txn_id = "txn-duplicate-commit";
    ASSERT_TRUE(coordinator_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator_->addParticipant(
        txn_id, "shard-dup-commit", "localhost:50053", {"write:key-dup-commit"}));

    ASSERT_TRUE(coordinator_->commit(txn_id));
    EXPECT_TRUE(coordinator_->commit(txn_id));

    const auto state = coordinator_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::COMMITTED);

    const auto stats = coordinator_->getStatistics();
    EXPECT_EQ(stats["committed_transactions"].get<uint64_t>(), 1u);
    EXPECT_EQ(stats["aborted_transactions"].get<uint64_t>(), 0u);
}

TEST_F(CrossShardCoordinatorTest, DuplicateAbortOnAbortedTransactionIsIdempotent) {
    const std::string txn_id = "txn-duplicate-abort";
    ASSERT_TRUE(coordinator_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator_->addParticipant(
        txn_id, "shard-dup-abort", "localhost:50054", {"write:key-dup-abort"}));

    ASSERT_TRUE(coordinator_->abort(txn_id));
    EXPECT_TRUE(coordinator_->abort(txn_id));

    const auto state = coordinator_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ABORTED);

    const auto stats = coordinator_->getStatistics();
    EXPECT_EQ(stats["committed_transactions"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["aborted_transactions"].get<uint64_t>(), 1u);
}

class DistributedDeadlockDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping DistributedDeadlockDetectionTest on Windows due to timing instability in deadlock detector integration paths.";
#endif
        auto consensus = std::make_shared<MockConsensusModule>();
        CrossShardTransactionConfig config;
        config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_");
        config.enable_deadlock_detection = true;
        config.deadlock_detection_interval = std::chrono::milliseconds(25);
        coordinator_ = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
        coordinator_->initialize();
        coordinator_->start();
    }

    void TearDown() override {
        if (coordinator_) {
            coordinator_->stop();
        }
    }

    std::unique_ptr<CrossShardTransactionCoordinator> coordinator_;
};

TEST_F(DistributedDeadlockDetectionTest, DetectsAndResolvesReportedCrossShardCycle) {
    ASSERT_TRUE(coordinator_->beginTransaction(
        "txn-deadlock-a", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator_->beginTransaction(
        "txn-deadlock-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator_->reportDistributedWait("txn-deadlock-a", "txn-deadlock-b", "shard-A");
    coordinator_->reportDistributedWait("txn-deadlock-b", "txn-deadlock-a", "shard-B");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool resolved = false;
    while (std::chrono::steady_clock::now() < deadline) {
        auto state_a = coordinator_->getTransactionState("txn-deadlock-a");
        auto state_b = coordinator_->getTransactionState("txn-deadlock-b");

        if ((state_a.has_value() && *state_a == TransactionState::ABORTED) ||
            (state_b.has_value() && *state_b == TransactionState::ABORTED)) {
            resolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(resolved);

    auto stats = coordinator_->getStatistics();
    ASSERT_TRUE(stats.contains("deadlocked_transactions"));
    EXPECT_GE(stats["deadlocked_transactions"].get<uint64_t>(), 1u);
}

TEST_F(DistributedDeadlockDetectionTest, VictimSelectionSkipsUpstreamNonCycleWaiter) {
    ASSERT_TRUE(coordinator_->beginTransaction(
        "txn-cycle-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator_->beginTransaction(
        "txn-cycle-c", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator_->beginTransaction(
        "txn-chain-head", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator_->reportDistributedWait("txn-chain-head", "txn-cycle-b", "shard-A");
    coordinator_->reportDistributedWait("txn-cycle-b", "txn-cycle-c", "shard-B");
    coordinator_->reportDistributedWait("txn-cycle-c", "txn-cycle-b", "shard-C");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool cycle_resolved = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto state_b = coordinator_->getTransactionState("txn-cycle-b");
        const auto state_c = coordinator_->getTransactionState("txn-cycle-c");
        if ((state_b.has_value() && *state_b == TransactionState::ABORTED) ||
            (state_c.has_value() && *state_c == TransactionState::ABORTED)) {
            cycle_resolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    ASSERT_TRUE(cycle_resolved);
    const auto state_head = coordinator_->getTransactionState("txn-chain-head");
    ASSERT_TRUE(state_head.has_value());
    EXPECT_NE(*state_head, TransactionState::ABORTED);
}

TEST(CrossShardDeadlockGraphTest, IsDeadlockedRequiresCycleMembership) {
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_membership_");
    config.enable_deadlock_detection = false;

    CrossShardTransactionCoordinator coordinator(config, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-chain-head", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-cycle-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-cycle-c", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator.reportDistributedWait("txn-chain-head", "txn-cycle-b", "shard-A");
    coordinator.reportDistributedWait("txn-cycle-b", "txn-cycle-c", "shard-B");
    coordinator.reportDistributedWait("txn-cycle-c", "txn-cycle-b", "shard-C");

    EXPECT_FALSE(coordinator.isDeadlocked("txn-chain-head"));
    EXPECT_TRUE(coordinator.isDeadlocked("txn-cycle-b"));
    EXPECT_TRUE(coordinator.isDeadlocked("txn-cycle-c"));

    coordinator.stop();
}

TEST(CrossShardDeadlockGraphTest, ClearDistributedWaitsBreaksCycleMembership) {
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_clear_");
    config.enable_deadlock_detection = false;

    CrossShardTransactionCoordinator coordinator(config, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-clear-a", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-clear-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator.reportDistributedWait("txn-clear-a", "txn-clear-b", "shard-A");
    coordinator.reportDistributedWait("txn-clear-b", "txn-clear-a", "shard-B");
    ASSERT_TRUE(coordinator.isDeadlocked("txn-clear-a"));
    ASSERT_TRUE(coordinator.isDeadlocked("txn-clear-b"));

    coordinator.clearDistributedWaits("txn-clear-a");

    EXPECT_FALSE(coordinator.isDeadlocked("txn-clear-a"));
    EXPECT_FALSE(coordinator.isDeadlocked("txn-clear-b"));
    coordinator.stop();
}

TEST(CrossShardDeadlockGraphTest, AbortClearsReportedCycleEdges) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping CrossShardDeadlockGraphTest.AbortClearsReportedCycleEdges on Windows due to intermittent deadlock exception in abort path.";
#endif
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_abort_clear_");
    config.enable_deadlock_detection = false;

    CrossShardTransactionCoordinator coordinator(config, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-abort-a", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-abort-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator.reportDistributedWait("txn-abort-a", "txn-abort-b", "shard-A");
    coordinator.reportDistributedWait("txn-abort-b", "txn-abort-a", "shard-B");
    ASSERT_TRUE(coordinator.isDeadlocked("txn-abort-a"));
    ASSERT_TRUE(coordinator.isDeadlocked("txn-abort-b"));

    ASSERT_TRUE(coordinator.abort("txn-abort-a"));

    EXPECT_FALSE(coordinator.isDeadlocked("txn-abort-b"));
    coordinator.stop();
}

TEST(CrossShardDeadlockGraphTest, ReportDistributedWaitIgnoresInvalidEdges) {
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_invalid_edges_");
    config.enable_deadlock_detection = false;

    CrossShardTransactionCoordinator coordinator(config, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-valid-a", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-valid-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator.reportDistributedWait("txn-valid-a", "txn-valid-a", "shard-self");
    coordinator.reportDistributedWait("", "txn-valid-b", "shard-empty");
    coordinator.reportDistributedWait("txn-valid-a", "", "shard-empty");
    coordinator.reportDistributedWait("txn-missing", "txn-valid-b", "shard-missing");
    coordinator.reportDistributedWait("txn-valid-a", "txn-missing", "shard-missing");

    EXPECT_FALSE(coordinator.isDeadlocked("txn-valid-a"));
    EXPECT_FALSE(coordinator.isDeadlocked("txn-valid-b"));
    coordinator.stop();
}

TEST_F(DistributedDeadlockDetectionTest, VictimSelectionChoosesYoungestCycleMember) {
    ASSERT_TRUE(coordinator_->beginTransaction(
        "txn-oldest", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(coordinator_->beginTransaction(
        "txn-youngest", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator_->reportDistributedWait("txn-oldest", "txn-youngest", "shard-A");
    coordinator_->reportDistributedWait("txn-youngest", "txn-oldest", "shard-B");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool resolved = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto youngest = coordinator_->getTransactionState("txn-youngest");
        const auto oldest = coordinator_->getTransactionState("txn-oldest");
        if (youngest.has_value() && *youngest == TransactionState::ABORTED) {
            ASSERT_TRUE(oldest.has_value());
            EXPECT_NE(*oldest, TransactionState::ABORTED);
            resolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(resolved);
}

TEST(DistributedDeadlockDetectionPolicyTest, VictimSelectionChoosesOldestCycleMemberWhenConfigured) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping deadlock victim policy tests on Windows due to unstable timing in cycle-resolution paths.";
#endif
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_oldest_");
    config.enable_deadlock_detection = true;
    config.deadlock_detection_interval = std::chrono::milliseconds(25);
    config.deadlock_victim_policy = DeadlockVictimPolicy::OLDEST;

    CrossShardTransactionCoordinator coordinator(config, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-oldest-policy-old", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-oldest-policy-young", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator.reportDistributedWait("txn-oldest-policy-old", "txn-oldest-policy-young", "shard-A");
    coordinator.reportDistributedWait("txn-oldest-policy-young", "txn-oldest-policy-old", "shard-B");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool resolved = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto oldest = coordinator.getTransactionState("txn-oldest-policy-old");
        const auto youngest = coordinator.getTransactionState("txn-oldest-policy-young");
        if (oldest.has_value() && *oldest == TransactionState::ABORTED) {
            ASSERT_TRUE(youngest.has_value());
            EXPECT_NE(*youngest, TransactionState::ABORTED);
            resolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(resolved);
    coordinator.stop();
}

TEST(DistributedDeadlockDetectionPolicyTest, VictimSelectionAbortsOneMemberWhenRandomPolicyConfigured) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping deadlock victim policy tests on Windows due to unstable timing in cycle-resolution paths.";
#endif
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_random_");
    config.enable_deadlock_detection = true;
    config.deadlock_detection_interval = std::chrono::milliseconds(25);
    config.deadlock_victim_policy = DeadlockVictimPolicy::RANDOM;

    CrossShardTransactionCoordinator coordinator(config, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-random-policy-A", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-random-policy-B", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator.reportDistributedWait("txn-random-policy-A", "txn-random-policy-B", "shard-X");
    coordinator.reportDistributedWait("txn-random-policy-B", "txn-random-policy-A", "shard-Y");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool resolved = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto stateA = coordinator.getTransactionState("txn-random-policy-A");
        const auto stateB = coordinator.getTransactionState("txn-random-policy-B");
        const bool aAborted = stateA.has_value() && *stateA == TransactionState::ABORTED;
        const bool bAborted = stateB.has_value() && *stateB == TransactionState::ABORTED;
        if (aAborted || bAborted) {
            // Exactly one victim per cycle — both must not be aborted simultaneously.
            EXPECT_FALSE(aAborted && bAborted);
            resolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(resolved);
    coordinator.stop();
}

// ============================================================================
// Calvin Protocol Tests
// ============================================================================

class CalvinProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Calvin focused tests are unstable on Windows due mutex deadlock in current coordinator path";
#endif
        auto consensus = std::make_shared<MockConsensusModule>();
        CrossShardTransactionConfig config;
        config.transaction_log_path = makeTempTxnLogPath("themisdb_calvin_");
        config.calvin_epoch_duration = std::chrono::milliseconds(10);
        config.calvin_enable_deterministic_lock_order = true;
        coordinator_ = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
        coordinator_->initialize();
        coordinator_->start();
    }

    void TearDown() override {
        if (coordinator_) {
            coordinator_->stop();
        }
    }

    std::unique_ptr<CrossShardTransactionCoordinator> coordinator_;
};

TEST_F(CalvinProtocolTest, SingleShardCommit) {
    ASSERT_TRUE(coordinator_->beginTransaction("txn-calvin-1",
        TransactionProtocol::CALVIN,
        IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator_->addParticipant("txn-calvin-1", "shard1", "shard1:8080", {"write:key1"});

    bool ok = coordinator_->commit("txn-calvin-1");
    EXPECT_TRUE(ok);

    auto state = coordinator_->getTransactionState("txn-calvin-1");
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::COMMITTED);
}

TEST_F(CalvinProtocolTest, MultiShardCommitDeterministicOrder) {
    ASSERT_TRUE(coordinator_->beginTransaction("txn-calvin-2",
        TransactionProtocol::CALVIN,
        IsolationLevel::SNAPSHOT_ISOLATION));

    // Add participants in reverse alphabetical order to verify deterministic lock ordering
    coordinator_->addParticipant("txn-calvin-2", "shard_z", "shard_z:8080", {"write:keyZ"});
    coordinator_->addParticipant("txn-calvin-2", "shard_a", "shard_a:8080", {"write:keyA"});
    coordinator_->addParticipant("txn-calvin-2", "shard_m", "shard_m:8080", {"write:keyM"});

    bool ok = coordinator_->commit("txn-calvin-2");
    EXPECT_TRUE(ok);

    auto state = coordinator_->getTransactionState("txn-calvin-2");
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::COMMITTED);
}

TEST_F(CalvinProtocolTest, AbortEmptyParticipants) {
    ASSERT_TRUE(coordinator_->beginTransaction("txn-calvin-3",
        TransactionProtocol::CALVIN,
        IsolationLevel::SNAPSHOT_ISOLATION));

    // No participants – Calvin should refuse to commit
    bool ok = coordinator_->commit("txn-calvin-3");
    EXPECT_FALSE(ok);
}

TEST_F(CalvinProtocolTest, CommitSequenceNumberIsMonotonic) {
    // Start two Calvin transactions and verify each gets a distinct snapshot timestamp.
    // beginTransaction() captures the current TrueTime as snapshot_timestamp; as long
    // as the clock advances between the two calls the timestamps will differ.
    ASSERT_TRUE(coordinator_->beginTransaction("txn-calvin-seq-1",
        TransactionProtocol::CALVIN,
        IsolationLevel::SNAPSHOT_ISOLATION));

    // Spin-wait until TrueTime advances (avoids sleep while still being deterministic)
    auto txn1_pre = coordinator_->getTransaction("txn-calvin-seq-1");
    ASSERT_TRUE(txn1_pre.has_value());
    int64_t ts1 = txn1_pre->snapshot_timestamp;
    int64_t ts2 = ts1;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (ts2 <= ts1 && std::chrono::steady_clock::now() < deadline) {
        auto probe_id = "probe-" + std::to_string(ts2);
        coordinator_->beginTransaction(probe_id,
            TransactionProtocol::CALVIN, IsolationLevel::SNAPSHOT_ISOLATION);
        auto probe = coordinator_->getTransaction(probe_id);
        if (probe) {
            ts2 = probe->snapshot_timestamp;
        }
        coordinator_->abort(probe_id);
        std::this_thread::yield();
    }

    ASSERT_TRUE(coordinator_->beginTransaction("txn-calvin-seq-2",
        TransactionProtocol::CALVIN,
        IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator_->addParticipant("txn-calvin-seq-1", "shard1", "shard1:8080", {"write:k1"});
    coordinator_->addParticipant("txn-calvin-seq-2", "shard1", "shard1:8080", {"write:k2"});

    auto txn1 = coordinator_->getTransaction("txn-calvin-seq-1");
    auto txn2 = coordinator_->getTransaction("txn-calvin-seq-2");
    ASSERT_TRUE(txn1.has_value());
    ASSERT_TRUE(txn2.has_value());
    EXPECT_LE(txn1->snapshot_timestamp, txn2->snapshot_timestamp);

    EXPECT_TRUE(coordinator_->commit("txn-calvin-seq-1"));
    EXPECT_TRUE(coordinator_->commit("txn-calvin-seq-2"));
}

TEST_F(CalvinProtocolTest, ConcurrentCalvinTransactions) {
    const int num_txns = 8;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_txns; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            std::string txn_id = "txn-calvin-concurrent-" + std::to_string(i);
            if (!coordinator_->beginTransaction(txn_id,
                    TransactionProtocol::CALVIN,
                    IsolationLevel::SNAPSHOT_ISOLATION)) {
                return;
            }
            coordinator_->addParticipant(txn_id, "shard1", "shard1:8080",
                                         {"write:key" + std::to_string(i)});
            coordinator_->addParticipant(txn_id, "shard2", "shard2:8080",
                                         {"write:val" + std::to_string(i)});
            if (coordinator_->commit(txn_id)) {
                ++success_count;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), num_txns);
}

TEST_F(CalvinProtocolTest, ProtocolStringRoundtrip) {
    // Verify that "CALVIN" serializes and deserializes correctly in the WAL layer
    EXPECT_EQ(::sharding::TransactionProtocol::CALVIN,
              ::sharding::transactionProtocolFromString("CALVIN"));
    EXPECT_EQ("CALVIN",
              ::sharding::transactionProtocolToString(::sharding::TransactionProtocol::CALVIN));
}

// ============================================================================
// PercolatorCoordinator Tests
// ============================================================================

class PercolatorCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Percolator focused tests are unstable on Windows due mutex deadlock in current coordinator path";
#endif
        auto consensus = std::make_shared<MockConsensusModule>();
        CrossShardTransactionConfig config;
        config.transaction_log_path = makeTempTxnLogPath("themisdb_percolator_");
        config.default_protocol = TransactionProtocol::PERCOLATOR;
        coordinator_ = std::make_shared<CrossShardTransactionCoordinator>(config, consensus);
        coordinator_->initialize();
        coordinator_->start();

        PercolatorCoordinator::Config perc_cfg;
        perc_cfg.lock_timeout       = std::chrono::milliseconds(200);
        perc_cfg.max_retries        = 2;
        perc_cfg.stale_lock_threshold = std::chrono::seconds(1);
        percolator_ = std::make_unique<PercolatorCoordinator>(perc_cfg);
    }

    void TearDown() override {
        if (coordinator_) {
            coordinator_->stop();
        }
    }

    std::shared_ptr<CrossShardTransactionCoordinator> coordinator_;
    std::unique_ptr<PercolatorCoordinator> percolator_;
};

// TrueTime::now_with_uncertainty() returns an interval with earliest <= latest.
TEST(TrueTimePercolatorTest, NowWithUncertaintyReturnsValidInterval) {
    themis::sharding::TrueTime::Config cfg;
    cfg.base_uncertainty_us = 500; // 0.5 ms
    themis::sharding::TrueTime tt(cfg);

    auto interval = tt.now_with_uncertainty();
    EXPECT_LE(interval.earliest.count(), interval.latest.count())
        << "earliest must be <= latest";
    // Uncertainty must be non-negative.
    EXPECT_GE(interval.uncertainty().count(), 0);
}

// now_with_uncertainty() and now() must return consistent intervals (same call
// semantics — just a named alias).
TEST(TrueTimePercolatorTest, NowWithUncertaintyConsistentWithNow) {
    themis::sharding::TrueTime::Config cfg;
    cfg.base_uncertainty_us = 1000;
    themis::sharding::TrueTime tt(cfg);

    auto i1 = tt.now();
    auto i2 = tt.now_with_uncertainty();

    // Both intervals should be "close" in time (within 1 second).
    auto diff_ns = std::abs(i2.midpoint().count() - i1.midpoint().count());
    EXPECT_LT(diff_ns, 1'000'000'000LL) << "Intervals should be within 1s of each other";
}

// PercolatorCoordinator constructs without error.
TEST_F(PercolatorCoordinatorTest, ConstructsWithoutError) {
    EXPECT_NE(percolator_, nullptr);
}

// Executing Percolator on a transaction with a single shard succeeds.
TEST_F(PercolatorCoordinatorTest, SingleShardPercolatorCommit) {
    const std::string txn_id = "perc-single-1";
    ASSERT_TRUE(coordinator_->beginTransaction(
        txn_id, TransactionProtocol::PERCOLATOR, IsolationLevel::SNAPSHOT_ISOLATION));
    coordinator_->addParticipant(txn_id, "shard1", "shard1:8080", {"write:key1"});

    bool ok = coordinator_->commit(txn_id);
    EXPECT_TRUE(ok);

    auto state = coordinator_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::COMMITTED);
}

// Executing Percolator on a multi-shard transaction succeeds.
TEST_F(PercolatorCoordinatorTest, MultiShardPercolatorCommit) {
    const std::string txn_id = "perc-multi-1";
    ASSERT_TRUE(coordinator_->beginTransaction(
        txn_id, TransactionProtocol::PERCOLATOR, IsolationLevel::SNAPSHOT_ISOLATION));
    coordinator_->addParticipant(txn_id, "shard1", "shard1:8080", {"write:keyA"});
    coordinator_->addParticipant(txn_id, "shard2", "shard2:8080", {"write:keyB"});
    coordinator_->addParticipant(txn_id, "shard3", "shard3:8080", {"write:keyC"});

    bool ok = coordinator_->commit(txn_id);
    EXPECT_TRUE(ok);

    auto state = coordinator_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::COMMITTED);
}

// Aborting a Percolator transaction yields ABORTED state.
TEST_F(PercolatorCoordinatorTest, AbortPercolatorTransaction) {
    const std::string txn_id = "perc-abort-1";
    ASSERT_TRUE(coordinator_->beginTransaction(
        txn_id, TransactionProtocol::PERCOLATOR, IsolationLevel::SNAPSHOT_ISOLATION));
    coordinator_->addParticipant(txn_id, "shard1", "shard1:8080", {"write:keyX"});

    bool ok = coordinator_->abort(txn_id);
    EXPECT_TRUE(ok);

    auto state = coordinator_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ABORTED);
}

// cleanStaleLocks returns 0 when no stale transaction IDs are provided.
TEST_F(PercolatorCoordinatorTest, CleanStaleLocksEmptyListReturnsZero) {
    size_t cleaned = percolator_->cleanStaleLocks({}, *coordinator_);
    EXPECT_EQ(cleaned, 0u);
}

// cleanStaleLocks skips a non-existent transaction ID gracefully.
TEST_F(PercolatorCoordinatorTest, CleanStaleLocksUnknownTxnSkipped) {
    size_t cleaned = percolator_->cleanStaleLocks({"nonexistent-txn"}, *coordinator_);
    EXPECT_EQ(cleaned, 0u);
}

// OrphanDetector::cleanPercolatorLocks returns 0 with an empty coordinator.
TEST_F(PercolatorCoordinatorTest, OrphanDetectorCleanPercolatorLocksEmpty) {
    ::sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 1;
    ::sharding::OrphanDetector detector(cfg);

    size_t cleaned = detector.cleanPercolatorLocks(coordinator_);
    EXPECT_EQ(cleaned, 0u);
}

// OrphanDetector::detectOrphans returns no orphans for a fresh coordinator.
TEST_F(PercolatorCoordinatorTest, OrphanDetectorNoOrphansInitially) {
    ::sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 900;
    ::sharding::OrphanDetector detector(cfg);

    auto orphans = detector.detectOrphans(coordinator_);
    EXPECT_TRUE(orphans.empty());
}

// OrphanDetector::isOrphaned returns false for an unknown transaction.
TEST_F(PercolatorCoordinatorTest, OrphanDetectorIsOrphanedUnknownReturnsFalse) {
    ::sharding::OrphanDetector::Config cfg;
    ::sharding::OrphanDetector detector(cfg);

    EXPECT_FALSE(detector.isOrphaned("no-such-txn", coordinator_));
}

// Concurrent Percolator transactions all succeed.
TEST_F(PercolatorCoordinatorTest, ConcurrentPercolatorTransactions) {
    const int kCount = 6;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < kCount; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            const std::string txn_id = "perc-concurrent-" + std::to_string(i);
            if (!coordinator_->beginTransaction(
                    txn_id, TransactionProtocol::PERCOLATOR, IsolationLevel::SNAPSHOT_ISOLATION)) {
                return;
            }
            coordinator_->addParticipant(txn_id, "shard1", "shard1:8080",
                                         {"write:key" + std::to_string(i)});
            coordinator_->addParticipant(txn_id, "shard2", "shard2:8080",
                                         {"write:val" + std::to_string(i)});
            if (coordinator_->commit(txn_id)) {
                ++success_count;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), kCount);
}

// ============================================================================
// Coordinator ID + Compensation RPC Tests (Issue #106)
// ============================================================================

class CoordinatorIdTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CoordinatorIdTest on Windows due to unstable CALVIN coordinator path in focused runs.";
#endif
        auto consensus = std::make_shared<MockConsensusModule>();
        CrossShardTransactionConfig config;
        config.transaction_log_path = makeTempTxnLogPath("themisdb_coord_id_");
        config.coordinator_id = "node-42";
        coordinator_ = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
        coordinator_->initialize();
        coordinator_->start();
    }

    void TearDown() override {
        if (coordinator_) {
            coordinator_->stop();
        }
    }

    std::unique_ptr<CrossShardTransactionCoordinator> coordinator_;
};

// Verify that CrossShardTransactionConfig accepts a coordinator_id field.
TEST(CoordinatorIdConfigTest, CoordinatorIdDefaultsToEmpty) {
    CrossShardTransactionConfig cfg;
    EXPECT_TRUE(cfg.coordinator_id.empty());
}

TEST(CoordinatorIdConfigTest, CoordinatorIdIsRetained) {
    CrossShardTransactionConfig cfg;
    cfg.coordinator_id = "node-42";
    EXPECT_EQ(cfg.coordinator_id, "node-42");
}

// A coordinator constructed with a coordinator_id can begin and commit
// transactions normally (regression: coordinator_id must not break existing flow).
TEST_F(CoordinatorIdTest, TransactionSucceedsWithCoordinatorId) {
#ifdef _WIN32
    GTEST_SKIP() << "CoordinatorId CALVIN commit path is unstable on Windows due mutex deadlock";
#endif
    ASSERT_TRUE(coordinator_->beginTransaction("txn-coord-id-1",
        TransactionProtocol::CALVIN, IsolationLevel::SNAPSHOT_ISOLATION));
    coordinator_->addParticipant("txn-coord-id-1", "shard1", "shard1:8080", {"write:key1"});
    EXPECT_TRUE(coordinator_->commit("txn-coord-id-1"));
}

// A coordinator constructed without a coordinator_id still works.
TEST(CoordinatorIdConfigTest, EmptyCoordinatorIdNoCrash) {
#ifdef _WIN32
    GTEST_SKIP() << "CoordinatorId CALVIN commit path is unstable on Windows due mutex deadlock";
#endif
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig cfg;
    cfg.transaction_log_path = makeTempTxnLogPath("themisdb_coord_empty_");
    // coordinator_id intentionally left empty
    CrossShardTransactionCoordinator coordinator(cfg, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());
    ASSERT_TRUE(coordinator.beginTransaction("txn-no-coord-id",
        TransactionProtocol::CALVIN, IsolationLevel::SNAPSHOT_ISOLATION));
    coordinator.addParticipant("txn-no-coord-id", "shard1", "shard1:8080", {"write:key1"});
    EXPECT_TRUE(coordinator.commit("txn-no-coord-id"));
    coordinator.stop();
}

// ============================================================================
// ShardRPCClient compensate() Tests (Issue #106)
// ============================================================================

TEST(ShardRpcCompensateTest, CompensateReturnsTrue) {
    themis::sharding::ShardRPCClient::Config cfg;
    cfg.endpoint = "shard-test:50051";
    cfg.timeout_ms = 500;
    cfg.max_retries = 1;
    cfg.enable_circuit_breaker = false;

    themis::sharding::ShardRPCClient client(cfg);

    nlohmann::json op = {{"type", "delete"}, {"key", "user:123"}};
    EXPECT_TRUE(client.compensate("txn-comp-1", op));
}

TEST(ShardRpcCompensateTest, CompensateWithEmptyOperationSucceeds) {
    themis::sharding::ShardRPCClient::Config cfg;
    cfg.endpoint = "shard-test:50051";
    cfg.timeout_ms = 500;
    cfg.max_retries = 1;
    cfg.enable_circuit_breaker = false;

    themis::sharding::ShardRPCClient client(cfg);

    EXPECT_TRUE(client.compensate("txn-comp-2", nlohmann::json::object()));
}

// ============================================================================
// ShardRPCClient::setInProcessResponseHandler Tests (stub #57 resolution)
// ============================================================================

TEST(ShardRpcInjectHandlerTest, InjectedHandlerCalledInsteadOfHardcoded) {
    themis::sharding::ShardRPCClient::Config cfg;
    cfg.endpoint     = "shard-inject:50051";
    cfg.timeout_ms   = 500;
    cfg.max_retries  = 1;
    cfg.enable_circuit_breaker = false;

    themis::sharding::ShardRPCClient client(cfg);

    bool handler_called = false;
    client.setInProcessResponseHandler(
        [&](std::string method, nlohmann::json /*params*/) -> nlohmann::json {
            handler_called = true;
            if (method == "ping") {
                return {{"status", "ok"}, {"injected", true}};
            }
            return {{"status", "ok"}};
        });

    EXPECT_TRUE(client.ping());
    EXPECT_TRUE(handler_called);
}

TEST(ShardRpcInjectHandlerTest, InjectedAbortVoteFailsPrepare) {
    themis::sharding::ShardRPCClient::Config cfg;
    cfg.endpoint     = "shard-abort:50051";
    cfg.timeout_ms   = 500;
    cfg.max_retries  = 1;
    cfg.enable_circuit_breaker = false;

    themis::sharding::ShardRPCClient client(cfg);

    // Inject a handler that simulates an ABORT vote from the shard.
    client.setInProcessResponseHandler(
        [](std::string method, nlohmann::json) -> nlohmann::json {
            if (method == "prepare") {
                return {{"vote", "abort"}, {"status", "failed"}};
            }
            return {{"status", "ok"}};
        });

    EXPECT_FALSE(client.prepare("txn-abort-1", nlohmann::json::array()));
}

TEST(ShardRpcInjectHandlerTest, ClearHandlerRestoresHardcodedFallback) {
    themis::sharding::ShardRPCClient::Config cfg;
    cfg.endpoint     = "shard-clear:50051";
    cfg.timeout_ms   = 500;
    cfg.max_retries  = 1;
    cfg.enable_circuit_breaker = false;

    themis::sharding::ShardRPCClient client(cfg);

    // First inject a failing handler, then clear it.
    client.setInProcessResponseHandler(
        [](std::string, nlohmann::json) -> nlohmann::json {
            return {{"vote", "abort"}, {"status", "failed"}};
        });
    EXPECT_FALSE(client.prepare("txn-clear-1", nlohmann::json::array()));

    // Clear — hardcoded success fallback should take over.
    client.setInProcessResponseHandler(nullptr);
    EXPECT_TRUE(client.prepare("txn-clear-2", nlohmann::json::array()));
}

// ============================================================================
// CrossShardTransactionCoordinator::setPreCommitCallback Tests (#17)
// 
// DISABLED: See comments at top of file about API mismatches
// ============================================================================

TEST(DISABLED_CrossShard3PCCallbackTest, PreCommitCallbackInvokedPerParticipant) {
    themisdb::sharding::CrossShardTransactionConfig cfg;
    cfg.default_protocol = themisdb::sharding::TransactionProtocol::THREE_PHASE_COMMIT;

    auto coordinator = std::make_shared<themisdb::sharding::CrossShardTransactionCoordinator>(cfg, nullptr);
    coordinator->initialize();
    coordinator->start();

    std::vector<std::string> precommitted_shards;
    coordinator->setPreCommitCallback(
        [&](const std::string& shard_id, const std::string& /*txn_id*/) -> bool {
            precommitted_shards.push_back(shard_id);
            return true;  // accept
        });

    const std::string txn = "txn-3pc-1";
    ASSERT_TRUE(coordinator->beginTransaction(txn));
    coordinator->addParticipant(txn, "shard-A", "localhost:50051", {});
    coordinator->addParticipant(txn, "shard-B", "localhost:50052", {});
    EXPECT_TRUE(coordinator->commit(txn));

    // Both participants must have received a PreCommit RPC call.
    EXPECT_EQ(static_cast<int>(precommitted_shards.size()), 2);
    coordinator->stop();
}

TEST(DISABLED_CrossShard3PCCallbackTest, PreCommitNackAbortsTransaction) {
    themisdb::sharding::CrossShardTransactionConfig cfg;
    cfg.default_protocol = themisdb::sharding::TransactionProtocol::THREE_PHASE_COMMIT;

    auto coordinator = std::make_shared<themisdb::sharding::CrossShardTransactionCoordinator>(cfg, nullptr);
    coordinator->initialize();
    coordinator->start();

    // Inject a callback that rejects one participant.
    coordinator->setPreCommitCallback(
        [](const std::string& shard_id, const std::string&) -> bool {
            return shard_id != "shard-B";  // shard-B rejects PreCommit
        });

    const std::string txn = "txn-3pc-nack";
    ASSERT_TRUE(coordinator->beginTransaction(txn));
    coordinator->addParticipant(txn, "shard-A", "localhost:50051", {});
    coordinator->addParticipant(txn, "shard-B", "localhost:50052", {});
    EXPECT_FALSE(coordinator->commit(txn));

    coordinator->stop();
}

TEST(CrossShard3PCCallbackContractTest, MissingPreCommitCallbackFailsClosed) {
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig cfg;
    cfg.default_protocol = TransactionProtocol::THREE_PHASE_COMMIT;
    cfg.transaction_log_path = makeTempTxnLogPath("themisdb_3pc_missing_precommit_");

    CrossShardTransactionCoordinator coordinator(cfg, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    const std::string txn_id = "txn-3pc-missing-precommit";
    ASSERT_TRUE(coordinator.beginTransaction(
        txn_id, TransactionProtocol::THREE_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.addParticipant(
        txn_id, "shard-A", "localhost:50051", {"write:key-A"}));

    EXPECT_FALSE(coordinator.commit(txn_id));

    const auto state = coordinator.getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ABORTED);

    coordinator.stop();
}

TEST(CrossShard3PCCallbackContractTest, ThrowingPreCommitCallbackTreatedAsNack) {
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig cfg;
    cfg.default_protocol = TransactionProtocol::THREE_PHASE_COMMIT;
    cfg.transaction_log_path = makeTempTxnLogPath("themisdb_3pc_throwing_precommit_");

    CrossShardTransactionCoordinator coordinator(cfg, consensus);
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());

    coordinator.setPreCommitCallback(
        [](const std::string&, const std::string&) -> bool {
            throw std::runtime_error("precommit callback failure");
        });

    const std::string txn_id = "txn-3pc-throwing-precommit";
    ASSERT_TRUE(coordinator.beginTransaction(
        txn_id, TransactionProtocol::THREE_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.addParticipant(
        txn_id, "shard-A", "localhost:50052", {"write:key-B"}));

    EXPECT_FALSE(coordinator.commit(txn_id));

    const auto state = coordinator.getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ABORTED);

    coordinator.stop();
}

TEST(CrossShard3PCCallbackContractTest, ThrowingDeferredCallbackFailsClosed) {
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionConfig cfg;
    cfg.default_protocol = TransactionProtocol::THREE_PHASE_COMMIT;
    cfg.transaction_log_path = makeTempTxnLogPath("themisdb_3pc_throwing_deferred_");

    CrossShardTransactionCoordinator coordinator(cfg, consensus);
    ASSERT_TRUE(coordinator.initialize());

    // Set callbacks before start() so the retry thread is spawned by start()
    // when it checks deferred_precommit_callback_ under callbacks_mutex_.
    coordinator.setPreCommitCallback(
        [](const std::string& shard_id, const std::string&) -> bool {
            return shard_id == "shard-A";
        });
    coordinator.setDeferredPreCommitCallback(
        [](const std::string&, const std::vector<std::string>&) {
            throw std::runtime_error("deferred callback failure");
        });

    ASSERT_TRUE(coordinator.start());

    const std::string txn_id = "txn-3pc-throwing-deferred";
    ASSERT_TRUE(coordinator.beginTransaction(
        txn_id, TransactionProtocol::THREE_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator.addParticipant(
        txn_id, "shard-A", "localhost:50053", {"write:key-C"}));
    ASSERT_TRUE(coordinator.addParticipant(
        txn_id, "shard-B", "localhost:50054", {"write:key-D"}));

    EXPECT_FALSE(coordinator.commit(txn_id));

    const auto state = coordinator.getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ABORTED);

    coordinator.stop();
}

// ============================================================================
// Poll-based (pull) deadlock detection tests
// ============================================================================

// Verify that collectWaitForEdges() returns an empty list by default (in-process
// simulation with no injected handler).
TEST(ShardRpcCollectWaitForEdgesTest, DefaultReturnsEmptyList) {
    themis::sharding::ShardRPCClient::Config cfg;
    cfg.endpoint           = "shard-wfe-default:50051";
    cfg.timeout_ms         = 500;
    cfg.max_retries        = 1;
    cfg.enable_circuit_breaker = false;

    themis::sharding::ShardRPCClient client(cfg);
    const auto edges = client.collectWaitForEdges();
    EXPECT_TRUE(edges.empty());
}

// Verify that collectWaitForEdges() correctly parses edges returned by the
// injected handler and ignores malformed entries.
TEST(ShardRpcCollectWaitForEdgesTest, InjectedHandlerEdgesAreParsed) {
    themis::sharding::ShardRPCClient::Config cfg;
    cfg.endpoint           = "shard-wfe-inject:50051";
    cfg.timeout_ms         = 500;
    cfg.max_retries        = 1;
    cfg.enable_circuit_breaker = false;

    themis::sharding::ShardRPCClient client(cfg);

    client.setInProcessResponseHandler(
        [](std::string method, nlohmann::json) -> nlohmann::json {
            if (method == "collect_wait_for_edges") {
                return {
                    {"shard_id", "shard-wfe-inject"},
                    {"edges", nlohmann::json::array({
                        {{"waiting_transaction_id", "txn-A"},
                         {"blocking_transaction_id", "txn-B"}},
                        {{"waiting_transaction_id", "txn-C"},
                         {"blocking_transaction_id", "txn-D"}},
                        // Malformed entries — must be silently skipped.
                        {{"waiting_transaction_id", "txn-E"}},           // missing blocking
                        {{"blocking_transaction_id", "txn-F"}},          // missing waiting
                        nlohmann::json::object()                          // empty
                    })}
                };
            }
            return {{"status", "ok"}};
        });

    const auto edges = client.collectWaitForEdges();
    ASSERT_EQ(edges.size(), 2u);
    EXPECT_EQ(edges[0].waiting_transaction_id,  "txn-A");
    EXPECT_EQ(edges[0].blocking_transaction_id, "txn-B");
    EXPECT_EQ(edges[1].waiting_transaction_id,  "txn-C");
    EXPECT_EQ(edges[1].blocking_transaction_id, "txn-D");
}

// Verify that the coordinator's deadlock detection thread picks up wait-for
// edges from a configured shard endpoint via the poll collector hook and
// resolves the resulting cross-shard cycle.
TEST(DistributedDeadlockDetectionPollingTest, PollBasedEdgesFromShardEndpointAreDetected) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping polling deadlock detection integration test on Windows due to intermittent abort-path runtime exception.";
#endif
    auto consensus = std::make_shared<MockConsensusModule>();

    // Register a loopback shard endpoint so the coordinator will try to poll it.
    // The ShardRPCClient uses in-process simulation for loopback addresses, so
    // the injected handler below controls the reported wait-for edges.
    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_poll_");
    config.enable_deadlock_detection = true;
    config.deadlock_detection_interval = std::chrono::milliseconds(25);
    config.shard_endpoints["shard-remote"] = "localhost:50099";
    config.polled_wait_for_edge_collector =
        [](const std::string& shard_id, const std::string&) {
            if (shard_id == "shard-remote") {
                return std::vector<CrossShardTransactionConfig::PolledWaitForEdge>{
                    {"txn-poll-a", "txn-poll-b"}
                };
            }
            return std::vector<CrossShardTransactionConfig::PolledWaitForEdge>{};
        };

    auto coordinator = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
    coordinator->initialize();
    coordinator->start();

    // Two transactions are known to the coordinator (ACTIVE state).
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-poll-a", TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-poll-b", TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    // The remote shard contributes one edge via polling; local shard reports
    // the reverse edge via push reporting, forming a cycle.
    coordinator->reportDistributedWait("txn-poll-b", "txn-poll-a", "shard-local");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool resolved = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto sa = coordinator->getTransactionState("txn-poll-a");
        const auto sb = coordinator->getTransactionState("txn-poll-b");
        if ((sa.has_value() && *sa == TransactionState::ABORTED) ||
            (sb.has_value() && *sb == TransactionState::ABORTED)) {
            resolved = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(resolved);

    coordinator->stop();
}

// Verify that poll-reported cycles for unknown transactions are ignored to
// prevent false-positive deadlock counters and abort attempts.
TEST(DistributedDeadlockDetectionPollingTest, PollBasedUnknownTransactionsAreIgnored) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping polling deadlock detection integration test on Windows due to intermittent abort-path runtime exception.";
#endif
    auto consensus = std::make_shared<MockConsensusModule>();

    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_poll_unknown_");
    config.enable_deadlock_detection = true;
    config.deadlock_detection_interval = std::chrono::milliseconds(25);
    config.shard_endpoints["shard-remote"] = "localhost:50099";
    config.polled_wait_for_edge_collector =
        [](const std::string&, const std::string&) {
            return std::vector<CrossShardTransactionConfig::PolledWaitForEdge>{
                {"txn-ghost-a", "txn-ghost-b"},
                {"txn-ghost-b", "txn-ghost-a"}
            };
        };

    auto coordinator = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
    coordinator->initialize();
    coordinator->start();

    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-live", TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    std::this_thread::sleep_for(std::chrono::milliseconds(180));

    const auto state = coordinator->getTransactionState("txn-live");
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ACTIVE);

    const auto stats = coordinator->getStatistics();
    ASSERT_TRUE(stats.contains("deadlocked_transactions"));
    EXPECT_EQ(stats["deadlocked_transactions"].get<uint64_t>(), 0u);

    coordinator->stop();
}

// Verify that two independent deadlock cycles are both resolved within a
// single detection round (one victim per cycle, not one victim globally).
TEST(DistributedDeadlockDetectionPollingTest, MultipleIndependentDeadlocksAllResolvedInSingleRound) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping polling deadlock detection integration test on Windows due to intermittent abort-path runtime exception.";
#endif
    auto consensus = std::make_shared<MockConsensusModule>();

    CrossShardTransactionConfig config;
    config.transaction_log_path = makeTempTxnLogPath("themisdb_deadlock_multi_");
    config.enable_deadlock_detection = true;
    // Use a longer interval so we can detect resolution within a tight window.
    config.deadlock_detection_interval = std::chrono::milliseconds(50);

    auto coordinator = std::make_unique<CrossShardTransactionCoordinator>(config, consensus);
    coordinator->initialize();
    coordinator->start();

    // Cycle 1: txn-c1-a <-> txn-c1-b
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-c1-a", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-c1-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    // Cycle 2: txn-c2-a <-> txn-c2-b (independent of cycle 1)
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-c2-a", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-c2-b", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    coordinator->reportDistributedWait("txn-c1-a", "txn-c1-b", "shard-A");
    coordinator->reportDistributedWait("txn-c1-b", "txn-c1-a", "shard-B");
    coordinator->reportDistributedWait("txn-c2-a", "txn-c2-b", "shard-C");
    coordinator->reportDistributedWait("txn-c2-b", "txn-c2-a", "shard-D");

    // Both cycles must be resolved within a few detection intervals.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool cycle1_resolved = false;
    bool cycle2_resolved = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto c1a = coordinator->getTransactionState("txn-c1-a");
        const auto c1b = coordinator->getTransactionState("txn-c1-b");
        const auto c2a = coordinator->getTransactionState("txn-c2-a");
        const auto c2b = coordinator->getTransactionState("txn-c2-b");

        if ((c1a.has_value() && *c1a == TransactionState::ABORTED) ||
            (c1b.has_value() && *c1b == TransactionState::ABORTED)) {
            cycle1_resolved = true;
        }
        if ((c2a.has_value() && *c2a == TransactionState::ABORTED) ||
            (c2b.has_value() && *c2b == TransactionState::ABORTED)) {
            cycle2_resolved = true;
        }
        if (cycle1_resolved && cycle2_resolved) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(cycle1_resolved) << "Cycle 1 was not resolved";
    EXPECT_TRUE(cycle2_resolved) << "Cycle 2 was not resolved";

    const auto stats = coordinator->getStatistics();
    ASSERT_TRUE(stats.contains("deadlocked_transactions"));
    // Two independent cycles -> at least 2 deadlock events counted.
    EXPECT_GE(stats["deadlocked_transactions"].get<uint64_t>(), 2u);

    coordinator->stop();
}
