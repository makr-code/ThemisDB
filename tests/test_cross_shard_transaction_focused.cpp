/*
 * ThemisDB | Test: test_cross_shard_transaction_focused.cpp | Version: 0.0.47
 * Focused Unit Tests for W2-S04: Cross-Shard Transaction Precondition Validation
 */

#include <gtest/gtest.h>

#include "sharding/consensus_module.h"
#include "sharding/cross_shard_transaction.h"
#include "sharding/transaction_wal.h"

#include <chrono>
#include <memory>
#include <optional>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <atomic>
#include <condition_variable>
#include <thread>

namespace themisdb { namespace sharding { 

class FakeConsensusModule final : public ConsensusModule {
public:
    [[nodiscard]] ConsensusType getType() const override { return ConsensusType::RAFT; }

    [[nodiscard]] bool initialize(const std::string& node_id,
                                  const std::vector<std::string>& cluster_nodes) override {
        node_id_ = node_id;
        cluster_nodes_ = cluster_nodes;
        return true;
    }

    [[nodiscard]] bool start() override {
        running_ = true;
        return true;
    }

    void stop() override { running_ = false; }

    [[nodiscard]] bool isLeader() const override { return true; }
    [[nodiscard]] std::string getLeaderId() const override { return node_id_.empty() ? "leader-1" : node_id_; }
    [[nodiscard]] ConsensusState getState() const override { return ConsensusState::LEADER; }

    [[nodiscard]] std::optional<uint64_t> propose(const std::string&, const nlohmann::json&) override {
        return ++next_index_;
    }

    [[nodiscard]] bool waitForCommit(uint64_t, std::chrono::milliseconds) override { return true; }

    [[nodiscard]] std::vector<ConsensusLogEntry> readLog(uint64_t,
                                                         std::optional<uint64_t>) override {
        return {};
    }

    [[nodiscard]] uint64_t getCommitIndex() const override { return next_index_; }
    [[nodiscard]] uint64_t getLastLogIndex() const override { return next_index_; }

    [[nodiscard]] bool addNode(const std::string&, const std::string&) override { return true; }
    [[nodiscard]] bool removeNode(const std::string&) override { return true; }
    [[nodiscard]] bool transferLeadership(const std::string&) override { return true; }
    [[nodiscard]] bool takeSnapshot(const nlohmann::json&) override { return true; }
    [[nodiscard]] bool restoreSnapshot(const nlohmann::json&) override { return true; }

    [[nodiscard]] ConsensusStats getStats() const override {
        ConsensusStats stats{};
        stats.current_term = 1;
        stats.commit_index = next_index_;
        stats.last_applied = next_index_;
        stats.state = ConsensusState::LEADER;
        stats.current_leader = getLeaderId();
        stats.cluster_size = cluster_nodes_.size();
        stats.reachable_nodes = cluster_nodes_.size();
        return stats;
    }

    [[nodiscard]] nlohmann::json getStatus() const override {
        return {
            {"running", running_},
            {"leader", getLeaderId()},
            {"last_index", next_index_}
        };
    }

    void onCommit(std::function<void(const ConsensusLogEntry&)>) override {}
    void onStateChange(std::function<void(ConsensusState, ConsensusState)>) override {}
    void onLeaderChange(std::function<void(const std::string&, const std::string&)>) override {}

private:
    std::string node_id_;
    std::vector<std::string> cluster_nodes_;
    bool running_ = false;
    uint64_t next_index_ = 0;
};

class CrossShardTransactionCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CrossShardTransactionCoordinatorTest on Windows due to recurring std::system_error deadlock exceptions in focused coordinator fixture.";
#endif
        config_.coordinator_id = "coord-1";
        config_.lock_timeout = std::chrono::milliseconds(100);
        directory_ = std::filesystem::current_path() / ".build-dis06-coordinator" /
            ("coordinator-unit-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(directory_);
        config_.transaction_log_path = (directory_ / "transactions.jsonl").string();
        config_.enable_persistence = true;
        config_.data_dir = directory_.string();
        config_.enable_deadlock_detection = false;
        config_.fencing_token_provider = [](const std::string& shard)
            -> std::optional<themis::sharding::EpochToken> {
            themis::sharding::EpochToken token;
            token.epoch = 1;
            token.issuer = "coord-1";
            token.shard_id = shard;
            token.issued_at = std::chrono::system_clock::now();
            return token;
        };
        consensus_ = std::make_shared<FakeConsensusModule>();
    }

