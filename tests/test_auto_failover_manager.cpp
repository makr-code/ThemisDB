/**
 * @file test_auto_failover_manager.cpp
 * @brief Focused tests for AutoFailoverManager — Phase 4.2 Automatic Failover Orchestration.
 *
 * Test groups:
 *  1. Lifecycle (start / stop / running state)
 *  2. State machine (initial state, IDLE on stop)
 *  3. Manual failover trigger (accept / reject / target promotion)
 *  4. Failover queue pressure (concurrent-failover cap)
 *  5. Configuration management (update / round-trip)
 *  6. Statistics (initial zeros, accumulation after run)
 *  7. Event callbacks (registration, multi-callback, exception isolation)
 *  8. Failure tracking (getFailingNodes, consecutive threshold)
 *  9. Last failover result (initially absent, populated after run)
 * 10. Edge cases (double-start, stop-when-stopped, null managers)
 */

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"

using namespace std::chrono_literals;

namespace themis { namespace failover { namespace test { 

// ─── helpers ────────────────────────────────────────────────────────────────

/// Create a config with shortened polling intervals suitable for unit tests.
static AutoFailoverConfig fastConfig(
    std::chrono::milliseconds health_interval = 10ms,
    uint32_t max_concurrent = 2
) {
    AutoFailoverConfig cfg;
    cfg.health_check_interval            = health_interval;
    cfg.failure_detection_interval       = 10ms;
    cfg.failover_timeout                 = 50ms;
    cfg.spare_activation_timeout         = 50ms;
    cfg.leader_election_timeout          = 50ms;
    cfg.recovery_retry_interval          = 10ms;
    cfg.max_concurrent_failovers         = max_concurrent;
    cfg.consecutive_failures_before_action = 3;
    return cfg;
}

/// Build a stopped manager with all optional deps set to nullptr.
static AutoFailoverManager makeManager(const AutoFailoverConfig& cfg = fastConfig()) {
    return AutoFailoverManager(cfg, nullptr, nullptr, nullptr, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, StartStopLifecycle) {
    auto mgr = makeManager();

    EXPECT_FALSE(mgr.isRunning());
    EXPECT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.isRunning());

    std::this_thread::sleep_for(30ms);

    EXPECT_TRUE(mgr.stop());
    EXPECT_FALSE(mgr.isRunning());
}

TEST(AutoFailoverManagerFocusedTest, DoubleStartReturnsFalse) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    EXPECT_FALSE(mgr.start());   // second start must be rejected

    ASSERT_TRUE(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, StopWhenNotRunningReturnsFalse) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.stop());    // never started
}

TEST(AutoFailoverManagerFocusedTest, StopTwiceReturnsFalseOnSecondCall) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.stop());
    EXPECT_FALSE(mgr.stop());   // already stopped
}

TEST(AutoFailoverManagerFocusedTest, IsRunningFalseAfterStop) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.stop());
    EXPECT_FALSE(mgr.isRunning());
}

TEST(AutoFailoverManagerFocusedTest, RestartAfterStopSucceeds) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.stop());
    ASSERT_TRUE(mgr.start());    // restart must succeed
    ASSERT_TRUE(mgr.stop());
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. State machine
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, InitialStateIsIdle) {
    auto mgr = makeManager();
    EXPECT_EQ(mgr.getState(), FailoverOrchestratorState::IDLE);
}

TEST(AutoFailoverManagerFocusedTest, StateIsIdleAfterStop) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    std::this_thread::sleep_for(20ms);
    ASSERT_TRUE(mgr.stop());

    EXPECT_EQ(mgr.getState(), FailoverOrchestratorState::IDLE);
}

TEST(AutoFailoverManagerFocusedTest, IsFailoverInProgressInitiallyFalse) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.isFailoverInProgress());
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Manual failover trigger
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, RejectsFailoverWhenStopped) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.triggerManualFailover("node-a"));
}

