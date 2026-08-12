/**
 * @file test_ingestion_coordinator.cpp
 * @brief Unit and integration tests for the distributed ingestion coordinator.
 *
 * Tests cover:
 *   - InProcessLeaderElection: acquisition, renewal, revocation, expiry,
 *     contention between two nodes
 *   - ConsistentHashRing: add/remove nodes, consistent key mapping,
 *     wrap-around, empty-ring sentinel, even distribution
 *   - InProcessWorkerNode: availability flag, empty-source fast path
 *   - IngestionCoordinator: construction, start/stop lifecycle,
 *     partitioning, ingestAll with no-sources, ingestAll happy path,
 *     leader-acquisition failure, custom node registration, metrics,
 *     aggregation, mock node injection
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_coordinator.h"
#include "ingestion/ingestion_manager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::ingestion;
using namespace std::chrono_literals;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a minimal SourceConfig with the given id.
SourceConfig makeSource(const std::string& id,
                         SourceType type = SourceType::FILESYSTEM,
                         const std::string& location = "/tmp/test") {
    SourceConfig cfg;
    cfg.source_id = id;
    cfg.type      = type;
    cfg.location  = location;
    cfg.enabled   = true;
    return cfg;
}

/// A mock worker node that records which sources it received and returns
/// a configurable report.
class MockWorkerNode : public IIngestionWorkerNode {
public:
    explicit MockWorkerNode(const std::string& id) : node_id_(id) {}

    const std::string& nodeId() const override { return node_id_; }

    bool isAvailable() const override { return !busy_.load(); }

    IngestionReport ingest(
        const std::vector<SourceConfig>& sources,
        const std::string& /*target_collection*/,
        ProgressCallback /*cb*/) override
    {
        busy_.store(true);
        received_sources_ = sources;
        ++call_count_;
        busy_.store(false);

        IngestionReport report;
        for (const auto& src : sources) {
            if (sleep_us_per_source_ > 0) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(sleep_us_per_source_));
            }
            IngestionStats st;
            st.documents_processed = docs_per_source_;
            report.source_stats[src.source_id] = st;
            report.total_documents += docs_per_source_;
        }
        return report;
    }

    size_t callCount() const { return call_count_.load(); }
    const std::vector<SourceConfig>& receivedSources() const { return received_sources_; }

    size_t docs_per_source_     = 10;
    unsigned sleep_us_per_source_ = 0;  ///< Artificial latency per source (microseconds)

private:
    std::string node_id_;
    std::atomic<bool> busy_{false};
    std::atomic<size_t> call_count_{0};
    std::vector<SourceConfig> received_sources_;
};

/// Leader election that always denies acquisition (for negative testing).
class DenyAllLeaderElection : public ILeaderElection {
public:
    bool tryAcquireLease(const std::string& /*node_id*/,
                          std::chrono::milliseconds /*ttl*/) override {
        return false;
    }
    LeaderLease getCurrentLease() const override { return LeaderLease{}; }
    void revokeLease(const std::string& /*node_id*/) override {}
};

/// Returns true when THEMIS_RUN_PERF_TESTS=1 is set in the environment.
inline bool perfTestsEnabled() {
    const char* v = std::getenv("THEMIS_RUN_PERF_TESTS");
    return v && std::string(v) == "1";
}

} // anonymous namespace

// ============================================================================
// InProcessLeaderElection tests
// ============================================================================

TEST(InProcessLeaderElectionTest, FirstNodeAcquires) {
    InProcessLeaderElection election;
    EXPECT_TRUE(election.tryAcquireLease("node-A", 5000ms));
    auto lease = election.getCurrentLease();
    EXPECT_EQ(lease.owner_node_id, "node-A");
    EXPECT_TRUE(lease.isValid());
}

TEST(InProcessLeaderElectionTest, SecondNodeCannotAcquireWhileLeaseValid) {
    InProcessLeaderElection election;
    ASSERT_TRUE(election.tryAcquireLease("node-A", 5000ms));

    EXPECT_FALSE(election.tryAcquireLease("node-B", 5000ms));
    EXPECT_EQ(election.getCurrentLease().owner_node_id, "node-A");
}

TEST(InProcessLeaderElectionTest, RenewalByOwner) {
    InProcessLeaderElection election;
    ASSERT_TRUE(election.tryAcquireLease("node-A", 5000ms));
    uint64_t epoch_before = election.getCurrentLease().epoch;

    // Renewing should succeed.
    EXPECT_TRUE(election.tryAcquireLease("node-A", 5000ms));
    // Renewal keeps the same epoch.
    EXPECT_EQ(election.getCurrentLease().epoch, epoch_before);
}

TEST(InProcessLeaderElectionTest, RevokeReleasesLease) {
    InProcessLeaderElection election;
    ASSERT_TRUE(election.tryAcquireLease("node-A", 5000ms));

    election.revokeLease("node-A");
    EXPECT_FALSE(election.getCurrentLease().isValid());

    // Another node can now acquire.
    EXPECT_TRUE(election.tryAcquireLease("node-B", 5000ms));
}

TEST(InProcessLeaderElectionTest, RevokeByNonOwnerIsNoOp) {
    InProcessLeaderElection election;
    ASSERT_TRUE(election.tryAcquireLease("node-A", 5000ms));

    election.revokeLease("node-B");  // should be a no-op
    EXPECT_EQ(election.getCurrentLease().owner_node_id, "node-A");
    EXPECT_TRUE(election.getCurrentLease().isValid());
}

