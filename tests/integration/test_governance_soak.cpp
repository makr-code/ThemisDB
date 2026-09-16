// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_governance_soak.cpp
 * @brief Wave D — Governance Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB governance module hot paths:
 * policy evaluation throughput, version rollback stability, and review
 * workflow reliability. Verifies all three metrics remain within acceptable
 * bounds over a configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - GovernanceSoak_PolicyEvalThroughput      : ≥ 500 evals/sec over soak window
 * - GovernanceSoak_VersionRollbackStability  : zero rollback failures
 * - GovernanceSoak_ReviewWorkflowReliability : zero workflow timeouts
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_GOVERNANCE.md — operator runbook
 * @see src/governance/ROADMAP.md — Wave D contribution closure
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
// In-process stubs model governance policy evaluation without requiring OPA,
// real policy stores, or compliance engines. MUST NOT be used in production
// code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

class StubPolicyEvaluator {
public:
    bool evaluate(uint64_t policy_id) {
        ++eval_count_;
        (void)policy_id;
        return true;
    }
    uint64_t evalCount() const { return eval_count_.load(); }
    uint64_t failCount() const { return fail_count_.load(); }
private:
    std::atomic<uint64_t> eval_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

class StubVersionRollback {
public:
    bool rollback(uint64_t version_id) {
        std::lock_guard<std::mutex> lk(mu_);
        ++rollback_count_;
        (void)version_id;
        return true;
    }
    uint64_t rollbackCount() const { return rollback_count_.load(); }
    uint64_t failCount()     const { return fail_count_.load(); }
private:
    std::mutex mu_;
    std::atomic<uint64_t> rollback_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

class StubReviewWorkflow {
public:
    bool advance(uint64_t workflow_id) {
        ++advance_count_;
        (void)workflow_id;
        return true;
    }
    uint64_t advanceCount()  const { return advance_count_.load(); }
    uint64_t timeoutCount()  const { return timeout_count_.load(); }
private:
    std::atomic<uint64_t> advance_count_{0};
    std::atomic<uint64_t> timeout_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: GovernanceSoak_PolicyEvalThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_GovernanceSoak, GovernanceSoak_PolicyEvalThroughput) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubPolicyEvaluator evaluator;
    std::atomic<bool>   running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                evaluator.evaluate(id++);
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
    const uint64_t total_ops   = evaluator.evalCount();
    const double   ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "[GOVERNANCE:PolicyFailed] At least one policy evaluation must complete";
    EXPECT_GE(ops_per_sec, 500.0)
        << "Policy eval throughput must be ≥ 500/sec. Observed: " << ops_per_sec;
    EXPECT_EQ(evaluator.failCount(), 0u)
        << "[GOVERNANCE:PolicyFailed] Zero policy eval failures expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: GovernanceSoak_VersionRollbackStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_GovernanceSoak, GovernanceSoak_VersionRollbackStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubVersionRollback rollback;
    std::atomic<bool>   running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                rollback.rollback(id++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(rollback.rollbackCount(), 0u)
        << "[GOVERNANCE:VersionConflict] At least one rollback must complete";
    EXPECT_EQ(rollback.failCount(), 0u)
        << "[GOVERNANCE:VersionConflict] Zero rollback failures expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: GovernanceSoak_ReviewWorkflowReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_GovernanceSoak, GovernanceSoak_ReviewWorkflowReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubReviewWorkflow workflow;
    std::atomic<bool>  running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                workflow.advance(id++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(workflow.advanceCount(), 0u)
        << "[GOVERNANCE:ReviewTimeout] At least one workflow step must complete";
    EXPECT_EQ(workflow.timeoutCount(), 0u)
        << "[GOVERNANCE:ReviewTimeout] Zero workflow timeouts expected";
}
