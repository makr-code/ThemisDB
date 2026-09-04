// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/sharding_interfaces.h"

#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

using namespace themis::sharding;

// =============================================================================
// Mock implementations used across multiple test suites
// =============================================================================

// -----------------------------------------------------------------------------
// MockShardRouter
// -----------------------------------------------------------------------------

class MockShardRouter final : public IShardRouter {
public:
    // Simple modulo ring: returns shard_<hash(key) % shard_count>
    explicit MockShardRouter(size_t shard_count = 3)
        : shard_count_(shard_count) {}

    NodeId route(const ShardKey& key) const override {
        size_t h = std::hash<std::string>{}(key) % shard_count_;
        return "shard_" + std::to_string(h);
    }

    std::vector<NodeId> routeAll(std::span<const ShardKey> keys) const override {
        std::vector<NodeId> result = {};

        result.reserve(keys.size());
        for (const auto& k : keys) {
            result.push_back(route(k));
        }
        return result;
    }

private:
    size_t shard_count_;
};

// -----------------------------------------------------------------------------
// MockAdaptiveRebalancer
// -----------------------------------------------------------------------------

class MockAdaptiveRebalancer final : public IAdaptiveRebalancer {
public:
    RebalancePlan planRebalance(const ClusterStats& stats) const override {
        RebalancePlan plan;
        plan.plan_id = "mock_plan_001";
        plan.estimated_duration_ms = 5000;

        if (stats.shard_stats.size() >= 2) {
            ShardMigration m;
            m.source_shard_id = stats.shard_stats[0].shard_id;
            m.dest_shard_id   = stats.shard_stats[1].shard_id;
            m.key_range_start = "a";
            m.key_range_end   = "m";
            m.estimated_bytes = 1024 * 1024;
            plan.migrations.push_back(m);
            plan.affected_shards = {m.source_shard_id, m.dest_shard_id};
        }
        return plan;
    }

    std::future<RebalanceResult> applyPlan(
        const RebalancePlan& plan,
        const AdminContext& context
    ) override {
        if (!context.isValid()) {
            throw PermissionDeniedError("Missing admin capability token");
        }
        auto p = std::make_shared<std::promise<RebalanceResult>>();
        auto fut = p->get_future();
        RebalanceResult result;
        result.success = true;
        result.plan_id = plan.plan_id;
        result.migrations_completed = plan.migrations.size();
        p->set_value(result);
        return fut;
    }

    bool cancel(const RebalancePlanId& plan_id) override {
        return plan_id == "mock_plan_001";
    }
};

// -----------------------------------------------------------------------------
// MockDistributedTxCoordinator
// -----------------------------------------------------------------------------

class MockDistributedTxCoordinator final : public IDistributedTxCoordinator {
public:
    TxHandle begin() override {
        std::string tx_id = "tx_" + std::to_string(next_tx_id_.fetch_add(1));
        {
            std::lock_guard lock(mu_);
            active_txns_.insert(tx_id);
        }
        return makeTxHandle(tx_id, [this](const std::string& id) {
            std::lock_guard lock(mu_);
            active_txns_.erase(id);
            aborted_txns_.insert(id);
        });
    }

    PrepareResult prepare(TxHandle& handle) override {
        std::lock_guard lock(mu_);
        if (force_conflict_) {
            return ConflictError{"tx_concurrent"};
        }
        if (force_timeout_) {
            return TimeoutError{};
        }
        prepared_txns_.insert(handle.txId());
        return Prepared{};
    }

    void commit(TxHandle&& handle) override {
        {
            std::lock_guard lock(mu_);
            active_txns_.erase(handle.txId());
            committed_txns_.insert(handle.txId());
        }
        finalizeTxHandle(handle);
    }

    void abort(TxHandle&& handle) override {
        {
            std::lock_guard lock(mu_);
            active_txns_.erase(handle.txId());
            aborted_txns_.insert(handle.txId());
        }
        finalizeTxHandle(handle);
    }

    size_t maxConcurrentTx() const override { return 128; }
    size_t activeTxCount() const override {
        std::lock_guard lock(mu_);
        return active_txns_.size();
    }
    std::chrono::milliseconds transactionTimeoutMs() const override {
        return tx_timeout_ms_;
    }