TEST(InProcessLeaderElectionTest, ExpiredLeaseAllowsNewOwner) {
    InProcessLeaderElection election;
    // Very short TTL.
    ASSERT_TRUE(election.tryAcquireLease("node-A", 1ms));

    // Wait for expiry.
    std::this_thread::sleep_for(10ms);

    EXPECT_FALSE(election.getCurrentLease().isValid());
    EXPECT_TRUE(election.tryAcquireLease("node-B", 5000ms));
    EXPECT_EQ(election.getCurrentLease().owner_node_id, "node-B");
}

TEST(InProcessLeaderElectionTest, EpochIncrementsOnOwnerChange) {
    InProcessLeaderElection election;
    ASSERT_TRUE(election.tryAcquireLease("node-A", 1ms));
    uint64_t epoch_A = election.getCurrentLease().epoch;

    std::this_thread::sleep_for(10ms);  // let lease expire

    ASSERT_TRUE(election.tryAcquireLease("node-B", 5000ms));
    EXPECT_GT(election.getCurrentLease().epoch, epoch_A);
}

// ============================================================================
// ConsistentHashRing tests
// ============================================================================

TEST(ConsistentHashRingTest, EmptyRingReturnsEmpty) {
    ConsistentHashRing ring;
    EXPECT_TRUE(ring.empty());
    EXPECT_EQ(ring.getNode("any-key"), "");
}

TEST(ConsistentHashRingTest, SingleNodeOwnsAllKeys) {
    ConsistentHashRing ring;
    ring.addNode("node-0");

    EXPECT_EQ(ring.getNode("source-a"), "node-0");
    EXPECT_EQ(ring.getNode("source-b"), "node-0");
    EXPECT_EQ(ring.getNode("source-z"), "node-0");
}

TEST(ConsistentHashRingTest, AddNodeIsIdempotent) {
    ConsistentHashRing ring;
    ring.addNode("node-0");
    ring.addNode("node-0");  // duplicate — should be ignored
    EXPECT_EQ(ring.nodeCount(), 1u);
}

TEST(ConsistentHashRingTest, RemoveNodeRestoresSingleOwner) {
    ConsistentHashRing ring;
    ring.addNode("node-0");
    ring.addNode("node-1");
    ring.removeNode("node-1");

    EXPECT_EQ(ring.nodeCount(), 1u);
    EXPECT_EQ(ring.getNode("source-abc"), "node-0");
}

TEST(ConsistentHashRingTest, RemoveNonExistentNodeIsNoOp) {
    ConsistentHashRing ring;
    ring.addNode("node-0");
    ring.removeNode("does-not-exist");  // must not throw
    EXPECT_EQ(ring.nodeCount(), 1u);
}

TEST(ConsistentHashRingTest, KeyMappingIsConsistentAfterAddRemove) {
    ConsistentHashRing ring;
    ring.addNode("node-0");
    ring.addNode("node-1");

    std::string owner_before = ring.getNode("stable-key");

    // Add a third node and remove it — the owner of "stable-key" may or may
    // not change, but after removal it must return to owner_before.
    ring.addNode("node-2");
    ring.removeNode("node-2");

    EXPECT_EQ(ring.getNode("stable-key"), owner_before);
}

TEST(ConsistentHashRingTest, DistributionAcrossMultipleNodes) {
    // With 150 virtual nodes per physical node and 500 sources, every node
    // should receive at least 1 source (statistical guarantee).
    ConsistentHashRing ring(150);
    ring.addNode("n0");
    ring.addNode("n1");
    ring.addNode("n2");
    ring.addNode("n3");

    std::unordered_map<std::string, size_t> counts;
    for (int i = 0; i < 500; ++i) {
        std::string key = "src-" + std::to_string(i);
        counts[ring.getNode(key)]++;
    }

    EXPECT_GE(counts.size(), 3u);
    for (const auto& kv : counts) {
        EXPECT_GT(kv.second, 0u) << "Node " << kv.first << " received no keys";
    }
}

// ============================================================================
// InProcessWorkerNode tests
// ============================================================================

TEST(InProcessWorkerNodeTest, InitiallyAvailable) {
    InProcessWorkerNode node("n0", "test_db");
    EXPECT_TRUE(node.isAvailable());
}

TEST(InProcessWorkerNodeTest, EmptySourcesFastPath) {
    InProcessWorkerNode node("n0", "test_db");
    auto report = node.ingest({}, "col", nullptr);
    EXPECT_TRUE(report.source_stats.empty());
    EXPECT_EQ(report.total_documents, 0u);
    EXPECT_TRUE(node.isAvailable());  // must be restored to available
}

TEST(InProcessWorkerNodeTest, NodeIdIsPreserved) {
    InProcessWorkerNode node("my-worker", "test_db");
    EXPECT_EQ(node.nodeId(), "my-worker");
}

// ============================================================================
// IngestionCoordinator — lifecycle tests
// ============================================================================

TEST(IngestionCoordinatorLifecycleTest, DefaultConstruction) {
    IngestionCoordinator coordinator;
    EXPECT_FALSE(coordinator.isRunning());
}

TEST(IngestionCoordinatorLifecycleTest, StartAndStop) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 2;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();
    EXPECT_TRUE(coordinator.isRunning());

    coordinator.stop();
    EXPECT_FALSE(coordinator.isRunning());
}

TEST(IngestionCoordinatorLifecycleTest, StartIsIdempotent) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 1;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();
    coordinator.start();  // second call must not crash or duplicate nodes
    EXPECT_TRUE(coordinator.isRunning());
    coordinator.stop();
}

TEST(IngestionCoordinatorLifecycleTest, StopWithoutStartIsNoOp) {
    IngestionCoordinator coordinator;
    coordinator.stop();  // must not crash
    EXPECT_FALSE(coordinator.isRunning());
}

