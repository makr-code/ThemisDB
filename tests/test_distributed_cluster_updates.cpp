// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_distributed_cluster_updates.cpp
 * @brief Focused unit tests for ClusterUpdateManager
 *        (v1.7.0, Issue #125 – Distributed Cluster Updates).
 *
 * All tests use only the public API.  Per-node update execution and health
 * checks are replaced by lightweight in-process lambdas so that no real
 * network communication is required.
 *
 * Test categories
 * ---------------
 *  Construction              – node list validation, leader ordering
 *  Accessors                 – totalNodes(), nodeStatuses(), isCancelled()
 *  Rolling update (success)  – nodes updated in non-leader → leader order
 *  Rolling update (failure)  – follower failure aborts update; rollback
 *  Health check              – failed health check triggers rollback
 *  Cancellation              – cancelUpdate() stops remaining nodes
 *  Progress callback         – incremental progress events
 *  Version skew protection   – leader is always updated last
 *  Options override          – per-call options respected
 */

#include <gtest/gtest.h>

#include "updates/cluster_update_manager.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::updates;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a 3-node config (node-a, node-b non-leaders; node-c leader).
static ClusterUpdateManager::Config make3NodeConfig() {
    ClusterUpdateManager::Config cfg;
    cfg.nodes = {
        { "node-a", "host-a:6543", false, "1.6.0" },
        { "node-b", "host-b:6543", false, "1.6.0" },
        { "node-c", "host-c:6543", true,  "1.6.0" },
    };
    cfg.default_options.rollback_on_failure  = false;
    cfg.default_options.health_check_timeout = std::chrono::seconds{5};
    return cfg;
}

/// Build a single-node config.
static ClusterUpdateManager::Config makeSingleNodeConfig(bool is_leader = true) {
    ClusterUpdateManager::Config cfg;
    cfg.nodes = { { "node-only", "host:6543", is_leader, "1.6.0" } };
    return cfg;
}

/// Build a 2-node config.
static ClusterUpdateManager::Config make2NodeConfig() {
    ClusterUpdateManager::Config cfg;
    cfg.nodes = {
        { "node-0", "host-0:6543", false, "1.6.0" },
        { "node-1", "host-1:6543", true,  "1.6.0" },
    };
    cfg.default_options.rollback_on_failure = false;
    return cfg;
}

// Simple always-succeeding NodeUpdateFunc.
static ClusterUpdateManager::NodeUpdateFunc alwaysOkUpdate() {
    return [](const ClusterNode&, const std::string&,
              const ClusterUpdateOptions&) { return true; };
}

// Simple always-failing NodeUpdateFunc.
static ClusterUpdateManager::NodeUpdateFunc alwaysFailUpdate() {
    return [](const ClusterNode&, const std::string&,
              const ClusterUpdateOptions&) { return false; };
}

// Simple always-healthy NodeHealthCheckFunc.
static ClusterUpdateManager::NodeHealthCheckFunc alwaysHealthy() {
    return [](const ClusterNode&, std::chrono::seconds) { return true; };
}

// Simple always-unhealthy NodeHealthCheckFunc.
static ClusterUpdateManager::NodeHealthCheckFunc alwaysUnhealthy() {
    return [](const ClusterNode&, std::chrono::seconds) { return false; };
}

// ---------------------------------------------------------------------------
// Construction tests
// ---------------------------------------------------------------------------

class DistributedClusterConstruction : public ::testing::Test {};

TEST_F(DistributedClusterConstruction, EmptyNodeList_Throws) {
    ClusterUpdateManager::Config cfg;
    EXPECT_THROW(ClusterUpdateManager{cfg}, std::invalid_argument);
}

TEST_F(DistributedClusterConstruction, ValidConfig_DoesNotThrow) {
    EXPECT_NO_THROW(ClusterUpdateManager{make3NodeConfig()});
}

TEST_F(DistributedClusterConstruction, SingleNode_DoesNotThrow) {
    EXPECT_NO_THROW(ClusterUpdateManager{makeSingleNodeConfig()});
}

TEST_F(DistributedClusterConstruction, TotalNodes_MatchesNodeList) {
    ClusterUpdateManager mgr(make3NodeConfig());
    EXPECT_EQ(mgr.totalNodes(), 3u);
}

TEST_F(DistributedClusterConstruction, TotalNodes_SingleNode) {
    ClusterUpdateManager mgr(makeSingleNodeConfig());
    EXPECT_EQ(mgr.totalNodes(), 1u);
}

// ---------------------------------------------------------------------------
// Accessor tests
// ---------------------------------------------------------------------------

class DistributedClusterAccessors : public ::testing::Test {
protected:
    ClusterUpdateManager mgr_{make3NodeConfig()};
};

TEST_F(DistributedClusterAccessors, InitialStatuses_AllPending) {
    for (const auto& s : mgr_.nodeStatuses()) {
        EXPECT_EQ(s.state, ClusterNodeState::PENDING)
            << "Node " << s.node_id << " is not PENDING";
    }
}

TEST_F(DistributedClusterAccessors, InitialStatuses_AppliedVersionEmpty) {
    for (const auto& s : mgr_.nodeStatuses()) {
        EXPECT_TRUE(s.applied_version.empty());
    }
}

TEST_F(DistributedClusterAccessors, InitialStatuses_ErrorMessageEmpty) {
    for (const auto& s : mgr_.nodeStatuses()) {
        EXPECT_TRUE(s.error_message.empty());
    }
}

TEST_F(DistributedClusterAccessors, IsCancelled_FalseByDefault) {
    EXPECT_FALSE(mgr_.isCancelled());
}

TEST_F(DistributedClusterAccessors, CancelUpdate_SetsCancelledFlag) {
    mgr_.cancelUpdate();
    EXPECT_TRUE(mgr_.isCancelled());
}

// ---------------------------------------------------------------------------
// Successful rolling update
// ---------------------------------------------------------------------------

class DistributedClusterRollingSuccess : public ::testing::Test {
protected:
    ClusterUpdateManager mgr_{make3NodeConfig()};

    void SetUp() override {
        mgr_.setNodeUpdateFunc(alwaysOkUpdate());
        mgr_.setNodeHealthCheckFunc(alwaysHealthy());
    }
};

TEST_F(DistributedClusterRollingSuccess, UpdateCluster_Succeeds) {
    auto result = mgr_.updateCluster("1.7.0");
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
}

TEST_F(DistributedClusterRollingSuccess, UpdateCluster_AllNodesCompleted) {
    auto result = mgr_.updateCluster("1.7.0");
    EXPECT_EQ(result.nodes_updated, 3u);
    EXPECT_EQ(result.nodes_failed, 0u);
    EXPECT_EQ(result.nodes_rolled_back, 0u);
}

TEST_F(DistributedClusterRollingSuccess, UpdateCluster_StatusesAllCompleted) {
    mgr_.updateCluster("1.7.0");
    for (const auto& s : mgr_.nodeStatuses()) {
        EXPECT_EQ(s.state, ClusterNodeState::COMPLETED)
            << "Node " << s.node_id << " not COMPLETED";
    }
}

TEST_F(DistributedClusterRollingSuccess, UpdateCluster_LeaderUpdatedLast) {
    // Capture the order in which nodes are updated.
    std::vector<std::string> update_order;
    mgr_.setNodeUpdateFunc(
        [&](const ClusterNode& n, const std::string&,
            const ClusterUpdateOptions&) {
            update_order.push_back(n.node_id);
            return true;
        });

    mgr_.updateCluster("1.7.0");

    ASSERT_EQ(update_order.size(), 3u);
    // Leader (node-c) must be last.
    EXPECT_EQ(update_order.back(), "node-c");
}

TEST_F(DistributedClusterRollingSuccess, UpdateCluster_NonLeadersBeforeLeader) {
    std::vector<std::string> order;
    mgr_.setNodeUpdateFunc(
        [&](const ClusterNode& n, const std::string&,
            const ClusterUpdateOptions&) {
            order.push_back(n.node_id);
            return true;
        });

    mgr_.updateCluster("1.7.0");

    // Both non-leaders must appear before the leader.
    ASSERT_GE(order.size(), 3u);
    auto leader_pos = std::find(order.begin(), order.end(), "node-c");
    auto a_pos      = std::find(order.begin(), order.end(), "node-a");
    auto b_pos      = std::find(order.begin(), order.end(), "node-b");
    EXPECT_LT(a_pos, leader_pos);
    EXPECT_LT(b_pos, leader_pos);
}

TEST_F(DistributedClusterRollingSuccess, UpdateCluster_AppliedVersionSet) {
    mgr_.updateCluster("1.7.0");
    for (const auto& s : mgr_.nodeStatuses()) {
        EXPECT_FALSE(s.applied_version.empty())
            << "Node " << s.node_id << " missing applied_version";
        EXPECT_EQ(s.applied_version, "1.7.0");
    }
}

TEST_F(DistributedClusterRollingSuccess, UpdateCluster_IsCancelledFalseAfterSuccess) {
    mgr_.updateCluster("1.7.0");
    EXPECT_FALSE(mgr_.isCancelled());
}

// ---------------------------------------------------------------------------
// Follower node failure
// ---------------------------------------------------------------------------

class DistributedClusterFollowerFail : public ::testing::Test {
protected:
    ClusterUpdateManager mgr_{make3NodeConfig()};
};

TEST_F(DistributedClusterFollowerFail, FollowerFails_UpdateFails) {
    mgr_.setNodeUpdateFunc(alwaysFailUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    auto result = mgr_.updateCluster("1.7.0");
    EXPECT_FALSE(result.success);
}

TEST_F(DistributedClusterFollowerFail, FollowerFails_LeaderSkipped_WhenRollbackOnFailure) {
    // With rollback_on_failure=true the manager should abort after the first
    // follower fails, so the leader should NOT be updated.
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = true;

    std::vector<std::string> updated;
    mgr_.setNodeUpdateFunc(
        [&](const ClusterNode& n, const std::string&,
            const ClusterUpdateOptions&) {
            updated.push_back(n.node_id);
            return false; // always fail
        });
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    mgr_.updateCluster("1.7.0", opts);

    // At most one node (the first follower) was attempted.
    EXPECT_LE(updated.size(), 1u);
    // Leader node-c must not have been attempted.
    EXPECT_EQ(std::find(updated.begin(), updated.end(), "node-c"),
              updated.end());
}

TEST_F(DistributedClusterFollowerFail, FollowerFails_NoRollbackOnFailure_LeaderStillTried) {
    // Without rollback_on_failure, the update continues even if a follower fails.
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = false;

    std::vector<std::string> updated;
    mgr_.setNodeUpdateFunc(
        [&](const ClusterNode& n, const std::string&,
            const ClusterUpdateOptions&) {
            updated.push_back(n.node_id);
            // Fail only for followers; succeed for the leader.
            return n.is_leader;
        });
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    mgr_.updateCluster("1.7.0", opts);

    // Leader must have been attempted.
    EXPECT_NE(std::find(updated.begin(), updated.end(), "node-c"),
              updated.end());
}

TEST_F(DistributedClusterFollowerFail, FailedNode_HasFailedOrRolledBackState) {
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = false;

    mgr_.setNodeUpdateFunc(alwaysFailUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    mgr_.updateCluster("1.7.0", opts);

    int fail_count = 0;
    for (const auto& s : mgr_.nodeStatuses()) {
        if (s.state == ClusterNodeState::FAILED ||
            s.state == ClusterNodeState::ROLLED_BACK) {
            ++fail_count;
        }
    }
    EXPECT_GT(fail_count, 0);
}

// ---------------------------------------------------------------------------
// Health check failure
// ---------------------------------------------------------------------------

class DistributedClusterHealthCheck : public ::testing::Test {
protected:
    ClusterUpdateManager mgr_{make3NodeConfig()};
};

TEST_F(DistributedClusterHealthCheck, HealthCheckFails_NodeMarkedFailed) {
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = false;

    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysUnhealthy());

    mgr_.updateCluster("1.7.0", opts);

    bool any_failed = false;
    for (const auto& s : mgr_.nodeStatuses()) {
        if (s.state == ClusterNodeState::FAILED ||
            s.state == ClusterNodeState::ROLLED_BACK) {
            any_failed = true;
        }
    }
    EXPECT_TRUE(any_failed);
}

TEST_F(DistributedClusterHealthCheck, HealthCheckFails_UpdateResultFails) {
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = false;

    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysUnhealthy());

    auto result = mgr_.updateCluster("1.7.0", opts);
    EXPECT_FALSE(result.success);
}

TEST_F(DistributedClusterHealthCheck, HealthCheckFails_WithRollback_NodeRolledBack) {
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = true;

    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysUnhealthy());

    mgr_.updateCluster("1.7.0", opts);

    bool any_rolled_back = false;
    for (const auto& s : mgr_.nodeStatuses()) {
        if (s.state == ClusterNodeState::ROLLED_BACK) {
            any_rolled_back = true;
        }
    }
    EXPECT_TRUE(any_rolled_back);
}

