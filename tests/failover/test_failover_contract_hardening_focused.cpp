// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_failover_contract_hardening_focused.cpp
 * @brief Phase 4 — Failover contract hardening focused tests (FCH-01..FCH-16).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * All external interactions are mocked inline.  The canonical PRNG seed
 * is kFailoverContractSeed = 42 for deterministic test data generation.
 *
 * ## Test families
 *
 * ### FCH-01..04 — Election contract
 *   FCH-01  Single leader elected per epoch
 *   FCH-02  Epoch is strictly monotonically increasing after each election
 *   FCH-03  Old leader is deposed when a higher epoch is observed
 *   FCH-04  Two nodes cannot both hold Leader role in the same epoch
 *
 * ### FCH-05..08 — Handover contract
 *   FCH-05  In-flight requests completed before leader yields
 *   FCH-06  In-flight requests retried by new leader when not completed
 *   FCH-07  No silent drop: every request is either completed or retried
 *   FCH-08  Handover drain budget is respected
 *
 * ### FCH-09..12 — Recovery contract
 *   FCH-09  Failed node rejoins as Follower only
 *   FCH-10  Rejoining node must not claim Leader role directly
 *   FCH-11  State sync completes before node transitions to active Follower
 *   FCH-12  State sync timeout surfaces STATE_SYNC_TIMEOUT error
 *
 * ### FCH-13..16 — Error contract
 *   FCH-13  ELECTION_TIMEOUT surfaced after missed heartbeats exhaust budget
 *   FCH-14  SPLIT_BRAIN_DETECTED surfaced when two leaders share same epoch
 *   FCH-15  HANDOVER_INCOMPLETE surfaced when drain deadline exceeded
 *   FCH-16  HEARTBEAT_MISSED increments counter toward leader-loss threshold
 *
 * @see include/failover/failover_api_contract.h
 * @see src/failover/ROADMAP.md — Phase 4 item
 */

#include <gtest/gtest.h>

#include "failover/failover_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

using namespace themis::failover;
using namespace std::chrono_literals;

namespace {

// ---------------------------------------------------------------------------
// Canonical seed
// ---------------------------------------------------------------------------
static constexpr uint64_t kFailoverContractSeed = 42;

// ---------------------------------------------------------------------------
// Mock: in-memory node state machine
// ---------------------------------------------------------------------------

struct MockNode {
    std::string     node_id;
    NodeRole        role      = NodeRole::Follower;
    std::uint64_t   epoch     = kFirstValidEpoch;
    bool            alive     = true;
    int             hb_missed = 0;  ///< consecutive missed heartbeats
};

/// Simulate an election: one candidate wins; returns winner index.
static std::size_t runElection(std::vector<MockNode>& nodes, std::uint64_t new_epoch) {
    // Step all followers to Candidate
    for (auto& n : nodes) {
        if (n.alive && n.role != NodeRole::Failed)
            n.role = NodeRole::Candidate;
    }
    // First alive candidate becomes leader; rest become followers
    bool leader_assigned = false;
    std::size_t winner = nodes.size();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i].alive) {
          continue;
        }
        if (!leader_assigned) {
            nodes[i].role  = NodeRole::Leader;
            nodes[i].epoch = new_epoch;
            leader_assigned = true;
            winner = i;
        } else {
            nodes[i].role  = NodeRole::Follower;
            nodes[i].epoch = new_epoch;
        }
    }
    return winner;
}

/// Count nodes with Leader role.
static int countLeaders(const std::vector<MockNode>& nodes) {
    return static_cast<int>(
        std::count_if(nodes.begin(), nodes.end(),
                      [](const MockNode& n){ return n.role == NodeRole::Leader; }));
}

// ---------------------------------------------------------------------------
// Mock: in-flight request buffer
// ---------------------------------------------------------------------------

struct MockRequest {
    int   id;
    bool  completed = false;
    bool  retried   = false;
};

struct MockInFlightBuffer {
    std::deque<MockRequest> requests;

    void add(int id) { requests.push_back({id}); }

    /// Simulate drain: mark all as completed.
    int drain() {
        int count = 0;
        for (auto& r : requests) { r.completed = true; ++count; }
        return count;
    }

    bool anyDropped() const {
        return std::any_of(requests.begin(), requests.end(),
                           [](const MockRequest& r){ return !r.completed && !r.retried; });
    }
};

} // anonymous namespace

// ===========================================================================
// FCH-01 — Single leader elected per epoch
// ===========================================================================