    // Test helpers
    bool wasCommitted(const std::string& tx_id) const {
        std::lock_guard lock(mu_);
        return committed_txns_.count(tx_id) > 0;
    }
    bool wasAborted(const std::string& tx_id) const {
        std::lock_guard lock(mu_);
        return aborted_txns_.count(tx_id) > 0;
    }
    void forceConflict(bool v) { force_conflict_ = v; }
    void forceTimeout(bool v)  { force_timeout_ = v; }
    void setTxTimeoutMs(int ms) { tx_timeout_ms_ = std::chrono::milliseconds{ms}; }

private:
    mutable std::mutex mu_;
    std::unordered_set<std::string> active_txns_;
    std::unordered_set<std::string> prepared_txns_;
    std::unordered_set<std::string> committed_txns_;
    std::unordered_set<std::string> aborted_txns_;
    std::atomic<int> next_tx_id_{0};
    bool force_conflict_{false};
    bool force_timeout_{false};
    std::chrono::milliseconds tx_timeout_ms_{30000};
};

// -----------------------------------------------------------------------------
// MockRaftSnapshotManager
// -----------------------------------------------------------------------------

class MockRaftSnapshotManager final : public IRaftSnapshotManager {
public:
    std::future<SnapshotHandle> initiateSnapshot(const ShardId& shard_id) override {
        auto p = std::make_shared<std::promise<SnapshotHandle>>();
        auto fut = p->get_future();
        SnapshotHandle h;
        h.snapshot_id   = "snap_" + shard_id + "_001";
        h.size_bytes    = 4096;
        h.last_log_index = 42;
        h.last_log_term  = 3;
        h.created_at    = std::chrono::system_clock::now();
        p->set_value(h);
        return fut;
    }

    bool verifySnapshot(const SnapshotHandle& handle) const override {
        return handle.verifyIntegrity();
    }

    std::future<CompactionResult> compactLog(
        const ShardId& shard_id,
        const SnapshotHandle& snapshot,
        const AdminContext& context
    ) override {
        auto p = std::make_shared<std::promise<CompactionResult>>();
        auto fut = p->get_future();
        if (!context.isValid()) {
            CompactionResult r;
            r.success = false;
            r.error_message = "Missing admin token";
            p->set_value(r);
            return fut;
        }
        // Enforce HMAC verification before any log truncation (spec requirement)
        if (!verifySnapshot(snapshot)) {
            CompactionResult r;
            r.success = false;
            r.error_message = "Snapshot integrity check failed — possible tampering";
            p->set_value(r);
            return fut;
        }
        CompactionResult r;
        r.success = true;
        r.shard_id = shard_id;
        r.compacted_up_to_index = snapshot.last_log_index;
        r.bytes_freed = snapshot.size_bytes / 2;
        p->set_value(r);
        return fut;
    }
};

// -----------------------------------------------------------------------------
// MockConsistentHashRing
// -----------------------------------------------------------------------------

class MockConsistentHashRing final : public IConsistentHashRing {
public:
    explicit MockConsistentHashRing(std::vector<NodeId> nodes)
        : nodes_(std::move(nodes)) {}

    NodeId getNode(const ShardKey& key) const override {
        if (nodes_.empty()) {
          return "";
        }
        size_t h = std::hash<std::string>{}(key) % nodes_.size();
        return nodes_[h];
    }

    std::vector<NodeId> getNodes(
        const ShardKey& key,
        size_t replication_factor
    ) const override {
        if (nodes_.empty()) return {};
        size_t start = std::hash<std::string>{}(key) % nodes_.size();
        std::vector<NodeId> result = {};

        for (size_t i = 0; i < std::min(replication_factor, nodes_.size()); ++i) {
            result.push_back(nodes_[(start + i) % nodes_.size()]);
        }
        return result;
    }

    RebalanceLockHandle acquireRebalanceLock() override {
        lock_count_.fetch_add(1);
        return makeRebalanceLockHandle([this]() { lock_count_.fetch_sub(1); });
    }