TEST_F(DistributedClusterHealthCheck, HealthCheckFails_RollbackFunc_IsCalled) {
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = true;

    std::atomic<int> rollback_calls{0};
    std::string captured_version;

    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysUnhealthy());
    mgr_.setNodeRollbackFunc(
        [&](const ClusterNode&, const std::string& ver) {
            ++rollback_calls;
            captured_version = ver;
            return true;
        });

    mgr_.updateCluster("1.7.0", opts);

    // Rollback function must have been called for the failed node(s).
    EXPECT_GT(rollback_calls.load(), 0);
    // The applied_version passed to the callback should be "1.7.0"
    // (set before the health check).
    EXPECT_EQ(captured_version, "1.7.0");
}

TEST_F(DistributedClusterHealthCheck, UpdateFails_RollbackFunc_IsCalled) {
    ClusterUpdateOptions opts = make3NodeConfig().default_options;
    opts.rollback_on_failure  = true;

    std::atomic<int> rollback_calls{0};

    mgr_.setNodeUpdateFunc(alwaysFailUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());
    mgr_.setNodeRollbackFunc(
        [&](const ClusterNode&, const std::string&) {
            ++rollback_calls;
            return true;
        });

    mgr_.updateCluster("1.7.0", opts);

    // NodeRollbackFunc is called even when the update itself fails (before
    // the health check), so the caller can undo partial changes.
    EXPECT_GT(rollback_calls.load(), 0);
}

