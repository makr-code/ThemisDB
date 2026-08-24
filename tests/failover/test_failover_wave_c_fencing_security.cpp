// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_failover_wave_c_fencing_security.cpp
 * @brief Wave C — Fencing security integration tests + executePlan idempotency.
 *
 * Canonical PRNG seed: kFencingSecuritySeed = 42.
 *
 * ## Test families
 *
 * ### FO-WC-IDEM-01..03 — executePlan idempotency (FO-IMPL-007)
 *   IDEM-01  Same plan_id executed twice → second call returns cached result,
 *            stats_.total_runs incremented only once.
 *   IDEM-02  Failed plan (empty plan_id validation error) is also cached.
 *   IDEM-03  Two different plan_ids execute independently; both cached.
 *
 * ### FO-WC-FENCE-01..04 — Epoch fencing enforcement
 *   FENCE-01  enforce_epoch_fencing=true, no fencing_mgr → plan fails with
 *             "epoch fencing manager required".
 *   FENCE-02  enforce_epoch_fencing=true, mock fencing returning epoch=0 →
 *             plan fails with "fencing returned invalid epoch".
 *   FENCE-03  enforce_epoch_fencing=true, mock fencing returning epoch=42 →
 *             plan succeeds; result.fenced_epoch == 42.
 *   FENCE-04  enforce_epoch_fencing=false → fencing step skipped, plan
 *             proceeds.
 *
 * ### FO-WC-CONCURRENT-01..02 — Concurrent safety
 *   CONCURRENT-01  Two threads, same plan_id (after first completes) → both
 *                  receive the idempotency-cached result.
 *   CONCURRENT-02  Two threads, different plan_ids, simultaneous launch →
 *                  one gets "concurrent execution rejected".
 *
 * @see include/failover/disaster_recovery_manager.h
 * @see FO-IMPL-007
 */

#include <gtest/gtest.h>

#include "failover/disaster_recovery_manager.h"
#include "sharding/epoch_fencing.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <string>
#include <thread>
#include <vector>

using namespace themis::failover;
using namespace themis::sharding;

namespace {

[[maybe_unused]] static constexpr uint64_t kFencingSecuritySeed = 42;

// ─── helpers ────────────────────────────────────────────────────────────────

/// Minimal test config: quorum not required, fencing disabled by default,
/// fast timeouts so the test suite completes quickly.
DisasterRecoveryConfig makeBaseConfig() {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum                = false;
    cfg.enforce_epoch_fencing         = false;
    cfg.allow_dry_run_without_managers = true;
    cfg.precheck_timeout              = std::chrono::milliseconds{50};
    cfg.catchup_timeout               = std::chrono::milliseconds{50};
    cfg.verification_timeout          = std::chrono::milliseconds{50};
    cfg.max_verification_retries      = 1;
    return cfg;
}

/// Minimal valid dry-run plan with a unique plan_id.
DisasterRecoveryPlan makeDryRunPlan(const std::string& plan_id = "plan-wc-42") {
    DisasterRecoveryPlan p;
    p.plan_id       = plan_id;
    p.primary_site  = "site-primary";
    p.recovery_site = "site-recovery";
    p.dry_run       = true;
    p.shift_traffic = false;
    return p;
}

/// Non-dry-run plan (requires replication_mgr and valid snapshot unless hooks
/// override every step).
DisasterRecoveryPlan makeLivePlan(const std::string& plan_id = "plan-wc-live-42") {
    DisasterRecoveryPlan p;
    p.plan_id       = plan_id;
    p.primary_site  = "site-primary";
    p.recovery_site = "site-recovery";
    p.snapshot_id   = "snap-001";
    p.dry_run       = false;
    p.shift_traffic = false;
    return p;
}

// ─── Mock EpochFencingManager ────────────────────────────────────────────────

/**
 * @brief Test double for EpochFencingManager.
 *
 * Overrides bumpEpoch() to return a token whose epoch is controlled by the
 * caller.  epoch_to_return == 0 exercises the "invalid epoch" guard in
 * applyEpochFencing().
 */
class MockEpochFencingManager final : public EpochFencingManager {
public:
    explicit MockEpochFencingManager(EpochNumber epoch_to_return)
        : EpochFencingManager([]() {
              EpochFencingConfig cfg;
              cfg.shard_id  = "test-shard";
              cfg.node_id   = "test-node";
              cfg.auto_stonith = false;
              return cfg;
          }()),
          epoch_to_return_(epoch_to_return) {}

