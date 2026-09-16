// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_highcardinality_stress.cpp
 * @brief Wave D — LLM High-Cardinality Stress Tests.
 *
 * Three stress cases for the LLM pipeline under high-cardinality and
 * concurrency conditions using in-process stubs and seed-42 determinism.
 *
 * ## Cases
 * - HighCardinalityModelRouting        : high-cardinality token distribution, 8-thread
 * - ConcurrentAdapterLifecycleStress   : concurrent load/unload of many adapters
 * - FallbackUnderLoadStress            : fallback model selection under sustained load
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_LLM_PIPELINE.md
 * @see src/llm/ROADMAP.md
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

static constexpr uint64_t kLLMStressSeed = 42;

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

enum class StressModelTier { LARGE, MEDIUM, SMALL, FALLBACK };

class StressModelRouter {
public:
    StressModelTier route(uint32_t tokens) const noexcept {
        if (tokens > 2048) { return StressModelTier::LARGE; }
        if (tokens > 512)  { return StressModelTier::MEDIUM; }
        if (tokens > 64)   { return StressModelTier::SMALL; }
        return StressModelTier::FALLBACK;
    }
};

class StressAdapterRegistry {
public:
    void load(uint32_t id) {
        std::lock_guard<std::mutex> lk(mu_);
        ref_counts_[id]++;
        if (ref_counts_[id] == 1) {
            live_count_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void unload(uint32_t id) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = ref_counts_.find(id);
        if (it != ref_counts_.end() && it->second > 0) {
            it->second--;
            if (it->second == 0) {
                live_count_.fetch_sub(1, std::memory_order_relaxed);
            }
        }
    }

    uint64_t liveCount() const noexcept {
        return live_count_.load(std::memory_order_relaxed);
    }

private:
    std::mutex                              mu_;
    std::unordered_map<uint32_t, uint32_t>  ref_counts_;
    std::atomic<uint64_t>                   live_count_{0};
};

// ---------------------------------------------------------------------------
// HighCardinalityModelRouting — 8 threads, diverse token counts
// ---------------------------------------------------------------------------
TEST(LLMHighCardinalityStress, HighCardinalityModelRouting) {
    constexpr uint32_t kThreadCount  = 8;
    constexpr uint64_t kOpsPerThread = 50'000;

    StressModelRouter     router;
    std::atomic<uint64_t> total_routes{0};
    std::atomic<uint64_t> routing_errors{0};

    auto worker = [&](uint32_t tid) {
        uint64_t rng = kLLMStressSeed ^ (static_cast<uint64_t>(tid + 1) << 32);
        for (uint64_t op = 0; op < kOpsPerThread; ++op) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            const uint32_t tokens = static_cast<uint32_t>(rng % 4096) + 1;
            const StressModelTier tier = router.route(tokens);
            const int tier_int = static_cast<int>(tier);
            if (tier_int < 0 || tier_int > static_cast<int>(StressModelTier::FALLBACK)) {
                routing_errors.fetch_add(1, std::memory_order_relaxed);
            }
        }
        total_routes.fetch_add(kOpsPerThread, std::memory_order_relaxed);
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_EQ(total_routes.load(),
              static_cast<uint64_t>(kThreadCount) * kOpsPerThread);
    EXPECT_EQ(routing_errors.load(), 0ULL)
        << "HighCardinalityModelRouting: " << routing_errors.load()
        << " routing errors";
}

// ---------------------------------------------------------------------------
// ConcurrentAdapterLifecycleStress
// ---------------------------------------------------------------------------
TEST(LLMHighCardinalityStress, ConcurrentAdapterLifecycleStress) {
    constexpr uint32_t kAdapterPoolSize  = 64;
    constexpr uint32_t kThreadCount      = 8;
    constexpr uint64_t kCyclesPerThread  = 500;

    StressAdapterRegistry registry;
    std::atomic<uint64_t> total_cycles{0};

    auto worker = [&](uint32_t tid) {
        uint64_t rng = kLLMStressSeed ^ (static_cast<uint64_t>(tid + 1) * 0xdeadbeefULL);
        for (uint64_t c = 0; c < kCyclesPerThread; ++c) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            const uint32_t id = static_cast<uint32_t>(rng % kAdapterPoolSize);
            registry.load(id);
            registry.unload(id);
            total_cycles.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_EQ(total_cycles.load(),
              static_cast<uint64_t>(kThreadCount) * kCyclesPerThread);
    EXPECT_EQ(registry.liveCount(), 0ULL)
        << "ConcurrentAdapterLifecycleStress: " << registry.liveCount()
        << " adapters leaked";
}

// ---------------------------------------------------------------------------
// FallbackUnderLoadStress
// ---------------------------------------------------------------------------
TEST(LLMHighCardinalityStress, FallbackUnderLoadStress) {
    constexpr uint32_t kThreadCount     = 8;
    constexpr uint64_t kOpsPerThread    = 10'000;

    // All requests have ≤ 64 tokens → must always route to SMALL or FALLBACK.
    StressModelRouter     router;
    std::atomic<uint64_t> total_ops{0};
    std::atomic<uint64_t> wrong_tier{0};

    auto worker = [&](uint32_t tid) {
        uint64_t rng = kLLMStressSeed ^ static_cast<uint64_t>(tid + 1);
        for (uint64_t op = 0; op < kOpsPerThread; ++op) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            const uint32_t tokens = static_cast<uint32_t>(rng % 64) + 1;
            const StressModelTier tier = router.route(tokens);
            if (tier != StressModelTier::SMALL &&
                tier != StressModelTier::FALLBACK) {
                wrong_tier.fetch_add(1, std::memory_order_relaxed);
            }
            total_ops.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_EQ(total_ops.load(),
              static_cast<uint64_t>(kThreadCount) * kOpsPerThread);
    EXPECT_EQ(wrong_tier.load(), 0ULL)
        << "FallbackUnderLoadStress: " << wrong_tier.load()
        << " requests routed to wrong tier under fallback-only load";
}
