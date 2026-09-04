/*
 * ThemisDB — Unified 2PC Recovery Integration Tests
 *
 * Covers the unified IRecoverableTwoPhaseCoordinator interface and the
 * GlobalTwoPhaseCommitRecoveryManager introduced by issue #5388.
 *
 * Tests:
 *   1. TwoPhaseCommitWALRecovery::reconstruct — shared WAL replay logic
 *   2. GlobalTwoPhaseCommitRecoveryManager::recoverAll — multi-coordinator pass
 *   3. IRecoverableTwoPhaseCoordinator contract on TwoPhaseCommitCoordinator
 *   4. IRecoverableTwoPhaseCoordinator contract on GlobalTransactionManager
 *   5. IRecoverableTwoPhaseCoordinator contract on DistributedTransactionManager
 *   6. getRecoverableTransactions() snapshot accuracy
 */

#include <gtest/gtest.h>

#include "transaction/recoverable_two_phase_coordinator.h"
#include "transaction/two_phase_commit_wal_recovery.h"
#include "sharding/two_phase_commit_coordinator.h"
#include "sharding/two_phase_commit_participant.h"
#include "transaction/global_transaction_manager.h"
#include "transaction/distributed_transaction_manager.h"
#include "sharding/truetime.h"
#include "sharding/wal_manager.h"

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace themis::transaction;
using namespace themis::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// RAII guard that removes a temporary directory when destroyed.
struct TempDirGuard {
    std::string path = {};
    ~TempDirGuard() { std::filesystem::remove_all(path); }
};