    [[nodiscard]] EpochToken bumpEpoch(const std::string& /*reason*/) override {
        EpochToken tok;
        tok.epoch    = epoch_to_return_;
        tok.issuer   = "test-node";
        tok.shard_id = "test-shard";
        tok.issued_at = std::chrono::system_clock::now();
        return tok;
    }

private:
    EpochNumber epoch_to_return_;
};

}  // namespace

// ============================================================================
// FO-WC-IDEM-01 — same plan_id executed twice, cached result returned
// ============================================================================

TEST(FailoverWaveCIdem01, SamePlanIdCachedResult) {
    DisasterRecoveryManager mgr(makeBaseConfig(), nullptr, nullptr);

    const auto plan = makeDryRunPlan("plan-idem-01");

    const auto result1 = mgr.executePlan(plan);
    EXPECT_TRUE(result1.success);
    EXPECT_EQ(result1.final_state, DisasterRecoveryState::COMPLETED);

    // Verify stats after first execution.
    EXPECT_EQ(mgr.getStatistics().total_runs, 1u);

    // Second call — must return identical result from cache.
    const auto result2 = mgr.executePlan(plan);
    EXPECT_EQ(result2.success,      result1.success);
    EXPECT_EQ(result2.final_state,  result1.final_state);
    EXPECT_EQ(result2.error_message, result1.error_message);

    // total_runs must NOT have been incremented again.
    EXPECT_EQ(mgr.getStatistics().total_runs, 1u)
        << "FO-IMPL-007: second call with same plan_id must not re-run the plan";
}

// ============================================================================
// FO-WC-IDEM-02 — failed plan (empty plan_id) is also cached
// ============================================================================

TEST(FailoverWaveCIdem02, FailedPlanNotCachedForEmptyId) {
    // The validatePlan() rejects empty plan_id and returns before the cache
    // insert (plan_id.empty() guard).  A second call must also fail in the
    // same way but is NOT cached (no plan_id to key on).  Verify the failure
    // result is consistent.
    DisasterRecoveryManager mgr(makeBaseConfig(), nullptr, nullptr);

    DisasterRecoveryPlan bad_plan;
    bad_plan.plan_id       = "";          // ← triggers validation failure
    bad_plan.primary_site  = "site-a";
    bad_plan.recovery_site = "site-b";
    bad_plan.dry_run       = true;

    const auto r1 = mgr.executePlan(bad_plan);
    EXPECT_FALSE(r1.success);
    EXPECT_EQ(r1.final_state, DisasterRecoveryState::FAILED);
    EXPECT_FALSE(r1.error_message.empty());

    // A named plan that does fail (non-dry-run with missing snapshot and no
    // replication manager) IS cached via the plan_id.
    DisasterRecoveryPlan named_fail;
    named_fail.plan_id       = "plan-idem-02-fail";
    named_fail.primary_site  = "site-a";
    named_fail.recovery_site = "site-b";
    named_fail.dry_run       = false;
    named_fail.snapshot_id   = "";        // will fail validatePlan

    const auto rf1 = mgr.executePlan(named_fail);
    EXPECT_FALSE(rf1.success);
    EXPECT_EQ(mgr.getStatistics().total_runs, 2u);  // bad_plan + named_fail

    // Second call for named plan → must return cached failure without re-running.
    const auto rf2 = mgr.executePlan(named_fail);
    EXPECT_EQ(rf2.success,       rf1.success);
    EXPECT_EQ(rf2.final_state,   rf1.final_state);
    EXPECT_EQ(rf2.error_message, rf1.error_message);

    // total_runs unchanged — the named failure was served from cache.
    EXPECT_EQ(mgr.getStatistics().total_runs, 2u)
        << "FO-IMPL-007: named failed plan cached; second call must not re-run";
}