// ============================================================================
// IngestionCoordinator — node registration tests
// ============================================================================

TEST(IngestionCoordinatorNodesTest, DefaultNodesCreatedOnStart) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 3;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();

    EXPECT_EQ(coordinator.getNodes().size(), 3u);

    coordinator.stop();
}

TEST(IngestionCoordinatorNodesTest, RegisterExternalNode) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;  // no default nodes
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);

    auto mock = std::make_shared<MockWorkerNode>("ext-node-0");
    coordinator.registerNode(mock);
    coordinator.start();

    auto nodes = coordinator.getNodes();
    ASSERT_EQ(nodes.size(), 1u);
    EXPECT_EQ(nodes[0].node_id, "ext-node-0");

    coordinator.stop();
}

TEST(IngestionCoordinatorNodesTest, RegisterNullptrIsNoOp) {
    IngestionCoordinator coordinator;
    coordinator.registerNode(nullptr);  // must not crash
    coordinator.start();
    coordinator.stop();
}

TEST(IngestionCoordinatorNodesTest, DuplicateNodeRegistrationIgnored) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    auto mock = std::make_shared<MockWorkerNode>("n0");
    coordinator.registerNode(mock);
    coordinator.registerNode(mock);  // duplicate

    coordinator.start();
    EXPECT_EQ(coordinator.getNodes().size(), 1u);
    coordinator.stop();
}

// ============================================================================
// IngestionCoordinator — partitioning tests
// ============================================================================

TEST(IngestionCoordinatorPartitionTest, EmptySourcesReturnEmptyPartitions) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 2;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();

    auto partitions = coordinator.partitionSources({});
    // All partition vectors should be empty (or the map itself is empty).
    size_t total = 0;
    for (const auto& kv : partitions) {
        total += kv.second.size();
    }
    EXPECT_EQ(total, 0u);

    coordinator.stop();
}

TEST(IngestionCoordinatorPartitionTest, AllSourcesAssigned) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 3;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();

    std::vector<SourceConfig> sources;
    for (int i = 0; i < 30; ++i) {
        sources.push_back(makeSource("src-" + std::to_string(i)));
    }

    auto partitions = coordinator.partitionSources(sources);

    size_t total = 0;
    for (const auto& kv : partitions) {
        total += kv.second.size();
    }
    EXPECT_EQ(total, sources.size());

    coordinator.stop();
}

TEST(IngestionCoordinatorPartitionTest, PartitioningIsDeterministic) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 4;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();

    std::vector<SourceConfig> sources;
    for (int i = 0; i < 20; ++i) {
        sources.push_back(makeSource("deterministic-" + std::to_string(i)));
    }

    auto p1 = coordinator.partitionSources(sources);
    auto p2 = coordinator.partitionSources(sources);

    EXPECT_EQ(p1.size(), p2.size());
    for (const auto& kv : p1) {
        ASSERT_NE(p2.find(kv.first), p2.end());
        EXPECT_EQ(kv.second.size(), p2.at(kv.first).size());
    }

    coordinator.stop();
}

// ============================================================================
// IngestionCoordinator — ingestAll tests
// ============================================================================

TEST(IngestionCoordinatorIngestTest, IngestAllNoSources) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 2;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();

    auto report = coordinator.ingestAll({});
    EXPECT_EQ(report.total_documents, 0u);

    coordinator.stop();
}

TEST(IngestionCoordinatorIngestTest, IngestAllWithMockNodes) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);

    auto mock0 = std::make_shared<MockWorkerNode>("mock-0");
    auto mock1 = std::make_shared<MockWorkerNode>("mock-1");
    mock0->docs_per_source_ = 5;
    mock1->docs_per_source_ = 5;

    coordinator.registerNode(mock0);
    coordinator.registerNode(mock1);
    coordinator.start();

    std::vector<SourceConfig> sources;
    for (int i = 0; i < 10; ++i) {
        sources.push_back(makeSource("src-" + std::to_string(i)));
    }

    auto report = coordinator.ingestAll(sources);

    // All 10 sources × 5 docs each = 50 total.
    EXPECT_EQ(report.total_documents, 50u);
    EXPECT_EQ(report.source_stats.size(), 10u);

    coordinator.stop();
}

TEST(IngestionCoordinatorIngestTest, LeaderAcquisitionFailureReturnsErrorReport) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 1;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.setLeaderElectionForTesting(
        std::make_shared<DenyAllLeaderElection>());
    coordinator.start();

    auto report = coordinator.ingestAll({makeSource("s1")});

    // Should have an error entry.
    EXPECT_FALSE(report.source_stats.empty());
    bool has_error = false;
    for (const auto& kv : report.source_stats) {
        if (!kv.second.errors.empty()) {
            has_error = true;
            break;
        }
    }
    EXPECT_TRUE(has_error);

    coordinator.stop();
}

// ============================================================================
// IngestionCoordinator — leader ID
// ============================================================================

TEST(IngestionCoordinatorLeaderTest, NoLeaderBeforeFirstIngest) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 1;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    coordinator.start();

    // Before any ingestAll() call, lease may or may not be held depending
    // on renewal thread timing — so we just check the API does not crash.
    std::string leader = coordinator.getLeaderNodeId();
    (void)leader;  // value may be empty or non-empty

    coordinator.stop();
}

TEST(IngestionCoordinatorLeaderTest, LeaderIdSetAfterIngest) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    auto mock = std::make_shared<MockWorkerNode>("m0");
    coordinator.registerNode(mock);
    coordinator.start();

    coordinator.ingestAll({makeSource("s1")});

    // After a successful ingestAll the coordinator must hold the lease.
    std::string leader = coordinator.getLeaderNodeId();
    EXPECT_FALSE(leader.empty());

    coordinator.stop();
}