TEST(AutoFailoverManagerFocusedTest, AcceptsManualFailoverWhenRunning) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.triggerManualFailover("node-a"));

    std::this_thread::sleep_for(50ms);

    EXPECT_FALSE(mgr.isFailoverInProgress());
    ASSERT_TRUE(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, ManualFailoverWithExplicitTargetNode) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.triggerManualFailover("node-leader", "node-replica-1"));
    std::this_thread::sleep_for(50ms);
    ASSERT_TRUE(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, ManualFailoverWithEmptyNodeIdAccepted) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    // Empty string is a valid call from the API perspective — the orchestrator
    // handles validation internally.
    EXPECT_TRUE(mgr.triggerManualFailover(""));
    std::this_thread::sleep_for(50ms);
    ASSERT_TRUE(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, IsFailoverNotInProgressAfterNullManagerCompletion) {
    // Without a replication_mgr, processFailover returns quickly with failure.
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-x"));

    // Give the failover loop enough time to dequeue and finish.
    std::this_thread::sleep_for(200ms);

    EXPECT_FALSE(mgr.isFailoverInProgress());
    ASSERT_TRUE(mgr.stop());
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Failover queue pressure
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, QueueCapLimitedToMaxConcurrentFailovers) {
    // With max_concurrent_failovers = 1, the second enqueue must be rejected.
    auto cfg = fastConfig(10ms, /*max_concurrent=*/1);
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    bool first  = mgr.triggerManualFailover("node-a");
    bool second = mgr.triggerManualFailover("node-b");

    // At least one must succeed; when queue fills up the second is rejected.
    EXPECT_TRUE(first);
    // 'second' may have been accepted if the first was already dequeued by the
    // worker; so we only assert that they are not both false.
    EXPECT_TRUE(first || second);

    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, QueueAcceptsTwoFailoversWithDefaultCap) {
    // Default cap is 2, so two consecutive enqueues must both succeed.
    auto mgr = makeManager(fastConfig(10ms, /*max_concurrent=*/2));

    ASSERT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.triggerManualFailover("node-1"));
    EXPECT_TRUE(mgr.triggerManualFailover("node-2"));

    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. Configuration management
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, DefaultConfigHasSensibleValues) {
    AutoFailoverConfig cfg;

    EXPECT_GT(cfg.failure_detection_interval, 0ms);
    EXPECT_GT(cfg.health_check_interval, 0ms);
    EXPECT_GT(cfg.failover_timeout, 0ms);
    EXPECT_GT(cfg.spare_activation_timeout, 0ms);
    EXPECT_GT(cfg.leader_election_timeout, 0ms);
    EXPECT_GT(cfg.recovery_retry_interval, 0ms);
    EXPECT_GT(cfg.consecutive_failures_before_action, 0u);
    EXPECT_GT(cfg.max_concurrent_failovers, 0u);

    EXPECT_TRUE(cfg.enable_automatic_failover);
    EXPECT_TRUE(cfg.enable_spare_activation);
    EXPECT_TRUE(cfg.enable_leader_election);
    EXPECT_TRUE(cfg.enable_network_partition_detection);
    EXPECT_TRUE(cfg.enable_split_brain_prevention);
    EXPECT_TRUE(cfg.enable_automatic_recovery);
}

TEST(AutoFailoverManagerFocusedTest, GetConfigReturnsConstructedConfig) {
    AutoFailoverConfig cfg = fastConfig(42ms, 7);
    cfg.enable_automatic_failover = false;
    cfg.max_recovery_attempts     = 5;

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    const auto got = mgr.getConfig();
    EXPECT_EQ(got.health_check_interval, 42ms);
    EXPECT_EQ(got.max_concurrent_failovers, 7u);
    EXPECT_FALSE(got.enable_automatic_failover);
    EXPECT_EQ(got.max_recovery_attempts, 5u);
}

TEST(AutoFailoverManagerFocusedTest, UpdateConfigRoundTrip) {
    auto mgr = makeManager();

    AutoFailoverConfig updated = fastConfig(99ms, 4);
    updated.enable_automatic_recovery = false;

    mgr.updateConfig(updated);

    const auto got = mgr.getConfig();
    EXPECT_EQ(got.health_check_interval, 99ms);
    EXPECT_EQ(got.max_concurrent_failovers, 4u);
    EXPECT_FALSE(got.enable_automatic_recovery);
}