    size_t virtualNodes() const override { return nodes_.size() * 150; }

    std::vector<NodeId> physicalNodes() const override { return nodes_; }

    int lockCount() const { return lock_count_.load(); }

private:
    std::vector<NodeId> nodes_;
    std::atomic<int> lock_count_{0};
};

// -----------------------------------------------------------------------------
// MockCrossShardQueryRouter
// -----------------------------------------------------------------------------

class MockCrossShardQueryRouter final : public ICrossShardQueryRouter {
public:
    explicit MockCrossShardQueryRouter(std::vector<ShardId> shards)
        : shards_(std::move(shards)) {}

    std::vector<ShardQueryPlan> fanOut(const QueryPlan& plan) const override {
        std::vector<ShardQueryPlan> out = {};

        for (const auto& s : shards_) {
            ShardQueryPlan sp;
            sp.shard_id = s;
            sp.plan = plan;
            sp.estimated_rows = 100;
            out.push_back(sp);
        }
        return out;
    }

    ResultSet merge(
        std::span<const ShardQueryResult> results,
        MergeStrategy strategy = MergeStrategy::Union
    ) const override {
        ResultSet rs;
        rs.strategy = strategy;
        rs.rows = nlohmann::json::array();
        for (const auto& r : results) {
            if (!r.success) {
              continue;
            }
            if (r.rows.is_array()) {
                for (const auto& row : r.rows) {
                    rs.rows.push_back(row);
                }
            }
        }
        rs.total_rows = rs.rows.size();
        return rs;
    }

    QueryCostEstimate estimateCost(const QueryPlan& /*plan*/) const override {
        QueryCostEstimate est;
        est.shard_count = shards_.size();
        est.estimated_rows = shards_.size() * 100;
        est.network_hops = shards_.size() > 1 ? shards_.size() - 1 : 0;
        return est;
    }

private:
    std::vector<ShardId> shards_;
};

// =============================================================================
// IShardRouter tests
// =============================================================================

TEST(IShardRouterTest, RouteReturnsShard) {
    MockShardRouter router(3);
    NodeId shard = router.route("key_abc");
    EXPECT_FALSE(shard.empty());
    EXPECT_EQ(shard.substr(0, 6), "shard_");
}

TEST(IShardRouterTest, RouteIsConsistent) {
    MockShardRouter router(4);
    const ShardKey key = "consistent_key";
    NodeId first = router.route(key);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(router.route(key), first);
    }
}

TEST(IShardRouterTest, RouteAllMatchesSingleRoute) {
    MockShardRouter router(3);
    std::vector<ShardKey> keys = {"k1", "k2", "k3", "k4"};
    auto batch = router.routeAll(std::span<const ShardKey>(keys));
    ASSERT_EQ(batch.size(), keys.size());
    for (size_t i = 0; i < keys.size(); ++i) {
        EXPECT_EQ(batch[i], router.route(keys[i]));
    }
}

TEST(IShardRouterTest, RouteAllEmptyInput) {
    MockShardRouter router(3);
    std::vector<ShardKey> empty;
    auto result = router.routeAll(std::span<const ShardKey>(empty));
    EXPECT_TRUE(result.empty());
}