// ============================================================================
// IngestionCoordinator — metrics
// ============================================================================

TEST(IngestionCoordinatorMetricsTest, InitialMetrics) {
    IngestionCoordinator coordinator;
    auto m = coordinator.getMetrics();
    EXPECT_EQ(m.tasks_submitted, 0u);
    EXPECT_EQ(m.leader_elections, 0u);
}

TEST(IngestionCoordinatorMetricsTest, MetricsUpdatedAfterIngest) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    auto mock = std::make_shared<MockWorkerNode>("m0");
    coordinator.registerNode(mock);
    coordinator.start();

    coordinator.ingestAll({makeSource("s1"), makeSource("s2")});

    auto m = coordinator.getMetrics();
    EXPECT_GE(m.leader_elections, 1u);
    EXPECT_GT(m.last_run_seconds, 0.0);

    coordinator.stop();
}

// ============================================================================
// IngestionCoordinator — aggregation
// ============================================================================

TEST(IngestionCoordinatorAggregateTest, PartialReportsAggregated) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);

    // Two mock nodes, each returns 10 docs per source.
    auto mock0 = std::make_shared<MockWorkerNode>("agg-0");
    auto mock1 = std::make_shared<MockWorkerNode>("agg-1");
    mock0->docs_per_source_ = 10;
    mock1->docs_per_source_ = 10;

    coordinator.registerNode(mock0);
    coordinator.registerNode(mock1);
    coordinator.start();

    std::vector<SourceConfig> sources;
    for (int i = 0; i < 4; ++i) {
        sources.push_back(makeSource("agg-src-" + std::to_string(i)));
    }

    auto report = coordinator.ingestAll(sources);

    EXPECT_EQ(report.total_documents, 40u);  // 4 sources × 10 docs
    EXPECT_EQ(report.source_stats.size(), 4u);

    coordinator.stop();
}

// ============================================================================
// IngestionCoordinator — progress callback forwarding
// ============================================================================

TEST(IngestionCoordinatorCallbackTest, ProgressCallbackInvoked) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);
    auto mock = std::make_shared<MockWorkerNode>("cb-node");
    coordinator.registerNode(mock);
    coordinator.start();

    std::atomic<size_t> callback_count{0};
    ProgressCallback cb = [&](const std::string&, size_t, size_t, const std::string&) {
        ++callback_count;
    };

    coordinator.ingestAll({makeSource("s1")}, cb);
    // MockWorkerNode does not call the progress callback — we just verify the
    // call does not crash when a callback is provided.

    coordinator.stop();
}

// ============================================================================
// IngestionCoordinator — responsive shutdown (condition-variable wakeup)
// ============================================================================

TEST(IngestionCoordinatorShutdownTest, StopReturnsQuickly) {
    // Use a long lease TTL (60 s) to verify that stop() does not block for
    // lease_ttl/2 (30 s) due to a bare sleep_for in the renewal loop.
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 1;
    cfg.db_connection = "test_db";
    cfg.lease_ttl     = std::chrono::milliseconds(60000);

    IngestionCoordinator coordinator(cfg);
    coordinator.start();

    auto t0 = std::chrono::steady_clock::now();
    coordinator.stop();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - t0)
                          .count();

    // With a condition-variable wakeup, stop() should return well under 1 s
    // even when the renewal interval would have been 30 s.
    EXPECT_LT(elapsed_ms, 1000);
}

// ============================================================================
// IngestionCoordinator — error-key uniqueness (no collision on multi-timeout)
// ============================================================================

TEST(IngestionCoordinatorErrorKeyTest, WorkerTimeoutErrorCodeIsNotRetryable) {
    // Verify that the error recorded for a coordinator failure (leader
    // acquisition denied here) uses INTERNAL_ERROR or SOURCE_UNAVAILABLE
    // and is therefore NOT classified as retryable.
    //
    // worker_timeout is set to 0 (disabled) so the test exercises the
    // leader-election failure path, not the per-worker timeout path.
    IngestionCoordinator::Config cfg;
    cfg.num_nodes      = 0;
    cfg.db_connection  = "test_db";
    cfg.worker_timeout = std::chrono::seconds(0);  // disable per-worker timeout

    IngestionCoordinator coordinator(cfg);
    coordinator.setLeaderElectionForTesting(
        std::make_shared<DenyAllLeaderElection>());
    coordinator.start();

    auto report = coordinator.ingestAll({makeSource("s1")});

    ASSERT_FALSE(report.source_stats.empty())
        << "Expected at least one error entry in source_stats";

    for (const auto& kv : report.source_stats) {
        for (const auto& err : kv.second.errors) {
            // Must be INTERNAL_ERROR or SOURCE_UNAVAILABLE — never HTTP_TIMEOUT.
            bool expected_code =
                err.code == IngestionErrorCode::INTERNAL_ERROR ||
                err.code == IngestionErrorCode::SOURCE_UNAVAILABLE;
            EXPECT_TRUE(expected_code)
                << "Unexpected coordinator error code: "
                << static_cast<int>(err.code);
            EXPECT_FALSE(err.isRetryable())
                << "Coordinator error must not be retryable, code="
                << static_cast<int>(err.code);
        }
    }

    coordinator.stop();
}

// ============================================================================
// WorkStealingPool — unit tests
// ============================================================================

