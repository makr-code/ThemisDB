// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_coordinated_update_manager.cpp
 * @brief Unit tests for CoordinatedUpdateManager
 *        (Phase 4 – multi-node coordinated update with replication-safe
 *        sequencing).
 *
 * All tests use only the public API.  HotReloadEngine is replaced by a
 * lightweight stub that overrides the two virtual methods used by the
 * manager: applyHotReload() and rollback().
 */

#include <gtest/gtest.h>

#include "updates/coordinated_update_manager.h"

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>

using namespace themis::updates;

// ---------------------------------------------------------------------------
// Stub HotReloadEngine
// ---------------------------------------------------------------------------

class StubEngine : public HotReloadEngine {
public:
    explicit StubEngine(bool apply_ok   = true,
                        bool rollback_ok = true)
        : HotReloadEngine(nullptr, nullptr,
                          []() {
                              HotReloadEngine::Config c;
                              c.download_directory = "/tmp/stub_coord_dl";
                              c.backup_directory   = "/tmp/stub_coord_bak";
                              c.verify_signatures  = false;
                              c.create_backup      = false;
                              return c;
                          }())
        , apply_ok_(apply_ok)
        , rollback_ok_(rollback_ok)
    {}

    ReloadResult applyHotReload(const std::string& version,
                                bool /*verify_only*/ = false) override {
        ++apply_count;
        ReloadResult r;
        r.success     = apply_ok_;
        r.rollback_id = "stub_rid_" + version;
        if (!apply_ok_) {
            r.error_message = "stub: apply failed";
        }
        return r;
    }

    bool rollback(const std::string& /*rid*/) override {
        ++rollback_count;
        return rollback_ok_;
    }

    std::atomic<int> apply_count{0};
    std::atomic<int> rollback_count{0};

private:
    bool apply_ok_;
    bool rollback_ok_;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<StubEngine> makeEngine(bool apply_ok   = true,
                                               bool rollback_ok = true) {
    return std::make_shared<StubEngine>(apply_ok, rollback_ok);
}

/// Build a 2-node config: node-0 (seq=0), node-1 (seq=1, leader).
static CoordinatedUpdateConfig make2NodeConfig(
    const std::string& local_id,
    bool leader_last = true)
{
    CoordinatedUpdateConfig cfg;
    cfg.version       = "1.7.0";
    cfg.local_node_id = local_id;
    cfg.leader_last   = leader_last;
    cfg.rollback_on_failure = false;
    cfg.nodes = {
        {"node-0", 0, false},
        {"node-1", 1, true},
    };
    return cfg;
}

/// Build a 3-node config with explicit sequence numbers.
static CoordinatedUpdateConfig make3NodeConfig(
    const std::string& local_id,
    bool rollback_on_failure = false)
{
    CoordinatedUpdateConfig cfg;
    cfg.version              = "1.7.0";
    cfg.local_node_id        = local_id;
    cfg.leader_last          = true;
    cfg.rollback_on_failure  = rollback_on_failure;
    cfg.nodes = {
        {"node-a", 0, false},
        {"node-b", 1, false},
        {"node-c", 2, true},   // leader
    };
    return cfg;
}

// ---------------------------------------------------------------------------
// Construction tests
// ---------------------------------------------------------------------------

class CoordinatedConstructionTest : public ::testing::Test {};

TEST_F(CoordinatedConstructionTest, NullEngine_Throws) {
    CoordinatedUpdateConfig cfg = make3NodeConfig("node-a");
    EXPECT_THROW(CoordinatedUpdateManager(nullptr, cfg),
                 std::invalid_argument);
}

TEST_F(CoordinatedConstructionTest, EmptyNodeList_Throws) {
    auto engine = makeEngine();
    CoordinatedUpdateConfig cfg;
    cfg.local_node_id = "node-a";
    cfg.nodes         = {};
    EXPECT_THROW(CoordinatedUpdateManager(engine, cfg),
                 std::invalid_argument);
}

TEST_F(CoordinatedConstructionTest, UnknownLocalNodeId_Throws) {
    auto engine = makeEngine();
    CoordinatedUpdateConfig cfg = make3NodeConfig("node-x"); // unknown
    EXPECT_THROW(CoordinatedUpdateManager(engine, cfg),
                 std::invalid_argument);
}

TEST_F(CoordinatedConstructionTest, EmptyLocalNodeId_Throws) {
    auto engine = makeEngine();
    CoordinatedUpdateConfig cfg = make3NodeConfig("");
    cfg.local_node_id = "";
    EXPECT_THROW(CoordinatedUpdateManager(engine, cfg),
                 std::invalid_argument);
}

TEST_F(CoordinatedConstructionTest, ValidConfig_DoesNotThrow) {
    auto engine = makeEngine();
    CoordinatedUpdateConfig cfg = make3NodeConfig("node-a");
    EXPECT_NO_THROW(CoordinatedUpdateManager(engine, cfg));
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

class CoordinatedAccessorTest : public ::testing::Test {
protected:
    std::shared_ptr<StubEngine> engine_ = makeEngine();
};

TEST_F(CoordinatedAccessorTest, TotalNodes_MatchesNodeList) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-a"));
    EXPECT_EQ(m.totalNodes(), 3u);
}

TEST_F(CoordinatedAccessorTest, LocalSequenceNumber_FirstNode) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-a"));
    EXPECT_EQ(m.localSequenceNumber(), 0u);
}

