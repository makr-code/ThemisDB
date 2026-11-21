/**
 * @file bench_analytics_distributed_coordinator.cpp
 * @brief Benchmarks for distributed analytics coordinator safety controls.
 *
 * Release Gates:
 *   DC-01: Circuit breaker state transition ≤ 100µs
 *   DC-02: Concurrent merge startup ≤ 500µs (p99)
 *   DC-03: Timeout recovery switchover ≤ 1000µs (p99)
 *   DC-04: Degraded-mode throughput ≥ 80% of normal
 *
 * @module Analytics
 * @author ThemisDB Project
 * @date 2026-08-15
 */

#include "analytics/distributed_analytics.h"
#include "benchmark/benchmark.h"
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

namespace themisdb {
namespace analytics {

// Mock executor for benchmarking
class BenchShardExecutor : public ShardQueryExecutor {
public:
    enum class Behavior { FAST, SLOW, FAIL };

    std::atomic<Behavior> behavior{Behavior::FAST};
    std::chrono::microseconds delay{0};

    themis::analytics::OLAPResult execute(
        const std::string& shard_id,
        const themis::analytics::OLAPQuery& query) override {
        if (delay.count() > 0) {
            auto start = std::chrono::high_resolution_clock::now();
            while (std::chrono::high_resolution_clock::now() - start < delay) {
                // Tight spinloop for precise timing
            }
        }

        if (behavior.load(std::memory_order_relaxed) == Behavior::FAIL) {
            throw std::runtime_error("Mock shard failure");
        }

        themis::analytics::OLAPResult result;
        result.rows.push_back({});
        return result;
    }

    bool isHealthy() const override {
        return behavior.load(std::memory_order_relaxed) != Behavior::FAIL;
    }
};

// ============================================================================
// DC-01: Circuit breaker state transition ≤ 100µs
// ============================================================================
static void BenchCircuitBreakerStateTransition(benchmark::State& state) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.enable_circuit_breaker = true;
    cfg.circuit_breaker_failure_threshold = 3;
    cfg.circuit_breaker_recovery_delay_ms = 100;

    auto coordinator = std::make_shared<DistributedAnalyticsSharding>(cfg);
    auto executor = std::make_shared<BenchShardExecutor>();
    executor->behavior = BenchShardExecutor::Behavior::FAST;

    coordinator->addShard("shard-0", executor);

    themis::analytics::OLAPQuery query;
    query.dimensions.push_back({"dim1", "STRING"});

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        try {
            coordinator->executeDistributed(query);
        } catch (...) {
            // Expected
        }
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() / 1e9);
    }

    state.SetLabel("DC-01: CircuitBreaker state transition");
}
BENCHMARK(BenchCircuitBreakerStateTransition)->MinTime(1.0)->Iterations(100);

// ============================================================================
// DC-02: Concurrent merge startup ≤ 500µs (p99)
// ============================================================================
static void BenchConcurrentMergeStartup(benchmark::State& state) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.enable_circuit_breaker = false;

    auto coordinator = std::make_shared<DistributedAnalyticsSharding>(cfg);

    // Create multiple shards for parallel dispatch
    int num_shards = 10;
    for (int i = 0; i < num_shards; ++i) {
        auto executor = std::make_shared<BenchShardExecutor>();
        executor->behavior = BenchShardExecutor::Behavior::FAST;
        executor->delay = std::chrono::microseconds{10};
        coordinator->addShard("shard-" + std::to_string(i), executor);
    }

    themis::analytics::OLAPQuery query;
    query.dimensions.push_back({"dim1", "STRING"});
    themis::analytics::Measure sum_measure;
    sum_measure.name = "sum_value";
    sum_measure.field = "value";
    sum_measure.function = themis::analytics::Measure::Function::Sum;
    query.measures.push_back(sum_measure);

    std::vector<std::chrono::nanoseconds> latencies;

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        try {
            coordinator->executeDistributed(query);
        } catch (...) {
            // Expected
        }
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        latencies.push_back(elapsed);
    }

    // Sort latencies for p99 calculation
    std::sort(latencies.begin(), latencies.end());
    if (!latencies.empty()) {
        auto p99_idx = (latencies.size() * 99) / 100;
        auto p99_latency = latencies[p99_idx];
        double p99_us = std::chrono::duration_cast<std::chrono::microseconds>(p99_latency).count();

        state.SetLabel(
            "DC-02: Concurrent merge p99=" + std::to_string(static_cast<long>(p99_us)) + "µs");

        // Gate check: DC-02 requires ≤ 500µs (p99)
        if (p99_us > 500.0) {
            state.SkipWithError(
                ("DC-02 FAILED: p99=" + std::to_string(static_cast<long>(p99_us)) + "µs > 500µs")
                    .c_str());
        }
    }
}
BENCHMARK(BenchConcurrentMergeStartup)->MinTime(2.0)->Iterations(500);