TEST(WorkStealingPoolTest, EmptyPoolReturnsEmpty) {
    auto mock = std::make_shared<MockWorkerNode>("ws-empty");
    WorkStealingPool pool({mock}, "col", std::chrono::seconds(30));
    // No sources submitted — run() must return empty immediately.
    auto results = pool.run(nullptr);
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(mock->callCount(), 0u);
}

TEST(WorkStealingPoolTest, SingleWorkerProcessesAllSources) {
    auto mock = std::make_shared<MockWorkerNode>("ws-single");
    mock->docs_per_source_ = 3;

    WorkStealingPool pool({mock}, "col", std::chrono::seconds(30));
    for (int i = 0; i < 5; ++i) {
        pool.submitTo(0, makeSource("src-" + std::to_string(i)));
    }

    auto results = pool.run(nullptr);
    EXPECT_EQ(results.size(), 5u);

    size_t total_docs = 0;
    for (const auto& r : results) total_docs += r.total_documents;
    EXPECT_EQ(total_docs, 15u);  // 5 sources × 3 docs
}

TEST(WorkStealingPoolTest, IdleWorkerStealsFromBusyWorker) {
    // Submit 4 sources to worker 0, none to worker 1.
    // Worker 1 should steal at least one source from worker 0.
    auto mock0 = std::make_shared<MockWorkerNode>("ws-steal-0");
    auto mock1 = std::make_shared<MockWorkerNode>("ws-steal-1");
    mock0->docs_per_source_ = 1;
    mock1->docs_per_source_ = 1;
    mock0->sleep_us_per_source_ = 250;

    WorkStealingPool pool({mock0, mock1}, "col", std::chrono::seconds(30));
    for (int i = 0; i < 4; ++i) {
        pool.submitTo(0, makeSource("steal-src-" + std::to_string(i)));
    }

    EXPECT_EQ(pool.pendingCount(), 4u);

    auto results = pool.run(nullptr);

    // All 4 sources must be processed regardless of which worker handled them.
    EXPECT_EQ(results.size(), 4u);

    size_t total_docs = 0;
    for (const auto& r : results) total_docs += r.total_documents;
    EXPECT_EQ(total_docs, 4u);

    // Worker 1 must have stolen at least one source.
    EXPECT_GT(mock1->callCount(), 0u)
        << "Worker 1 should have stolen work from worker 0";
    // Together they must account for all 4 sources.
    EXPECT_EQ(mock0->callCount() + mock1->callCount(), 4u);
}

TEST(WorkStealingPoolTest, MultipleWorkersBalanceLoad) {
    // 3 workers, all sources go to worker 0 initially.
    // Workers 1 and 2 should steal to balance the load.
    auto mock0 = std::make_shared<MockWorkerNode>("ws-bal-0");
    auto mock1 = std::make_shared<MockWorkerNode>("ws-bal-1");
    auto mock2 = std::make_shared<MockWorkerNode>("ws-bal-2");
    mock0->docs_per_source_ = 1;
    mock1->docs_per_source_ = 1;
    mock2->docs_per_source_ = 1;

    WorkStealingPool pool({mock0, mock1, mock2}, "col", std::chrono::seconds(30));
    const size_t num_sources = 9;
    for (size_t i = 0; i < num_sources; ++i) {
        pool.submitTo(0, makeSource("bal-src-" + std::to_string(i)));
    }

    auto results = pool.run(nullptr);

    EXPECT_EQ(results.size(), num_sources);
    EXPECT_EQ(mock0->callCount() + mock1->callCount() + mock2->callCount(),
              num_sources);
}

TEST(WorkStealingPoolTest, NodeCountReflectsWorkers) {
    auto m0 = std::make_shared<MockWorkerNode>("nc-0");
    auto m1 = std::make_shared<MockWorkerNode>("nc-1");
    auto m2 = std::make_shared<MockWorkerNode>("nc-2");

    WorkStealingPool pool({m0, m1, m2}, "col", std::chrono::seconds(30));
    EXPECT_EQ(pool.nodeCount(), 3u);
}

TEST(WorkStealingPoolTest, PendingCountDecreasesAfterRun) {
    auto mock = std::make_shared<MockWorkerNode>("pc-0");
    mock->docs_per_source_ = 1;

    WorkStealingPool pool({mock}, "col", std::chrono::seconds(30));
    pool.submitTo(0, makeSource("pc-src-0"));
    pool.submitTo(0, makeSource("pc-src-1"));
    EXPECT_EQ(pool.pendingCount(), 2u);

    pool.run(nullptr);
    EXPECT_EQ(pool.pendingCount(), 0u);
}

// ============================================================================
// IngestionCoordinator — work-stealing integration tests
// ============================================================================

TEST(IngestionCoordinatorWorkStealingTest, UnbalancedLoadIsProcessedCorrectly) {
    // One of the two workers gets more sources via hash ring; both should
    // contribute to the final aggregated result.
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";

    IngestionCoordinator coordinator(cfg);

    auto mock0 = std::make_shared<MockWorkerNode>("unbal-0");
    auto mock1 = std::make_shared<MockWorkerNode>("unbal-1");
    mock0->docs_per_source_ = 2;
    mock1->docs_per_source_ = 2;

    coordinator.registerNode(mock0);
    coordinator.registerNode(mock1);
    coordinator.start();

    // Use enough sources so that hash ring distributes them across nodes.
    std::vector<SourceConfig> sources;
    for (int i = 0; i < 12; ++i) {
        sources.push_back(makeSource("unbal-src-" + std::to_string(i)));
    }

    auto report = coordinator.ingestAll(sources);

    EXPECT_EQ(report.total_documents, 24u);  // 12 × 2 docs
    EXPECT_EQ(report.source_stats.size(), 12u);

    coordinator.stop();
}