// ============================================================================
// FO-WC-IDEM-03 — two different plan_ids execute independently, both cached
// ============================================================================

TEST(FailoverWaveCIdem03, TwoDifferentPlanIdsIndependent) {
    DisasterRecoveryManager mgr(makeBaseConfig(), nullptr, nullptr);

    const auto plan_a = makeDryRunPlan("plan-idem-03-a");
    const auto plan_b = makeDryRunPlan("plan-idem-03-b");

    const auto ra = mgr.executePlan(plan_a);
    const auto rb = mgr.executePlan(plan_b);

    EXPECT_TRUE(ra.success);
    EXPECT_TRUE(rb.success);

    // Both should have run → total_runs == 2.
    EXPECT_EQ(mgr.getStatistics().total_runs, 2u)
        << "Two distinct plan_ids must each execute once";

    // Third call for plan_a → cached.
    const auto ra2 = mgr.executePlan(plan_a);
    EXPECT_EQ(ra2.success,     ra.success);
    EXPECT_EQ(ra2.final_state, ra.final_state);
    EXPECT_EQ(mgr.getStatistics().total_runs, 2u)
        << "Repeat of plan_a must hit cache";
}

// ============================================================================
// FO-WC-FENCE-01 — enforce_epoch_fencing=true, no fencing_mgr → plan fails
// ============================================================================

TEST(FailoverWaveCFence01, NoFencingManagerFails) {
    DisasterRecoveryConfig cfg = makeBaseConfig();
    cfg.enforce_epoch_fencing = true;

    DisasterRecoveryManager mgr(cfg, nullptr, nullptr /*no fencing_mgr*/);

    // Non-dry-run plan so that applyEpochFencing() is actually called.
    // We hook all steps except EPOCH_FENCING so only that step fails.
    auto plan = makeLivePlan("plan-fence-01");

    // Override steps that would otherwise fail due to missing replication mgr.
    auto pass = [](const DisasterRecoveryPlan&, std::string& d) -> bool {
        d = "hook-pass";
        return true;
    };
    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,          pass);
    mgr.setStepHook(DisasterRecoveryStep::SNAPSHOT_VALIDATION, pass);
    // Do NOT hook EPOCH_FENCING — let it run the real implementation.
    mgr.setStepHook(DisasterRecoveryStep::RESTORE,            pass);
    mgr.setStepHook(DisasterRecoveryStep::REPLICA_CATCHUP,    pass);
    mgr.setStepHook(DisasterRecoveryStep::TRAFFIC_SHIFT,      pass);
    mgr.setStepHook(DisasterRecoveryStep::VERIFICATION,       pass);

    const auto result = mgr.executePlan(plan);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_NE(result.error_message.find("epoch fencing manager required"),
              std::string::npos)
        << "Expected 'epoch fencing manager required' in: " << result.error_message;
}

// ============================================================================
// FO-WC-FENCE-02 — mock fencing returns epoch=0 → plan fails
// ============================================================================

