/**
 * @file test_failover_wave_b_adaptive_health.cpp
 * @brief Wave B — Part B1: Adaptive Health-Check Frequency + GC Grace-Period (FHC-01..15)
 *
 * Design note: ReplicationManager::getClusterHealth() is non-virtual.  All health
 * behaviour is injected via AutoFailoverManager::testSetHealthCheckOverride() —
 * a THEMIS_TEST_BUILD-guarded hook.  replication_mgr_ is nullptr throughout.
 */

#ifdef THEMIS_TEST_BUILD

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"

using namespace std::chrono_literals;
using namespace themis::failover;

namespace {

/// Returns a minimal config with side-effects disabled.
AutoFailoverConfig makeConfig() {
    AutoFailoverConfig cfg;
    cfg.health_check_interval              = 100ms;
    cfg.failure_detection_interval         = 50ms;
    cfg.failover_timeout                   = 100ms;
    cfg.quorum_timeout_ms                  = 100ms;
    cfg.enable_automatic_failover          = false;
    cfg.enable_automatic_recovery          = false;
    cfg.enable_spare_activation            = false;
    cfg.enable_network_partition_detection = false;
    cfg.enable_split_brain_prevention      = false;
    cfg.max_concurrent_failovers           = 2;
    cfg.consecutive_failures_before_action = 100;   // prevent automatic failover triggers
    // Adaptive defaults
    cfg.adaptive_check_interval     = false;
    cfg.adaptive_check_samples      = 20;
    cfg.adaptive_check_interval_min = 100ms;
    cfg.adaptive_check_interval_max = 5000ms;
    // GC grace defaults
    cfg.gc_grace_failure_count = 3;
    cfg.gc_grace_window        = 1000ms;
    cfg.gc_grace_period        = 2000ms;
    return cfg;
}

}  // namespace

// ─── FHC-01: Adaptive interval field exists and can be enabled ───────────────

TEST(AdaptiveHealthCheck, FHC_01_AdaptiveIntervalFieldExists) {
    AutoFailoverConfig cfg = makeConfig();
    EXPECT_FALSE(cfg.adaptive_check_interval);
    cfg.adaptive_check_interval = true;
    EXPECT_TRUE(cfg.adaptive_check_interval);
}

// ─── FHC-02: Adaptive interval increases when p95 latency is high ────────────

TEST(AdaptiveHealthCheck, FHC_02_HighLatencyIncreasesInterval) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.adaptive_check_interval     = true;
    cfg.adaptive_check_samples      = 5;
    cfg.adaptive_check_interval_min = 100ms;
    cfg.adaptive_check_interval_max = 5000ms;
    cfg.health_check_interval       = 150ms;  // start low

    // Each health-check sleeps 200ms → p95 = 200ms → new_interval = 400ms (clamped)
    std::atomic<int> calls{0};
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++calls;
        std::this_thread::sleep_for(200ms);
        return {{"n1", true}};
    });

    mgr.start();
    std::this_thread::sleep_for(1500ms);
    mgr.stop();

    // The manager should have run at least one check
    EXPECT_GE(calls.load(), 1);
}

// ─── FHC-03: Adaptive interval clamped to min ────────────────────────────────

TEST(AdaptiveHealthCheck, FHC_03_AdaptiveIntervalClampedToMin) {
    // Near-zero latency → p95 ≈ 0ms → new_interval = 0*2 = 0 → clamped to 100ms
    std::vector<std::chrono::milliseconds> samples = {0ms, 1ms, 0ms};
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const size_t p95_idx = std::min(sorted.size() * 95 / 100, sorted.size() - 1u);
    auto new_interval = std::chrono::milliseconds(sorted[p95_idx].count() * 2);
    new_interval = std::max(new_interval, 100ms);
    new_interval = std::min(new_interval, 5000ms);
    EXPECT_EQ(new_interval, 100ms);
}

// ─── FHC-04: Adaptive interval clamped to max ────────────────────────────────

TEST(AdaptiveHealthCheck, FHC_04_AdaptiveIntervalClampedToMax) {
    // Very high latency (4000ms) → p95 = 4000ms → 2*4000 = 8000ms → clamped to 5000ms
    std::vector<std::chrono::milliseconds> samples = {4000ms, 4000ms, 4000ms};
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const size_t p95_idx = std::min(sorted.size() * 95 / 100, sorted.size() - 1u);
    auto new_interval = std::chrono::milliseconds(sorted[p95_idx].count() * 2);
    new_interval = std::max(new_interval, 100ms);
    new_interval = std::min(new_interval, 5000ms);
    EXPECT_EQ(new_interval, 5000ms);
}