TEST_F(CoordinatedAccessorTest, LocalSequenceNumber_MiddleNode) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-b"));
    EXPECT_EQ(m.localSequenceNumber(), 1u);
}

TEST_F(CoordinatedAccessorTest, LocalSequenceNumber_LeaderNode_IsLast) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-c"));
    // leader_last=true → leader gets the highest sequence
    EXPECT_EQ(m.localSequenceNumber(), 2u);
}

TEST_F(CoordinatedAccessorTest, IsLeader_TrueForLeaderNode) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-c"));
    EXPECT_TRUE(m.isLeader());
}

TEST_F(CoordinatedAccessorTest, IsLeader_FalseForNonLeader) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-a"));
    EXPECT_FALSE(m.isLeader());
}

TEST_F(CoordinatedAccessorTest, InitialStatuses_AllPending) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-a"));
    for (const auto& s : m.nodeStatuses()) {
        EXPECT_EQ(s.state, NodeUpdateState::PENDING);
        EXPECT_TRUE(s.rollback_id.empty());
        EXPECT_TRUE(s.error_message.empty());
    }
}

TEST_F(CoordinatedAccessorTest, InitialStatus_LocalFlagSet) {
    CoordinatedUpdateManager m(engine_, make3NodeConfig("node-b"));
    bool found_local = false;
    for (const auto& s : m.nodeStatuses()) {
        if (s.is_local) {
            EXPECT_EQ(s.node_id, "node-b");
            found_local = true;
        }
    }
    EXPECT_TRUE(found_local);
}

// ---------------------------------------------------------------------------
// applyLocalUpdate – first node in sequence (no predecessor)
// ---------------------------------------------------------------------------

class CoordinatedFirstNodeTest : public ::testing::Test {
protected:
    std::shared_ptr<StubEngine> engine_ = makeEngine();
    // node-a has sequence 0 → no predecessor
    CoordinatedUpdateManager mgr_{engine_, make3NodeConfig("node-a")};
};

TEST_F(CoordinatedFirstNodeTest, Apply_CallsEngine) {
    auto result = mgr_.applyLocalUpdate();
    EXPECT_EQ(engine_->apply_count.load(), 1);
}

TEST_F(CoordinatedFirstNodeTest, Apply_Succeeds) {
    auto result = mgr_.applyLocalUpdate();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodes_updated, 1u);
    EXPECT_EQ(result.nodes_failed, 0u);
}

TEST_F(CoordinatedFirstNodeTest, Apply_SetsLocalStatusCompleted) {
    mgr_.applyLocalUpdate();
    for (const auto& s : mgr_.nodeStatuses()) {
        if (s.is_local) {
            EXPECT_EQ(s.state, NodeUpdateState::COMPLETED);
            EXPECT_EQ(s.rollback_id, "stub_rid_1.7.0");
        }
    }
}

TEST_F(CoordinatedFirstNodeTest, Apply_DoesNotCallWaitFunc_WhenNoPredecessor) {
    bool wait_called = false;
    mgr_.setWaitForPreviousFunc(
        [&](const std::string&, std::chrono::milliseconds) {
            wait_called = true;
            return true;
        });
    mgr_.applyLocalUpdate();
    EXPECT_FALSE(wait_called);
}