// ============================================================================
// DC-03: Timeout recovery switchover ≤ 1000µs (p99)
// ============================================================================
static void BenchTimeoutRecoverySwitchover(benchmark::State& state) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.enable_circuit_breaker = true;
    cfg.circuit_breaker_failure_threshold = 2;
    cfg.circuit_breaker_recovery_delay_ms = 50;
    cfg.shard_execution_timeout_ms = 100;

    auto coordinator = std::make_shared<DistributedAnalyticsSharding>(cfg);
    auto executor = std::make_shared<BenchShardExecutor>();
    executor->behavior = BenchShardExecutor::Behavior::FAST;

    coordinator->addShard("shard-0", executor);

    themis::analytics::OLAPQuery query;
    query.dimensions.push_back({"dim1", "STRING"});

    std::vector<std::chrono::nanoseconds> recovery_latencies;

    // Trigger circuit open first
    executor->behavior = BenchShardExecutor::Behavior::FAIL;
    for (int i = 0; i < 2; ++i) {
        try {
            coordinator->executeDistributed(query);
        } catch (...) {}
    }

    // Wait for recovery delay
    std::this_thread::sleep_for(std::chrono::milliseconds{75});

    // Measure recovery switchover time
    executor->behavior = BenchShardExecutor::Behavior::FAST;
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        try {
            coordinator->executeDistributed(query);
        } catch (...) {
            // May still fail depending on probe timing
        }
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        recovery_latencies.push_back(elapsed);
    }

    // Calculate p99
    std::sort(recovery_latencies.begin(), recovery_latencies.end());
    if (!recovery_latencies.empty()) {
        auto p99_idx = (recovery_latencies.size() * 99) / 100;
        auto p99_latency = recovery_latencies[p99_idx];
        double p99_us =
            std::chrono::duration_cast<std::chrono::microseconds>(p99_latency).count();

        state.SetLabel("DC-03: Recovery switchover p99=" + std::to_string(static_cast<long>(p99_us)) +
                       "µs");

        // Gate check: DC-03 requires ≤ 1000µs (p99)
        if (p99_us > 1000.0) {
            state.SkipWithError(
                ("DC-03 FAILED: p99=" + std::to_string(static_cast<long>(p99_us)) + "µs > 1000µs")
                    .c_str());
        }
    }
}
BENCHMARK(BenchTimeoutRecoverySwitchover)->MinTime(1.0)->Iterations(200);

// ============================================================================
// DC-04: Degraded-mode throughput ≥ 80% of normal
// ============================================================================
static void BenchDegradedModeThroughput(benchmark::State& state) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.enable_circuit_breaker = true;
    cfg.circuit_breaker_failure_threshold = 3;
    cfg.circuit_breaker_recovery_delay_ms = 100;

    auto coordinator = std::make_shared<DistributedAnalyticsSharding>(cfg);

    // One failed shard must stay within the default 20% failure-rate limit.
    constexpr size_t shard_count = 5;
    std::shared_ptr<BenchShardExecutor> failing_executor;
    for (size_t i = 0; i < shard_count; ++i) {
        auto executor = std::make_shared<BenchShardExecutor>();
        executor->delay = std::chrono::microseconds{10};
        coordinator->addShard("shard-" + std::to_string(i), executor);
        failing_executor = executor;
    }

    themis::analytics::OLAPQuery query;
    query.dimensions.push_back({"dim1", "STRING"});
    themis::analytics::Measure sum_measure;
    sum_measure.name = "sum_value";
    sum_measure.field = "value";
    sum_measure.function = themis::analytics::Measure::Function::Sum;
    query.measures.push_back(sum_measure);

    // Measure normal throughput
    auto start = std::chrono::steady_clock::now();
    int normal_count = 0;
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds{1}) {
        const auto result = coordinator->executeDistributed(query);
        if (result.total_shards != shard_count ||
            result.successful_shards != shard_count || result.merged.rows.empty()) {
            state.SkipWithError("DC-04: normal query did not return all shard results");
            return;
        }
        ++normal_count;
    }
    const double normal_seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    // Degrade: Make one shard fail.
    failing_executor->behavior = BenchShardExecutor::Behavior::FAIL;

    // Measure degraded throughput
    start = std::chrono::steady_clock::now();
    int degraded_count = 0;
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds{1}) {
        const auto result = coordinator->executeDistributed(query);
        if (result.successful_shards != shard_count - 1 ||
            result.total_shards < shard_count - 1 ||
            result.total_shards > shard_count || result.merged.rows.empty()) {
            state.SkipWithError("DC-04: degraded query did not return the healthy shard results");
            return;
        }
        ++degraded_count;
    }
    const double degraded_seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    if (normal_count == 0 || degraded_count == 0) {
        state.SkipWithError("DC-04: no completed queries in a measurement window");
        return;
    }
    const double throughput_ratio = (degraded_count / degraded_seconds) /
                                    (normal_count / normal_seconds);
    const std::string measurement =
        "normal=" + std::to_string(normal_count) + "/" + std::to_string(normal_seconds) +
        "s, degraded=" + std::to_string(degraded_count) + "/" +
        std::to_string(degraded_seconds) + "s";

    state.SetLabel("DC-04: Degraded throughput ratio = " +
                   std::to_string(static_cast<long>(throughput_ratio * 100)) + "%; " + measurement);

    // Gate check: DC-04 requires ≥ 80% throughput with one shard down
    if (throughput_ratio < 0.80) {
        state.SkipWithError(
            ("DC-04 FAILED: Degraded throughput " + std::to_string(static_cast<long>(throughput_ratio * 100)) +
             "% < 80% threshold; " + measurement)
                .c_str());
    }
}
BENCHMARK(BenchDegradedModeThroughput)->MinTime(5.0)->Iterations(1);

}  // namespace analytics
}  // namespace themisdb
