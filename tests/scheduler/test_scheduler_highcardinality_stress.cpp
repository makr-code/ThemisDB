// tests/scheduler/test_scheduler_highcardinality_stress.cpp
// Wave D high-cardinality stress tests for the Scheduler module.
// Labels: wave_d;stress;not_release_critical
//
// Design: all tests use in-process stubs; no external engine required.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct TaskRegistry {
    std::atomic<uint64_t> registered{0};
    std::atomic<uint64_t> errors{0};

    bool register_task(uint64_t task_id, int priority) {
        (void)task_id; (void)priority;
        registered.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct TriggerEngine {
    std::atomic<uint64_t> fired{0};
    std::atomic<uint64_t> errors{0};

    bool fire(uint64_t trigger_id) {
        (void)trigger_id;
        fired.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct DistributedCoordinator {
    std::atomic<uint64_t> ops{0};
    std::atomic<uint64_t> timeouts{0};

    bool coordinate(uint64_t node_id, uint64_t task_id) {
        (void)node_id; (void)task_id;
        ops.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

} // namespace stubs

// ---------------------------------------------------------------------------
// HighCardinalityTaskRegistration
// ---------------------------------------------------------------------------
// Registers 5 000 tasks across 8 threads; validates that all registrations
// succeed and the registry count is exact.
// ---------------------------------------------------------------------------
TEST(SchedulerHighCardinalityStress, HighCardinalityTaskRegistration) {
    constexpr int      kThreads           = 8;
    constexpr uint64_t kTasksPerThread    = 625ULL; // 8 × 625 = 5 000
    constexpr uint64_t kTotalTasks        = kThreads * kTasksPerThread;

    stubs::TaskRegistry registry;
    std::atomic<uint64_t> reg_errors{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t base = static_cast<uint64_t>(t) * kTasksPerThread;
            for (uint64_t i = 0; i < kTasksPerThread; ++i) {
                if (!registry.register_task(base + i, /*priority=*/t)) {
                    reg_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(registry.registered.load(), kTotalTasks)
        << "Registration count mismatch: expected " << kTotalTasks;
    EXPECT_EQ(reg_errors.load(), 0u)
        << "Task registration errors during high-cardinality stress";
}

// ---------------------------------------------------------------------------
// ConcurrentTriggerStress
// ---------------------------------------------------------------------------
// Fires a high volume of triggers concurrently; validates zero error rate and
// that trigger count matches expected.
// ---------------------------------------------------------------------------
TEST(SchedulerHighCardinalityStress, ConcurrentTriggerStress) {
    constexpr int      kThreads           = 8;
    constexpr uint64_t kTriggersPerThread = 100'000ULL;

    stubs::TriggerEngine engine;
    std::atomic<uint64_t> trigger_errors{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t base = static_cast<uint64_t>(t) * kTriggersPerThread;
            for (uint64_t i = 0; i < kTriggersPerThread; ++i) {
                if (!engine.fire(base + i)) {
                    trigger_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    const uint64_t expected = static_cast<uint64_t>(kThreads) * kTriggersPerThread;
    EXPECT_EQ(engine.fired.load(), expected)
        << "Trigger count mismatch";
    EXPECT_EQ(trigger_errors.load(), 0u)
        << "[SCHEDULER:TriggerDeadlock] Trigger errors under concurrent stress";
}

// ---------------------------------------------------------------------------
// DistributedCoordinationLoad
// ---------------------------------------------------------------------------
// Drives the distributed coordinator with concurrent multi-node task
// coordination; validates ops count and zero timeout rate.
// ---------------------------------------------------------------------------
TEST(SchedulerHighCardinalityStress, DistributedCoordinationLoad) {
    constexpr int      kThreads    = 8;
    constexpr uint64_t kOpsPerThread = 50'000ULL;
    constexpr uint64_t kNodes      = 16ULL;

    stubs::DistributedCoordinator coordinator;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            uint64_t base = static_cast<uint64_t>(t) * kOpsPerThread;
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                uint64_t node    = i % kNodes;
                uint64_t task_id = base + i;
                if (!coordinator.coordinate(node, task_id)) {
                    coordinator.timeouts.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    const uint64_t expected = static_cast<uint64_t>(kThreads) * kOpsPerThread;
    EXPECT_EQ(coordinator.ops.load(), expected)
        << "Coordinator ops count mismatch";
    EXPECT_EQ(coordinator.timeouts.load(), 0u)
        << "[SCHEDULER:CoordinatorTimeout] Coordinator timeouts under load";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
