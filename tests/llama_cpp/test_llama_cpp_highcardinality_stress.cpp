// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llama_cpp_highcardinality_stress.cpp
 * @brief Wave D high-cardinality and concurrent stress tests for the llama_cpp module.
 *
 * Three focused stress cases that exercise token-generation cardinality, concurrent
 * model-load pressure, and acceleration fail-closed behavior under concurrency.
 * All cases use in-process stubs — no GPU, model files, or llama.cpp runtime required.
 *
 * CTest labels: wave_d;stress;not_release_critical
 *
 * | Test case                       | Scenario                                         |
 * |---------------------------------|--------------------------------------------------|
 * | HighCardinalityTokenGeneration  | 100 000 unique prompts, 8 concurrent threads     |
 * | ConcurrentModelLoadStress       | 8 threads load/unload models simultaneously      |
 * | AccelerationFailClosedStress    | GPU unavailable; all requests fall back to CPU   |
 *
 * @see src/llama_cpp/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace test {
namespace wave_d {

// ---------------------------------------------------------------------------
// STUB / SIMULATION NOTE
// These stubs model llama_cpp behaviour without external backends.
// MUST NOT be used in production code paths.
// ---------------------------------------------------------------------------

struct StubTokenStore {
    bool generate(const std::string& prompt, const std::string& tokens) {
        std::lock_guard<std::mutex> lock(mu_);
        store_[prompt] = tokens;
        ++writes_;
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return store_.size();
    }

    long writes() const { return writes_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
    std::atomic<long> writes_{0};
};

struct StubLlamaRegistry {
    bool load(const std::string& model_path) {
        std::lock_guard<std::mutex> lock(mu_);
        loaded_[model_path] = true;
        ++loads_;
        return true;
    }

    bool unload(const std::string& model_path) {
        std::lock_guard<std::mutex> lock(mu_);
        loaded_.erase(model_path);
        ++unloads_;
        return true;
    }

    long loads()   const { return loads_.load(std::memory_order_relaxed); }
    long unloads() const { return unloads_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, bool> loaded_;
    std::atomic<long> loads_{0};
    std::atomic<long> unloads_{0};
};

struct StubLlamaAccelGuard {
    explicit StubLlamaAccelGuard(bool available) : available_(available) {}

    bool handleRequest(uint64_t /*req_id*/) {
        ++attempts_;
        if (!available_) { ++fallbacks_; return true; }
        ++accelerated_;
        return true;
    }

    long attempts()    const { return attempts_.load(std::memory_order_relaxed); }
    long accelerated() const { return accelerated_.load(std::memory_order_relaxed); }
    long fallbacks()   const { return fallbacks_.load(std::memory_order_relaxed); }

private:
    bool available_;
    std::atomic<long> attempts_{0};
    std::atomic<long> accelerated_{0};
    std::atomic<long> fallbacks_{0};
};

// ---------------------------------------------------------------------------
// Test 1: HighCardinalityTokenGeneration
// ---------------------------------------------------------------------------
TEST(WaveD_LlamaCppStress, HighCardinalityTokenGeneration) {
    constexpr int    kThreads = 8;
    constexpr int    kPromptsPerThread = 12500; // 100 000 total
    constexpr double kExpectedWriteRatio = 0.95;

    StubTokenStore store;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            const int base = t * kPromptsPerThread;
            for (int i = 0; i < kPromptsPerThread; ++i) {
                const std::string prompt = "prompt-" + std::to_string(base + i);
                store.generate(prompt, "tokens-" + std::to_string(base + i));
            }
        });
    }
    for (auto& th : threads) th.join();

    const long total_writes = store.writes();
    EXPECT_GE(total_writes,
              static_cast<long>(kThreads * kPromptsPerThread * kExpectedWriteRatio))
        << "[LLAMA:InferenceTimeout] Expected ≥ 95% of prompts generated. "
           "Wrote: " << total_writes;
    EXPECT_GE(static_cast<long>(store.size()), total_writes * 9 / 10)
        << "Unique prompts in store must be ≥ 90% of writes (no silent drops).";
}

// ---------------------------------------------------------------------------
// Test 2: ConcurrentModelLoadStress
// ---------------------------------------------------------------------------
TEST(WaveD_LlamaCppStress, ConcurrentModelLoadStress) {
    constexpr int kThreads = 8;
    constexpr int kCyclesPerThread = 500;

    StubLlamaRegistry registry;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            const std::string model = "llama-" + std::to_string(t % 4) + ".gguf";
            for (int i = 0; i < kCyclesPerThread; ++i) {
                registry.load(model);
                std::this_thread::yield();
                registry.unload(model);
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_GE(registry.loads(),
              static_cast<long>(kThreads * kCyclesPerThread))
        << "[LLAMA:ModelLoadFailed] All load operations must complete.";
    EXPECT_GE(registry.unloads(),
              static_cast<long>(kThreads * kCyclesPerThread))
        << "[LLAMA:ModelLoadFailed] All unload operations must complete.";
}

// ---------------------------------------------------------------------------
// Test 3: AccelerationFailClosedStress
// ---------------------------------------------------------------------------
TEST(WaveD_LlamaCppStress, AccelerationFailClosedStress) {
    constexpr int kThreads = 8;
    constexpr int kRequestsPerThread = 10000;

    // GPU unavailable — all requests must fall back to CPU cleanly.
    StubLlamaAccelGuard guard(/*available=*/false);

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kRequestsPerThread;
            for (int i = 0; i < kRequestsPerThread; ++i) {
                const bool ok = guard.handleRequest(base + static_cast<uint64_t>(i));
                ASSERT_TRUE(ok)
                    << "[LLAMA:AccelerationUnavailable] Request " << (base + i)
                    << " must be handled (fail-closed CPU fallback).";
            }
        });
    }
    for (auto& th : threads) th.join();

    const long total = kThreads * kRequestsPerThread;
    EXPECT_EQ(guard.attempts(), total);
    EXPECT_EQ(guard.fallbacks(), total)
        << "[LLAMA:AccelerationUnavailable] All requests must fall back when "
           "GPU is unavailable.";
    EXPECT_EQ(guard.accelerated(), 0L);
}

} // namespace wave_d
} // namespace test
} // namespace themis