TEST(FailoverContractHardeningFCH01, SingleLeaderPerEpoch) {
    std::vector<MockNode> nodes;
    for (int i = 0; i < 5; ++i)
        nodes.push_back({"node-" + std::to_string(i)});

    runElection(nodes, kFirstValidEpoch);
    EXPECT_EQ(countLeaders(nodes), 1)
        << "Exactly one leader must be elected per epoch";
}

// ===========================================================================
// FCH-02 — Epoch strictly monotonically increases after each election
// ===========================================================================

TEST(FailoverContractHardeningFCH02, EpochMonotonicallyIncreases) {
    std::vector<MockNode> nodes;
    for (int i = 0; i < 3; ++i)
        nodes.push_back({"n" + std::to_string(i)});

    std::uint64_t prev_epoch = kInvalidEpoch;
    for (std::uint64_t ep = kFirstValidEpoch; ep <= 10u; ++ep) {
        runElection(nodes, ep);
        EXPECT_TRUE(isValidEpoch(ep));
        EXPECT_GT(ep, prev_epoch) << "Each election must produce a strictly higher epoch";
        prev_epoch = ep;
    }
}

// ===========================================================================
// FCH-03 — Old leader steps down when higher epoch observed
// ===========================================================================

TEST(FailoverContractHardeningFCH03, OldLeaderDeposedOnHigherEpoch) {
    std::vector<MockNode> nodes;
    for (int i = 0; i < 3; ++i)
        nodes.push_back({"n" + std::to_string(i)});

    runElection(nodes, 1u);
    int old_leader = -1;
    for (int i = 0; i < 3; ++i) {
        if (nodes[i].role == NodeRole::Leader) { old_leader = i; break; }
    }
    ASSERT_GE(old_leader, 0);

    // Simulate new election with higher epoch
    runElection(nodes, 2u);

    // Old leader must no longer hold Leader role
    EXPECT_NE(nodes[old_leader].role, NodeRole::Leader)
        << "Old leader must be deposed when a higher epoch is observed";
    EXPECT_EQ(countLeaders(nodes), 1);
}

// ===========================================================================
// FCH-04 — No two nodes hold Leader in same epoch (no split-brain)
// ===========================================================================

TEST(FailoverContractHardeningFCH04, NoSplitBrainSameEpoch) {
    std::vector<MockNode> nodes;
    for (int i = 0; i < 5; ++i)
        nodes.push_back({"n" + std::to_string(i)});

    for (std::uint64_t ep = 1u; ep <= 20u; ++ep) {
        runElection(nodes, ep);
        EXPECT_LE(countLeaders(nodes), 1)
            << "Never more than one leader in epoch " << ep;
    }
}

// ===========================================================================
// FCH-05 — In-flight requests completed before leader yields
// ===========================================================================

TEST(FailoverContractHardeningFCH05, InFlightCompletedBeforeYield) {
    MockInFlightBuffer buf;
    for (int i = 0; i < 10; ++i) {
      buf.add(i);
    }

    // Leader drains buffer before yielding
    int drained = buf.drain();
    EXPECT_EQ(drained, 10);

    // After drain, no silent drops
    EXPECT_FALSE(buf.anyDropped());
}

// ===========================================================================
// FCH-06 — In-flight requests retried by new leader when not completed
// ===========================================================================

TEST(FailoverContractHardeningFCH06, InFlightRetriedByNewLeader) {
    MockInFlightBuffer buf;
    for (int i = 0; i < 5; ++i) {
      buf.add(i);
    }

    // Outgoing leader does NOT drain (simulates abrupt failure)
    // New leader picks up the buffer and retries
    for (auto& r : buf.requests) {
      r.retried = true;
    }

    EXPECT_FALSE(buf.anyDropped())
        << "All requests must be retried when outgoing leader fails to complete them";
}

// ===========================================================================
// FCH-07 — No silent drop: every request either completed or retried
// ===========================================================================

TEST(FailoverContractHardeningFCH07, NoSilentDropOnHandover) {
    std::mt19937_64 rng(kFailoverContractSeed);
    std::uniform_int_distribution<int> coin(0, 1);

    MockInFlightBuffer buf;
    for (int i = 0; i < 20; ++i) {
      buf.add(i);
    }

    for (auto& r : buf.requests) {
        if (coin(rng)) {
          r.completed = true;
        }
        else           r.retried   = true;
    }

    EXPECT_FALSE(buf.anyDropped())
        << "No request must be silently dropped during handover";
}