TEST_F(DistributedClusterHealthCheck, HealthCheckSucceeds_NodeCompleted) {
    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    auto result = mgr_.updateCluster("1.7.0");
    EXPECT_TRUE(result.success);

    for (const auto& s : mgr_.nodeStatuses()) {
        EXPECT_EQ(s.state, ClusterNodeState::COMPLETED);
    }
}

// ---------------------------------------------------------------------------
// Cancellation
// ---------------------------------------------------------------------------

class DistributedClusterCancellation : public ::testing::Test {};

TEST_F(DistributedClusterCancellation, CancelBeforeUpdate_StopsRemainingNodes) {
    ClusterUpdateManager mgr(make3NodeConfig());

    // Cancel the update after the very first NodeUpdateFunc call.
    // Because cancellation is checked before each node is processed,
    // only the first node reaches the update callback; subsequent nodes are
    // skipped once the flag is seen.
    std::atomic<int> update_calls{0};
    mgr.setNodeUpdateFunc([&](const ClusterNode&, const std::string&,
                               const ClusterUpdateOptions&) {
        ++update_calls;
        mgr.cancelUpdate();
        return true;
    });
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    auto result = mgr.updateCluster("1.7.0");

    // The cancellation flag is set inside the first callback, so at most
    // one NodeUpdateFunc invocation occurs before the loop exits.
    EXPECT_LE(update_calls.load(), 1);
    EXPECT_FALSE(result.success);
}

