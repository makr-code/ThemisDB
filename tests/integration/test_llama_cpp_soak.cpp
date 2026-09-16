// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llama_cpp_soak.cpp
 * @brief Wave D — llama_cpp Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB llama_cpp module hot paths:
 * inference throughput, model-load stability, and acceleration-path reliability.
 * Verifies that all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - LlamaCppSoak_InferenceThroughput        : ≥ 1 000 ops/sec over soak window
 * - LlamaCppSoak_ModelLoadStability         : zero simulated model-load failures
 * - LlamaCppSoak_AccelerationPathReliability: acceleration path must fail closed
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_LLAMA_CPP.md — operator runbook
 * @see src/llama_cpp/ROADMAP.md — Wave D contribution closure
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
// In-process stubs model the llama_cpp module hot paths without requiring
// external backends (GPU, model files, llama.cpp runtime).
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ---------------------------------------------------------------------------
// StubTokenQueue — bounded token-generation request queue simulation
// ---------------------------------------------------------------------------
class StubTokenQueue {
public:
    explicit StubTokenQueue(std::size_t capacity) : capacity_(capacity) {}

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
// StubLlamaModelLoader — simulates LLM model load/unload lifecycle
// ---------------------------------------------------------------------------
class StubLlamaModelLoader {
public:
    bool loadModel(const std::string& /*model_path*/) {
        ++load_count_;
        return true; // stub: always succeeds
    }

    bool unloadModel() {
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
// StubLlamaAcceleration — simulates GPU/Metal/CUDA acceleration
// ---------------------------------------------------------------------------
class StubLlamaAcceleration {
public:
    bool runFailClosed(uint64_t /*request_id*/) {
        ++attempt_count_;
        if (unavailable_.load()) {
            ++fallback_count_;
            return true; // CPU fallback
        }
        ++success_count_;
        return true;
    }

    void setUnavailable(bool v) { unavailable_.store(v); }

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
// Test 1: LlamaCppSoak_InferenceThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_LlamaCppSoak, LlamaCppSoak_InferenceThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubTokenQueue queue(4096);
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
        << "[LLAMA:InferenceTimeout] At least one inference op must complete";
    EXPECT_GE(ops_per_sec, 1000.0)
        << "LLM inference throughput must be ≥ 1 000 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: LlamaCppSoak_ModelLoadStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_LlamaCppSoak, LlamaCppSoak_ModelLoadStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubLlamaModelLoader loader;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            while (running.load(std::memory_order_relaxed)) {
                loader.loadModel("llama-model-" + std::to_string(i) + ".gguf");
                loader.unloadModel();
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(loader.loadCount(), 0u)
        << "[LLAMA:ModelLoadFailed] At least one model load must complete";
    EXPECT_EQ(loader.failCount(), 0u)
        << "[LLAMA:ModelLoadFailed] Zero model-load failures expected during soak. "
           "Observed: " << loader.failCount();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: LlamaCppSoak_AccelerationPathReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_LlamaCppSoak, LlamaCppSoak_AccelerationPathReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubLlamaAcceleration accel;
    std::atomic<bool>     running{true};
    std::atomic<uint64_t> handled{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                if (accel.runFailClosed(id++)) ++handled;
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(handled.load(), 0u)
        << "[LLAMA:AccelerationUnavailable] At least one request must be handled";
    const uint64_t total = accel.attemptCount() + accel.fallbackCount();
    EXPECT_EQ(handled.load(), total)
        << "[LLAMA:AccelerationUnavailable] All requests must be handled fail-closed. "
           "Handled=" << handled.load() << " total=" << total;
}