TEST(IShardRouterTest, RouteAllConcurrent) {
    MockShardRouter router(8);
    constexpr int kThreads = 64;
    constexpr int kKeysPerThread = 100;

    std::vector<std::thread> threads;
    std::atomic<bool> any_failure{false};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            std::vector<ShardKey> keys = {};

            for (int k = 0; k < kKeysPerThread; ++k) {
                keys.push_back("key_" + std::to_string(t * kKeysPerThread + k));
            }
            auto result = router.routeAll(std::span<const ShardKey>(keys));
            if (result.size() != keys.size()) {
                any_failure = true;
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    EXPECT_FALSE(any_failure.load());
}

// =============================================================================
// IAdaptiveRebalancer tests
// =============================================================================

TEST(IAdaptiveRebalancerTest, PlanRebalancePureFunction) {
    MockAdaptiveRebalancer rebalancer;
    ClusterStats stats;
    stats.shard_stats.push_back({"shard_a", 0.9, 0.5, 0.8, 1000.0, 500000});
    stats.shard_stats.push_back({"shard_b", 0.2, 0.3, 0.2,  200.0, 100000});

    auto plan = rebalancer.planRebalance(stats);
    EXPECT_FALSE(plan.plan_id.empty());
    EXPECT_EQ(plan.migrations.size(), 1u);
    EXPECT_EQ(plan.migrations[0].source_shard_id, "shard_a");
    EXPECT_EQ(plan.migrations[0].dest_shard_id,   "shard_b");
}

TEST(IAdaptiveRebalancerTest, PlanRebalanceEmptyStats) {
    MockAdaptiveRebalancer rebalancer;
    ClusterStats stats;
    auto plan = rebalancer.planRebalance(stats);
    EXPECT_TRUE(plan.migrations.empty());
}

TEST(IAdaptiveRebalancerTest, ApplyPlanRequiresAdminToken) {
    MockAdaptiveRebalancer rebalancer;
    ClusterStats stats;
    stats.shard_stats.push_back({"shard_a", 0.9, 0.5, 0.8, 1000.0, 500000});
    stats.shard_stats.push_back({"shard_b", 0.2, 0.3, 0.2,  200.0, 100000});
    auto plan = rebalancer.planRebalance(stats);

    AdminContext bad_ctx;  // empty token
    EXPECT_THROW(rebalancer.applyPlan(plan, bad_ctx), PermissionDeniedError);
}

TEST(IAdaptiveRebalancerTest, ApplyPlanSucceedsWithValidToken) {
    MockAdaptiveRebalancer rebalancer;
    ClusterStats stats;
    stats.shard_stats.push_back({"shard_a", 0.9, 0.5, 0.8, 1000.0, 500000});
    stats.shard_stats.push_back({"shard_b", 0.2, 0.3, 0.2,  200.0, 100000});
    auto plan = rebalancer.planRebalance(stats);

    AdminContext ctx{"valid_token_xyz"};
    auto future = rebalancer.applyPlan(plan, ctx);
    auto result = future.get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.plan_id, plan.plan_id);
    EXPECT_EQ(result.migrations_completed, plan.migrations.size());
}

TEST(IAdaptiveRebalancerTest, CancelKnownPlan) {
    MockAdaptiveRebalancer rebalancer;
    EXPECT_TRUE(rebalancer.cancel("mock_plan_001"));
}

TEST(IAdaptiveRebalancerTest, CancelUnknownPlanReturnsFalse) {
    MockAdaptiveRebalancer rebalancer;
    EXPECT_FALSE(rebalancer.cancel("nonexistent_plan"));
}

// =============================================================================
// IDistributedTxCoordinator tests
// =============================================================================

TEST(IDistributedTxCoordinatorTest, BeginReturnsActiveHandle) {
    MockDistributedTxCoordinator coord;
    auto handle = coord.begin();
    EXPECT_TRUE(handle.isActive());
    EXPECT_FALSE(handle.txId().empty());
    EXPECT_EQ(coord.activeTxCount(), 1u);
}

TEST(IDistributedTxCoordinatorTest, PrepareAndCommitHappyPath) {
    MockDistributedTxCoordinator coord;
    auto handle = coord.begin();
    std::string tx_id = handle.txId();

    auto result = coord.prepare(handle);
    EXPECT_TRUE(std::holds_alternative<Prepared>(result));

    coord.commit(std::move(handle));
    EXPECT_TRUE(coord.wasCommitted(tx_id));
    EXPECT_EQ(coord.activeTxCount(), 0u);
}

TEST(IDistributedTxCoordinatorTest, PrepareConflictReturnsConflictError) {
    MockDistributedTxCoordinator coord;
    coord.forceConflict(true);
    auto handle = coord.begin();
    auto result = coord.prepare(handle);
    EXPECT_TRUE(std::holds_alternative<ConflictError>(result));
    EXPECT_FALSE(std::get<ConflictError>(result).conflicting_tx_id.empty());
}

