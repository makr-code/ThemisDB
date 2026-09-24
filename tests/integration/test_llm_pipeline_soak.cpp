// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_pipeline_soak.cpp
 * @brief Wave D — LLM Pipeline Soak Tests (sustained inference workload).
 *
 * Three soak cases that validate LLM model routing throughput, adapter
 * lifecycle stability, and fallback reliability over a configurable soak
 * window using in-process stubs.
 *
 * ## Acceptance criteria
 * - LLMSoak_ModelRoutingThroughput   : routing completes without stalls
 * - LLMSoak_AdapterLifecycleStability: no leaked adapters over soak window
 * - LLMSoak_FallbackReliability      : all fallbacks resolve to a valid model
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_LLM_PIPELINE.md
 * @see src/llm/ROADMAP.md — Wave D contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ---------------------------------------------------------------------------
// In-process LLM stubs
// MUST NOT be used in production code paths.
// ---------------------------------------------------------------------------

enum class StubModelTier { LARGE, MEDIUM, SMALL, FALLBACK };

/// Stub router — selects model tier based on token count policy.
class StubModelRouter {
public:
    StubModelTier route(uint32_t token_count) const noexcept {
        if (token_count > 2048) { return StubModelTier::LARGE; }
        if (token_count > 512)  { return StubModelTier::MEDIUM; }
        if (token_count > 64)   { return StubModelTier::SMALL; }
        return StubModelTier::FALLBACK;
    }
};

/// Stub adapter — represents a LoRA adapter lifecycle.
class StubAdapter {
public:
    explicit StubAdapter(uint32_t id) : id_(id), loaded_(true) {}

    void unload() noexcept { loaded_ = false; }
    bool isLoaded() const noexcept { return loaded_; }
    uint32_t id() const noexcept { return id_; }

private:
    uint32_t id_;
    bool     loaded_;
};

/// Adapter registry — tracks all live adapters to detect leaks.
class StubAdapterRegistry {
public:
    std::shared_ptr<StubAdapter> load(uint32_t id) {
        std::lock_guard<std::mutex> lk(mu_);
        auto a = std::make_shared<StubAdapter>(id);
        live_[id] = a;
        peak_live_ = std::max(peak_live_, live_.size());
        return a;
    }

    void unload(uint32_t id) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = live_.find(id);
        if (it != live_.end()) {
            it->second->unload();
            live_.erase(it);
        }
    }

    std::size_t liveCount() {
        std::lock_guard<std::mutex> lk(mu_);
        return live_.size();
    }

    std::size_t peakLive() const noexcept { return peak_live_; }

private:
    std::mutex                                          mu_;
    std::unordered_map<uint32_t, std::shared_ptr<StubAdapter>> live_;
    std::size_t                                         peak_live_{0};
};

// ---------------------------------------------------------------------------
// LLMSoak_ModelRoutingThroughput
// ---------------------------------------------------------------------------
TEST(LLMSoak, LLMSoak_ModelRoutingThroughput) {
    constexpr uint32_t kWorkerCount = 4;

    StubModelRouter       router;
    std::atomic<uint64_t> total_routes{0};
    std::atomic<uint64_t> routing_errors{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    auto worker = [&](uint32_t seed) {
        uint32_t token_count = (seed % 4096) + 1;
        while (std::chrono::steady_clock::now() < end_time) {
            for (int i = 0; i < 128; ++i) {
                const StubModelTier tier = router.route(token_count);
                if (static_cast<int>(tier) < 0 ||
                    static_cast<int>(tier) > static_cast<int>(StubModelTier::FALLBACK)) {
                    routing_errors.fetch_add(1, std::memory_order_relaxed);
                }
                token_count = (token_count * 6364136223846793005ULL + 1) % 4096 + 1;
            }
            total_routes.fetch_add(128, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kWorkerCount);
    for (uint32_t i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back(worker, i * 997);
    }
    for (auto& t : workers) t.join();

    EXPECT_GT(total_routes.load(), 0ULL) << "No routing calls completed";
    EXPECT_EQ(routing_errors.load(), 0ULL)
        << "LLMSoak_ModelRoutingThroughput: " << routing_errors.load()
        << " routing errors detected";
}

// ---------------------------------------------------------------------------
// LLMSoak_AdapterLifecycleStability
// ---------------------------------------------------------------------------
TEST(LLMSoak, LLMSoak_AdapterLifecycleStability) {
    constexpr uint32_t kAdapterPoolSize = 16;
    constexpr uint32_t kWorkerCount     = 4;

    StubAdapterRegistry    registry;
    std::atomic<uint64_t>  load_cycles{0};
    std::atomic<uint32_t>  next_adapter_id{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    std::atomic<uint64_t> adapter_failures{0};

    auto worker = [&](uint32_t /*seed*/) {
        while (std::chrono::steady_clock::now() < end_time) {
            const uint32_t adapter_id = next_adapter_id.fetch_add(1, std::memory_order_relaxed) % 1'000'000U;
            auto adapter = registry.load(adapter_id);
            if (adapter == nullptr || !adapter->isLoaded()) {
                adapter_failures.fetch_add(1, std::memory_order_relaxed);
                return;
            }
            registry.unload(adapter_id);
            load_cycles.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kWorkerCount);
    for (uint32_t i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_GT(load_cycles.load(), 0ULL) << "No adapter load cycles completed";
    EXPECT_EQ(adapter_failures.load(), 0ULL)
        << "LLMSoak_AdapterLifecycleStability: adapter load/ready failed "
        << adapter_failures.load() << " times in worker threads";
    // All adapters must be unloaded by the end of the soak — no leaks.
    EXPECT_EQ(registry.liveCount(), 0ULL)
        << "LLMSoak_AdapterLifecycleStability: " << registry.liveCount()
        << " adapters still live at soak end (possible leak)";
}

// ---------------------------------------------------------------------------
// LLMSoak_FallbackReliability
// ---------------------------------------------------------------------------
TEST(LLMSoak, LLMSoak_FallbackReliability) {
    StubModelRouter       router;
    std::atomic<uint64_t> fallback_resolutions{0};
    std::atomic<uint64_t> unresolved_fallbacks{0};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    // Simulate requests that intentionally hit the fallback tier (≤ 64 tokens).
    uint32_t token_count = 1;
    while (std::chrono::steady_clock::now() < end_time) {
        for (int i = 0; i < 256; ++i) {
            const StubModelTier tier = router.route(token_count % 64 + 1);
            if (tier == StubModelTier::FALLBACK || tier == StubModelTier::SMALL) {
                fallback_resolutions.fetch_add(1, std::memory_order_relaxed);
            } else {
                // With token_count clamped to ≤ 64, we should never reach LARGE/MEDIUM.
                unresolved_fallbacks.fetch_add(1, std::memory_order_relaxed);
            }
            ++token_count;
        }
    }

    EXPECT_GT(fallback_resolutions.load(), 0ULL) << "No fallback resolutions recorded";
    EXPECT_EQ(unresolved_fallbacks.load(), 0ULL)
        << "LLMSoak_FallbackReliability: " << unresolved_fallbacks.load()
        << " requests routed to wrong tier under fallback conditions";
}