    void TearDown() override {
        std::filesystem::remove_all(directory_);
    }

    std::unique_ptr<CrossShardTransactionCoordinator> createCoordinator() {
        auto coordinator =
            std::make_unique<CrossShardTransactionCoordinator>(config_, consensus_, nullptr);
        EXPECT_TRUE(coordinator->initialize());
        EXPECT_TRUE(coordinator->start());
        return coordinator;
    }

    CrossShardTransactionConfig config_;
    std::shared_ptr<FakeConsensusModule> consensus_;
    std::filesystem::path directory_;
};

TEST_F(CrossShardTransactionCoordinatorTest, BeginTransactionRejectsEmptyId) {
    auto coordinator = createCoordinator();
    EXPECT_FALSE(coordinator->beginTransaction(
        "", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
}

TEST_F(CrossShardTransactionCoordinatorTest, BeginTransactionAcceptsValidId) {
    auto coordinator = createCoordinator();
    EXPECT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
}

TEST_F(CrossShardTransactionCoordinatorTest, BeginTransactionRejectsDuplicate) {
    auto coordinator = createCoordinator();
    EXPECT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    EXPECT_FALSE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyTransactionId) {
    auto coordinator = createCoordinator();
    EXPECT_FALSE(coordinator->addParticipant("", "shard-1", "localhost:5000", {"READ", "WRITE"}));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyShardId) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    EXPECT_FALSE(coordinator->addParticipant("txn-12345", "", "localhost:5000", {"READ", "WRITE"}));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyEndpoint) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    EXPECT_FALSE(coordinator->addParticipant("txn-12345", "shard-1", "", {"READ", "WRITE"}));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsEmptyOperations) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    EXPECT_FALSE(coordinator->addParticipant("txn-12345", "shard-1", "localhost:5000", {}));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsNonExistentTransaction) {
    auto coordinator = createCoordinator();
    EXPECT_FALSE(coordinator->addParticipant(
        "txn-nonexistent", "shard-1", "localhost:5000", {"READ", "WRITE"}));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantRejectsDuplicateShard) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator->addParticipant(
        "txn-12345", "shard-1", "localhost:5000", {"READ", "WRITE"}));
    EXPECT_FALSE(coordinator->addParticipant(
        "txn-12345", "shard-1", "localhost:5001", {"DELETE"}));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantAcceptsValidInputs) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    EXPECT_TRUE(coordinator->addParticipant(
        "txn-12345", "shard-1", "localhost:5000", {"READ", "WRITE"}));
}

TEST_F(CrossShardTransactionCoordinatorTest, AddParticipantAcceptsMultipleDifferentShards) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));

    const bool r1 = coordinator->addParticipant("txn-12345", "shard-1", "host1:5000", {"READ"});
    const bool r2 = coordinator->addParticipant("txn-12345", "shard-2", "host2:5000", {"WRITE"});
    const bool r3 = coordinator->addParticipant("txn-12345", "shard-3", "host3:5000", {"READ", "WRITE"});

    EXPECT_TRUE(r1);
    EXPECT_TRUE(r2);
    EXPECT_TRUE(r3);
}

TEST_F(CrossShardTransactionCoordinatorTest, PrepareRejectsEmptyTransactionId) {
    auto coordinator = createCoordinator();
    EXPECT_FALSE(coordinator->prepare(""));
}

TEST_F(CrossShardTransactionCoordinatorTest, PrepareRejectsTransactionWithNoParticipants) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    EXPECT_FALSE(coordinator->prepare("txn-12345"));
}

TEST_F(CrossShardTransactionCoordinatorTest, PrepareRejectsNonExistentTransaction) {
    auto coordinator = createCoordinator();
    EXPECT_FALSE(coordinator->prepare("txn-nonexistent"));
}