TEST_F(DistributedClusterCancellation, CancelUpdate_SetsFlag) {
    ClusterUpdateManager mgr(make3NodeConfig());
    EXPECT_FALSE(mgr.isCancelled());
    mgr.cancelUpdate();
    EXPECT_TRUE(mgr.isCancelled());
}

TEST_F(DistributedClusterCancellation, CancelledFlagReset_OnNewUpdateCall) {
    ClusterUpdateManager mgr(make2NodeConfig());
    mgr.setNodeUpdateFunc(alwaysOkUpdate());
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    // Cancel before the first update call.
    mgr.cancelUpdate();
    EXPECT_TRUE(mgr.isCancelled());

    // updateCluster() resets the cancellation flag at the start of each call.
    // The second call should therefore succeed normally.
    mgr.updateCluster("1.7.0");
    auto result = mgr.updateCluster("1.7.0");
    EXPECT_TRUE(result.success);
}

// ---------------------------------------------------------------------------
// Progress callback
// ---------------------------------------------------------------------------

class DistributedClusterProgress : public ::testing::Test {
protected:
    ClusterUpdateManager mgr_{make3NodeConfig()};
};

TEST_F(DistributedClusterProgress, ProgressCallback_CalledAtLeastOnce) {
    std::atomic<int> cb_count{0};
    mgr_.setProgressCallback([&](const ClusterUpdateProgress&) {
        ++cb_count;
    });
    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    mgr_.updateCluster("1.7.0");
    EXPECT_GT(cb_count.load(), 0);
}