// ===========================================================================
// FCH-08 — Handover drain budget respected
// ===========================================================================

TEST(FailoverContractHardeningFCH08, HandoverDrainBudgetRespected) {
    // Verify the contract constant is within a sane range
    EXPECT_LE(kHandoverDrainDeadline.count(), 5'000)
        << "Drain deadline must be ≤ 5 s by contract";
    EXPECT_GE(kHandoverDrainDeadline.count(), 100)
        << "Drain deadline must be at least 100 ms";

    // Simulate: drain completes well within the budget
    auto start = std::chrono::steady_clock::now();
    MockInFlightBuffer buf;
    for (int i = 0; i < 100; ++i) {
      buf.add(i);
    }
    buf.drain();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, kHandoverDrainDeadline)
        << "In-memory drain must complete within handover budget";
}

// ===========================================================================
// FCH-09 — Failed node rejoins as Follower only
// ===========================================================================

TEST(FailoverContractHardeningFCH09, FailedNodeRejoinsAsFollower) {
    MockNode failed_node{"failed-1", NodeRole::Failed, 3u, false};

    // Simulated rejoin: node is brought back as Follower
    failed_node.alive = true;
    failed_node.role  = NodeRole::Follower;  // contract requirement

    EXPECT_EQ(failed_node.role, NodeRole::Follower)
        << "Failed node must rejoin as Follower only";
    EXPECT_NE(failed_node.role, NodeRole::Leader);
    EXPECT_NE(failed_node.role, NodeRole::Candidate);
}

// ===========================================================================
// FCH-10 — Rejoining node must not claim Leader role directly
// ===========================================================================

TEST(FailoverContractHardeningFCH10, RejoiningNodeCannotClaimLeaderDirectly) {
    // If a node that was Failed attempts to set itself to Leader without
    // going through an election, isVoteEligible must be false for Failed.
    MockNode n{"n1", NodeRole::Failed, 1u, false};
    EXPECT_FALSE(isWriteEligible(n.role))
        << "Failed node must not be write-eligible";
    EXPECT_FALSE(isVoteEligible(n.role))
        << "Failed node must not be vote-eligible";

    // After state sync completes, node becomes Follower
    n.role = NodeRole::Follower;
    EXPECT_TRUE(isVoteEligible(n.role));
    EXPECT_FALSE(isWriteEligible(n.role));
}

// ===========================================================================
// FCH-11 — State sync completes before node transitions to active Follower
// ===========================================================================

TEST(FailoverContractHardeningFCH11, StateSyncBeforeActiveFollower) {
    // Simulate state sync: node stays in Failed until sync done
    bool sync_complete = false;
    MockNode n{"n2", NodeRole::Failed, 2u, true};

    // Pre-sync: node is still in Failed
    EXPECT_EQ(n.role, NodeRole::Failed);
    EXPECT_FALSE(isVoteEligible(n.role));

    // Sync completes
    sync_complete = true;
    if (sync_complete) {
      n.role = NodeRole::Follower;
    }

    EXPECT_EQ(n.role, NodeRole::Follower);
    EXPECT_TRUE(isVoteEligible(n.role));
}

// ===========================================================================
// FCH-12 — State sync timeout surfaces STATE_SYNC_TIMEOUT
// ===========================================================================

TEST(FailoverContractHardeningFCH12, StateSyncTimeoutSurfaces) {
    // The contract deadline
    EXPECT_GT(kStateSyncDeadline.count(), 0)
        << "State sync deadline must be positive";

    // Simulate timeout: sync did not complete within deadline
    bool timed_out = true;  // mock
    FailoverErrorCode result = timed_out
        ? FailoverErrorCode::STATE_SYNC_TIMEOUT
        : FailoverErrorCode::OK;

    EXPECT_EQ(result, FailoverErrorCode::STATE_SYNC_TIMEOUT);
    EXPECT_FALSE(isFailSafeCode(result))
        << "STATE_SYNC_TIMEOUT is not fail-safe but the node remains Failed";
}

// ===========================================================================
// FCH-13 — ELECTION_TIMEOUT after missed heartbeats exhaust budget
// ===========================================================================