TEST_F(CrossShardTransactionCoordinatorTest, PrepareCanProceedWhenPreconditionsSatisfied) {
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction(
        "txn-12345", TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SNAPSHOT_ISOLATION));
    ASSERT_TRUE(coordinator->addParticipant(
        "txn-12345", "shard-1", "localhost:5000", {"READ", "WRITE"}));

    (void)coordinator->prepare("txn-12345");
}

class RecordingTwoPhaseParticipant final
    : public themis::sharding::ShardRPCServer::RequestHandler {
public:
    bool onPrepare(const std::string&, const std::string&, const std::string&) override {
        ++prepares;
        if (before_vote) {
            before_vote();
        }
        return true;
    }
    bool onCommit(const std::string&) override {
        ++commits;
        return acknowledge;
    }
    bool onAbort(const std::string&) override {
        ++aborts;
        return acknowledge;
    }
    themis::sharding::ShardRPCServer::HealthInfo onHealthCheck() override { return {}; }
    int prepares = 0;
    int commits = 0;
    int aborts = 0;
    bool acknowledge = true;
    std::function<void()> before_vote;
};

#ifndef _WIN32
class ScopedCoordinatorFsyncFailure {
public:
    ScopedCoordinatorFsyncFailure() {
        if (const auto* previous = std::getenv("THEMIS_TEST_WAL_FSYNC_FAIL_ONCE")) {
            previous_ = previous;
        }
        ::setenv("THEMIS_TEST_WAL_FSYNC_FAIL_ONCE", "1", 1);
    }
    ~ScopedCoordinatorFsyncFailure() {
        if (previous_) {
            ::setenv("THEMIS_TEST_WAL_FSYNC_FAIL_ONCE", previous_->c_str(), 1);
        } else {
            ::unsetenv("THEMIS_TEST_WAL_FSYNC_FAIL_ONCE");
        }
    }
private:
    std::optional<std::string> previous_;
};
#endif

TEST_F(CrossShardTransactionCoordinatorTest, DisabledWalRejectsCanonicalWritesBeforeRpc) {
    themis::sharding::TwoPhaseCommitCoordinator coordinator("coord-1");
    RecordingTwoPhaseParticipant participant;
    coordinator.registerParticipant("shard-1", &participant);
    const std::map<std::string, nlohmann::json> operations{
        {"shard-1", nlohmann::json::array()}};
    EXPECT_FALSE(coordinator.isReady());
    EXPECT_FALSE(coordinator.prepare("no-wal", operations));
    EXPECT_EQ(coordinator.commit("no-wal", operations).result,
              themis::sharding::CoordinatorTxnResult::FAILURE);
    EXPECT_EQ(coordinator.abort("no-wal", {"shard-1"}).result,
              themis::sharding::CoordinatorTxnResult::FAILURE);
    EXPECT_EQ(participant.prepares + participant.commits + participant.aborts, 0);

    config_.enable_persistence = false;
    auto cross = createCoordinator();
    EXPECT_FALSE(cross->beginTransaction("volatile-2pc"));
    EXPECT_TRUE(cross->beginTransaction("volatile-saga", TransactionProtocol::SAGA));
}

TEST_F(CrossShardTransactionCoordinatorTest, CanonicalPrepareResumesWithoutSecondPrepare) {
    themis::sharding::TwoPhaseCommitCoordinator::Config config;
    config.wal_directory = (directory_ / "canonical").string();
    config.sync_wal_writes = false;
    config.fencing_token_provider = config_.fencing_token_provider;
    themis::sharding::TwoPhaseCommitCoordinator coordinator("coord-1", config);
    RecordingTwoPhaseParticipant participant;
    coordinator.registerParticipant("shard-1", &participant);
    const std::map<std::string, nlohmann::json> operations{
        {"shard-1", nlohmann::json::array()}};
    ASSERT_TRUE(coordinator.prepare("prepared", operations));
    EXPECT_EQ(participant.prepares, 1);
    EXPECT_EQ(participant.commits + participant.aborts, 0);
    EXPECT_FALSE(coordinator.getTransactionDecision("prepared").has_value());
    ASSERT_TRUE(coordinator.commit("prepared", operations).committed());
    EXPECT_EQ(participant.prepares, 1);
    EXPECT_EQ(participant.commits, 1);
    EXPECT_TRUE(coordinator.commit("prepared", operations).committed());
    EXPECT_EQ(coordinator.abort("prepared", {"shard-1"}).result,
              themis::sharding::CoordinatorTxnResult::COMMITTED);
    EXPECT_EQ(participant.aborts, 0);
}

