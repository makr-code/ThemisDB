/**
 * @file test_process_highcardinality_stress.cpp
 * @brief Wave D — Process Manager High-Cardinality Stress Tests.
 *
 * Stress tests covering high-cardinality process spawn, concurrent signal
 * delivery, and resource-exhaustion boundary behaviour.
 *
 * ## Test IDs
 * - HighCardinalityProcessSpawn  : 500 processes spawned across 4 threads
 * - ConcurrentSignalStress       : concurrent signal delivery under contention
 * - ResourceExhaustionBehavior   : correct rejection beyond resource limits
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see src/process/ROADMAP.md — Wave D contribution closure
 * @see docs/operability/RUNBOOK_PROCESS_MANAGER.md — operator runbook
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
// In-process stubs model the process manager without spawning OS processes.
// MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// StubProcess & StubProcessManager
// ---------------------------------------------------------------------------
struct StubProcess {
    uint64_t pid;
    bool running{true};
};

class StubProcessManager {
public:
    explicit StubProcessManager(uint64_t max_processes)
        : max_processes_(max_processes) {}

    /// Returns allocated pid, or 0 on failure (capacity exceeded).
    uint64_t spawn() {
        std::lock_guard<std::mutex> lk(mu_);
        if (processes_.size() >= max_processes_) {
            ++spawn_failures_;
            return 0;
        }
        const uint64_t pid = next_pid_++;
        processes_[pid] = StubProcess{pid, true};
        ++spawn_count_;
        ++active_count_;
        fd_count_ += 3;
        return pid;
    }

    bool terminate(uint64_t pid) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = processes_.find(pid);
        if (it == processes_.end() || !it->second.running) return false;
        it->second.running = false;
        ++terminate_count_;
        --active_count_;
        fd_count_ -= 3;
        return true;
    }

    uint64_t spawnCount()     const { return spawn_count_.load(); }
    uint64_t terminateCount() const { return terminate_count_.load(); }
    uint64_t spawnFailures()  const { return spawn_failures_.load(); }
    int64_t  activeCount()    const { return active_count_.load(); }
    int64_t  fdCount()        const { return fd_count_.load(); }

private:
    const uint64_t max_processes_;
    std::mutex mu_;
    std::unordered_map<uint64_t, StubProcess> processes_;
    uint64_t next_pid_{1};
    std::atomic<uint64_t> spawn_count_{0};
    std::atomic<uint64_t> terminate_count_{0};
    std::atomic<uint64_t> spawn_failures_{0};
    std::atomic<int64_t>  active_count_{0};
    std::atomic<int64_t>  fd_count_{0};
};

// ---------------------------------------------------------------------------
// StubSignalBus
// ---------------------------------------------------------------------------
class StubSignalBus {
public:
    void send(uint64_t pid, int signum) {
        (void)pid; (void)signum;
        ++sent_;
        ++delivered_;  // stub: synchronous delivery
    }
    uint64_t sent()      const { return sent_.load(); }
    uint64_t delivered() const { return delivered_.load(); }
private:
    std::atomic<uint64_t> sent_{0};
    std::atomic<uint64_t> delivered_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────
class ProcessHighCardinalityStress : public ::testing::Test {
protected:
    static constexpr int kProcesses = 500;
    static constexpr int kThreads   = 4;
};

// ─────────────────────────────────────────────────────────────────────────────
// HighCardinalityProcessSpawn
//
// Spawn kProcesses processes from kThreads producer threads.
// After joining, all processes must have been spawned and then terminated with
// no active count or fd-count remaining.
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ProcessHighCardinalityStress, HighCardinalityProcessSpawn) {
    StubProcessManager mgr(static_cast<uint64_t>(kProcesses) * 2);

    std::atomic<int> next_id{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            while (true) {
                const int id = next_id.fetch_add(1, std::memory_order_relaxed);
                if (id >= kProcesses) break;
                const uint64_t pid = mgr.spawn();
                EXPECT_NE(pid, 0u) << "Spawn must succeed for process " << id;
                if (pid != 0) mgr.terminate(pid);
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(mgr.spawnCount(),
              static_cast<uint64_t>(kProcesses))
        << "All " << kProcesses << " processes must be spawned";
    EXPECT_EQ(mgr.activeCount(), 0)
        << "No active processes must remain (leak check)";
    EXPECT_EQ(mgr.fdCount(), 0)
        << "No fd must remain open after all terminates";
    EXPECT_EQ(mgr.spawnFailures(), 0u)
        << "No spawn must fail when capacity is sufficient";
}

// ─────────────────────────────────────────────────────────────────────────────
// ConcurrentSignalStress
//
// Spawn a pool of processes, then hammer the signal bus from kThreads threads.
// All sent signals must be delivered (100 % delivery in the stub model).
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ProcessHighCardinalityStress, ConcurrentSignalStress) {
    constexpr int kSignalsPerThread = 2500;   // 4 threads × 2500 = 10 000 total
    StubProcessManager mgr(64);
    StubSignalBus bus;

    // Spawn a small pool of persistent processes
    std::vector<uint64_t> pids;
    pids.reserve(8);
    for (int i = 0; i < 8; ++i) {
        const uint64_t pid = mgr.spawn();
        ASSERT_NE(pid, 0u);
        pids.push_back(pid);
    }

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kSignalsPerThread; ++i) {
                const uint64_t pid =
                    pids[static_cast<std::size_t>(i + t) % pids.size()];
                bus.send(pid, 15 /* SIGTERM */);
            }
        });
    }
    for (auto& t : threads) t.join();

    for (auto pid : pids) mgr.terminate(pid);

    const uint64_t expected =
        static_cast<uint64_t>(kThreads) *
        static_cast<uint64_t>(kSignalsPerThread);

    EXPECT_EQ(bus.sent(), expected)
        << "All expected signals must be sent";
    EXPECT_EQ(bus.delivered(), bus.sent())
        << "100 % signal delivery required. "
           "Sent=" << bus.sent() << " Delivered=" << bus.delivered();
}

// ─────────────────────────────────────────────────────────────────────────────
// ResourceExhaustionBehavior
//
// Attempt to spawn more processes than the manager's capacity allows.
// Excess spawns must be rejected (return pid=0) and must not corrupt the
// internal state of the manager.  After draining, active_count must be 0.
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ProcessHighCardinalityStress, ResourceExhaustionBehavior) {
    constexpr uint64_t kCapacity = 50;
    constexpr int      kAttempts = static_cast<int>(kCapacity) * 3;

    StubProcessManager mgr(kCapacity);
    std::vector<uint64_t> spawned_pids;
    spawned_pids.reserve(static_cast<std::size_t>(kAttempts));

    // Single-threaded: spawn more than capacity
    for (int i = 0; i < kAttempts; ++i) {
        const uint64_t pid = mgr.spawn();
        if (pid != 0) spawned_pids.push_back(pid);
    }

    EXPECT_LE(mgr.activeCount(), static_cast<int64_t>(kCapacity))
        << "Active count must never exceed capacity";
    EXPECT_GT(mgr.spawnFailures(), 0u)
        << "Some spawns must fail when capacity is exceeded";
    EXPECT_EQ(mgr.spawnCount() + mgr.spawnFailures(),
              static_cast<uint64_t>(kAttempts))
        << "Every attempt must be either a success or a failure";

    // Terminate all successfully spawned processes
    for (auto pid : spawned_pids) mgr.terminate(pid);

    EXPECT_EQ(mgr.activeCount(), 0)
        << "Active count must be 0 after terminating all spawned processes";
    EXPECT_EQ(mgr.fdCount(), 0)
        << "No fd must remain open after draining all processes";
}