TEST_F(CoordinatedFirstNodeTest, Apply_CallsSignalFunc) {
    bool signal_called = false;
    std::string signalled_node;
    bool signalled_ok = false;
    mgr_.setSignalReadyFunc(
        [&](const std::string& id, bool ok) {
            signal_called = true;
            signalled_node = id;
            signalled_ok   = ok;
        });
    mgr_.applyLocalUpdate();
    EXPECT_TRUE(signal_called);
    EXPECT_EQ(signalled_node, "node-a");
    EXPECT_TRUE(signalled_ok);
}

TEST_F(CoordinatedFirstNodeTest, Apply_Idempotent_SecondCallSkipsEngine) {
    mgr_.applyLocalUpdate();
    auto second = mgr_.applyLocalUpdate();
    EXPECT_TRUE(second.success);
    EXPECT_EQ(engine_->apply_count.load(), 1); // engine called only once
}

// ---------------------------------------------------------------------------
// applyLocalUpdate – middle node waits for predecessor
// ---------------------------------------------------------------------------

class CoordinatedMiddleNodeTest : public ::testing::Test {
protected:
    std::shared_ptr<StubEngine> engine_ = makeEngine();
    // node-b has sequence 1 → predecessor is node-a
    CoordinatedUpdateManager mgr_{engine_, make3NodeConfig("node-b")};
};

TEST_F(CoordinatedMiddleNodeTest, Apply_CallsWaitFunc) {
    std::string waited_for;
    mgr_.setWaitForPreviousFunc(
        [&](const std::string& id, std::chrono::milliseconds) {
            waited_for = id;
            return true;
        });
    mgr_.applyLocalUpdate();
    EXPECT_EQ(waited_for, "node-a");
}

TEST_F(CoordinatedMiddleNodeTest, Apply_Succeeds_WhenPredecessorReady) {
    mgr_.setWaitForPreviousFunc(
        [](const std::string&, std::chrono::milliseconds) { return true; });
    auto result = mgr_.applyLocalUpdate();
    EXPECT_TRUE(result.success);
}

TEST_F(CoordinatedMiddleNodeTest, Apply_Fails_WhenPredecessorTimesOut) {
    mgr_.setWaitForPreviousFunc(
        [](const std::string&, std::chrono::milliseconds) { return false; });
    auto result = mgr_.applyLocalUpdate();
    EXPECT_FALSE(result.success);
    EXPECT_EQ(engine_->apply_count.load(), 0);
    EXPECT_EQ(result.nodes_failed, 1u);
}

TEST_F(CoordinatedMiddleNodeTest, Apply_SetsFailedState_OnTimeout) {
    mgr_.setWaitForPreviousFunc(
        [](const std::string&, std::chrono::milliseconds) { return false; });
    mgr_.applyLocalUpdate();
    for (const auto& s : mgr_.nodeStatuses()) {
        if (s.is_local) {
            EXPECT_EQ(s.state, NodeUpdateState::FAILED);
        }
    }
}

// ---------------------------------------------------------------------------
// applyLocalUpdate – engine failure
// ---------------------------------------------------------------------------

class CoordinatedEngineFailureTest : public ::testing::Test {
protected:
    std::shared_ptr<StubEngine> engine_ = makeEngine(/*apply_ok=*/false);
    CoordinatedUpdateConfig cfg_{make3NodeConfig("node-a",
                                                  /*rollback=*/false)};
    CoordinatedUpdateManager mgr_{engine_, cfg_};
};

TEST_F(CoordinatedEngineFailureTest, Apply_ReturnsFailed) {
    auto result = mgr_.applyLocalUpdate();
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.nodes_failed, 1u);
}

TEST_F(CoordinatedEngineFailureTest, Apply_SetsLocalStatusFailed) {
    mgr_.applyLocalUpdate();
    for (const auto& s : mgr_.nodeStatuses()) {
        if (s.is_local) {
            EXPECT_EQ(s.state, NodeUpdateState::FAILED);
        }
    }
}

TEST_F(CoordinatedEngineFailureTest, Apply_SignalsFailure_ToNextNode) {
    bool signalled_ok = true;
    mgr_.setSignalReadyFunc(
        [&](const std::string&, bool ok) { signalled_ok = ok; });
    mgr_.applyLocalUpdate();
    EXPECT_FALSE(signalled_ok);
}

// ---------------------------------------------------------------------------
// Auto-rollback on failure
// ---------------------------------------------------------------------------

class CoordinatedAutoRollbackTest : public ::testing::Test {};