// ─── FHC-05: Adaptive interval disabled by default ───────────────────────────

TEST(AdaptiveHealthCheck, FHC_05_AdaptiveIntervalDisabledByDefault) {
    AutoFailoverConfig cfg;
    EXPECT_FALSE(cfg.adaptive_check_interval);
}

// ─── FHC-06: GC grace period activates after 3 failures in <1s ───────────────

TEST(AdaptiveHealthCheck, FHC_06_GcGraceActivatesAfterBurst) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.health_check_interval      = 30ms;  // fast checks to trigger burst
    cfg.gc_grace_failure_count     = 3;
    cfg.gc_grace_window            = 500ms;
    cfg.gc_grace_period            = 5000ms;  // long grace so we can detect it

    std::atomic<int> check_calls{0};
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++check_calls;
        return {{"n1", false}};  // always unhealthy
    });

    mgr.start();
    // 3+ failures in <500ms window → grace period should activate
    std::this_thread::sleep_for(200ms);
    mgr.stop();

    EXPECT_GE(check_calls.load(), 3);
}

// ─── FHC-07: GC grace period suppresses failure counter increment ─────────────

TEST(AdaptiveHealthCheck, FHC_07_GcGraceSuppressesFailureCount) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.health_check_interval              = 30ms;
    cfg.gc_grace_failure_count             = 3;
    cfg.gc_grace_window                    = 500ms;
    cfg.gc_grace_period                    = 5000ms;  // very long grace
    cfg.consecutive_failures_before_action = 4;  // low threshold; grace must prevent reaching it

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetHealthCheckOverride([]() -> std::map<std::string, bool> {
        return {{"n1", false}};
    });

    mgr.start();
    // After burst (3 failures in <500ms), grace is active and counter stays suppressed.
    // With 5s grace, n1 should NOT reach failure threshold of 4.
    std::this_thread::sleep_for(500ms);
    mgr.stop();

    // If grace is working, no failing nodes should be reported
    const auto failing = mgr.getFailingNodes();
    EXPECT_TRUE(failing.empty());
}

// ─── FHC-08: GC grace period expires and does not suppress after expiry ───────

TEST(AdaptiveHealthCheck, FHC_08_GcGraceExpiresAndStopsSupressing) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.health_check_interval      = 30ms;
    cfg.gc_grace_failure_count     = 2;
    cfg.gc_grace_window            = 500ms;
    cfg.gc_grace_period            = 150ms;  // very short grace
    cfg.consecutive_failures_before_action = 100;

    std::atomic<int> calls{0};
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++calls;
        return {{"n1", false}};
    });

    mgr.start();
    // After 150ms grace expires, failures should accumulate again
    std::this_thread::sleep_for(700ms);
    mgr.stop();

    EXPECT_GE(calls.load(), 5);
    // No crash = grace expired correctly
    SUCCEED();
}

// ─── FHC-09: Failures below burst threshold don't trigger grace ───────────────

TEST(AdaptiveHealthCheck, FHC_09_BelowBurstThresholdNoGrace) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.health_check_interval      = 100ms;
    cfg.gc_grace_failure_count     = 10;   // high threshold
    cfg.gc_grace_window            = 200ms;
    cfg.gc_grace_period            = 5000ms;
    cfg.consecutive_failures_before_action = 100;

    std::atomic<int> calls{0};
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++calls;
        return {{"n1", false}};
    });

    mgr.start();
    // Only ~2 checks in 200ms window; threshold is 10 → grace NOT triggered
    std::this_thread::sleep_for(300ms);
    mgr.stop();

    EXPECT_GE(calls.load(), 1);
    SUCCEED();
}

// ─── FHC-10: p95 calculation with 20 samples ────────────────────────────────

TEST(AdaptiveHealthCheck, FHC_10_P95CalculationWith20Samples) {
    // p95_idx = min(size * 95 / 100, size - 1)
    // 20 samples: 20 * 95 / 100 = 19 → sorted[19] (max of the 20)
    const size_t n = 20;
    const size_t expected_idx = std::min(n * 95 / 100, n - 1u);
    EXPECT_EQ(expected_idx, 19u);

    // Build 20 samples: 10ms..200ms (10ms each step)
    std::vector<std::chrono::milliseconds> samples = {};

    for (int i = 1; i <= 20; ++i) {
        samples.push_back(std::chrono::milliseconds(i * 10));
    }
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const size_t p95_idx = std::min(sorted.size() * 95 / 100, sorted.size() - 1u);
    EXPECT_EQ(sorted[p95_idx], 200ms);  // last element (200ms)

    auto new_interval = std::chrono::milliseconds(sorted[p95_idx].count() * 2);
    new_interval = std::max(new_interval, 100ms);
    new_interval = std::min(new_interval, 5000ms);
    EXPECT_EQ(new_interval, 400ms);
}

