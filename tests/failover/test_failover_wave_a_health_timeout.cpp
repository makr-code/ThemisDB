/**
 * @file test_failover_wave_a_health_timeout.cpp
 * @brief Wave A gate FO-Detect-01: health-check call-timeout enforcement.
 *
 * Covers:
 * - FO-Detect-01-TIMEOUT : getClusterHealth() that blocks > timeout_ms causes
 *                          a HEARTBEAT_MISSED diagnostic and no node-tracking update.
 * - FO-Detect-01-FAST    : getClusterHealth() that returns promptly triggers
 *                          normal node-tracking via updateFailureTracking().
 * - FO-Detect-01-CONFIG  : health_check_call_timeout_ms is configurable at runtime
 *                          via updateConfig().
 *
 * Design note: ReplicationManager::getClusterHealth() is non-virtual, so tests
 * inject behaviour via AutoFailoverManager::testSetHealthCheckOverride() — a
 * THEMIS_TEST_BUILD-guarded hook that replaces the internal callable used by
 * performHealthChecks().  No external I/O.
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

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Returns a minimal config with disabled side-effects for unit tests.
AutoFailoverConfig makeBaseConfig() {
    AutoFailoverConfig cfg;
    cfg.health_check_interval              = 20ms;
    cfg.failure_detection_interval         = 20ms;
    cfg.failover_timeout                   = 50ms;
    cfg.spare_activation_timeout           = 50ms;
    cfg.leader_election_timeout            = 50ms;
    cfg.recovery_retry_interval            = 20ms;
    cfg.max_recovery_attempts              = 1;
    cfg.enable_automatic_failover          = false;
    cfg.enable_automatic_recovery          = false;
    cfg.enable_spare_activation            = false;
    cfg.enable_network_partition_detection = false;
    cfg.enable_split_brain_prevention      = false;
    cfg.max_concurrent_failovers           = 0;
    cfg.health_check_call_timeout_ms       = 5000ms;  // tests override this as needed
    return cfg;
}

}  // namespace

namespace themis { namespace failover { namespace test {

// ── FO-Detect-01-TIMEOUT ─────────────────────────────────────────────────────

TEST(HealthCheckTimeout, TimeoutEmitsDiagnosticAndSkipsTracking) {
    // Arrange: override that blocks for 2s; timeout set to 300ms.
    AutoFailoverConfig cfg           = makeBaseConfig();
    cfg.health_check_call_timeout_ms = 300ms;

    // null replication_mgr is fine — the override replaces the call entirely.
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    std::atomic<int> call_count{0};
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++call_count;
        std::this_thread::sleep_for(2000ms);  // far exceeds 300ms timeout
        return {{"node-A", true}};
    });

    // Capture FAILOVER_CANCELLED events — emitDiagnostic(HEARTBEAT_MISSED) maps
    // to FailoverEventType::FAILOVER_CANCELLED via the default branch.
    std::atomic<int> heartbeat_missed_count{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string& /*node_id*/,
            const std::string& /*detail*/) {
            if (type == FailoverEventType::FAILOVER_CANCELLED) {
                ++heartbeat_missed_count;
            }
        });

    ASSERT_TRUE(mgr.start());
    // Wait long enough for at least one health-check cycle to trigger and time out.
    // 300ms timeout + 20ms scheduling = ~320ms; wait 800ms to be safe.
    std::this_thread::sleep_for(800ms);
    ASSERT_TRUE(mgr.stop());

    EXPECT_GT(heartbeat_missed_count.load(), 0)
        << "Expected at least one HEARTBEAT_MISSED diagnostic when getClusterHealth() times out";

    // The override returned {node-A: true}, which would reset consecutive_failures_
    // if updateFailureTracking were called.  Since we timed out, node-A must NOT
    // be in getFailingNodes() via the healthy path — but the important assertion is
    // the diagnostic was fired above.
    const auto failing = mgr.getFailingNodes();
    // node-A should NOT have been promoted to the failing list via the is_healthy=true
    // path (that would have done nothing); we verify no false "healthy clear" happened.
    // (There is no way to mark healthy=false for node-A since the timeout fired,
    //  so the failing list should remain empty for node-A.)
    for (const auto& n : failing) {
        EXPECT_NE(n, "node-A")
            << "node-A should not appear in failing list from a timed-out health check";
    }
}

// ── FO-Detect-01-FAST ────────────────────────────────────────────────────────

TEST(HealthCheckTimeout, FastReturnTracksNodesNormally) {
    // Arrange: override that returns immediately with one unhealthy node.
    AutoFailoverConfig cfg           = makeBaseConfig();
    cfg.health_check_call_timeout_ms = 5000ms;  // wide — won't fire
    cfg.consecutive_failures_before_action = 3;

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    std::atomic<int> call_count{0};
    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        ++call_count;
        return {{"node-X", false}};  // always unhealthy
    });

    ASSERT_TRUE(mgr.start());
    // Let the monitoring loop run enough cycles to exceed consecutive_failures_before_action.
    // health_check_interval = 20ms → 3 failures in ~60ms; wait 300ms for margin.
    std::this_thread::sleep_for(300ms);
    ASSERT_TRUE(mgr.stop());

    EXPECT_GT(call_count.load(), 0)
        << "Expected the health-check override to have been called at least once";

    const auto failing = mgr.getFailingNodes();
    EXPECT_NE(std::find(failing.begin(), failing.end(), "node-X"), failing.end())
        << "Expected node-X to be tracked as failing after repeated unhealthy fast responses";
}

// ── FO-Detect-01-CONFIG ──────────────────────────────────────────────────────

TEST(HealthCheckTimeout, ConfigurableTimeoutIsRespected) {
    // Arrange: override that takes 400ms; start with wide timeout (5s), then
    // hot-update to 150ms so subsequent cycles time out.
    AutoFailoverConfig cfg           = makeBaseConfig();
    cfg.health_check_call_timeout_ms = 5000ms;  // wide at first

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    mgr.testSetHealthCheckOverride([&]() -> std::map<std::string, bool> {
        std::this_thread::sleep_for(400ms);
        return {{"node-Y", true}};
    });

    std::atomic<int> timeout_events{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string& /*node*/, const std::string& /*detail*/) {
            if (type == FailoverEventType::FAILOVER_CANCELLED) {
                ++timeout_events;
            }
        });

    ASSERT_TRUE(mgr.start());

    // Let at least one wide-timeout cycle begin (but it will take 400ms to complete).
    std::this_thread::sleep_for(50ms);

    // Hot-update: tighten the timeout to 150ms — now the 400ms override will time out.
    AutoFailoverConfig tight                = mgr.getConfig();
    tight.health_check_call_timeout_ms      = 150ms;
    mgr.updateConfig(tight);

    // Wait for at least one timed-out cycle.
    std::this_thread::sleep_for(800ms);
    ASSERT_TRUE(mgr.stop());

    EXPECT_GT(timeout_events.load(), 0)
        << "Expected HEARTBEAT_MISSED events after tightening health_check_call_timeout_ms";

    EXPECT_EQ(mgr.getConfig().health_check_call_timeout_ms, 150ms)
        << "getConfig() should reflect the updated timeout value";
}

}}}  // namespace themis::failover::test

#endif  // THEMIS_TEST_BUILD