TEST_F(CoordinatedAutoRollbackTest, AutoRollback_TriggeredOnFailure) {
    auto engine = makeEngine(/*apply_ok=*/false);
    CoordinatedUpdateConfig cfg = make3NodeConfig("node-a", /*rollback=*/true);
    cfg.rollback_on_failure = true;
    CoordinatedUpdateManager mgr(engine, cfg);

    auto result = mgr.applyLocalUpdate();
    EXPECT_FALSE(result.success);
    // Rollback is called even though apply failed (no rollback_id was stored
    // so engine->rollback is NOT called, but the state should be ROLLED_BACK).
    EXPECT_EQ(result.nodes_rolled_back, 1u);
    for (const auto& s : mgr.nodeStatuses()) {
        if (s.is_local) {
            EXPECT_EQ(s.state, NodeUpdateState::ROLLED_BACK);
        }
    }
}

TEST_F(CoordinatedAutoRollbackTest, AutoRollback_UsesRollbackId_OnSuccessThenFail) {
    // This scenario tests that when applyHotReload *succeeds* but then some
    // external caller triggers rollback(), the stored rollback_id is used.
    auto engine = makeEngine(/*apply_ok=*/true, /*rollback_ok=*/true);
    CoordinatedUpdateConfig cfg = make3NodeConfig("node-a");
    cfg.rollback_on_failure = false;
    CoordinatedUpdateManager mgr(engine, cfg);

    mgr.applyLocalUpdate();
    EXPECT_EQ(engine->rollback_count.load(), 0);

    bool ok = mgr.rollback("post-update failure");
    EXPECT_TRUE(ok);
    EXPECT_EQ(engine->rollback_count.load(), 1);
}

// ---------------------------------------------------------------------------
// Rollback tests
// ---------------------------------------------------------------------------

class CoordinatedRollbackTest : public ::testing::Test {
protected:
    std::shared_ptr<StubEngine> engine_ = makeEngine();
    CoordinatedUpdateManager mgr_{engine_, make3NodeConfig("node-a")};

    void SetUp() override { mgr_.applyLocalUpdate(); }
};

TEST_F(CoordinatedRollbackTest, Rollback_CallsEngine) {
    mgr_.rollback();
    EXPECT_EQ(engine_->rollback_count.load(), 1);
}

TEST_F(CoordinatedRollbackTest, Rollback_SetsRolledBackState) {
    mgr_.rollback();
    for (const auto& s : mgr_.nodeStatuses()) {
        if (s.is_local) {
            EXPECT_EQ(s.state, NodeUpdateState::ROLLED_BACK);
        }
    }
}

TEST_F(CoordinatedRollbackTest, Rollback_CalledTwice_ReturnsFalse) {
    EXPECT_TRUE(mgr_.rollback());
    EXPECT_FALSE(mgr_.rollback());
    EXPECT_EQ(engine_->rollback_count.load(), 1);
}

TEST_F(CoordinatedRollbackTest, Rollback_EngineFailure_ReturnsFalse) {
    auto engine = makeEngine(/*apply_ok=*/true, /*rollback_ok=*/false);
    CoordinatedUpdateManager mgr(engine, make3NodeConfig("node-a"));
    mgr.applyLocalUpdate();
    EXPECT_FALSE(mgr.rollback());
}

TEST_F(CoordinatedRollbackTest, Rollback_WithoutPriorApply_NoEngineCall) {
    auto engine = makeEngine();
    CoordinatedUpdateManager mgr(engine, make3NodeConfig("node-a"));
    // No applyLocalUpdate called → rollback_id is empty
    bool ok = mgr.rollback("precautionary");
    EXPECT_TRUE(ok);
    EXPECT_EQ(engine->rollback_count.load(), 0);
}

TEST_F(CoordinatedRollbackTest, Apply_AfterRollback_Rejected) {
    mgr_.rollback();
    auto result = mgr_.applyLocalUpdate();
    EXPECT_FALSE(result.success);
    EXPECT_EQ(engine_->apply_count.load(), 1); // only initial apply
}

// ---------------------------------------------------------------------------
// Progress callback
// ---------------------------------------------------------------------------

class CoordinatedProgressTest : public ::testing::Test {};