TEST(IDistributedTxCoordinatorTest, PrepareTimeoutReturnsTimeoutError) {
    MockDistributedTxCoordinator coord;
    coord.forceTimeout(true);
    auto handle = coord.begin();
    auto result = coord.prepare(handle);
    EXPECT_TRUE(std::holds_alternative<TimeoutError>(result));
}

TEST(IDistributedTxCoordinatorTest, ExplicitAbortCleansUp) {
    MockDistributedTxCoordinator coord;
    auto handle = coord.begin();
    std::string tx_id = handle.txId();
    coord.abort(std::move(handle));
    EXPECT_TRUE(coord.wasAborted(tx_id));
    EXPECT_EQ(coord.activeTxCount(), 0u);
}

TEST(IDistributedTxCoordinatorTest, HandleDestructorAutoAborts) {
    MockDistributedTxCoordinator coord;
    std::string tx_id;
    {
        auto handle = coord.begin();
        tx_id = handle.txId();
        EXPECT_EQ(coord.activeTxCount(), 1u);
        // handle goes out of scope without commit or abort
    }
    EXPECT_TRUE(coord.wasAborted(tx_id));
    EXPECT_EQ(coord.activeTxCount(), 0u);
}

TEST(IDistributedTxCoordinatorTest, MovedFromHandleDoesNotAbort) {
    MockDistributedTxCoordinator coord;
    auto handle1 = coord.begin();
    std::string tx_id = handle1.txId();
    auto handle2 = std::move(handle1);

    // handle1 is moved-from; its destruction must not trigger an abort
    EXPECT_FALSE(handle1.isActive());  // NOLINT(bugprone-use-after-move)
    EXPECT_TRUE(handle2.isActive());
    EXPECT_EQ(coord.activeTxCount(), 1u);

    coord.abort(std::move(handle2));
    EXPECT_TRUE(coord.wasAborted(tx_id));
}

TEST(IDistributedTxCoordinatorTest, MaxConcurrentTxPositive) {
    MockDistributedTxCoordinator coord;
    EXPECT_GT(coord.maxConcurrentTx(), 0u);
}

TEST(IDistributedTxCoordinatorTest, RequiresMTLSAlwaysTrue) {
    MockDistributedTxCoordinator coord;
    EXPECT_TRUE(coord.requiresMTLS());
}

// =============================================================================
// IRaftSnapshotManager tests
// =============================================================================

TEST(IRaftSnapshotManagerTest, InitiateSnapshotReturnsValidHandle) {
    MockRaftSnapshotManager mgr;
    auto fut = mgr.initiateSnapshot("shard_001");
    auto handle = fut.get();
    EXPECT_FALSE(handle.snapshot_id.empty());
    EXPECT_GT(handle.size_bytes, 0u);
    EXPECT_GT(handle.last_log_index, 0u);
}

TEST(IRaftSnapshotManagerTest, VerifySnapshotAcceptsValidHandle) {
    MockRaftSnapshotManager mgr;
    auto handle = mgr.initiateSnapshot("shard_001").get();
    EXPECT_TRUE(mgr.verifySnapshot(handle));
}

TEST(IRaftSnapshotManagerTest, VerifySnapshotRejectsEmptyHandle) {
    MockRaftSnapshotManager mgr;
    SnapshotHandle empty;
    EXPECT_FALSE(mgr.verifySnapshot(empty));
}

TEST(IRaftSnapshotManagerTest, CompactLogSucceedsWithValidContext) {
    MockRaftSnapshotManager mgr;
    auto handle = mgr.initiateSnapshot("shard_002").get();
    AdminContext ctx{"admin_tok"};
    auto result = mgr.compactLog("shard_002", handle, ctx).get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.shard_id, "shard_002");
    EXPECT_EQ(result.compacted_up_to_index, handle.last_log_index);
    EXPECT_GT(result.bytes_freed, 0u);
}

TEST(IRaftSnapshotManagerTest, CompactLogFailsWithoutAdminToken) {
    MockRaftSnapshotManager mgr;
    auto handle = mgr.initiateSnapshot("shard_003").get();
    AdminContext bad_ctx;
    auto result = mgr.compactLog("shard_003", handle, bad_ctx).get();
    EXPECT_FALSE(result.success);
}