TEST(FailoverWaveCFence02, InvalidEpochZeroFails) {
    DisasterRecoveryConfig cfg = makeBaseConfig();
    cfg.enforce_epoch_fencing = true;

    auto mock_fencing = std::make_shared<MockEpochFencingManager>(0 /*epoch=0*/);
    DisasterRecoveryManager mgr(cfg, nullptr, mock_fencing);

    auto plan = makeLivePlan("plan-fence-02");

    auto pass = [](const DisasterRecoveryPlan&, std::string& d) -> bool {
        d = "hook-pass"; return true;
    };
    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,          pass);
    mgr.setStepHook(DisasterRecoveryStep::SNAPSHOT_VALIDATION, pass);
    mgr.setStepHook(DisasterRecoveryStep::RESTORE,            pass);
    mgr.setStepHook(DisasterRecoveryStep::REPLICA_CATCHUP,    pass);
    mgr.setStepHook(DisasterRecoveryStep::TRAFFIC_SHIFT,      pass);
    mgr.setStepHook(DisasterRecoveryStep::VERIFICATION,       pass);

    const auto result = mgr.executePlan(plan);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_NE(result.error_message.find("fencing returned invalid epoch"),
              std::string::npos)
        << "Expected 'fencing returned invalid epoch' in: " << result.error_message;
}

// ============================================================================
// FO-WC-FENCE-03 — valid epoch (42) returned → result.fenced_epoch == 42
// ============================================================================

TEST(FailoverWaveCFence03, ValidEpochPopulatesResult) {
    DisasterRecoveryConfig cfg = makeBaseConfig();
    cfg.enforce_epoch_fencing = true;

    auto mock_fencing = std::make_shared<MockEpochFencingManager>(42 /*epoch=42*/);
    DisasterRecoveryManager mgr(cfg, nullptr, mock_fencing);

    auto plan = makeLivePlan("plan-fence-03");

    auto pass = [](const DisasterRecoveryPlan&, std::string& d) -> bool {
        d = "hook-pass"; return true;
    };
    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,          pass);
    mgr.setStepHook(DisasterRecoveryStep::SNAPSHOT_VALIDATION, pass);
    mgr.setStepHook(DisasterRecoveryStep::RESTORE,            pass);
    mgr.setStepHook(DisasterRecoveryStep::REPLICA_CATCHUP,    pass);
    mgr.setStepHook(DisasterRecoveryStep::TRAFFIC_SHIFT,      pass);
    mgr.setStepHook(DisasterRecoveryStep::VERIFICATION,       pass);

    const auto result = mgr.executePlan(plan);

    EXPECT_TRUE(result.success)
        << "Plan should succeed with valid epoch; error: " << result.error_message;
    EXPECT_EQ(result.fenced_epoch, 42u)
        << "fenced_epoch must equal the value returned by bumpEpoch()";
}

// ============================================================================
// FO-WC-FENCE-04 — enforce_epoch_fencing=false → fencing step skipped
// ============================================================================

TEST(FailoverWaveCFence04, FencingDisabledSkipsStep) {
    DisasterRecoveryConfig cfg = makeBaseConfig();
    cfg.enforce_epoch_fencing = false;   // explicitly disabled

    // No fencing manager — if the fencing step were called it would fail.
    DisasterRecoveryManager mgr(cfg, nullptr, nullptr);

    auto plan = makeLivePlan("plan-fence-04");

    auto pass = [](const DisasterRecoveryPlan&, std::string& d) -> bool {
        d = "hook-pass"; return true;
    };
    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,          pass);
    mgr.setStepHook(DisasterRecoveryStep::SNAPSHOT_VALIDATION, pass);
    // No hook for EPOCH_FENCING — real impl runs but should be skipped.
    mgr.setStepHook(DisasterRecoveryStep::RESTORE,            pass);
    mgr.setStepHook(DisasterRecoveryStep::REPLICA_CATCHUP,    pass);
    mgr.setStepHook(DisasterRecoveryStep::TRAFFIC_SHIFT,      pass);
    mgr.setStepHook(DisasterRecoveryStep::VERIFICATION,       pass);

    const auto result = mgr.executePlan(plan);

    EXPECT_TRUE(result.success)
        << "Plan should succeed when fencing is disabled; error: "
        << result.error_message;
    // Epoch not fenced → fenced_epoch stays 0.
    EXPECT_EQ(result.fenced_epoch, 0u);
}