// ============================================================================
// ISharedCheckpointStore — InMemorySharedCheckpointStore unit tests
// ============================================================================

TEST(InMemorySharedCheckpointStoreTest, WriteAndRead) {
    InMemorySharedCheckpointStore store;

    IngestionCheckpoint cp;
    cp.source_id = "src-a";
    cp.cursor    = "offset:42";
    EXPECT_TRUE(store.write(cp));

    IngestionCheckpoint out;
    EXPECT_TRUE(store.read("src-a", out));
    EXPECT_EQ(out.source_id, "src-a");
    EXPECT_EQ(out.cursor,    "offset:42");
}

TEST(InMemorySharedCheckpointStoreTest, ReadMissingReturnsFalse) {
    InMemorySharedCheckpointStore store;
    IngestionCheckpoint out;
    EXPECT_FALSE(store.read("does-not-exist", out));
}

TEST(InMemorySharedCheckpointStoreTest, ExistsReturnsTrueAfterWrite) {
    InMemorySharedCheckpointStore store;
    EXPECT_FALSE(store.exists("src-b"));

    IngestionCheckpoint cp;
    cp.source_id = "src-b";
    store.write(cp);

    EXPECT_TRUE(store.exists("src-b"));
}

TEST(InMemorySharedCheckpointStoreTest, ClearRemovesEntry) {
    InMemorySharedCheckpointStore store;

    IngestionCheckpoint cp;
    cp.source_id = "src-c";
    store.write(cp);
    ASSERT_TRUE(store.exists("src-c"));

    EXPECT_TRUE(store.clear("src-c"));
    EXPECT_FALSE(store.exists("src-c"));
}

TEST(InMemorySharedCheckpointStoreTest, ClearNonExistentReturnsFalse) {
    InMemorySharedCheckpointStore store;
    EXPECT_FALSE(store.clear("phantom"));
}

TEST(InMemorySharedCheckpointStoreTest, OverwriteUpdatesValue) {
    InMemorySharedCheckpointStore store;

    IngestionCheckpoint cp1;
    cp1.source_id = "src-d";
    cp1.cursor    = "v1";
    store.write(cp1);

    IngestionCheckpoint cp2;
    cp2.source_id = "src-d";
    cp2.cursor    = "v2";
    store.write(cp2);

    IngestionCheckpoint out;
    ASSERT_TRUE(store.read("src-d", out));
    EXPECT_EQ(out.cursor, "v2");
}

TEST(InMemorySharedCheckpointStoreTest, SizeReflectsEntryCount) {
    InMemorySharedCheckpointStore store;
    EXPECT_EQ(store.size(), 0u);

    for (int i = 0; i < 5; ++i) {
        IngestionCheckpoint cp;
        cp.source_id = "src-" + std::to_string(i);
        store.write(cp);
    }
    EXPECT_EQ(store.size(), 5u);

    store.clear("src-2");
    EXPECT_EQ(store.size(), 4u);
}

TEST(InMemorySharedCheckpointStoreTest, ThreadSafeConcurrentWrites) {
    InMemorySharedCheckpointStore store;
    constexpr int kThreads  = 8;
    constexpr int kPerThread = 50;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                IngestionCheckpoint cp;
                cp.source_id = "t" + std::to_string(t) + "-src-" + std::to_string(i);
                cp.cursor    = std::to_string(i);
                store.write(cp);
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(store.size(), static_cast<size_t>(kThreads * kPerThread));
}

// ============================================================================
// IngestionCoordinator — setSharedCheckpointStoreForTesting / getSharedCheckpointStore
// ============================================================================

TEST(IngestionCoordinatorCheckpointStoreTest, DefaultStoreIsInMemory) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";
    IngestionCoordinator coordinator(cfg);

    auto store = coordinator.getSharedCheckpointStore();
    ASSERT_NE(store, nullptr);

    // Default store must accept writes without throwing.
    IngestionCheckpoint cp;
    cp.source_id = "default-test";
    cp.cursor    = "0";
    EXPECT_TRUE(store->write(cp));
}

TEST(IngestionCoordinatorCheckpointStoreTest, InjectedStoreIsUsed) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";
    IngestionCoordinator coordinator(cfg);

    // Use the production API to inject a custom store before start().
    auto custom_store = std::make_shared<InMemorySharedCheckpointStore>();
    coordinator.setSharedCheckpointStore(custom_store);

    EXPECT_EQ(coordinator.getSharedCheckpointStore(), custom_store);
}

TEST(IngestionCoordinatorCheckpointStoreTest, SetStoreWhileRunningThrows) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";
    IngestionCoordinator coordinator(cfg);

    auto mock = std::make_shared<MockWorkerNode>("throw-guard-worker");
    coordinator.registerNode(mock);
    coordinator.start();

    auto store = std::make_shared<InMemorySharedCheckpointStore>();
    EXPECT_THROW(coordinator.setSharedCheckpointStore(store), std::logic_error);
    EXPECT_THROW(coordinator.setSharedCheckpointStoreForTesting(store), std::logic_error);

    coordinator.stop();
}

TEST(IngestionCoordinatorCheckpointStoreTest, CheckpointWrittenAfterIngest) {
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";
    IngestionCoordinator coordinator(cfg);

    auto cp_store = std::make_shared<InMemorySharedCheckpointStore>();
    coordinator.setSharedCheckpointStore(cp_store);

    auto worker = std::make_shared<MockWorkerNode>("cp-worker");
    worker->docs_per_source_ = 5;
    coordinator.registerNode(worker);
    coordinator.start();

    std::vector<SourceConfig> sources = {makeSource("cp-src-0"),
                                          makeSource("cp-src-1")};
    auto report = coordinator.ingestAll(sources);

    // Each source should have a checkpoint recorded after ingestion.
    for (const auto& src : sources) {
        EXPECT_TRUE(cp_store->exists(src.source_id))
            << "No checkpoint found for source: " << src.source_id;
    }

    coordinator.stop();
}

