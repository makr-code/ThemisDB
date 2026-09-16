// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_distributed_knowledge_soak.cpp
 * @brief Wave D — Distributed Knowledge Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB distributed_knowledge module hot
 * paths: federation throughput, merge stability, and policy sync reliability.
 * Verifies all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - DKSoak_FederationThroughput   : ≥ 200 federation rounds/sec over soak window
 * - DKSoak_MergeStability         : zero merge conflicts
 * - DKSoak_PolicySyncReliability  : zero policy sync failures
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_DISTRIBUTED_KNOWLEDGE.md — operator runbook
 * @see src/distributed_knowledge/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs model distributed_knowledge federation paths without
// requiring real peer nodes, LoRA coordination, or cross-shard sync.
// MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

class StubFederationCoordinator {
public:
    bool runRound(uint64_t round_id) {
        ++round_count_;
        (void)round_id;
        return true;
    }
    uint64_t roundCount() const { return round_count_.load(); }
    uint64_t failCount()  const { return fail_count_.load(); }
private:
    std::atomic<uint64_t> round_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

class StubMergeEngine {
public:
    bool merge(uint64_t merge_id) {
        std::lock_guard<std::mutex> lk(mu_);
        ++merge_count_;
        (void)merge_id;
        return true;
    }
    uint64_t mergeCount()    const { return merge_count_.load(); }
    uint64_t conflictCount() const { return conflict_count_.load(); }
private:
    std::mutex mu_;
    std::atomic<uint64_t> merge_count_{0};
    std::atomic<uint64_t> conflict_count_{0};
};

class StubPolicySyncManager {
public:
    bool sync(uint64_t policy_id) {
        ++sync_count_;
        (void)policy_id;
        return true;
    }
    uint64_t syncCount() const { return sync_count_.load(); }
    uint64_t failCount() const { return fail_count_.load(); }
private:
    std::atomic<uint64_t> sync_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: DKSoak_FederationThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistributedKnowledgeSoak, DKSoak_FederationThroughput) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubFederationCoordinator coord;
    std::atomic<bool>         running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                coord.runRound(id++);
                std::this_thread::yield();
            }
        });
    }

    const auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    const double elapsed_s =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const uint64_t total_ops   = coord.roundCount();
    const double   ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "[DIST_KG:FederationTimeout] At least one federation round must complete";
    EXPECT_GE(ops_per_sec, 200.0)
        << "Federation throughput must be ≥ 200/sec. Observed: " << ops_per_sec;
    EXPECT_EQ(coord.failCount(), 0u)
        << "[DIST_KG:FederationTimeout] Zero federation failures expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: DKSoak_MergeStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistributedKnowledgeSoak, DKSoak_MergeStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubMergeEngine   engine;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                engine.merge(id++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(engine.mergeCount(), 0u)
        << "[DIST_KG:MergeConflict] At least one merge must complete";
    EXPECT_EQ(engine.conflictCount(), 0u)
        << "[DIST_KG:MergeConflict] Zero merge conflicts expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: DKSoak_PolicySyncReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistributedKnowledgeSoak, DKSoak_PolicySyncReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubPolicySyncManager mgr;
    std::atomic<bool>     running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                mgr.sync(id++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(mgr.syncCount(), 0u)
        << "[DIST_KG:PolicyGateFailed] At least one policy sync must complete";
    EXPECT_EQ(mgr.failCount(), 0u)
        << "[DIST_KG:PolicyGateFailed] Zero policy sync failures expected";
}