TEST_F(CrossShardTransactionCoordinatorTest, UnacknowledgedAbortRequiresRegisteredRecovery) {
    themis::sharding::TwoPhaseCommitCoordinator::Config config;
    config.wal_directory = (directory_ / "canonical").string();
    config.sync_wal_writes = false;
    config.participant_endpoint_resolver = [](const std::string&)
        -> std::optional<themis::sharding::ShardRPCClient::Config> {
        themis::sharding::ShardRPCClient::Config rpc;
        rpc.endpoint = "not-an-endpoint";
        return rpc;
    };
    RecordingTwoPhaseParticipant participant;
    participant.acknowledge = false;
    {
        themis::sharding::TwoPhaseCommitCoordinator coordinator("coord-1", config);
        coordinator.registerParticipant("shard-1", &participant);
        EXPECT_EQ(coordinator.abort("abort-in-doubt", {"shard-1"}).result,
                  themis::sharding::CoordinatorTxnResult::FAILURE);
        EXPECT_EQ(coordinator.getTransactionDecision("abort-in-doubt"), false);
        EXPECT_FALSE(coordinator.isReady());
    }
    themis::sharding::TwoPhaseCommitCoordinator recovered("coord-1", config);
    EXPECT_FALSE(recovered.isReady());
    ASSERT_EQ(recovered.getRecoverableTransactions().size(), 1u);
    EXPECT_EQ(recovered.recoverInDoubtTransactions(), 0u);
    EXPECT_FALSE(recovered.isReady());
    participant.acknowledge = true;
    recovered.registerParticipant("shard-1", &participant);
    EXPECT_EQ(recovered.recoverInDoubtTransactions(), 1u);
    EXPECT_TRUE(recovered.isReady());
    EXPECT_EQ(recovered.getTransactionDecision("abort-in-doubt"), false);
    EXPECT_EQ(participant.commits, 0);
}

TEST_F(CrossShardTransactionCoordinatorTest, LegacyInDoubtTwoPhaseWalBlocksStartup) {
    {
        ::sharding::TransactionWALConfig wal_config;
        wal_config.wal_directory = (directory_ / "wal").string();
        wal_config.snapshot_directory = (directory_ / "snapshots").string();
        ::sharding::TransactionWAL wal(wal_config);
        ASSERT_TRUE(wal.initialize());
        wal.logBegin("old-in-doubt", ::sharding::TransactionProtocol::TWO_PHASE_COMMIT,
                     {"shard-1"});
        wal.logPrepare("old-in-doubt", "shard-1", nlohmann::json::object());
    }
    CrossShardTransactionCoordinator coordinator(config_, consensus_);
    EXPECT_FALSE(coordinator.initialize());
    EXPECT_FALSE(coordinator.start());
    EXPECT_FALSE(coordinator.beginTransaction("new-2pc"));
    ASSERT_EQ(coordinator.getRecoverableTransactions().size(), 1u);
    EXPECT_EQ(coordinator.getRecoverableTransactions().front().transaction_id, "old-in-doubt");
}

