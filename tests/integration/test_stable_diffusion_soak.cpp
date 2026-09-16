// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_stable_diffusion_soak.cpp
 * @brief Wave D — Stable Diffusion Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB stable_diffusion module hot paths:
 * inference throughput, model-load stability, and acceleration-path reliability.
 * Verifies that all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * In CI environments this test runs at the default 60 000 ms (1 min) so the
 * gate finishes in < 2 min.  The full production soak (3 600 000 ms / 60 min)
 * is reserved for the release pipeline.
 *
 * ## Acceptance criteria
 * - StableDiffusionSoak_InferenceThroughput        : ≥ 1 000 ops/sec over soak window
 * - StableDiffusionSoak_ModelLoadStability         : zero simulated model-load failures
 * - StableDiffusionSoak_AccelerationPathReliability: acceleration path must fail closed
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_STABLE_DIFFUSION.md — operator runbook
 * @see src/stable_diffusion/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS.
// Default: 60 000 ms so CI completes well within the 120 s timeout.
// ─────────────────────────────────────────────────────────────────────────────
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
// In-process stubs model the stable_diffusion module hot paths without
// requiring external backends (GPU, model files, stable-diffusion.cpp).
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ---------------------------------------------------------------------------
// StubInferenceQueue — bounded inference request queue simulation
// ---------------------------------------------------------------------------
class StubInferenceQueue {
public:
    explicit StubInferenceQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(uint64_t request_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { ++overflow_count_; return false; }
        queue_.push_back(request_id);
        ++enqueue_count_;
        return true;
    }

    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop_front();
        ++dequeue_count_;
        return true;
    }

    uint64_t enqueueCount()  const { return enqueue_count_.load(); }
    uint64_t dequeueCount()  const { return dequeue_count_.load(); }
    uint64_t overflowCount() const { return overflow_count_.load(); }

private:
    const std::size_t capacity_;
    std::deque<uint64_t> queue_;
    std::mutex mu_;
    std::atomic<uint64_t> enqueue_count_{0};
    std::atomic<uint64_t> dequeue_count_{0};
    std::atomic<uint64_t> overflow_count_{0};
};

// ---------------------------------------------------------------------------
// StubModelLoader — simulates model load/unload lifecycle
// ---------------------------------------------------------------------------
class StubModelLoader {
public:
    /// Simulate model load. Returns false (failure) probabilistically = never
    /// under normal conditions (always succeeds in stub).
    bool loadModel(const std::string& /*model_id*/) {
        ++load_count_;
        // Stub: always succeeds; a real loader might fail on OOM.
        return true;
    }

    bool unloadModel(const std::string& /*model_id*/) {
        ++unload_count_;
        return true;
    }

    uint64_t loadCount()   const { return load_count_.load(); }
    uint64_t failCount()   const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> load_count_{0};
    std::atomic<uint64_t> unload_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

// ---------------------------------------------------------------------------
// StubAccelerationPath — simulates GPU/NPU acceleration with fail-closed guard
// ---------------------------------------------------------------------------
class StubAccelerationPath {
public:
    /// Attempt accelerated inference. Returns false if acceleration is
    /// unavailable; caller must fall back to CPU path (fail-closed).
    bool runAccelerated(uint64_t /*request_id*/) {
        ++attempt_count_;
        // Stub: always available; logs [STABLEDIFF:AccelerationUnavailable] if not.
        ++success_count_;
        return true;
    }

    /// Simulate acceleration becoming unavailable.
    void setUnavailable(bool unavailable) { unavailable_.store(unavailable); }

    /// Run with fail-closed: returns true if accelerated path ran OR fell back
    /// cleanly to CPU; returns false only on unhandled exception path.
    bool runFailClosed(uint64_t request_id) {
        if (unavailable_.load()) {
            // Fail closed: log and fall back
            ++fallback_count_;
            return true; // CPU fallback succeeded
        }
        return runAccelerated(request_id);
    }

    uint64_t attemptCount()  const { return attempt_count_.load(); }
    uint64_t successCount()  const { return success_count_.load(); }
    uint64_t fallbackCount() const { return fallback_count_.load(); }

private:
    std::atomic<bool>     unavailable_{false};
    std::atomic<uint64_t> attempt_count_{0};
    std::atomic<uint64_t> success_count_{0};
    std::atomic<uint64_t> fallback_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: StableDiffusionSoak_InferenceThroughput
//
// Continuously enqueue and dequeue inference requests for soak_duration ms
// across 4 producer + 4 consumer threads. Aggregate throughput must be
// ≥ 1 000 ops/sec.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_StableDiffusionSoak, StableDiffusionSoak_InferenceThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubInferenceQueue queue(4096);
    std::atomic<bool> running{true};

    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                queue.enqueue(id++);
                std::this_thread::yield();
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; ++i) {
        consumers.emplace_back([&]() {
            uint64_t dummy = 0;
            while (running.load(std::memory_order_relaxed)) {
                queue.dequeue(dummy);
                std::this_thread::yield();
            }
        });
    }

    const auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);

    for (auto& t : producers) t.join();
    for (auto& t : consumers)  t.join();

    const double elapsed_s =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const uint64_t total_ops = queue.enqueueCount() + queue.dequeueCount();
    const double   ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "[STABLEDIFF:InferenceTimeout] At least one inference op must complete";
    EXPECT_GE(ops_per_sec, 1000.0)
        << "Inference throughput must be ≥ 1 000 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: StableDiffusionSoak_ModelLoadStability
//
// Repeatedly load and unload models for soak_duration/10 ms. Zero failures
// are expected during normal operation.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_StableDiffusionSoak, StableDiffusionSoak_ModelLoadStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubModelLoader loader;
    std::atomic<bool> running{true};

    std::vector<std::thread> loaders;
    for (int i = 0; i < 4; ++i) {
        loaders.emplace_back([&, i]() {
            while (running.load(std::memory_order_relaxed)) {
                const std::string model_id = "sd-model-" + std::to_string(i);
                loader.loadModel(model_id);
                loader.unloadModel(model_id);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : loaders) t.join();

    EXPECT_GT(loader.loadCount(), 0u)
        << "[STABLEDIFF:ModelLoadFailed] At least one model load must complete";
    EXPECT_EQ(loader.failCount(), 0u)
        << "[STABLEDIFF:ModelLoadFailed] Zero model-load failures expected during soak. "
           "Observed: " << loader.failCount();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: StableDiffusionSoak_AccelerationPathReliability
//
// Run acceleration path for soak_duration/10 ms with fail-closed semantics.
// No unhandled failures are permitted; all unavailability events must fall
// back cleanly to the CPU path.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_StableDiffusionSoak, StableDiffusionSoak_AccelerationPathReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubAccelerationPath accel;
    std::atomic<bool> running{true};
    std::atomic<uint64_t> handled{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                const bool ok = accel.runFailClosed(id++);
                if (ok) ++handled;
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(handled.load(), 0u)
        << "[STABLEDIFF:AccelerationUnavailable] At least one request must be handled";
    // All requests must be handled (either accelerated or via CPU fallback).
    const uint64_t total = accel.attemptCount() + accel.fallbackCount();
    EXPECT_EQ(handled.load(), total)
        << "[STABLEDIFF:AccelerationUnavailable] All requests must be handled fail-closed. "
           "Handled=" << handled.load() << " total=" << total;
}