static std::string makeTempDir(const std::string& suffix) {
    const auto dir =
        (std::filesystem::temp_directory_path() / ("2pc_unified_test_" + suffix +
            "_" + std::to_string(::getpid()))).string();
    std::filesystem::create_directories(dir);
    return dir;
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. TwoPhaseCommitWALRecovery::reconstruct
// ─────────────────────────────────────────────────────────────────────────────

TEST(TwoPhaseCommitWALRecoveryTest, EmptyEntriesReturnsEmptyMap) {
    const auto result = TwoPhaseCommitWALRecovery::reconstruct({});
    EXPECT_TRUE(result.empty());
}

TEST(TwoPhaseCommitWALRecoveryTest, BeginOnlyIsNotCompleted) {
    std::vector<WALEntry> entries;

    WALEntry begin;
    begin.type           = WALEntryType::BEGIN_TX;
    begin.transaction_id = "txn-1";
    begin.data = {
        {"transaction_id", "txn-1"},
        {"coordinator_id", "coord-A"},
        {"shards",         {"shard-1", "shard-2"}}
    };
    entries.push_back(begin);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    ASSERT_EQ(result.size(), 1u);
    const auto& rec = result.at("txn-1");
    EXPECT_EQ(rec.transaction_id, "txn-1");
    EXPECT_EQ(rec.coordinator_id, "coord-A");
    EXPECT_FALSE(rec.completed);
    EXPECT_FALSE(rec.has_decision);
    ASSERT_EQ(rec.participants.size(), 2u);
    EXPECT_EQ(rec.participants[0], "shard-1");
    EXPECT_EQ(rec.participants[1], "shard-2");
}

TEST(TwoPhaseCommitWALRecoveryTest, CommitDecisionSetsCorrctly) {
    std::vector<WALEntry> entries;

    WALEntry begin;
    begin.type           = WALEntryType::BEGIN_TX;
    begin.transaction_id = "txn-commit";
    begin.data           = {{"coordinator_id", "coord-B"}};
    entries.push_back(begin);

    WALEntry decision;
    decision.type           = WALEntryType::COMMIT_TX;
    decision.transaction_id = "txn-commit";
    decision.data           = {{"phase", "decision"}, {"decision", "commit"}};
    entries.push_back(decision);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    const auto& rec   = result.at("txn-commit");
    EXPECT_TRUE(rec.has_decision);
    EXPECT_TRUE(rec.decision_commit);
    EXPECT_FALSE(rec.completed);
}

TEST(TwoPhaseCommitWALRecoveryTest, AbortDecisionSetsCorrctly) {
    std::vector<WALEntry> entries;

    WALEntry begin;
    begin.type           = WALEntryType::BEGIN_TX;
    begin.transaction_id = "txn-abort";
    begin.data           = {{"coordinator_id", "coord-C"}};
    entries.push_back(begin);

    WALEntry decision;
    decision.type           = WALEntryType::ABORT_TX;
    decision.transaction_id = "txn-abort";
    decision.data           = {{"phase", "decision"}, {"decision", "abort"}};
    entries.push_back(decision);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    const auto& rec   = result.at("txn-abort");
    EXPECT_TRUE(rec.has_decision);
    EXPECT_FALSE(rec.decision_commit);
    EXPECT_FALSE(rec.completed);
}

TEST(TwoPhaseCommitWALRecoveryTest, CompleteEntryMarksCompleted) {
    std::vector<WALEntry> entries;

    WALEntry begin;
    begin.type           = WALEntryType::BEGIN_TX;
    begin.transaction_id = "txn-done";
    begin.data           = {};
    entries.push_back(begin);

    WALEntry decision;
    decision.type           = WALEntryType::COMMIT_TX;
    decision.transaction_id = "txn-done";
    decision.data           = {{"phase", "decision"}, {"decision", "commit"}};
    entries.push_back(decision);

    WALEntry complete;
    complete.type           = WALEntryType::COMMIT_TX;
    complete.transaction_id = "txn-done";
    complete.data           = {{"phase", "complete"}};
    entries.push_back(complete);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    const auto& rec   = result.at("txn-done");
    EXPECT_TRUE(rec.completed);
}

TEST(TwoPhaseCommitWALRecoveryTest, PrepareEntryOnlyIsInDoubt) {
    // DistributedTransactionManager style: PREPARE_TX with no decision.
    std::vector<WALEntry> entries;

    WALEntry begin;
    begin.type           = WALEntryType::BEGIN_TX;
    begin.transaction_id = "txn-dtm";
    begin.data           = {};
    entries.push_back(begin);

    WALEntry prep;
    prep.type           = WALEntryType::PREPARE_TX;
    prep.transaction_id = "txn-dtm";
    prep.data           = {};
    entries.push_back(prep);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    const auto& rec   = result.at("txn-dtm");
    EXPECT_FALSE(rec.has_decision);
    EXPECT_FALSE(rec.completed);
}

TEST(TwoPhaseCommitWALRecoveryTest, EntriesWithEmptyTxnIdAreIgnored) {
    std::vector<WALEntry> entries;

    WALEntry noise;
    noise.type           = WALEntryType::BEGIN_TX;
    noise.transaction_id = "";  // empty → ignored
    noise.data           = {};
    entries.push_back(noise);

    WALEntry real;
    real.type           = WALEntryType::BEGIN_TX;
    real.transaction_id = "txn-real";
    real.data           = {};
    entries.push_back(real);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_TRUE(result.count("txn-real"));
}

TEST(TwoPhaseCommitWALRecoveryTest, RegionsFieldUsedAsParticipants) {
    // GlobalTransactionManager uses "regions" instead of "shards".
    std::vector<WALEntry> entries;

    WALEntry begin;
    begin.type           = WALEntryType::BEGIN_TX;
    begin.transaction_id = "txn-global";
    begin.data           = {
        {"coordinator_id", "global-coord"},
        {"regions",        {"us-east-1", "eu-west-1"}}
    };
    entries.push_back(begin);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    const auto& rec   = result.at("txn-global");
    ASSERT_EQ(rec.participants.size(), 2u);
    EXPECT_EQ(rec.participants[0], "us-east-1");
    EXPECT_EQ(rec.participants[1], "eu-west-1");
}

TEST(TwoPhaseCommitWALRecoveryTest, CommitTimestampExtracted) {
    std::vector<WALEntry> entries;

    WALEntry begin;
    begin.type           = WALEntryType::BEGIN_TX;
    begin.transaction_id = "txn-ts";
    begin.data           = {};
    entries.push_back(begin);

    WALEntry decision;
    decision.type           = WALEntryType::COMMIT_TX;
    decision.transaction_id = "txn-ts";
    decision.data           = {
        {"phase",              "decision"},
        {"decision",           "commit"},
        {"commit_timestamp_ns", int64_t{1234567890LL}}
    };
    entries.push_back(decision);

    const auto result = TwoPhaseCommitWALRecovery::reconstruct(entries);
    const auto& rec   = result.at("txn-ts");
    ASSERT_TRUE(rec.commit_timestamp_ns.has_value());
    EXPECT_EQ(rec.commit_timestamp_ns.value(), 1234567890LL);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. GlobalTwoPhaseCommitRecoveryManager — fake coordinator stubs
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Minimal stub implementing IRecoverableTwoPhaseCoordinator.
class StubRecoverableCoordinator : public IRecoverableTwoPhaseCoordinator {
public:
    StubRecoverableCoordinator(
        std::string name,
        std::string backend,
        size_t      in_doubt_before,
        size_t      will_resolve,
        size_t      in_doubt_after
    )
        : name_(std::move(name))
        , backend_(std::move(backend))
        , in_doubt_before_(in_doubt_before)
        , will_resolve_(will_resolve)
        , in_doubt_after_(in_doubt_after)
    {}

    size_t recoverInDoubtTransactions() override {
        recovered_ = true;
        return will_resolve_;
    }

    std::string recoveryCoordinatorName() const override { return name_; }
    std::string recoveryBackendName()     const override { return backend_; }

    std::vector<RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override {
        const size_t count = recovered_ ? in_doubt_after_ : in_doubt_before_;
        std::vector<RecoverableTwoPhaseTransaction> txns;
        txns.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            RecoverableTwoPhaseTransaction rt;
            rt.transaction_id = name_ + "-txn-" + std::to_string(i);
            rt.state          = RecoverableTwoPhaseState::PREPARED;
            txns.push_back(rt);
        }
        return txns;
    }

    bool wasRecovered() const { return recovered_; }

private:
    std::string name_;
    std::string backend_;
    size_t      in_doubt_before_;
    size_t      will_resolve_;
    size_t      in_doubt_after_;
    bool        recovered_{false};
};

} // namespace