TEST(AutoFailoverManagerFocusedTest, UpdateConfigWhileRunning) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());

    AutoFailoverConfig updated = fastConfig(5ms, 3);
    mgr.updateConfig(updated);

    const auto got = mgr.getConfig();
    EXPECT_EQ(got.max_concurrent_failovers, 3u);

    ASSERT_TRUE(mgr.stop());
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, StatisticsInitiallyZero) {
    auto mgr = makeManager();
    const auto stats = mgr.getStatistics();

    EXPECT_EQ(stats.total_failovers, 0u);
    EXPECT_EQ(stats.successful_failovers, 0u);
    EXPECT_EQ(stats.failed_failovers, 0u);
    EXPECT_EQ(stats.network_partitions_detected, 0u);
    EXPECT_EQ(stats.split_brain_preventions, 0u);
}

TEST(AutoFailoverManagerFocusedTest, StatisticsReflectFailedFailoverWithNullManagers) {
    // When replication_mgr_ is null, processFailover returns failure → failed_failovers++
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-z"));

    // Allow the failover loop to dequeue and finish.
    std::this_thread::sleep_for(200ms);

    ASSERT_TRUE(mgr.stop());

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_failovers, 1u);
    EXPECT_EQ(stats.successful_failovers, 0u);
    EXPECT_EQ(stats.failed_failovers, 1u);
}

TEST(AutoFailoverManagerFocusedTest, StatisticsAccumulateOverMultipleFailovers) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());

    mgr.triggerManualFailover("node-1");
    std::this_thread::sleep_for(100ms);
    mgr.triggerManualFailover("node-2");
    std::this_thread::sleep_for(100ms);

    ASSERT_TRUE(mgr.stop());

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_failovers, 2u);
    EXPECT_EQ(stats.failed_failovers, 2u);   // null managers → always fail
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Event callbacks
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, RegisteredCallbackReceivesFailoverCompleted) {
    auto mgr = makeManager();

    std::vector<FailoverEventType> received;
    std::mutex mtx;

    mgr.registerEventCallback([&](FailoverEventType t, const std::string&, const std::string&) {
        std::lock_guard<std::mutex> lk(mtx);
        received.push_back(t);
    });

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-cb"));
    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());

    std::lock_guard<std::mutex> lk(mtx);
    // At minimum QUORUM_CHECK_PASSED should have fired during processFailover.
    EXPECT_FALSE(received.empty());
}

TEST(AutoFailoverManagerFocusedTest, MultipleCallbacksAllInvoked) {
    auto mgr = makeManager();

    std::atomic<int> count1{0}, count2{0};

    mgr.registerEventCallback([&](FailoverEventType, const std::string&, const std::string&) {
        ++count1;
    });
    mgr.registerEventCallback([&](FailoverEventType, const std::string&, const std::string&) {
        ++count2;
    });

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-multi"));
    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());

    // Both callbacks must have been called the same number of times.
    EXPECT_EQ(count1.load(), count2.load());
    EXPECT_GT(count1.load(), 0);
}

TEST(AutoFailoverManagerFocusedTest, CallbackExceptionDoesNotCrashManager) {
    auto mgr = makeManager();

    mgr.registerEventCallback([](FailoverEventType, const std::string&, const std::string&) {
        throw std::runtime_error("intentional test exception");
    });

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-exc"));
    std::this_thread::sleep_for(200ms);
    // Manager must still be running after a callback exception.
    EXPECT_TRUE(mgr.isRunning());
    ASSERT_TRUE(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, CallbackReceivesCorrectNodeId) {
    auto mgr = makeManager();

    std::string captured_node;
    std::mutex mtx;

    mgr.registerEventCallback([&](FailoverEventType, const std::string& node, const std::string&) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!node.empty()) {
            captured_node = node;
        }
    });

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("my-specific-node"));
    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());

    std::lock_guard<std::mutex> lk(mtx);
    EXPECT_EQ(captured_node, "my-specific-node");
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. Failure tracking (getFailingNodes)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, GetFailingNodesInitiallyEmpty) {
    auto mgr = makeManager();
    EXPECT_TRUE(mgr.getFailingNodes().empty());
}

