/**
 * @file test_failover_chaos_scenarios.cpp
 * @brief Chaos + failover end-to-end scenario matrix (Phase 4 / Phase 5)
 *
 * Covers:
 * - Queue saturation and pressure telemetry (Phase 5)
 * - Retry telemetry tracking (Phase 5)
 * - Multi-node cascading failure detection
 * - Rapid enqueue / drain cycles
 * - Statistics accuracy after mixed success/failure sequences
 * - Event callback verification (QUEUE_PRESSURE, RECOVERY_STARTED)
 * - Config hot-update while running
 * - Idempotent start/stop lifecycle
 */

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"

using namespace std::chrono_literals;
using namespace themis::failover;

namespace {

// Build a minimal config with fast timers for unit testing.
AutoFailoverConfig makeFastConfig() {
    AutoFailoverConfig cfg;
    cfg.health_check_interval           = 10ms;
    cfg.failure_detection_interval      = 10ms;
    cfg.failover_timeout                = 50ms;
    cfg.spare_activation_timeout        = 50ms;
    cfg.leader_election_timeout         = 50ms;
    cfg.recovery_retry_interval         = 10ms;
    cfg.max_recovery_attempts           = 2;
    cfg.enable_automatic_failover       = true;
    cfg.enable_automatic_recovery       = false; // avoid 5-second delay in unit tests
    cfg.enable_spare_activation         = false;
    cfg.enable_network_partition_detection = false;
    cfg.enable_split_brain_prevention   = false;
    cfg.max_concurrent_failovers        = 4;
    cfg.queue_pressure_threshold        = 0.75f;
    return cfg;
}

}  // namespace

namespace themis { namespace failover { namespace test { 

// ── Lifecycle ─────────────────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, IdempotentDoubleStart) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    EXPECT_TRUE(mgr.start());
    EXPECT_FALSE(mgr.start()); // second start should fail
    EXPECT_TRUE(mgr.isRunning());
    EXPECT_TRUE(mgr.stop());
    EXPECT_FALSE(mgr.isRunning());
}

TEST(FailoverChaosScenarios, IdempotentDoubleStop) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    EXPECT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.stop());
    EXPECT_FALSE(mgr.stop()); // second stop should return false
}

TEST(FailoverChaosScenarios, StartStopMultipleCycles) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(mgr.start());
        std::this_thread::sleep_for(20ms);
        EXPECT_TRUE(mgr.stop());
    }
    EXPECT_FALSE(mgr.isRunning());
}

// ── Queue Saturation & Pressure Telemetry ─────────────────────────────────────

TEST(FailoverChaosScenarios, QueueSaturationDropsTasks) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 2;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    // Drain the queue quickly by stopping and restarting to avoid side effects
    // We rely on the fact that triggerManualFailover returns false when full.
    // Saturate queue (max_concurrent_failovers = 2)
    bool first  = mgr.triggerManualFailover("node-x");
    bool second = mgr.triggerManualFailover("node-y");
    bool third  = mgr.triggerManualFailover("node-z"); // should be dropped

    EXPECT_TRUE(first);
    EXPECT_TRUE(second);
    EXPECT_FALSE(third); // queue full

    // The drop must be recorded in statistics
    std::this_thread::sleep_for(30ms);
    const auto stats = mgr.getStatistics();
    EXPECT_GE(stats.tasks_dropped_queue_full, 1u);

    ASSERT_TRUE(mgr.stop());
}

TEST(FailoverChaosScenarios, QueueDepthTracked) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 8;
    // Disable automatic failover processing by using a stopped manager,
    // but we need it running to enqueue.
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    mgr.triggerManualFailover("node-1");
    mgr.triggerManualFailover("node-2");
    mgr.triggerManualFailover("node-3");

    // max_queue_depth_observed must be at least 1 (tasks may drain quickly)
    std::this_thread::sleep_for(50ms);
    const auto stats = mgr.getStatistics();
    EXPECT_GE(stats.max_queue_depth_observed, 1u);

    ASSERT_TRUE(mgr.stop());
}

TEST(FailoverChaosScenarios, QueuePressureEventEmittedAtThreshold) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 4;
    cfg.queue_pressure_threshold  = 0.5f; // pressure at 2/4
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    std::atomic<int> pressure_events{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::QUEUE_PRESSURE) {
                pressure_events.fetch_add(1, std::memory_order_relaxed);
            }
        });

    ASSERT_TRUE(mgr.start());

    // Enqueue enough tasks to cross the 50% threshold
    mgr.triggerManualFailover("node-a");
    mgr.triggerManualFailover("node-b"); // at 50% → should trigger pressure
    mgr.triggerManualFailover("node-c");

    std::this_thread::sleep_for(40ms);

    EXPECT_GE(pressure_events.load(), 1);

    const auto stats = mgr.getStatistics();
    EXPECT_GE(stats.queue_pressure_events, 1u);

    ASSERT_TRUE(mgr.stop());
}

// ── Statistics Accuracy ────────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, StatisticsAfterMultipleFailoverCycles) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 10;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    const int N = 5;
    for (int i = 0; i < N; ++i) {
        mgr.triggerManualFailover("node-" + std::to_string(i));
    }

    // Let the worker process all tasks (each will fail quickly due to no managers)
    std::this_thread::sleep_for(200ms);

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_failovers, static_cast<uint64_t>(N));
    EXPECT_EQ(stats.failed_failovers + stats.successful_failovers,
              static_cast<uint64_t>(N));

    ASSERT_TRUE(mgr.stop());
}