TEST(GlobalTwoPhaseCommitRecoveryManagerTest, EmptyCoordinatorListReturnsZeros) {
    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll({});
    EXPECT_EQ(report.coordinator_count, 0u);
    EXPECT_EQ(report.in_doubt_before, 0u);
    EXPECT_EQ(report.resolved, 0u);
    EXPECT_EQ(report.in_doubt_after, 0u);
    EXPECT_TRUE(report.coordinators.empty());
}

TEST(GlobalTwoPhaseCommitRecoveryManagerTest, NullCoordinatorsAreSkipped) {
    std::vector<IRecoverableTwoPhaseCoordinator*> list = {nullptr, nullptr};
    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll(list);
    EXPECT_EQ(report.coordinator_count, 2u);
    EXPECT_TRUE(report.coordinators.empty());
    EXPECT_EQ(report.in_doubt_before, 0u);
    EXPECT_EQ(report.resolved, 0u);
}

TEST(GlobalTwoPhaseCommitRecoveryManagerTest, SingleCoordinatorAggregatesCorrectly) {
    StubRecoverableCoordinator coord("TestCoord", "WAL", 3, 3, 0);

    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll({&coord});

    EXPECT_EQ(report.coordinator_count, 1u);
    EXPECT_EQ(report.in_doubt_before, 3u);
    EXPECT_EQ(report.resolved, 3u);
    EXPECT_EQ(report.in_doubt_after, 0u);
    ASSERT_EQ(report.coordinators.size(), 1u);

    const auto& cr = report.coordinators[0];
    EXPECT_EQ(cr.coordinator_name, "TestCoord");
    EXPECT_EQ(cr.backend_name, "WAL");
    EXPECT_EQ(cr.in_doubt_before, 3u);
    EXPECT_EQ(cr.resolved, 3u);
    EXPECT_EQ(cr.in_doubt_after, 0u);
    EXPECT_TRUE(coord.wasRecovered());
}

TEST(GlobalTwoPhaseCommitRecoveryManagerTest, MultipleCoordinatorsAggregateCorrectly) {
    StubRecoverableCoordinator c1("Coord-A", "WAL",      2, 2, 0);
    StubRecoverableCoordinator c2("Coord-B", "disabled", 1, 0, 1);
    StubRecoverableCoordinator c3("Coord-C", "WAL",      0, 0, 0);

    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll({&c1, &c2, &c3});

    EXPECT_EQ(report.coordinator_count, 3u);
    EXPECT_EQ(report.in_doubt_before, 3u);  // 2 + 1 + 0
    EXPECT_EQ(report.resolved, 2u);          // 2 + 0 + 0
    EXPECT_EQ(report.in_doubt_after, 1u);   // 0 + 1 + 0
    EXPECT_EQ(report.coordinators.size(), 3u);

    EXPECT_TRUE(c1.wasRecovered());
    EXPECT_TRUE(c2.wasRecovered());
    EXPECT_TRUE(c3.wasRecovered());
}