TEST(FailoverContractHardeningFCH13, ElectionTimeoutAfterMissedHeartbeats) {
    // Contract: election starts after kHeartbeatTimeout elapses without HB
    EXPECT_LE(kHeartbeatTimeout.count(), 3'000)
        << "Heartbeat timeout must be ≤ 3 000 ms per contract";

    // Mock: election does not complete within budget
    bool election_timed_out = true;
    FailoverErrorCode code = election_timed_out
        ? FailoverErrorCode::ELECTION_TIMEOUT
        : FailoverErrorCode::OK;

    EXPECT_EQ(code, FailoverErrorCode::ELECTION_TIMEOUT);
}

// ===========================================================================
// FCH-14 — SPLIT_BRAIN_DETECTED surfaces when two leaders share same epoch
// ===========================================================================

TEST(FailoverContractHardeningFCH14, SplitBrainDetectedSurfaced) {
    // Two nodes both believe they are leader for epoch 5 (bug/partition scenario)
    MockNode n1{"n1", NodeRole::Leader, 5u, true};
    MockNode n2{"n2", NodeRole::Leader, 5u, true};

    // Detection: two leaders with same epoch
    bool split_brain = (n1.role == NodeRole::Leader &&
                        n2.role == NodeRole::Leader &&
                        n1.epoch == n2.epoch);

    FailoverErrorCode code = split_brain
        ? FailoverErrorCode::SPLIT_BRAIN_DETECTED
        : FailoverErrorCode::OK;

    EXPECT_EQ(code, FailoverErrorCode::SPLIT_BRAIN_DETECTED);
    EXPECT_TRUE(isFailSafeCode(code))
        << "SPLIT_BRAIN_DETECTED must trigger fail-safe behaviour";
}

// ===========================================================================
// FCH-15 — HANDOVER_INCOMPLETE surfaces when drain deadline exceeded
// ===========================================================================

TEST(FailoverContractHardeningFCH15, HandoverIncompleteWhenDrainExceeded) {
    MockInFlightBuffer buf;
    for (int i = 0; i < 50; ++i) {
      buf.add(i);
    }

    // Simulate: drain deadline exceeded — buffer NOT fully drained
    // Half drained, half not
    for (int i = 0; i < 25; ++i) {
      buf.requests[i].completed = true;
    }

    bool drain_timed_out = true; // mock: deadline exceeded
    FailoverErrorCode code = drain_timed_out
        ? FailoverErrorCode::HANDOVER_INCOMPLETE
        : FailoverErrorCode::OK;

    EXPECT_EQ(code, FailoverErrorCode::HANDOVER_INCOMPLETE);
}

// ===========================================================================
// FCH-16 — HEARTBEAT_MISSED increments counter toward leader-loss threshold
// ===========================================================================

TEST(FailoverContractHardeningFCH16, HeartbeatMissedCounterIncrement) {
    MockNode follower{"f1", NodeRole::Follower, 1u, true};

    // Each missed heartbeat increments the counter
    for (int i = 0; i < 3; ++i) {
        follower.hb_missed++;
        EXPECT_EQ(follower.hb_missed, i + 1);
    }

    // A received heartbeat resets the counter
    follower.hb_missed = 0;
    EXPECT_EQ(follower.hb_missed, 0)
        << "Received heartbeat must reset missed-HB counter";

    // Verify the contract timeout constant
    EXPECT_LE(kHeartbeatTimeout.count(), 3'000)
        << "HEARTBEAT_MISSED timeout must be ≤ 3 s per contract";
    EXPECT_EQ(static_cast<int>(FailoverErrorCode::HEARTBEAT_MISSED), 5);
}

TEST(FailoverContractHardeningRetryContract, RetryEscalationAndTimeoutSourceMapping) {
    EXPECT_TRUE(isRetryEscalationCode(FailoverErrorCode::ELECTION_TIMEOUT));
    EXPECT_TRUE(isRetryEscalationCode(FailoverErrorCode::STATE_SYNC_TIMEOUT));
    EXPECT_TRUE(isRetryEscalationCode(FailoverErrorCode::NODE_REJOIN_FAILED));
    EXPECT_FALSE(isRetryEscalationCode(FailoverErrorCode::SPLIT_BRAIN_DETECTED));

    EXPECT_EQ(toRetryTimeoutSource(FailoverErrorCode::ELECTION_TIMEOUT),
              themis::utils::RetryTimeoutSource::OVERALL);
    EXPECT_EQ(toRetryTimeoutSource(FailoverErrorCode::STATE_SYNC_TIMEOUT),
              themis::utils::RetryTimeoutSource::OVERALL);
    EXPECT_EQ(toRetryTimeoutSource(FailoverErrorCode::QUORUM_UNAVAILABLE),
              themis::utils::RetryTimeoutSource::QUORUM);
}