// ============================================================================
// FO-WC-CONCURRENT-01 — two threads, same plan_id (serial) → both cached
// ============================================================================

TEST(FailoverWaveCConcurrent01, SamePlanIdBothGetCachedResult) {
    DisasterRecoveryManager mgr(makeBaseConfig(), nullptr, nullptr);

    const auto plan = makeDryRunPlan("plan-concurrent-01");

    // First execution (single-threaded) to prime the cache.
    const auto r1 = mgr.executePlan(plan);
    EXPECT_TRUE(r1.success);
    EXPECT_EQ(mgr.getStatistics().total_runs, 1u);

    // Now two threads race to call executePlan with the same plan_id — both
    // should hit the cache immediately without executing the plan again.
    std::atomic<int> cache_hits{0};

    auto call = [&]() {
        auto r = mgr.executePlan(plan);
        if (r.success && r.final_state == DisasterRecoveryState::COMPLETED) {
            cache_hits.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::thread t1(call);
    std::thread t2(call);
    t1.join();
    t2.join();

    EXPECT_EQ(cache_hits.load(), 2)
        << "Both threads must receive the cached successful result";
    EXPECT_EQ(mgr.getStatistics().total_runs, 1u)
        << "total_runs must remain 1; cache must prevent re-execution";
}

// ============================================================================
// FO-WC-CONCURRENT-02 — two threads, different plan_ids, simultaneous launch
//                        → one gets "concurrent execution rejected"
// ============================================================================

TEST(FailoverWaveCConcurrent02, DifferentPlanIdsOneRejected) {
    // Use a slightly longer catchup timeout so that the first thread is still
    // holding execution_mutex_ when the second thread attempts to acquire it.
    DisasterRecoveryConfig cfg = makeBaseConfig();
    // Hook all steps for plan-a with a tiny sleep so plan-b's thread is
    // guaranteed to arrive while plan-a holds the execution lock.
    DisasterRecoveryManager mgr(cfg, nullptr, nullptr);

    const auto plan_a = makeDryRunPlan("plan-concurrent-02-a");
    const auto plan_b = makeDryRunPlan("plan-concurrent-02-b");

    // Install a hook on plan-a that sleeps long enough for plan-b to attempt
    // concurrently.  We use a barrier to synchronise both threads.
    std::atomic<bool> plan_a_started{false};
    std::atomic<bool> plan_b_launched{false};

    // Barrier: plan-a's PRECHECKS hook signals it has started, then waits for
    // plan-b to have been launched.
    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,
        [&](const DisasterRecoveryPlan&, std::string& d) -> bool {
            plan_a_started.store(true, std::memory_order_release);
            // Spin until plan-b thread has been launched.
            while (!plan_b_launched.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
            // Small extra sleep to ensure plan-b actually calls executePlan.
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            d = "plan-a prechecks";
            return true;
        });

    DisasterRecoveryResult result_a, result_b;

    std::thread ta([&]() {
        result_a = mgr.executePlan(plan_a);
    });

    // Wait until plan-a has started (and is holding the execution lock).
    while (!plan_a_started.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    std::thread tb([&]() {
        plan_b_launched.store(true, std::memory_order_release);
        result_b = mgr.executePlan(plan_b);
    });

    ta.join();
    tb.join();

    // Exactly one of the two must have been rejected.
    const bool a_rejected = !result_a.success &&
        result_a.error_message.find("concurrent execution rejected") != std::string::npos;
    const bool b_rejected = !result_b.success &&
        result_b.error_message.find("concurrent execution rejected") != std::string::npos;

    EXPECT_TRUE(a_rejected || b_rejected)
        << "One of the concurrent plan executions must be rejected; "
        << "result_a=" << result_a.error_message
        << " result_b=" << result_b.error_message;

    // The other must have succeeded.
    EXPECT_TRUE((a_rejected && result_b.success) || (b_rejected && result_a.success))
        << "The non-rejected plan must complete successfully";
}