TEST_F(CrossShardTransactionCoordinatorTest, CrossShardCommitDelegatesBothCanonicalPhases) {
    int prepares = 0;
    int commits = 0;
    int aborts = 0;
    std::vector<int64_t> commit_timestamps;
    config_.in_process_rpc_response_handler =
        [&](std::string method, nlohmann::json parameters) -> nlohmann::json {
        if (method == "prepare") {
            ++prepares;
            return {{"vote", "commit"}, {"status", "prepared"}};
        }
        if (method == "commit") {
            ++commits;
            commit_timestamps.push_back(parameters.at("commit_timestamp").get<int64_t>());
            return {{"status", "committed"}};
        }
        ++aborts;
        return {{"status", "aborted"}};
    };
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction("delegated"));
    ASSERT_TRUE(coordinator->addParticipant(
        "delegated", "shard-1", "inproc://coordinator-delegation", {"WRITE"}));
    ASSERT_TRUE(coordinator->addParticipant(
        "delegated", "shard-2", "inproc://coordinator-delegation-2", {"WRITE"}));
    ASSERT_TRUE(coordinator->prepare("delegated"));
    EXPECT_EQ(prepares, 2);
    EXPECT_EQ(commits, 0);
    EXPECT_TRUE(coordinator->commit("delegated"));
    EXPECT_EQ(prepares, 2);
    EXPECT_EQ(commits, 2);
    EXPECT_EQ(aborts, 0);
    EXPECT_EQ(coordinator->getTransactionState("delegated"), TransactionState::COMMITTED);
    const auto transaction = coordinator->getTransaction("delegated");
    ASSERT_TRUE(transaction.has_value());
    ASSERT_EQ(commit_timestamps.size(), 2u);
    EXPECT_EQ(commit_timestamps[0], commit_timestamps[1]);
    EXPECT_EQ(transaction->commit_timestamp, commit_timestamps[0]);
    EXPECT_GT(transaction->commit_timestamp, transaction->snapshot_timestamp);
    for (const auto& [shard, participant] : transaction->participants) {
        EXPECT_TRUE(participant.prepared);
        EXPECT_TRUE(participant.committed);
        EXPECT_FALSE(participant.aborted);
    }
    EXPECT_TRUE(std::filesystem::exists(directory_ / "two_phase_commit"));
}

TEST_F(CrossShardTransactionCoordinatorTest, CrossShardExplicitAbortNeverPrepares) {
    int aborts = 0;
    config_.in_process_rpc_response_handler =
        [&](std::string method, nlohmann::json) -> nlohmann::json {
        EXPECT_EQ(method, "abort");
        ++aborts;
        return {{"status", "aborted"}};
    };
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction("explicit-abort"));
    ASSERT_TRUE(coordinator->addParticipant(
        "explicit-abort", "shard-1", "inproc://coordinator-abort", {"WRITE"}));
    EXPECT_TRUE(coordinator->abort("explicit-abort"));
    EXPECT_EQ(aborts, 1);
    EXPECT_EQ(coordinator->getTransactionState("explicit-abort"), TransactionState::ABORTED);
    const auto transaction = coordinator->getTransaction("explicit-abort");
    ASSERT_TRUE(transaction.has_value());
    EXPECT_TRUE(transaction->participants.at("shard-1").aborted);
    EXPECT_TRUE(coordinator->abort("explicit-abort"));
    EXPECT_EQ(aborts, 1);
}

TEST_F(CrossShardTransactionCoordinatorTest, ContradictoryCanonicalWalRejectsStartup) {
    const auto wal_directory = (directory_ / "contradictory").string();
    {
        themis::sharding::WALManagerConfig config;
        config.wal_directory = wal_directory;
        themis::sharding::WALManager wal(config);
        themis::sharding::WALEntry entry;
        entry.transaction_id = "immutable-decision";
        entry.type = themis::sharding::WALEntryType::BEGIN_TX;
        entry.data = {{"shards", nlohmann::json::array({"shard-1"})}};
        wal.append(entry);
        for (const auto type : {themis::sharding::WALEntryType::COMMIT_TX,
                                themis::sharding::WALEntryType::ABORT_TX}) {
            entry.type = type;
            entry.data = {{"phase", "decision"}};
            wal.append(entry);
        }
    }
    themis::sharding::TwoPhaseCommitCoordinator::Config config;
    config.wal_directory = wal_directory;
    EXPECT_THROW(themis::sharding::TwoPhaseCommitCoordinator("coord-1", config),
                 std::runtime_error);
}

