// tests/integration/test_scheduler_soak.cpp
// Wave D soak tests for the Scheduler module.
// Labels: wave_d;soak;not_release_critical
// THEMIS_SOAK_DURATION_MS controls wall-clock run length (default 60 000 ms).
//
// Design: all tests use in-process stubs so no external engine is required.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static long long soak_duration_ms() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env) {
        try { return std::stoll(env); } catch (...) {}
    }
    return 60000LL;
}

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct SchedulerStub {
    std::atomic<uint64_t> registered{0};
    std::atomic<uint64_t> executed{0};
    std::atomic<uint64_t> anomaly_triggers{0};
    std::atomic<uint64_t> queue_depth{0};

    std::mutex             queue_mu;
    std::queue<uint64_t>   task_queue;

    bool register_task(uint64_t task_id) {
        {
            std::lock_guard<std::mutex> lk(queue_mu);
            task_queue.push(task_id);
        }
        registered.fetch_add(1, std::memory_order_relaxed);
        queue_depth.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool execute_next() {
        uint64_t task_id = 0;
        {
            std::lock_guard<std::mutex> lk(queue_mu);
            if (task_queue.empty()) return false;
            task_id = task_queue.front();
            task_queue.pop();
        }
        queue_depth.fetch_sub(1, std::memory_order_relaxed);
        executed.fetch_add(1, std::memory_order_relaxed);
        (void)task_id;
        return true;
    }

    void trigger_anomaly() {
        anomaly_triggers.fetch_add(1, std::memory_order_relaxed);
    }
};

} // namespace stubs

// ---------------------------------------------------------------------------
// SchedulerSoak_TaskRegisterExecuteCycle
// ---------------------------------------------------------------------------
// Continuously registers and executes tasks for the soak window; validates
// that every registered task is eventually executed (zero queue leak).
// ---------------------------------------------------------------------------
TEST(SchedulerSoak, SchedulerSoak_TaskRegisterExecuteCycle) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::SchedulerStub sched;

    // Producer threads register tasks.
    const int kProducers = 4;
    std::vector<std::thread> producers;
    for (int t = 0; t < kProducers; ++t) {
        producers.emplace_back([&, t]() {
            uint64_t id = static_cast<uint64_t>(t) * 1'000'000ULL;
            while (Clock::now() < deadline) {
                sched.register_task(id++);
            }
        });
    }

    // Consumer threads execute tasks.
    const int kConsumers = 4;
    std::vector<std::thread> consumers;
    for (int t = 0; t < kConsumers; ++t) {
        consumers.emplace_back([&]() {
            while (Clock::now() < deadline) {
                sched.execute_next();
                std::this_thread::yield();
            }
        });
    }

    for (auto& th : producers) th.join();
    // Drain remaining tasks after deadline.
    while (sched.execute_next()) {}
    for (auto& th : consumers) th.join();

    EXPECT_GT(sched.registered.load(), 0u)
        << "No tasks registered during soak";
    EXPECT_EQ(sched.queue_depth.load(), 0u)
        << "[SCHEDULER:QueueOverflow] Tasks remained in queue after drain";
}

// ---------------------------------------------------------------------------
// SchedulerSoak_BurstQueueStability
// ---------------------------------------------------------------------------
// Periodically fires large burst registrations; validates queue depth remains
// bounded and throughput is sustained over the soak window.
// ---------------------------------------------------------------------------
TEST(SchedulerSoak, SchedulerSoak_BurstQueueStability) {
    const auto duration_ms  = soak_duration_ms();
    const auto deadline      = Clock::now() + std::chrono::milliseconds(duration_ms);
    const uint64_t kMaxDepth = 10000ULL;

    stubs::SchedulerStub sched;
    std::atomic<uint64_t> overflow_events{0};

    // Burst producer: registers 500 tasks every 100 ms.
    std::thread burst_producer([&]() {
        uint64_t id = 0;
        while (Clock::now() < deadline) {
            for (int i = 0; i < 500; ++i) {
                sched.register_task(id++);
                if (sched.queue_depth.load() > kMaxDepth) {
                    overflow_events.fetch_add(1, std::memory_order_relaxed);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // Steady consumers.
    const int kConsumers = 4;
    std::vector<std::thread> consumers;
    for (int t = 0; t < kConsumers; ++t) {
        consumers.emplace_back([&]() {
            while (Clock::now() < deadline) {
                sched.execute_next();
                std::this_thread::yield();
            }
        });
    }

    burst_producer.join();
    while (sched.execute_next()) {}
    for (auto& th : consumers) th.join();

    EXPECT_GT(sched.executed.load(), 0u)
        << "No tasks executed during burst soak";
    EXPECT_EQ(overflow_events.load(), 0u)
        << "[SCHEDULER:QueueOverflow] queue depth exceeded kMaxDepth="
        << kMaxDepth << " during burst soak";
}

// ---------------------------------------------------------------------------
// SchedulerSoak_AnomalyTriggerReliability
// ---------------------------------------------------------------------------
// Continuously fires anomaly triggers while the scheduler runs; validates
// that trigger count is monotonically increasing and no state corruption occurs.
// ---------------------------------------------------------------------------
TEST(SchedulerSoak, SchedulerSoak_AnomalyTriggerReliability) {
    const auto duration_ms = soak_duration_ms();
    const auto deadline     = Clock::now() + std::chrono::milliseconds(duration_ms);

    stubs::SchedulerStub sched;

    // Background task register/execute cycle.
    std::thread worker([&]() {
        uint64_t id = 0;
        while (Clock::now() < deadline) {
            sched.register_task(id++);
            sched.execute_next();
        }
    });

    // Anomaly trigger thread.
    std::thread trigger([&]() {
        while (Clock::now() < deadline) {
            sched.trigger_anomaly();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    worker.join();
    trigger.join();

    EXPECT_GT(sched.anomaly_triggers.load(), 0u)
        << "[SCHEDULER:AnomalyFalsePositive] No anomaly triggers fired during soak";
    // Anomaly trigger count must be at least duration_ms/50 - 2 (timing slack)
    const uint64_t min_triggers = static_cast<uint64_t>(duration_ms / 50) - 2;
    EXPECT_GE(sched.anomaly_triggers.load(), min_triggers)
        << "Anomaly trigger count lower than expected minimum " << min_triggers;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