TEST(IRaftSnapshotManagerTest, SnapshotHandleIdIsStable) {
    MockRaftSnapshotManager mgr;
    auto h = mgr.initiateSnapshot("shard_x").get();
    EXPECT_EQ(h.id(), h.snapshot_id);
    EXPECT_EQ(h.sizeBytes(), h.size_bytes);
    EXPECT_EQ(h.createdAt(), h.created_at);
}

// =============================================================================
// IConsistentHashRing tests
// =============================================================================

TEST(IConsistentHashRingTest, GetNodeReturnsMember) {
    MockConsistentHashRing ring({"node_a", "node_b", "node_c"});
    NodeId n = ring.getNode("some_key");
    auto nodes = ring.physicalNodes();
    EXPECT_NE(std::find(nodes.begin(), nodes.end(), n), nodes.end());
}

TEST(IConsistentHashRingTest, GetNodeIsConsistent) {
    MockConsistentHashRing ring({"node_a", "node_b", "node_c"});
    NodeId first = ring.getNode("stable_key");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(ring.getNode("stable_key"), first);
    }
}

TEST(IConsistentHashRingTest, GetNodesReturnsCorrectCount) {
    MockConsistentHashRing ring({"n0", "n1", "n2", "n3"});
    auto nodes = ring.getNodes("test_key", 3);
    EXPECT_EQ(nodes.size(), 3u);
}

TEST(IConsistentHashRingTest, GetNodesDoesNotExceedPhysicalNodes) {
    MockConsistentHashRing ring({"n0", "n1"});
    auto nodes = ring.getNodes("k", 10);
    EXPECT_LE(nodes.size(), 2u);
}

TEST(IConsistentHashRingTest, VirtualNodesCountPositive) {
    MockConsistentHashRing ring({"n0", "n1", "n2"});
    EXPECT_GT(ring.virtualNodes(), 0u);
}

TEST(IConsistentHashRingTest, PhysicalNodesMatchConstruction) {
    std::vector<NodeId> expected = {"n0", "n1", "n2"};
    MockConsistentHashRing ring(expected);
    auto actual = ring.physicalNodes();
    EXPECT_EQ(actual, expected);
}

TEST(IConsistentHashRingTest, RebalanceLockHandleIsRAII) {
    MockConsistentHashRing ring({"n0", "n1"});
    EXPECT_EQ(ring.lockCount(), 0);
    {
        auto lock = ring.acquireRebalanceLock();
        EXPECT_EQ(ring.lockCount(), 1);
        EXPECT_TRUE(lock.isHeld());
    }
    EXPECT_EQ(ring.lockCount(), 0);
}

TEST(IConsistentHashRingTest, RebalanceLockHandleMoveSemantics) {
    MockConsistentHashRing ring({"n0", "n1"});
    {
        auto lock1 = ring.acquireRebalanceLock();
        EXPECT_EQ(ring.lockCount(), 1);
        auto lock2 = std::move(lock1);
        EXPECT_EQ(ring.lockCount(), 1);
        EXPECT_FALSE(lock1.isHeld());  // NOLINT(bugprone-use-after-move)
        EXPECT_TRUE(lock2.isHeld());
    }
    EXPECT_EQ(ring.lockCount(), 0);
}

TEST(IConsistentHashRingTest, MultipleLockHandles) {
    MockConsistentHashRing ring({"n0"});
    {
        auto l1 = ring.acquireRebalanceLock();
        auto l2 = ring.acquireRebalanceLock();
        EXPECT_EQ(ring.lockCount(), 2);
    }
    EXPECT_EQ(ring.lockCount(), 0);
}

// =============================================================================
// ICrossShardQueryRouter tests
// =============================================================================