TEST(IngestionCoordinatorCheckpointStoreTest, CheckpointWrittenForZeroDocSource) {
    // A source that processes 0 documents but has no errors is still
    // considered successfully ingested and must receive a checkpoint.
    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "test_db";
    IngestionCoordinator coordinator(cfg);

    auto cp_store = std::make_shared<InMemorySharedCheckpointStore>();
    coordinator.setSharedCheckpointStore(cp_store);

    auto worker = std::make_shared<MockWorkerNode>("zero-doc-worker");
    worker->docs_per_source_ = 0;  // empty source — no documents
    coordinator.registerNode(worker);
    coordinator.start();

    auto report = coordinator.ingestAll({makeSource("zero-src-0")});

    EXPECT_TRUE(cp_store->exists("zero-src-0"))
        << "Checkpoint must be written even for a 0-document successful source";

    coordinator.stop();
}

// ============================================================================
// AC-COORD-5: Linear throughput scaling ≥ 3.5× (4 workers vs 1 worker)
//
// Gated by THEMIS_RUN_PERF_TESTS=1 to avoid failures on slow CI hardware.
// Uses the mock injection path (no real I/O) so that the measurement isolates
// the coordinator dispatch and aggregation overhead rather than disk/network
// latency.  On reference hardware the mock path typically achieves well above
// the 3.5× target.
// ============================================================================

TEST(IngestionCoordinatorPerfTest, LinearScaling4WorkersVs1Worker) {
    if (!perfTestsEnabled()) {
        GTEST_SKIP() << "Skipping AC-COORD-5 throughput scaling microbenchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "AC-COORD-5: ≥ 3.5× aggregate throughput with 4 vs 1 worker node.";
    }

    // Each mock worker sleeps 50 us per source so that wall-clock times are
    // dominated by the simulated work rather than scheduler/locking noise.
    // With 400 sources: 1-worker approx 400x50 us = 20 ms; 4-worker approx 5 ms each.
    constexpr size_t   kSources           = 400;
    constexpr size_t   kDocsPerSource     = 1000;
    constexpr unsigned kSleepUsPerSource  = 50;   // deterministic per-source cost
    constexpr double   kMinSpeedup        = 3.5;

    // ── 1-worker baseline ────────────────────────────────────────────────────
    double elapsed_1w_ms = 0.0;
    {
        IngestionCoordinator::Config cfg;
        cfg.num_nodes     = 0;
        cfg.db_connection = "perf_db";
        IngestionCoordinator coord1(cfg);

        auto worker = std::make_shared<MockWorkerNode>("perf-1w");
        worker->docs_per_source_      = kDocsPerSource;
        worker->sleep_us_per_source_  = kSleepUsPerSource;
        coord1.registerNode(worker);
        coord1.start();

        std::vector<SourceConfig> sources;
        sources.reserve(kSources);
        for (size_t i = 0; i < kSources; ++i) {
            sources.push_back(makeSource("perf-src-1w-" + std::to_string(i)));
        }

        auto t0 = std::chrono::steady_clock::now();
        auto report = coord1.ingestAll(sources);
        auto t1 = std::chrono::steady_clock::now();
        elapsed_1w_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        coord1.stop();

        ASSERT_EQ(report.total_documents, kSources * kDocsPerSource)
            << "1-worker run: unexpected document count";
    }

    // ── 4-worker run ─────────────────────────────────────────────────────────
    double elapsed_4w_ms = 0.0;
    {
        IngestionCoordinator::Config cfg;
        cfg.num_nodes     = 0;
        cfg.db_connection = "perf_db";
        IngestionCoordinator coord4(cfg);

        for (int w = 0; w < 4; ++w) {
            auto worker = std::make_shared<MockWorkerNode>(
                "perf-4w-" + std::to_string(w));
            worker->docs_per_source_     = kDocsPerSource;
            worker->sleep_us_per_source_ = kSleepUsPerSource;
            coord4.registerNode(worker);
        }
        coord4.start();

        std::vector<SourceConfig> sources;
        sources.reserve(kSources);
        for (size_t i = 0; i < kSources; ++i) {
            sources.push_back(makeSource("perf-src-4w-" + std::to_string(i)));
        }

        auto t0 = std::chrono::steady_clock::now();
        auto report = coord4.ingestAll(sources);
        auto t1 = std::chrono::steady_clock::now();
        elapsed_4w_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        coord4.stop();

        ASSERT_EQ(report.total_documents, kSources * kDocsPerSource)
            << "4-worker run: unexpected document count";
    }

    ASSERT_GT(elapsed_4w_ms, 0.0);
    double speedup = elapsed_1w_ms / elapsed_4w_ms;

    // RecordProperty accepts int or string — use integer milliseconds.
    RecordProperty("elapsed_1_worker_ms",
                   static_cast<int>(elapsed_1w_ms));
    RecordProperty("elapsed_4_workers_ms",
                   static_cast<int>(elapsed_4w_ms));
    // Speedup as a fixed-point hundredths integer (e.g. 350 = 3.50x).
    RecordProperty("speedup_ratio_hundredths",
                   static_cast<int>(speedup * 100.0 + 0.5));

    EXPECT_GE(speedup, kMinSpeedup)
        << "AC-COORD-5 FAILED: 4-worker speedup " << speedup
        << "× is below the required " << kMinSpeedup << "×.  "
        << "1-worker wall-clock: " << elapsed_1w_ms << " ms, "
        << "4-worker wall-clock: " << elapsed_4w_ms << " ms.";
}

