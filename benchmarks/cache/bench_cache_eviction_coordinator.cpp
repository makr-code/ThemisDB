/**
 * @file bench_cache_eviction_coordinator.cpp
 * @brief Phase 3 latency benchmarks: cache→coordinator→storage integration.
 * @version 1.0.0
 * 
 * Benchmarks for:
 * - L1→L2 promotion latency (gate: ≤50µs)
 * - Cache eviction→storage feedback latency (gate: ≤100µs)
 * - Capacity pressure detection overhead
 * 
 * @see include/cache/eviction_listener.h
 * @see src/cache/ROADMAP.md Phase 3
 * @see benchmarks/MEASUREMENT_HYGIENE.md
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

#include "cache/eviction_listener.h"

namespace themis {
namespace cache {

/**
 * @brief High-performance test listener for benchmarking.
 */
class BenchListener : public IEvictionListener {
public:
    std::atomic<std::size_t> event_count{0};
    std::atomic<uint64_t> total_access_count{0};

    void onCacheEvicted(const CacheEvictionEvent& event) override {
        event_count.fetch_add(1, std::memory_order_relaxed);
        total_access_count.fetch_add(event.access_count, std::memory_order_relaxed);
    }

    void onCapacityPressure(TierLevel, uint32_t, std::size_t) override {
        // No-op for latency measurement
    }
};

/**
 * @brief GATE-CAP-01: Single eviction event emission latency.
 * 
 * Measures time to emit one eviction event to a single listener.
 * Gate: ≤10µs (single event, single listener)
 */
static void BenchEvictionEventEmissionSingleListener(benchmark::State& state) {
    auto listener_manager = createEvictionListenerManager();
    auto listener = std::make_shared<BenchListener>();
    listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "bench_key";
    event.from_tier = TierLevel::L1;
    event.access_count = 5;
    event.size_bytes = 1024;

    for (auto _ : state) {
        benchmark::DoNotOptimize(event);
        listener_manager->emitEvictionEvent(event);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(1);
}
BENCHMARK(BenchEvictionEventEmissionSingleListener)->UseRealTime();

/**
 * @brief GATE-CAP-02: Multiple listener event delivery.
 * 
 * Measures event emission to N listeners in sequence.
 * Gate: ≤50µs for 10 listeners
 */
static void BenchEvictionEventEmissionMultipleListeners(benchmark::State& state) {
    auto listener_manager = createEvictionListenerManager();
    
    // Register N listeners
    for (int i = 0; i < state.range(0); i++) {
        listener_manager->registerListener(std::make_shared<BenchListener>());
    }

    CacheEvictionEvent event;
    event.key = "bench_key";
    event.from_tier = TierLevel::L1;
    event.access_count = 5;

    for (auto _ : state) {
        benchmark::DoNotOptimize(event);
        listener_manager->emitEvictionEvent(event);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(state.range(0));
}
BENCHMARK(BenchEvictionEventEmissionMultipleListeners)
    ->UseRealTime()
    ->Arg(1)
    ->Arg(5)
    ->Arg(10);

/**
 * @brief GATE-CAP-03: Capacity pressure notification latency.
 * 
 * Measures time to notify listeners of capacity pressure.
 * Gate: ≤30µs
 */
static void BenchCapacityPressureNotification(benchmark::State& state) {
    auto listener_manager = createEvictionListenerManager();
    auto listener = std::make_shared<BenchListener>();
    listener_manager->registerListener(listener);

    for (auto _ : state) {
        listener_manager->emitCapacityPressure(TierLevel::L1, 85, 10);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(1);
}
BENCHMARK(BenchCapacityPressureNotification)->UseRealTime();

/**
 * @brief GATE-CAP-04: Listener registration and unregistration.
 * 
 * Measures overhead of dynamic listener management.
 * Gate: ≤5µs per operation
 */
static void BenchListenerRegistration(benchmark::State& state) {
    auto listener_manager = createEvictionListenerManager();
    auto listener = std::make_shared<BenchListener>();

    for (auto _ : state) {
        uint64_t handle = listener_manager->registerListener(listener);
        benchmark::DoNotOptimize(handle);
        listener_manager->unregisterListener(handle);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(1);
}
BENCHMARK(BenchListenerRegistration)->UseRealTime();

/**
 * @brief GATE-CAP-05: L1→L2 promotion decision latency (via high-access signal).
 * 
 * Simulates detecting high-access eviction and making promotion decision.
 * Gate: ≤50µs for event analysis + decision
 */
static void BenchPromotionDecisionLatency(benchmark::State& state) {
    struct DecisionListener : public IEvictionListener {
    public:
        std::atomic<bool> should_promote{false};

        void onCacheEvicted(const CacheEvictionEvent& event) override {
            // Gate-CAP-05: Promotion decision logic
            if (event.access_count >= 10) {
                should_promote.store(true, std::memory_order_relaxed);
            }
        }
    };

    auto listener_manager = createEvictionListenerManager();
    auto listener = std::make_shared<DecisionListener>();
    listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "promotion_candidate";
    event.from_tier = TierLevel::L2;
    event.access_count = 15;  // High access count triggers promotion

    for (auto _ : state) {
        benchmark::DoNotOptimize(event);
        listener_manager->emitEvictionEvent(event);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(1);
}
BENCHMARK(BenchPromotionDecisionLatency)->UseRealTime();

/**
 * @brief GATE-CAP-06: Cache→storage feedback loop latency.
 * 
 * Measures end-to-end latency from cache eviction to storage demotion signal.
 * Gate: ≤100µs
 */
static void BenchCacheStorageFeedbackLatency(benchmark::State& state) {
    struct FeedbackListener : public IEvictionListener {
    public:
        std::vector<std::string> demotion_queue;
        std::atomic<std::size_t> processed{0};

        void onCacheEvicted(const CacheEvictionEvent& event) override {
            // Gate-CAP-06: Queue demotion to storage
            if (event.access_count < 3) {
                demotion_queue.push_back(event.key);
                processed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    auto listener_manager = createEvictionListenerManager();
    auto listener = std::make_shared<FeedbackListener>();
    listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "demotion_candidate";
    event.from_tier = TierLevel::L1;
    event.access_count = 1;  // Low access triggers demotion

    for (auto _ : state) {
        benchmark::DoNotOptimize(event);
        listener_manager->emitEvictionEvent(event);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(1);
}
BENCHMARK(BenchCacheStorageFeedbackLatency)->UseRealTime();

}  // namespace cache
}  // namespace themis

// Register release-gate manifest entries
// See: benchmarks/cache/release_gate_manifest_cache.json
// 
// GATE-CAP-01: Single eviction event emission ≤10µs
// GATE-CAP-02: Multiple listener event delivery ≤50µs (10 listeners)
// GATE-CAP-03: Capacity pressure notification ≤30µs
// GATE-CAP-04: Listener registration ≤5µs per operation
// GATE-CAP-05: L1→L2 promotion decision ≤50µs
// GATE-CAP-06: Cache→storage feedback ≤100µs