TEST_F(DistributedClusterProgress, ProgressCallback_TotalNodesCorrect) {
    size_t last_total = 0;
    mgr_.setProgressCallback([&](const ClusterUpdateProgress& p) {
        last_total = p.total_nodes;
    });
    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    mgr_.updateCluster("1.7.0");
    EXPECT_EQ(last_total, 3u);
}

TEST_F(DistributedClusterProgress, ProgressCallback_FinalProgressAllUpdated) {
    ClusterUpdateProgress last_progress;
    mgr_.setProgressCallback([&](const ClusterUpdateProgress& p) {
        last_progress = p;
    });
    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    mgr_.updateCluster("1.7.0");
    EXPECT_EQ(last_progress.nodes_updated, 3u);
    EXPECT_EQ(last_progress.nodes_failed,  0u);
}

TEST_F(DistributedClusterProgress, ProgressCallback_ThrowingCallback_DoesNotCrash) {
    mgr_.setProgressCallback([](const ClusterUpdateProgress&) {
        throw std::runtime_error("test exception from callback");
    });
    mgr_.setNodeUpdateFunc(alwaysOkUpdate());
    mgr_.setNodeHealthCheckFunc(alwaysHealthy());

    // Should not propagate the exception.
    EXPECT_NO_THROW(mgr_.updateCluster("1.7.0"));
}

// ---------------------------------------------------------------------------
// Version skew protection — leader is always last
// ---------------------------------------------------------------------------

class DistributedClusterVersionSkew : public ::testing::Test {};

TEST_F(DistributedClusterVersionSkew, FiveNodes_LeaderAlwaysLast) {
    ClusterUpdateManager::Config cfg;
    cfg.nodes = {
        { "n1", "", false, "1.6.0" },
        { "n2", "", false, "1.6.0" },
        { "n3", "", true,  "1.6.0" }, // leader
        { "n4", "", false, "1.6.0" },
        { "n5", "", false, "1.6.0" },
    };
    cfg.default_options.rollback_on_failure = false;

    ClusterUpdateManager mgr(cfg);

    std::vector<std::string> order;
    mgr.setNodeUpdateFunc(
        [&](const ClusterNode& n, const std::string&,
            const ClusterUpdateOptions&) {
            order.push_back(n.node_id);
            return true;
        });
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    mgr.updateCluster("1.7.0");

    ASSERT_EQ(order.size(), 5u);
    EXPECT_EQ(order.back(), "n3") << "Leader (n3) must be updated last";
}

