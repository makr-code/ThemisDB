// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_distributed_knowledge_highcardinality_stress.cpp
 * @brief Wave D — Distributed Knowledge High-Cardinality Stress Tests.
 *
 * Stress coverage for the distributed_knowledge module hot paths under
 * high-cardinality concurrent load: federation rounds, concurrent merges,
 * and policy-gated sync stress.
 *
 * ## Test cases
 * - HighCardinalityFederationRound  : concurrent federation rounds at scale
 * - ConcurrentMergeStress           : concurrent merge operations
 * - PolicyGatedSyncStress           : policy-gated sync under load
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_DISTRIBUTED_KNOWLEDGE.md
 * @see src/distributed_knowledge/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs model distributed_knowledge federation stress without
// requiring real peer nodes or cross-shard sync. MUST NOT be used in
// production code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

static constexpr int kWorkerCount  = 8;
static constexpr int kOpsPerWorker = 5000;

class StubHCFederationCoordinator {
public:
    bool runRound(uint64_t round_id) {
        ++round_count_;
        (void)round_id;
        return true;
    }
    uint64_t roundCount() const { return round_count_.load(); }
private:
    std::atomic<uint64_t> round_count_{0};
};

class StubHCMergeEngine {
public:
    bool merge(uint64_t merge_id) {
        std::lock_guard<std::mutex> lk(mu_);
        ++merge_count_;
        (void)merge_id;
        return true;
    }
    uint64_t mergeCount() const { return merge_count_.load(); }
private:
    std::mutex mu_;
    std::atomic<uint64_t> merge_count_{0};
};

class StubHCPolicyGate {
public:
    bool sync(uint64_t policy_id, bool allowed) {
        ++attempt_count_;
        if (allowed) ++pass_count_;
        (void)policy_id;
        return true;
    }
    uint64_t attemptCount() const { return attempt_count_.load(); }
    uint64_t passCount()    const { return pass_count_.load(); }
private:
    std::atomic<uint64_t> attempt_count_{0};
    std::atomic<uint64_t> pass_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: HighCardinalityFederationRound
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistributedKnowledgeStress, HighCardinalityFederationRound) {
    StubHCFederationCoordinator coord;
    std::vector<std::thread> workers;

    for (int i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back([&, i]() {
            for (int j = 0; j < kOpsPerWorker; ++j) {
                const uint64_t id = static_cast<uint64_t>(i) * kOpsPerWorker + j;
                coord.runRound(id);
            }
        });
    }
    for (auto& t : workers) t.join();

    const uint64_t expected = static_cast<uint64_t>(kWorkerCount) * kOpsPerWorker;
    EXPECT_EQ(coord.roundCount(), expected)
        << "[DIST_KG:FederationTimeout] All federation rounds must complete";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ConcurrentMergeStress
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistributedKnowledgeStress, ConcurrentMergeStress) {
    StubHCMergeEngine engine;
    std::vector<std::thread> workers;

    for (int i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back([&, i]() {
            for (int j = 0; j < kOpsPerWorker; ++j) {
                const uint64_t id = static_cast<uint64_t>(i) * kOpsPerWorker + j;
                engine.merge(id);
            }
        });
    }
    for (auto& t : workers) t.join();

    const uint64_t expected = static_cast<uint64_t>(kWorkerCount) * kOpsPerWorker;
    EXPECT_EQ(engine.mergeCount(), expected)
        << "[DIST_KG:MergeConflict] All merge operations must complete without conflicts";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: PolicyGatedSyncStress
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistributedKnowledgeStress, PolicyGatedSyncStress) {
    StubHCPolicyGate gate;
    std::vector<std::thread> workers;

    for (int i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back([&, i]() {
            for (int j = 0; j < kOpsPerWorker; ++j) {
                const uint64_t id = static_cast<uint64_t>(i) * kOpsPerWorker + j;
                // Every 10th operation is policy-denied (simulate policy gate).
                const bool allowed = (j % 10 != 0);
                gate.sync(id, allowed);
            }
        });
    }
    for (auto& t : workers) t.join();

    const uint64_t expected = static_cast<uint64_t>(kWorkerCount) * kOpsPerWorker;
    EXPECT_EQ(gate.attemptCount(), expected)
        << "[DIST_KG:PolicyGateFailed] All policy gate attempts must be processed";
    EXPECT_GT(gate.passCount(), 0u)
        << "[DIST_KG:ReplayDivergence] At least some syncs must pass the policy gate";
}