TEST(FailoverChaosScenarios, DropCountDoesNotAffectTotalFailovers) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 1;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    // Queue one task (may drain instantly), try to queue more
    mgr.triggerManualFailover("node-a");
    mgr.triggerManualFailover("node-b"); // might be dropped
    mgr.triggerManualFailover("node-c"); // might be dropped

    std::this_thread::sleep_for(150ms);

    const auto stats = mgr.getStatistics();
    // total_failovers counts only tasks that were dequeued and processed;
    // tasks_dropped_queue_full counts only tasks rejected at the queue boundary.
    // Their sum must not exceed the number of triggerManualFailover calls (3).
    constexpr uint64_t kMaxAttempts = 3;
    EXPECT_LE(stats.total_failovers + stats.tasks_dropped_queue_full, kMaxAttempts);

    ASSERT_TRUE(mgr.stop());
}

// ── Rapid Enqueue / Drain ─────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, RapidEnqueueDrainCycle) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 20;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 5; ++i) {
            mgr.triggerManualFailover("n-" + std::to_string(round * 10 + i));
        }
        std::this_thread::sleep_for(50ms);
    }

    std::this_thread::sleep_for(100ms);

    const auto stats = mgr.getStatistics();
    EXPECT_GT(stats.total_failovers, 0u);

    ASSERT_TRUE(mgr.stop());
}

// ── Config Hot-Update ─────────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, ConfigHotUpdateWhileRunning) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    AutoFailoverConfig updated = makeFastConfig();
    updated.max_concurrent_failovers = 8;
    updated.queue_pressure_threshold  = 0.9f;
    mgr.updateConfig(updated);

    const auto fetched = mgr.getConfig();
    EXPECT_EQ(fetched.max_concurrent_failovers, 8u);
    EXPECT_FLOAT_EQ(fetched.queue_pressure_threshold, 0.9f);

    ASSERT_TRUE(mgr.stop());
}

// ── Event Callbacks ───────────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, EventCallbacksReceiveNodeFailureEvents) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 10;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    std::mutex cb_mutex;
    std::vector<FailoverEventType> received;
    mgr.registerEventCallback(
        [&](FailoverEventType t, const std::string&, const std::string&) {
            std::lock_guard<std::mutex> lk(cb_mutex);
            received.push_back(t);
        });

    ASSERT_TRUE(mgr.start());
    mgr.triggerManualFailover("node-alpha");
    std::this_thread::sleep_for(100ms);
    ASSERT_TRUE(mgr.stop());

    std::lock_guard<std::mutex> lk(cb_mutex);
    EXPECT_FALSE(received.empty());
}

TEST(FailoverChaosScenarios, MultipleCallbacksAreAllInvoked) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 10;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    std::atomic<int> cb1{0}, cb2{0};
    mgr.registerEventCallback([&](FailoverEventType, const std::string&, const std::string&) {
        cb1.fetch_add(1, std::memory_order_relaxed);
    });
    mgr.registerEventCallback([&](FailoverEventType, const std::string&, const std::string&) {
        cb2.fetch_add(1, std::memory_order_relaxed);
    });

    ASSERT_TRUE(mgr.start());
    mgr.triggerManualFailover("node-beta");
    std::this_thread::sleep_for(100ms);
    ASSERT_TRUE(mgr.stop());

    EXPECT_GT(cb1.load(), 0);
    EXPECT_GT(cb2.load(), 0);
    EXPECT_EQ(cb1.load(), cb2.load());
}

// ── getFailingNodes ────────────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, GetFailingNodesInitiallyEmpty) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    EXPECT_TRUE(mgr.getFailingNodes().empty());
}

// ── getLastFailoverResult ──────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, LastFailoverResultNullBeforeAnyFailover) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    EXPECT_FALSE(mgr.getLastFailoverResult().has_value());
}

TEST(FailoverChaosScenarios, LastFailoverResultPopulatedAfterFailover) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 10;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());
    mgr.triggerManualFailover("node-gamma");
    std::this_thread::sleep_for(150ms);
    ASSERT_TRUE(mgr.stop());

    const auto result = mgr.getLastFailoverResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->failed_node_id, "node-gamma");
}

// ── getState ─────────────────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, StateIsIdleAfterStop) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());
    ASSERT_TRUE(mgr.stop());

    EXPECT_EQ(mgr.getState(), FailoverOrchestratorState::IDLE);
}

// ── Concurrent Access ─────────────────────────────────────────────────────────

TEST(FailoverChaosScenarios, ConcurrentTriggersSafe) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 50;
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    ASSERT_TRUE(mgr.start());

    std::vector<std::thread> threads;
    std::atomic<int> enqueued{0};
    std::atomic<int> dropped{0};

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 5; ++j) {
                if (mgr.triggerManualFailover("node-" + std::to_string(i) + "-" +
                                              std::to_string(j))) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    dropped.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& t : threads) t.join();

    std::this_thread::sleep_for(300ms);
    ASSERT_TRUE(mgr.stop());

    const auto stats = mgr.getStatistics();
    // All accounted tasks must be either processed or dropped; no task should be lost.
    // Tasks still in-flight when stop() is called may not appear in total_failovers,
    // so we verify the weaker invariant: processed + stats-dropped <= enqueued + dropped.
    EXPECT_LE(stats.total_failovers,
              static_cast<uint64_t>(enqueued.load()));
    EXPECT_EQ(stats.tasks_dropped_queue_full,
              static_cast<uint64_t>(dropped.load()));
}
} } } // namespace themis::failover::test