TEST_F(DistributedClusterVersionSkew, MultipleLeaders_AllAtEnd) {
    // When multiple nodes are marked as leaders they must all come after
    // all non-leader nodes in the update order.
    ClusterUpdateManager::Config cfg;
    cfg.nodes = {
        { "follower-1", "", false, "1.6.0" },
        { "leader-a",   "", true,  "1.6.0" },
        { "follower-2", "", false, "1.6.0" },
        { "leader-b",   "", true,  "1.6.0" },
    };
    cfg.default_options.rollback_on_failure = false;

    ClusterUpdateManager mgr(cfg);

    std::vector<std::string> order;
    mgr.setNodeUpdateFunc(
        [&](const ClusterNode& n, const std::string&,
            const ClusterUpdateOptions&) {
            order.push_back(n.node_id);
            return true;
        });
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    mgr.updateCluster("1.7.0");

    // Followers must come before any leader.
    auto f1_pos = std::find(order.begin(), order.end(), "follower-1");
    auto f2_pos = std::find(order.begin(), order.end(), "follower-2");
    auto la_pos = std::find(order.begin(), order.end(), "leader-a");
    auto lb_pos = std::find(order.begin(), order.end(), "leader-b");
    ASSERT_NE(f1_pos, order.end());
    ASSERT_NE(f2_pos, order.end());
    ASSERT_NE(la_pos, order.end());
    ASSERT_NE(lb_pos, order.end());
    EXPECT_LT(f1_pos, la_pos);
    EXPECT_LT(f1_pos, lb_pos);
    EXPECT_LT(f2_pos, la_pos);
    EXPECT_LT(f2_pos, lb_pos);
}

// ---------------------------------------------------------------------------
// Options override
// ---------------------------------------------------------------------------

class DistributedClusterOptionsOverride : public ::testing::Test {};

TEST_F(DistributedClusterOptionsOverride, PerCallOptions_Respected) {
    ClusterUpdateManager mgr(make3NodeConfig());
    mgr.setNodeUpdateFunc(alwaysOkUpdate());
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    // Override: rollback_on_failure = true
    ClusterUpdateOptions opts;
    opts.rollback_on_failure  = true;
    opts.health_check_timeout = std::chrono::seconds{10};

    auto result = mgr.updateCluster("1.7.0", opts);
    EXPECT_TRUE(result.success);
}

TEST_F(DistributedClusterOptionsOverride, DefaultOptions_UsedWhenNoOverride) {
    ClusterUpdateManager::Config cfg = make3NodeConfig();
    cfg.default_options.rollback_on_failure = true;

    ClusterUpdateManager mgr(cfg);
    mgr.setNodeUpdateFunc(alwaysOkUpdate());
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    // Single-arg overload should pick up default_options.
    auto result = mgr.updateCluster("1.7.0");
    EXPECT_TRUE(result.success);
}

// ---------------------------------------------------------------------------
// Single-node cluster (edge case)
// ---------------------------------------------------------------------------

class DistributedClusterSingleNode : public ::testing::Test {};

TEST_F(DistributedClusterSingleNode, SingleNode_Success) {
    ClusterUpdateManager mgr(makeSingleNodeConfig(true));
    mgr.setNodeUpdateFunc(alwaysOkUpdate());
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    auto result = mgr.updateCluster("1.7.0");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodes_updated, 1u);
}

TEST_F(DistributedClusterSingleNode, SingleNode_UpdateFunctionCalled) {
    std::atomic<int> calls{0};
    ClusterUpdateManager mgr(makeSingleNodeConfig(false));
    mgr.setNodeUpdateFunc([&](const ClusterNode&, const std::string&,
                               const ClusterUpdateOptions&) {
        ++calls;
        return true;
    });
    mgr.setNodeHealthCheckFunc(alwaysHealthy());

    mgr.updateCluster("1.7.0");
    EXPECT_EQ(calls.load(), 1);
}

// ---------------------------------------------------------------------------
// No registered callbacks (default behaviour)
// ---------------------------------------------------------------------------

class DistributedClusterNoCallbacks : public ::testing::Test {};

TEST_F(DistributedClusterNoCallbacks, NoNodeUpdateFunc_Succeeds) {
    // When no NodeUpdateFunc is registered the no-op default returns true.
    ClusterUpdateManager mgr(make3NodeConfig());
    // No callbacks registered.

    auto result = mgr.updateCluster("1.7.0");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodes_updated, 3u);
}

TEST_F(DistributedClusterNoCallbacks, NoProgressCallback_DoesNotCrash) {
    ClusterUpdateManager mgr(make2NodeConfig());
    mgr.setNodeUpdateFunc(alwaysOkUpdate());
    // No progress callback registered.

    EXPECT_NO_THROW(mgr.updateCluster("1.7.0"));
}