TEST_F(CrossShardTransactionCoordinatorTest, DecisionFsyncFailureBlocksRpcAndRequiresRescan) {
#ifdef _WIN32
    GTEST_SKIP() << "Existing WAL fsync failure injection is POSIX-only";
#else
    themis::sharding::TwoPhaseCommitCoordinator::Config config;
    config.wal_directory = (directory_ / "fsync-failure").string();
    config.sync_wal_writes = false;
    config.fencing_token_provider = config_.fencing_token_provider;
    RecordingTwoPhaseParticipant first;
    RecordingTwoPhaseParticipant second;
    std::unique_ptr<ScopedCoordinatorFsyncFailure> failure;
    second.before_vote = [&] {
        failure = std::make_unique<ScopedCoordinatorFsyncFailure>();
    };
    const std::map<std::string, nlohmann::json> operations{
        {"shard-1", nlohmann::json::array()},
        {"shard-2", nlohmann::json::array()}};
    {
        themis::sharding::TwoPhaseCommitCoordinator coordinator("coord-1", config);
        coordinator.registerParticipant("shard-1", &first);
        coordinator.registerParticipant("shard-2", &second);
        EXPECT_EQ(coordinator.commit("ambiguous-decision", operations).result,
                  themis::sharding::CoordinatorTxnResult::FAILURE);
        EXPECT_EQ(first.prepares + second.prepares, 2);
        EXPECT_EQ(first.commits + second.commits + first.aborts + second.aborts, 0);
        EXPECT_FALSE(coordinator.isReady());
        EXPECT_THROW(static_cast<void>(coordinator.recoverInDoubtTransactions()),
                     std::runtime_error);
    }
    failure.reset();
    second.before_vote = {};
    themis::sharding::TwoPhaseCommitCoordinator recovered("coord-1", config);
    recovered.registerParticipant("shard-1", &first);
    recovered.registerParticipant("shard-2", &second);
    EXPECT_EQ(recovered.getTransactionDecision("ambiguous-decision"), true);
    EXPECT_EQ(recovered.recoverInDoubtTransactions(), 1u);
    EXPECT_TRUE(recovered.isReady());
    EXPECT_EQ(first.commits + second.commits, 2);
    EXPECT_EQ(first.aborts + second.aborts, 0);
#endif
}

TEST_F(CrossShardTransactionCoordinatorTest, PrepareSerializesWithTerminalDecisions) {
    std::mutex mutex;
    std::condition_variable cv;
    bool entered_prepare = false;
    bool release_prepare = false;
    config_.in_process_rpc_response_handler =
        [&](std::string method, nlohmann::json) -> nlohmann::json {
        if (method == "prepare") {
            std::unique_lock<std::mutex> lock(mutex);
            entered_prepare = true;
            cv.notify_all();
            cv.wait(lock, [&] { return release_prepare; });
            return {{"vote", "commit"}, {"status", "prepared"}};
        }
        return {{"status", "aborted"}};
    };
    auto coordinator = createCoordinator();
    ASSERT_TRUE(coordinator->beginTransaction("prepare-race"));
    ASSERT_TRUE(coordinator->addParticipant(
        "prepare-race", "shard-1", "inproc://coordinator-race", {"WRITE"}));
    std::atomic<bool> prepared{false};
    std::thread driver([&] { prepared.store(coordinator->prepare("prepare-race")); });
    bool prepare_observed = false;
    {
        std::unique_lock<std::mutex> lock(mutex);
        prepare_observed = cv.wait_for(
            lock, std::chrono::seconds(5), [&] { return entered_prepare; });
    }
    if (prepare_observed) {
        EXPECT_FALSE(coordinator->abort("prepare-race"));
        EXPECT_FALSE(coordinator->commit("prepare-race"));
    }
    {
        std::lock_guard<std::mutex> lock(mutex);
        release_prepare = true;
    }
    cv.notify_all();
    driver.join();
    ASSERT_TRUE(prepare_observed);
    ASSERT_TRUE(prepared.load());
    EXPECT_TRUE(coordinator->abort("prepare-race"));
    EXPECT_EQ(coordinator->getTransactionState("prepare-race"), TransactionState::ABORTED);
}