TEST_F(CoordinatedProgressTest, ProgressCallback_InvokedDuringApply) {
    auto engine = makeEngine();
    CoordinatedUpdateManager mgr(engine, make3NodeConfig("node-a"));

    std::vector<std::string> messages;
    mgr.setProgressCallback(
        [&](uint32_t /*done*/, uint32_t /*total*/, const std::string& msg) {
            messages.push_back(msg);
        });

    mgr.applyLocalUpdate();
    EXPECT_FALSE(messages.empty());
}

// ---------------------------------------------------------------------------
// Leader-last sequencing
// ---------------------------------------------------------------------------

class CoordinatedLeaderLastTest : public ::testing::Test {};

TEST_F(CoordinatedLeaderLastTest, LeaderHasHighestSequence) {
    auto engine = makeEngine();
    // Provide nodes with scrambled sequence numbers; leader_last should reorder.
    CoordinatedUpdateConfig cfg;
    cfg.version       = "1.7.0";
    cfg.local_node_id = "leader";
    cfg.leader_last   = true;
    cfg.nodes = {
        {"leader",   5, true},
        {"replica-1", 0, false},
        {"replica-2", 0, false},
    };
    CoordinatedUpdateManager mgr(engine, cfg);
    // Leader should get sequence 2 (last of 3 nodes).
    EXPECT_EQ(mgr.localSequenceNumber(), 2u);
    EXPECT_TRUE(mgr.isLeader());
}

TEST_F(CoordinatedLeaderLastTest, NonLeader_WithLeaderLastFalse_UsesOriginalSeq) {
    auto engine = makeEngine();
    CoordinatedUpdateConfig cfg = make2NodeConfig("node-0",
                                                   /*leader_last=*/false);
    CoordinatedUpdateManager mgr(engine, cfg);
    EXPECT_EQ(mgr.localSequenceNumber(), 0u);
}

// ---------------------------------------------------------------------------
// Full lifecycle: 2-node sequential update
// ---------------------------------------------------------------------------

class CoordinatedLifecycleTest : public ::testing::Test {};

TEST_F(CoordinatedLifecycleTest, TwoNodeSequence_FirstThenLeader) {
    auto engine_a = makeEngine();
    auto engine_c = makeEngine();

    CoordinatedUpdateConfig cfg_a = make2NodeConfig("node-0");
    cfg_a.rollback_on_failure = false;
    CoordinatedUpdateConfig cfg_c = make2NodeConfig("node-1");
    cfg_c.rollback_on_failure = false;

    CoordinatedUpdateManager mgr_a(engine_a, cfg_a);
    CoordinatedUpdateManager mgr_c(engine_c, cfg_c);

    // node-0 (seq=0): no predecessor, applies immediately
    auto result_a = mgr_a.applyLocalUpdate();
    EXPECT_TRUE(result_a.success);

    // node-1 (seq=1, leader): waits for node-0 (simulated as already ready)
    mgr_c.setWaitForPreviousFunc(
        [](const std::string& /*id*/, std::chrono::milliseconds /*t*/) {
            return true; // predecessor ready
        });
    auto result_c = mgr_c.applyLocalUpdate();
    EXPECT_TRUE(result_c.success);
    EXPECT_TRUE(mgr_c.isLeader());

    EXPECT_EQ(engine_a->apply_count.load(), 1);
    EXPECT_EQ(engine_c->apply_count.load(), 1);
}

TEST_F(CoordinatedLifecycleTest, FailureInFirst_StopsLeaderUpdate) {
    auto engine_a = makeEngine(/*apply_ok=*/false);
    auto engine_c = makeEngine();

    CoordinatedUpdateConfig cfg_a = make2NodeConfig("node-0");
    cfg_a.rollback_on_failure = false;
    CoordinatedUpdateConfig cfg_c = make2NodeConfig("node-1");
    cfg_c.rollback_on_failure = false;

    CoordinatedUpdateManager mgr_a(engine_a, cfg_a);
    CoordinatedUpdateManager mgr_c(engine_c, cfg_c);

    auto result_a = mgr_a.applyLocalUpdate();
    EXPECT_FALSE(result_a.success);

    // Leader sees failure signal from node-0 → wait returns false
    mgr_c.setWaitForPreviousFunc(
        [](const std::string& /*id*/, std::chrono::milliseconds) {
            return false; // predecessor failed
        });
    auto result_c = mgr_c.applyLocalUpdate();
    EXPECT_FALSE(result_c.success);
    EXPECT_EQ(engine_c->apply_count.load(), 0); // leader never tried
}