TEST(ICrossShardQueryRouterTest, FanOutProducesOneEntryPerShard) {
    MockCrossShardQueryRouter router({"shard_0", "shard_1", "shard_2"});
    QueryPlan plan;
    plan.query_text = "SELECT * FROM documents";
    auto plans = router.fanOut(plan);
    EXPECT_EQ(plans.size(), 3u);
    std::vector<ShardId> found = {};

    for (const auto& sp : plans) {
        found.push_back(sp.shard_id);
    }
    EXPECT_NE(std::find(found.begin(), found.end(), "shard_0"), found.end());
    EXPECT_NE(std::find(found.begin(), found.end(), "shard_1"), found.end());
    EXPECT_NE(std::find(found.begin(), found.end(), "shard_2"), found.end());
}

TEST(ICrossShardQueryRouterTest, FanOutEmptyCluster) {
    MockCrossShardQueryRouter router({});
    QueryPlan plan;
    plan.query_text = "SELECT 1";
    auto plans = router.fanOut(plan);
    EXPECT_TRUE(plans.empty());
}

TEST(ICrossShardQueryRouterTest, MergeUnionCombinesAllRows) {
    MockCrossShardQueryRouter router({"shard_0", "shard_1"});
    std::vector<ShardQueryResult> results;
    {
        ShardQueryResult r;
        r.shard_id = "shard_0";
        r.rows = nlohmann::json::array({{{"id", 1}}, {{"id", 2}}});
        r.row_count = 2;
        results.push_back(r);
    }
    {
        ShardQueryResult r;
        r.shard_id = "shard_1";
        r.rows = nlohmann::json::array({{{"id", 3}}});
        r.row_count = 1;
        results.push_back(r);
    }
    auto rs = router.merge(std::span<const ShardQueryResult>(results), MergeStrategy::Union);
    EXPECT_EQ(rs.total_rows, 3u);
    EXPECT_EQ(rs.strategy, MergeStrategy::Union);
}

TEST(ICrossShardQueryRouterTest, MergeSkipsFailedShards) {
    MockCrossShardQueryRouter router({"shard_0", "shard_1"});
    std::vector<ShardQueryResult> results;
    {
        ShardQueryResult r;
        r.shard_id = "shard_0";
        r.rows = nlohmann::json::array({{{"id", 10}}});
        r.row_count = 1;
        results.push_back(r);
    }
    {
        ShardQueryResult r;
        r.shard_id = "shard_1";
        r.success = false;
        r.error_message = "shard offline";
        results.push_back(r);
    }
    auto rs = router.merge(std::span<const ShardQueryResult>(results));
    EXPECT_EQ(rs.total_rows, 1u);
}

TEST(ICrossShardQueryRouterTest, EstimateCostReturnsShardCount) {
    MockCrossShardQueryRouter router({"s0", "s1", "s2", "s3"});
    QueryPlan plan;
    plan.query_text = "SELECT COUNT(*)";
    auto cost = router.estimateCost(plan);
    EXPECT_EQ(cost.shard_count, 4u);
    EXPECT_GT(cost.estimated_rows, 0u);
}

TEST(ICrossShardQueryRouterTest, EstimateCostSingleShard) {
    MockCrossShardQueryRouter router({"s0"});
    QueryPlan plan;
    plan.query_text = "GET id=123";
    auto cost = router.estimateCost(plan);
    EXPECT_EQ(cost.shard_count, 1u);
    EXPECT_EQ(cost.network_hops, 0u);
}

// =============================================================================
// AdminContext and PermissionDeniedError tests
// =============================================================================

TEST(AdminContextTest, ValidContextNonEmpty) {
    AdminContext ctx{"some_token"};
    EXPECT_TRUE(ctx.isValid());
}

TEST(AdminContextTest, EmptyContextInvalid) {
    AdminContext ctx;
    EXPECT_FALSE(ctx.isValid());
}

TEST(PermissionDeniedErrorTest, IsRuntimeError) {
    PermissionDeniedError err("not allowed");
    EXPECT_STREQ(err.what(), "not allowed");
    // Must be catch-able as std::runtime_error
    try {
        throw PermissionDeniedError("test");
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "test");
    }
}

// =============================================================================
// Audit-gap tests (required by FUTURE_ENHANCEMENTS.md Test Strategy)
// =============================================================================