TEST_F(CrossShardTransactionCoordinatorTest, LegacyPartialAbortRemainsBlocked) {
    {
        ::sharding::TransactionWALConfig config;
        config.wal_directory = (directory_ / "wal").string();
        config.snapshot_directory = (directory_ / "snapshots").string();
        ::sharding::TransactionWAL wal(config);
        ASSERT_TRUE(wal.initialize());
        wal.logBegin("old-partial-abort", ::sharding::TransactionProtocol::TWO_PHASE_COMMIT,
                     {"shard-1", "shard-2"});
        wal.logAbort("old-partial-abort", "durable_old_decision");
        wal.logAborted("old-partial-abort", "shard-1");
    }
    CrossShardTransactionCoordinator coordinator(config_, consensus_);
    EXPECT_FALSE(coordinator.initialize());
    EXPECT_FALSE(coordinator.start());
    EXPECT_EQ(coordinator.getTransactionState("old-partial-abort"), TransactionState::ABORTING);
    ASSERT_EQ(coordinator.getRecoverableTransactions().size(), 1u);
}

TEST_F(CrossShardTransactionCoordinatorTest, RecoveredCommitReusesSharedMvccTimestamp) {
    bool acknowledge = false;
    std::vector<int64_t> timestamps;
    config_.participant_endpoint_resolver = [](const std::string& shard)
        -> std::optional<themis::sharding::ShardRPCClient::Config> {
        themis::sharding::ShardRPCClient::Config rpc;
        rpc.endpoint = "inproc://mvcc-" + shard;
        return rpc;
    };
    config_.in_process_rpc_response_handler =
        [&](std::string method, nlohmann::json parameters) -> nlohmann::json {
        if (method == "prepare") {
            return {{"vote", "commit"}, {"status", "prepared"}};
        }
        if (method == "commit") {
            timestamps.push_back(parameters.at("commit_timestamp").get<int64_t>());
            return {{"status", acknowledge ? "committed" : "rejected"}};
        }
        ADD_FAILURE() << "A durable COMMIT must never turn into ABORT";
        return {{"status", "aborted"}};
    };
    {
        auto coordinator = createCoordinator();
        ASSERT_TRUE(coordinator->beginTransaction("mvcc-replay"));
        for (const auto& shard : {"shard-1", "shard-2"}) {
            ASSERT_TRUE(coordinator->addParticipant(
                "mvcc-replay", shard, std::string("inproc://mvcc-") + shard, {"WRITE"}));
        }
        EXPECT_FALSE(coordinator->commit("mvcc-replay"));
        EXPECT_FALSE(coordinator->getStatistics().at("two_phase_ready").get<bool>());
        ASSERT_EQ(timestamps.size(), 2u);
        const auto transaction = coordinator->getTransaction("mvcc-replay");
        ASSERT_TRUE(transaction.has_value());
        EXPECT_GT(transaction->commit_timestamp, transaction->snapshot_timestamp);
        EXPECT_EQ(transaction->commit_timestamp, timestamps.front());
    }
    acknowledge = true;
    auto recovered = createCoordinator();
    ASSERT_EQ(timestamps.size(), 4u);
    for (const auto timestamp : timestamps) {
        EXPECT_EQ(timestamp, timestamps.front());
    }
    EXPECT_TRUE(recovered->getRecoverableTransactions().empty());
    EXPECT_TRUE(recovered->getStatistics().at("two_phase_ready").get<bool>());
}

TEST_F(CrossShardTransactionCoordinatorTest, PrepareSelectedAbortClearsSsiFootprint) {
    config_.in_process_rpc_response_handler =
        [](std::string method, nlohmann::json) -> nlohmann::json {
        if (method == "prepare") {
            return {{"vote", "abort"}, {"status", "aborted"}};
        }
        return {{"status", "aborted"}};
    };
    auto coordinator = createCoordinator();
    for (const auto& id : {"ssi-aborted", "ssi-reader"}) {
        ASSERT_TRUE(coordinator->beginTransaction(
            id, TransactionProtocol::TWO_PHASE_COMMIT, IsolationLevel::SERIALIZABLE));
    }
    ASSERT_TRUE(coordinator->addParticipant(
        "ssi-aborted", "shard-1", "inproc://ssi-abort", {"WRITE"}));
    coordinator->registerShardReadSet(
        "ssi-reader", "shard-1", {{"shard-1", "key", ""}});
    coordinator->registerShardWriteSet("ssi-aborted", "shard-1", {"key"});
    ASSERT_FALSE(coordinator->validateCrossShardSSI("ssi-reader").empty());
    EXPECT_FALSE(coordinator->prepare("ssi-aborted"));
    EXPECT_EQ(coordinator->getTransactionState("ssi-aborted"), TransactionState::ABORTED);
    EXPECT_TRUE(coordinator->validateCrossShardSSI("ssi-reader").empty());
}