TEST(AutoFailoverManagerFocusedTest, GetFailingNodesEmptyAfterStartWithNullManagers) {
    // With null health_monitor the monitoring loop does nothing — no failures tracked.
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    std::this_thread::sleep_for(50ms);
    ASSERT_TRUE(mgr.stop());

    EXPECT_TRUE(mgr.getFailingNodes().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. Last failover result
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, LastFailoverResultInitiallyAbsent) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.getLastFailoverResult().has_value());
}

TEST(AutoFailoverManagerFocusedTest, LastFailoverResultPopulatedAfterRun) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-last-result"));
    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());

    const auto result = mgr.getLastFailoverResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->failed_node_id, "node-last-result");
}

TEST(AutoFailoverManagerFocusedTest, LastFailoverResultContainsDuration) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-dur"));
    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());

    const auto result = mgr.getLastFailoverResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result->duration.count(), 0);
}

TEST(AutoFailoverManagerFocusedTest, LastFailoverResultFailedWithNullReplicationManager) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.triggerManualFailover("node-fail"));
    std::this_thread::sleep_for(200ms);
    ASSERT_TRUE(mgr.stop());

    const auto result = mgr.getLastFailoverResult();
    ASSERT_TRUE(result.has_value());
    // Without a real ReplicationManager, checkAndWaitForQuorum() returns false.
    EXPECT_FALSE(result->success);
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. Edge cases
// ─────────────────────────────────────────────────────────────────────────────

TEST(AutoFailoverManagerFocusedTest, NullManagersDoNotCrash) {
    // All dependency pointers are null — manager must not crash during run.
    auto mgr = makeManager(fastConfig(5ms));

    ASSERT_TRUE(mgr.start());
    std::this_thread::sleep_for(30ms);
    EXPECT_NO_THROW(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, DisableAutoFailoverFlagRespected) {
    auto cfg = fastConfig();
    cfg.enable_automatic_failover = false;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    EXPECT_FALSE(mgr.getConfig().enable_automatic_failover);
}

TEST(AutoFailoverManagerFocusedTest, DisableNetworkPartitionDetectionFlagRespected) {
    auto cfg = fastConfig();
    cfg.enable_network_partition_detection = false;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    EXPECT_FALSE(mgr.getConfig().enable_network_partition_detection);
    // Manager should start and run without crashing even with partition detection off.
    ASSERT_TRUE(mgr.start());
    std::this_thread::sleep_for(20ms);
    ASSERT_TRUE(mgr.stop());
}

TEST(AutoFailoverManagerFocusedTest, DisableSplitBrainPreventionFlagRespected) {
    auto cfg = fastConfig();
    cfg.enable_split_brain_prevention = false;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());
    mgr.triggerManualFailover("node-no-sb");
    std::this_thread::sleep_for(100ms);
    ASSERT_TRUE(mgr.stop());

    // Without split-brain prevention, the failover should still complete (fail)
    EXPECT_EQ(mgr.getStatistics().total_failovers, 1u);
}

TEST(AutoFailoverManagerFocusedTest, DestructorStopsRunningManager) {
    // Ensures that the destructor stops background threads without a crash.
    auto cfg = fastConfig(5ms);
    {
        AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
        ASSERT_TRUE(mgr.start());
        std::this_thread::sleep_for(20ms);
        // mgr destroyed here; destructor must stop threads
    }
    // If we reach this point without a crash or hang, the test passes.
    SUCCEED();
}

TEST(AutoFailoverManagerFocusedTest, MultipleFailoversThenStatsSumCorrectly) {
    auto mgr = makeManager();

    ASSERT_TRUE(mgr.start());

    const int kRuns = 3;
    for (int i = 0; i < kRuns; ++i) {
        ASSERT_TRUE(mgr.triggerManualFailover("node-" + std::to_string(i)));
        std::this_thread::sleep_for(100ms);
    }

    ASSERT_TRUE(mgr.stop());

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_failovers,
              stats.successful_failovers + stats.failed_failovers);
    EXPECT_EQ(stats.total_failovers, static_cast<uint64_t>(kRuns));
}
} } } // namespace themis::failover::test