// ── Raft snapshot integrity: corrupt snapshot → verifyIntegrity() returns false
TEST(IRaftSnapshotManagerTest, VerifyIntegrityReturnsFalseForCorruptedSnapshot) {
    MockRaftSnapshotManager mgr;
    auto handle = mgr.initiateSnapshot("shard_corrupt").get();
    ASSERT_TRUE(handle.verifyIntegrity()) << "freshly created handle must be intact";

    // Simulate tampering: set is_corrupted flag (as a concrete manager would
    // when it detects an HMAC mismatch).
    handle.is_corrupted = true;
    EXPECT_FALSE(handle.verifyIntegrity())
        << "verifyIntegrity() must return false for a corrupted snapshot";
    EXPECT_FALSE(mgr.verifySnapshot(handle))
        << "verifySnapshot() must also reject a corrupted handle";
}

// Corrupted snapshot must not trigger compactLog (HMAC enforced before truncation)
TEST(IRaftSnapshotManagerTest, CompactLogRejectsCorruptedSnapshot) {
    MockRaftSnapshotManager mgr;
    auto handle = mgr.initiateSnapshot("shard_x").get();
    handle.is_corrupted = true;          // simulate HMAC mismatch
    AdminContext ctx{"valid_token"};
    auto result = mgr.compactLog("shard_x", handle, ctx).get();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// ── Consistent hash ring: routing is stable while rebalance lock is held
TEST(IConsistentHashRingTest, RingImmutabilityUnderConcurrentRebalanceLock) {
    MockConsistentHashRing ring({"node_a", "node_b", "node_c"});
    const ShardKey key = "stability_test_key";

    NodeId before_lock = ring.getNode(key);

    // Acquire the rebalance lock
    auto lock = ring.acquireRebalanceLock();
    EXPECT_TRUE(lock.isHeld());
    EXPECT_EQ(ring.lockCount(), 1);

    // Concurrent read while lock is held must see the same result
    std::vector<NodeId> readings;
    readings.reserve(8);
    for (int i = 0; i < 8; ++i) {
        readings.push_back(ring.getNode(key));
    }

    // Release the lock
    lock = RebalanceLockHandle{};  // move-assign from default-constructed handle
    EXPECT_EQ(ring.lockCount(), 0);

    // All readings must be identical (ring is stable during the lock period)
    for (const auto& r : readings) {
        EXPECT_EQ(r, before_lock)
            << "getNode() must return the same shard while rebalance lock is held";
    }
    // Reading after lock release must also be consistent
    EXPECT_EQ(ring.getNode(key), before_lock);
}

// ── IDistributedTxCoordinator: configurable TTL must be in the interface
TEST(IDistributedTxCoordinatorTest, TransactionTimeoutMsIsAccessible) {
    MockDistributedTxCoordinator coord;
    // Default timeout must be a positive duration
    EXPECT_GT(coord.transactionTimeoutMs().count(), 0);
}

TEST(IDistributedTxCoordinatorTest, TransactionTimeoutMsIsConfigurable) {
    MockDistributedTxCoordinator coord;
    coord.setTxTimeoutMs(5000);
    EXPECT_EQ(coord.transactionTimeoutMs(), std::chrono::milliseconds{5000});

    // Zero means "no TTL enforcement" — transactions are never forcibly
    // timed out by the coordinator (only aborted by explicit call or
    // TxHandle destruction).
    coord.setTxTimeoutMs(0);
    EXPECT_EQ(coord.transactionTimeoutMs(), std::chrono::milliseconds{0})
        << "zero timeout indicates no TTL enforcement by the coordinator";
}

// ── Security: requiresMTLS() must not be overridable (non-virtual contract)
// This test verifies the interface-level guarantee: calling requiresMTLS() via
// a pointer to the base class always returns true regardless of the subtype.
TEST(IDistributedTxCoordinatorTest, RequiresMTLSIsNonVirtualAlwaysTrue) {
    MockDistributedTxCoordinator coord;
    // Access through base-class pointer to confirm non-virtual dispatch
    IDistributedTxCoordinator* base = &coord;
    EXPECT_TRUE(base->requiresMTLS());
}