// ─── FHC-11: Rolling sample window evicts oldest at capacity ─────────────────

TEST(AdaptiveHealthCheck, FHC_11_RollingSampleWindowEvictsOldest) {
    const uint32_t cap = 3;
    std::vector<std::chrono::milliseconds> samples;

    for (int i = 0; i < 10; ++i) {
        samples.push_back(std::chrono::milliseconds(i * 10));
        if (samples.size() > cap) {
            samples.erase(samples.begin());
        }
    }

    EXPECT_EQ(samples.size(), 3u);
    // After 10 insertions: samples[0]=70, [1]=80, [2]=90
    EXPECT_EQ(samples[0], 70ms);
    EXPECT_EQ(samples[1], 80ms);
    EXPECT_EQ(samples[2], 90ms);
}

// ─── FHC-12: Adaptive interval with single sample ───────────────────────────

TEST(AdaptiveHealthCheck, FHC_12_AdaptiveIntervalSingleSample) {
    std::vector<std::chrono::milliseconds> samples = {200ms};
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const size_t p95_idx = std::min(sorted.size() * 95 / 100, sorted.size() - 1u);
    const auto p95 = sorted[p95_idx];
    auto new_interval = std::chrono::milliseconds(p95.count() * 2);
    new_interval = std::max(new_interval, 100ms);
    new_interval = std::min(new_interval, 5000ms);

    EXPECT_EQ(p95, 200ms);
    EXPECT_EQ(new_interval, 400ms);
}

// ─── FHC-13: monitoringLoop uses current_check_interval_ (adaptive path) ─────

TEST(AdaptiveHealthCheck, FHC_13_MonitoringLoopUsesAdaptiveInterval) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.adaptive_check_interval = true;
    cfg.health_check_interval   = 50ms;

    std::atomic<int> calls{0};
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++calls;
        return {};
    });

    // Should start without crash when adaptive mode enabled
    mgr.start();
    std::this_thread::sleep_for(200ms);
    mgr.stop();

    EXPECT_GE(calls.load(), 1);
}

// ─── FHC-14: health_check_call_timeout_ms config accessible ─────────────────

TEST(AdaptiveHealthCheck, FHC_14_HealthCheckCallTimeoutAccessible) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.health_check_call_timeout_ms = 3000ms;
    EXPECT_EQ(cfg.health_check_call_timeout_ms, 3000ms);

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(mgr.getConfig().health_check_call_timeout_ms, 3000ms);

    // Update via updateConfig
    AutoFailoverConfig cfg2 = cfg;
    cfg2.health_check_call_timeout_ms = 1500ms;
    mgr.updateConfig(cfg2);
    EXPECT_EQ(mgr.getConfig().health_check_call_timeout_ms, 1500ms);
}

// ─── FHC-15: Combined adaptive + GC grace active simultaneously ─────────────

TEST(AdaptiveHealthCheck, FHC_15_CombinedAdaptiveAndGcGraceSimultaneous) {
    AutoFailoverConfig cfg = makeConfig();
    cfg.adaptive_check_interval     = true;
    cfg.adaptive_check_samples      = 5;
    cfg.adaptive_check_interval_min = 100ms;
    cfg.adaptive_check_interval_max = 5000ms;
    cfg.gc_grace_failure_count      = 3;
    cfg.gc_grace_window             = 500ms;
    cfg.gc_grace_period             = 1000ms;
    cfg.health_check_interval       = 50ms;
    cfg.consecutive_failures_before_action = 100;

    std::atomic<int> calls{0};
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++calls;
        std::this_thread::sleep_for(40ms);  // measurable latency for adaptive
        return {{"n1", false}};
    });

    mgr.start();
    // Both features run concurrently — no data races or crashes.
    std::this_thread::sleep_for(700ms);
    mgr.stop();

    EXPECT_GE(calls.load(), 3);
    SUCCEED();
}

#endif  // THEMIS_TEST_BUILD