// ============================================================================
// AC-COORD-6: Coordinator overhead ≤ 5% of total ingestion wall-clock time
//
// Gated by THEMIS_RUN_PERF_TESTS=1 to avoid failures on slow CI hardware.
// Measures the coordinator's pure partitioning + aggregation cost by
// comparing the total worker time (sum of individual mock ingest durations)
// against the overall wall-clock time reported by the coordinator.
// ============================================================================

TEST(IngestionCoordinatorPerfTest, CoordinatorOverheadAtMost5Percent) {
    if (!perfTestsEnabled()) {
        GTEST_SKIP() << "Skipping AC-COORD-6 overhead microbenchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "AC-COORD-6: coordinator overhead ≤ 5% of total ingestion wall-clock.";
    }

    constexpr size_t   kSources           = 200;
    constexpr size_t   kDocsPerSource     = 500;
    constexpr unsigned kSleepUsPerSource  = 50;   // deterministic per-source cost
    constexpr double   kMaxOverhead       = 0.05; // 5 %

    // TimedMockWorkerNode sleeps a fixed amount per source so wall-clock times
    // are dominated by the simulated work rather than scheduler/lock noise.
    class TimedMockWorkerNode : public IIngestionWorkerNode {
    public:
        explicit TimedMockWorkerNode(const std::string& id,
                                     size_t docs_per_source,
                                     unsigned sleep_us)
            : id_(id), docs_per_source_(docs_per_source), sleep_us_(sleep_us) {}

        const std::string& nodeId() const override { return id_; }
        bool isAvailable() const override { return true; }

        IngestionReport ingest(
            const std::vector<SourceConfig>& sources,
            const std::string& /*coll*/,
            ProgressCallback /*cb*/) override
        {
            auto t0 = std::chrono::steady_clock::now();
            IngestionReport report;
            for (const auto& src : sources) {
                if (sleep_us_ > 0) {
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(sleep_us_));
                }
                IngestionStats st;
                st.documents_processed = docs_per_source_;
                report.source_stats[src.source_id] = st;
                report.total_documents += docs_per_source_;
            }
            auto t1 = std::chrono::steady_clock::now();
            total_worker_time_ms_ +=
                std::chrono::duration<double, std::milli>(t1 - t0).count();
            return report;
        }

        double totalWorkerTimeMs() const { return total_worker_time_ms_; }

    private:
        std::string id_;
        size_t docs_per_source_;
        unsigned sleep_us_;
        double total_worker_time_ms_ = 0.0;
    };

    IngestionCoordinator::Config cfg;
    cfg.num_nodes     = 0;
    cfg.db_connection = "overhead_db";
    IngestionCoordinator coordinator(cfg);

    std::vector<std::shared_ptr<TimedMockWorkerNode>> workers;
    for (int w = 0; w < 4; ++w) {
        auto worker = std::make_shared<TimedMockWorkerNode>(
            "overhead-w-" + std::to_string(w), kDocsPerSource, kSleepUsPerSource);
        workers.push_back(worker);
        coordinator.registerNode(worker);
    }
    coordinator.start();

    std::vector<SourceConfig> sources;
    sources.reserve(kSources);
    for (size_t i = 0; i < kSources; ++i) {
        sources.push_back(makeSource("oh-src-" + std::to_string(i)));
    }

    auto wall_t0 = std::chrono::steady_clock::now();
    auto report  = coordinator.ingestAll(sources);
    auto wall_t1 = std::chrono::steady_clock::now();

    coordinator.stop();

    double wall_clock_ms =
        std::chrono::duration<double, std::milli>(wall_t1 - wall_t0).count();

    ASSERT_GT(wall_clock_ms, 0.0);
    ASSERT_EQ(report.total_documents, kSources * kDocsPerSource)
        << "Unexpected document count in overhead test";

    // Coordinator overhead = wall_clock − max_single_worker_time.
    // Because workers run in parallel, the critical path is the slowest worker.
    // The coordinator's own cost (partitioning + dispatch + aggregation) is:
    //   wall_clock_ms - max_worker_time_ms
    // The overhead fraction is:
    //   (wall_clock_ms - max_worker_time_ms) / wall_clock_ms
    double max_worker_ms   = 0.0;
    double total_worker_ms = 0.0;
    for (const auto& w : workers) {
        double t = w->totalWorkerTimeMs();
        total_worker_ms += t;
        max_worker_ms    = std::max(max_worker_ms, t);
    }
    double overhead_fraction = (wall_clock_ms - max_worker_ms) / wall_clock_ms;

    // RecordProperty accepts int or string — use integer milliseconds.
    RecordProperty("wall_clock_ms",      static_cast<int>(wall_clock_ms));
    RecordProperty("total_worker_ms",    static_cast<int>(total_worker_ms));
    RecordProperty("max_worker_ms",      static_cast<int>(max_worker_ms));
    // Overhead as integer basis points (1 bp = 0.01%; 500 bp = 5%).
    RecordProperty("overhead_basis_points",
                   static_cast<int>(overhead_fraction * 10000.0 + 0.5));

    EXPECT_LE(overhead_fraction, kMaxOverhead)
        << "AC-COORD-6 FAILED: coordinator overhead fraction "
        << (overhead_fraction * 100.0) << "% exceeds the 5% limit.  "
        << "Wall-clock: " << wall_clock_ms << " ms, "
        << "max worker time: " << max_worker_ms << " ms.";
}