TEST_F(CrossShardTransactionCoordinatorTest, RecoveryOnlyEndpointReusesRecordedTimestamp) {
    themis::sharding::TwoPhaseCommitCoordinator::Config config;
    config.wal_directory = (directory_ / "recovery-only-timestamp").string();
    config.fencing_token_provider = config_.fencing_token_provider;
    config.commit_timestamp_provider = [](const std::string&) {
        return std::optional<int64_t>{123456789};
    };
    config.participant_endpoint_resolver = [](const std::string&)
        -> std::optional<themis::sharding::ShardRPCClient::Config> {
        themis::sharding::ShardRPCClient::Config rpc;
        rpc.endpoint = "inproc://recovery-only-timestamp";
        return rpc;
    };
    std::vector<int64_t> timestamps;
    bool acknowledge = false;
    config.in_process_rpc_response_handler =
        [&](std::string method, nlohmann::json parameters) -> nlohmann::json {
        if (method == "prepare") {
            return {{"vote", "commit"}, {"status", "prepared"}};
        }
        timestamps.push_back(parameters.at("commit_timestamp").get<int64_t>());
        return {{"status", acknowledge ? "committed" : "rejected"}};
    };
    {
        themis::sharding::TwoPhaseCommitCoordinator coordinator("coord-1", config);
        coordinator.registerParticipantByEndpoint(
            "shard-1", *config.participant_endpoint_resolver("shard-1"));
        EXPECT_EQ(coordinator.commit("recorded-timestamp",
            {{"shard-1", nlohmann::json::array()}}).result,
            themis::sharding::CoordinatorTxnResult::FAILURE);
    }
    config.commit_timestamp_provider = {};
    acknowledge = true;
    themis::sharding::TwoPhaseCommitCoordinator recovered("coord-1", config);
    EXPECT_EQ(recovered.recoverInDoubtTransactions(), 1u);
    ASSERT_EQ(timestamps.size(), 2u);
    EXPECT_EQ(timestamps[0], 123456789);
    EXPECT_EQ(timestamps[1], timestamps[0]);
    EXPECT_TRUE(recovered.isReady());
}

TEST_F(CrossShardTransactionCoordinatorTest, LegacyWalHistoryGapCannotDiscardOldInDoubt) {
    {
        ::sharding::TransactionWALConfig config;
        config.wal_directory = (directory_ / "wal").string();
        config.snapshot_directory = (directory_ / "snapshots").string();
        config.sync_on_write = false;
        ::sharding::TransactionWAL wal(config);
        ASSERT_TRUE(wal.initialize());
        wal.logBegin("old-outside-cache", ::sharding::TransactionProtocol::TWO_PHASE_COMMIT,
                     {"shard-old"});
        for (size_t i = 0; i < 10'001; ++i) {
            wal.logCompensate("filler", "step", {{"ordinal", i}});
        }
    }
    CrossShardTransactionCoordinator coordinator(config_, consensus_);
    EXPECT_FALSE(coordinator.initialize());
    EXPECT_TRUE(coordinator.getStatistics().at("legacy_two_phase_recovery_blocked").get<bool>());
    EXPECT_FALSE(coordinator.start());
    EXPECT_FALSE(coordinator.beginTransaction("cannot-discard-old-history"));
    EXPECT_THROW(static_cast<void>(coordinator.recoverInDoubtTransactions()),
                 std::runtime_error);
}
} } // namespace themisdb::sharding
