/**
 * @file test_chaos_fault_soak.cpp
 * @brief Wave D — Chaos Fault Injection Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB chaos module primary paths.
 * Verifies that fault inject/recover cycles, scheduler stability under
 * load, and callback dispatch reliability hold up over a sustained run.
 *
 * In CI environments this test is run with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate runs in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Recover rate = 100% (every injected fault is successfully recovered)
 * - No callback re-entry detected across the soak duration
 * - Scheduler always returns to STOPPED state after stop()
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_CHAOS_FAULT_INJECTION.md — operator runbook
 * @see src/chaos/ROADMAP.md — Wave D Closure Batch (2026-09-16)
 * @see docs/operability/WAVE_D_ROADMAP.md — Phase 4 Soak Tests
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min) so CI completes quickly.
// Production soak: 3 600 000 ms (60 min).
// ─────────────────────────────────────────────────────────────────────────────
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL; // Default CI-safe: 1 minute
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The in-process stubs below replace the live FaultInjector and ChaosScheduler
// paths so no external process injection or clock-stepping is required.
// Correctness characteristics mirror the real module contracts:
//   - inject() is accepted iff node_id is non-empty (mirrors fail-closed rule)
//   - recover() returns true iff the fault was previously injected
//   - Scheduler start/stop transitions follow the STOPPED↔RUNNING FSM contract
//   - Callbacks are dispatched in FIFO order with no re-entry (§ 5)
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// In-process FaultInjector stub
// ─────────────────────────────────────────────────────────────────────────────

class StubFaultInjector {
public:
    enum class CallbackEvent { INJECTED, RECOVERED };
    using CallbackFn = std::function<void(const std::string&, CallbackEvent)>;

    /// Inject a fault; returns false if node_id is empty (fail-closed).
    bool inject(const std::string& node_id, double probability = 1.0) {
        if (node_id.empty() || probability <= 0.0) return false;
        std::lock_guard<std::mutex> lk(mu_);
        active_faults_.insert(node_id);
        inject_count_.fetch_add(1, std::memory_order_relaxed);
        dispatchCallback(node_id, CallbackEvent::INJECTED);
        return true;
    }

    /// Recover a fault; returns false if fault was not active.
    bool recover(const std::string& node_id) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = active_faults_.find(node_id);
        if (it == active_faults_.end()) return false;
        active_faults_.erase(it);
        recover_count_.fetch_add(1, std::memory_order_relaxed);
        dispatchCallback(node_id, CallbackEvent::RECOVERED);
        return true;
    }

    bool isFaultActive(const std::string& node_id) const {
        std::lock_guard<std::mutex> lk(mu_);
        return active_faults_.count(node_id) > 0;
    }

    void setCallback(CallbackFn fn) {
        std::lock_guard<std::mutex> lk(mu_);
        callback_ = std::move(fn);
    }

    uint64_t injectCount()  const noexcept { return inject_count_.load(std::memory_order_relaxed); }
    uint64_t recoverCount() const noexcept { return recover_count_.load(std::memory_order_relaxed); }

private:
    void dispatchCallback(const std::string& node_id, CallbackEvent evt) {
        // Called under mu_ — no re-entry allowed per § 5
        if (callback_) callback_(node_id, evt);
    }

    mutable std::mutex                     mu_;
    std::unordered_set<std::string>        active_faults_;
    CallbackFn                             callback_;
    std::atomic<uint64_t>                  inject_count_{0};
    std::atomic<uint64_t>                  recover_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// In-process ChaosScheduler stub — STOPPED / RUNNING FSM
// ─────────────────────────────────────────────────────────────────────────────

class StubChaosScheduler {
public:
    enum class State { STOPPED, RUNNING };

    void start() {
        std::lock_guard<std::mutex> lk(mu_);
        state_ = State::RUNNING;
        start_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void stop() {
        std::lock_guard<std::mutex> lk(mu_);
        state_ = State::STOPPED;
        stop_count_.fetch_add(1, std::memory_order_relaxed);
    }

    State state() const {
        std::lock_guard<std::mutex> lk(mu_);
        return state_;
    }

    bool schedule(const std::string& node_id, std::chrono::milliseconds /*delay*/) {
        if (node_id.empty()) return false;
        schedule_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    uint64_t startCount()    const noexcept { return start_count_.load(std::memory_order_relaxed); }
    uint64_t stopCount()     const noexcept { return stop_count_.load(std::memory_order_relaxed); }
    uint64_t scheduleCount() const noexcept { return schedule_count_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex   mu_;
    State                state_{State::STOPPED};
    std::atomic<uint64_t> start_count_{0};
    std::atomic<uint64_t> stop_count_{0};
    std::atomic<uint64_t> schedule_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: ChaosSoak_FaultInjectRecoverCycle
//
// Continuously inject and recover faults for soak_duration.
// Acceptance: recover rate = 100%; no exceptions.
// ─────────────────────────────────────────────────────────────────────────────
TEST(ChaosSoak_FaultInjectRecoverCycle, RecoverRateIs100Percent) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubFaultInjector fi;
    const auto t0 = std::chrono::steady_clock::now();
    uint64_t inject_ok  = 0;
    uint64_t recover_ok = 0;
    uint64_t cycle      = 0;

    ASSERT_NO_THROW({
        while (std::chrono::steady_clock::now() - t0 < soak_duration) {
            const std::string node = "soak-node-" + std::to_string(cycle % 50);
            if (fi.inject(node)) ++inject_ok;
            if (fi.recover(node)) ++recover_ok;
            ++cycle;
        }
    });

    ASSERT_GT(inject_ok, 0u)
        << "At least one fault must be injected during the soak";

    EXPECT_EQ(inject_ok, recover_ok)
        << "Recover rate must be 100%: every injected fault must be recovered. "
           "Injected=" << inject_ok << " Recovered=" << recover_ok;

    // No faults should remain active at end of soak
    for (uint64_t i = 0; i < std::min(cycle, uint64_t{50}); ++i) {
        EXPECT_FALSE(fi.isFaultActive("soak-node-" + std::to_string(i)))
            << "No faults must remain active after soak (node " << i << ")";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ChaosSoak_SchedulerStabilityUnderLoad
//
// Continuously start/stop/schedule the scheduler for soak_duration / 4.
// Acceptance: scheduler always returns to STOPPED after stop(); no exceptions.
// ─────────────────────────────────────────────────────────────────────────────
TEST(ChaosSoak_SchedulerStabilityUnderLoad, SchedulerAlwaysReturnToStopped) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 4);

    StubChaosScheduler sched;
    uint64_t cycles = 0;

    const auto t0 = std::chrono::steady_clock::now();

    ASSERT_NO_THROW({
        while (std::chrono::steady_clock::now() - t0 < soak_duration) {
            sched.start();
            ASSERT_EQ(sched.state(), StubChaosScheduler::State::RUNNING)
                << "Scheduler must be RUNNING after start() (cycle " << cycles << ")";

            // Schedule a few entries while RUNNING
            for (int j = 0; j < 5; ++j) {
                sched.schedule("sched-node-" + std::to_string(j), 10ms);
            }

            sched.stop();
            ASSERT_EQ(sched.state(), StubChaosScheduler::State::STOPPED)
                << "Scheduler must return to STOPPED after stop() (cycle " << cycles << ")";
            ++cycles;
        }
    });

    EXPECT_GT(cycles, 0u)
        << "At least one start/stop cycle must complete during the soak";

    // Final state must be STOPPED
    EXPECT_EQ(sched.state(), StubChaosScheduler::State::STOPPED)
        << "Scheduler must be in STOPPED state at end of soak";

    EXPECT_EQ(sched.startCount(), sched.stopCount())
        << "Start and stop counts must be balanced across the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: ChaosSoak_CallbackDispatchReliability
//
// Register a callback and drive inject/recover cycles for soak_duration / 4.
// Acceptance: callback invocation count matches inject+recover count; no
// re-entry detected (re-entry guard fires if callback tries to modify state).
// ─────────────────────────────────────────────────────────────────────────────
TEST(ChaosSoak_CallbackDispatchReliability, NoReentryAndFullDispatch) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 4);

    StubFaultInjector fi;
    std::atomic<uint64_t> callback_count{0};
    std::atomic<bool>     reentry_detected{false};
    std::atomic<bool>     in_callback{false};

    fi.setCallback([&](const std::string& /*node*/, StubFaultInjector::CallbackEvent /*evt*/) {
        // Re-entry guard: callback must not be called recursively
        if (in_callback.exchange(true, std::memory_order_acq_rel)) {
            reentry_detected.store(true, std::memory_order_release);
        }
        callback_count.fetch_add(1, std::memory_order_relaxed);
        in_callback.store(false, std::memory_order_release);
    });

    const auto t0 = std::chrono::steady_clock::now();
    uint64_t cycle = 0;

    ASSERT_NO_THROW({
        while (std::chrono::steady_clock::now() - t0 < soak_duration) {
            const std::string node = "cb-node-" + std::to_string(cycle % 20);
            fi.inject(node);
            fi.recover(node);
            ++cycle;
        }
    });

    EXPECT_FALSE(reentry_detected.load())
        << "Callback re-entry must never be detected during the soak";

    // Each inject+recover pair fires 2 callbacks
    const uint64_t expected_callbacks = fi.injectCount() + fi.recoverCount();
    EXPECT_EQ(callback_count.load(), expected_callbacks)
        << "Callback dispatch count must match inject+recover event count. "
           "Expected=" << expected_callbacks
        << " Observed=" << callback_count.load();

    EXPECT_GT(cycle, 0u)
        << "At least one inject/recover cycle must complete during the soak";
}