TEST(GlobalTwoPhaseCommitRecoveryManagerTest, CoordinatorNamesAndBackendsPreserved) {
    StubRecoverableCoordinator c1("AlphaCoord", "WAL/snapshot", 0, 0, 0);
    StubRecoverableCoordinator c2("BetaCoord",  "disabled",     0, 0, 0);

    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll({&c1, &c2});

    ASSERT_EQ(report.coordinators.size(), 2u);
    EXPECT_EQ(report.coordinators[0].coordinator_name, "AlphaCoord");
    EXPECT_EQ(report.coordinators[0].backend_name,     "WAL/snapshot");
    EXPECT_EQ(report.coordinators[1].coordinator_name, "BetaCoord");
    EXPECT_EQ(report.coordinators[1].backend_name,     "disabled");
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. TwoPhaseCommitCoordinator — IRecoverableTwoPhaseCoordinator contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(TwoPhaseCommitCoordinatorRecoverableTest, NameAndBackendWithoutWAL) {
    TwoPhaseCommitCoordinator coord("coord-no-wal");
    EXPECT_EQ(coord.recoveryCoordinatorName(), "TwoPhaseCommitCoordinator");
    EXPECT_EQ(coord.recoveryBackendName(), "disabled");
}

TEST(TwoPhaseCommitCoordinatorRecoverableTest, NameAndBackendWithWAL) {
    const auto wal_dir = makeTempDir("2pc_coord_name");
    TempDirGuard guard{wal_dir};

    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory   = wal_dir;
    cfg.sync_wal_writes = false;

    TwoPhaseCommitCoordinator coord("coord-with-wal", cfg);
    EXPECT_EQ(coord.recoveryCoordinatorName(), "TwoPhaseCommitCoordinator");
    EXPECT_EQ(coord.recoveryBackendName(), "WAL");
}

TEST(TwoPhaseCommitCoordinatorRecoverableTest, GetRecoverableTransactionsEmptyWhenNoTransactions) {
    TwoPhaseCommitCoordinator coord("coord-empty");
    EXPECT_TRUE(coord.getRecoverableTransactions().empty());
}

TEST(TwoPhaseCommitCoordinatorRecoverableTest, PolymorphicRecoveryViaInterface) {
    // Verify that TwoPhaseCommitCoordinator can be used as IRecoverableTwoPhaseCoordinator.
    TwoPhaseCommitCoordinator coord("coord-poly");

    IRecoverableTwoPhaseCoordinator* ptr = &coord;
    EXPECT_EQ(ptr->recoveryCoordinatorName(), "TwoPhaseCommitCoordinator");
    EXPECT_EQ(ptr->recoveryBackendName(), "disabled");
    EXPECT_EQ(ptr->recoverInDoubtTransactions(), 0u);
    EXPECT_TRUE(ptr->getRecoverableTransactions().empty());
}

TEST(TwoPhaseCommitCoordinatorRecoverableTest, RecoverViaGlobalManagerWithCleanState) {
    // A freshly constructed coordinator has no in-doubt transactions.
    TwoPhaseCommitCoordinator coord("coord-global-test");

    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll({&coord});
    EXPECT_EQ(report.in_doubt_before, 0u);
    EXPECT_EQ(report.resolved, 0u);
    EXPECT_EQ(report.in_doubt_after, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. GlobalTransactionManager — IRecoverableTwoPhaseCoordinator contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(GlobalTransactionManagerRecoverableTest, NameAndBackendWithoutWAL) {
    auto tt = std::make_shared<TrueTime>(TrueTime::Config{});
    GlobalTransactionManager gtm("gtm-no-wal", tt);
    EXPECT_EQ(gtm.recoveryCoordinatorName(), "GlobalTransactionManager");
    EXPECT_EQ(gtm.recoveryBackendName(), "disabled");
}

TEST(GlobalTransactionManagerRecoverableTest, NameAndBackendWithWAL) {
    const auto wal_dir = makeTempDir("gtm_wal");
    TempDirGuard guard{wal_dir};

    auto tt = std::make_shared<TrueTime>(TrueTime::Config{});
    GlobalTransactionManager::Config cfg;
    cfg.wal_directory   = wal_dir;
    cfg.sync_wal_writes = false;

    GlobalTransactionManager gtm("gtm-wal", tt, cfg);
    EXPECT_EQ(gtm.recoveryBackendName(), "WAL");
}

TEST(GlobalTransactionManagerRecoverableTest, GetRecoverableTransactionsEmptyInitially) {
    auto tt = std::make_shared<TrueTime>(TrueTime::Config{});
    GlobalTransactionManager gtm("gtm-empty", tt);
    EXPECT_TRUE(gtm.getRecoverableTransactions().empty());
}

TEST(GlobalTransactionManagerRecoverableTest, PolymorphicRecoveryViaInterface) {
    auto tt = std::make_shared<TrueTime>(TrueTime::Config{});
    GlobalTransactionManager gtm("gtm-poly", tt);

    IRecoverableTwoPhaseCoordinator* ptr = &gtm;
    EXPECT_EQ(ptr->recoveryCoordinatorName(), "GlobalTransactionManager");
    EXPECT_EQ(ptr->recoverInDoubtTransactions(), 0u);
    EXPECT_TRUE(ptr->getRecoverableTransactions().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. DistributedTransactionManager — IRecoverableTwoPhaseCoordinator contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(DistributedTransactionManagerRecoverableTest, NameAndBackendWithoutWAL) {
    DistributedTransactionManager dtm("dtm-no-wal");
    EXPECT_EQ(dtm.recoveryCoordinatorName(), "DistributedTransactionManager");
    EXPECT_EQ(dtm.recoveryBackendName(), "disabled");
}

TEST(DistributedTransactionManagerRecoverableTest, NameAndBackendWithWAL) {
    const auto wal_dir = makeTempDir("dtm_wal");
    TempDirGuard guard{wal_dir};

    DistributedTxnManagerConfig cfg;
    cfg.wal_directory = wal_dir;

    DistributedTransactionManager dtm("dtm-wal", cfg);
    EXPECT_EQ(dtm.recoveryBackendName(), "WAL");
}

TEST(DistributedTransactionManagerRecoverableTest, GetRecoverableTransactionsEmptyInitially) {
    DistributedTransactionManager dtm("dtm-empty");
    EXPECT_TRUE(dtm.getRecoverableTransactions().empty());
}

TEST(DistributedTransactionManagerRecoverableTest, PolymorphicRecoveryViaInterface) {
    DistributedTransactionManager dtm("dtm-poly");

    IRecoverableTwoPhaseCoordinator* ptr = &dtm;
    EXPECT_EQ(ptr->recoveryCoordinatorName(), "DistributedTransactionManager");
    EXPECT_EQ(ptr->recoverInDoubtTransactions(), 0u);
    EXPECT_TRUE(ptr->getRecoverableTransactions().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. getRecoverableTransactions snapshot accuracy
//    (TwoPhaseCommitCoordinator, WAL round-trip with committed tx)
// ─────────────────────────────────────────────────────────────────────────────

TEST(RecoverableSnapshotTest, CommittedTransactionNotInRecoverableList) {
    const auto wal_dir = makeTempDir("snapshot_committed");
    TempDirGuard guard{wal_dir};

    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory   = wal_dir;
    cfg.sync_wal_writes = false;

    TwoPhaseCommitCoordinator coord("coord-snap", cfg);

    TwoPhaseCommitParticipant::Config p_cfg;
    p_cfg.wal_directory = "";
    auto participant    = std::make_unique<TwoPhaseCommitParticipant>("shard-snap", p_cfg);
    coord.registerParticipant("shard-snap", participant.get());

    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"key", "snap-k1"}});
    const auto outcome = coord.commit("snap-txn-1", {{"shard-snap", ops}});
    EXPECT_TRUE(outcome.committed());

    // Committed transactions must not appear in the recoverable list.
    const auto recoverable = coord.getRecoverableTransactions();
    EXPECT_TRUE(recoverable.empty());
}

TEST(RecoverableSnapshotTest, MultiCoordinatorAggregationWithActiveTransactions) {
    // Verify that GlobalTwoPhaseCommitRecoveryManager correctly aggregates
    // in-doubt counts across coordinators with and without active transactions.
    StubRecoverableCoordinator c1("C1", "WAL",      5, 5, 0);
    StubRecoverableCoordinator c2("C2", "disabled", 2, 0, 2);

    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll({&c1, &c2});

    EXPECT_EQ(report.in_doubt_before, 7u);  // 5 + 2
    EXPECT_EQ(report.resolved, 5u);          // 5 + 0
    EXPECT_EQ(report.in_doubt_after, 2u);   // 0 + 2
}
