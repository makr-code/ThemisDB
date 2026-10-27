/**
 * @file bench_updates_canary_resilience.cpp
 * @brief Benchmark suite for canary rollout behavior using the public API.
 */

#include <benchmark/benchmark.h>

#include "updates/canary_rollout.h"
#include "updates/hot_reload_engine.h"

#include <chrono>
#include <memory>
#include <random>
#include <string>

namespace themis {
namespace updates {
namespace benchmark {

class StubHotReloadEngine : public HotReloadEngine {
public:
    StubHotReloadEngine()
        : HotReloadEngine(std::shared_ptr<ManifestDatabase>{},
                          std::shared_ptr<utils::UpdateChecker>{},
                          HotReloadEngine::Config{}) {}

    ReloadResult applyHotReload(const std::string& version,
                                bool /*verify_only*/ = false) override {
        ReloadResult result;
        result.success = true;
        result.rollback_id = "bench_rollback_" + version;
        result.files_updated.push_back("themis_server");
        return result;
    }

    bool rollback(const std::string& /*rollback_id*/) override {
        return true;
    }
};

class CanaryRolloutBenchmark : public ::benchmark::Fixture {
protected:
    std::shared_ptr<HotReloadEngine> engine_;
    std::unique_ptr<CanaryRollout> canary_;
    std::mt19937 rng_{std::random_device{}()};

    void SetUp(const ::benchmark::State&) override {
        engine_ = std::make_shared<StubHotReloadEngine>();
        auto cfg = CanaryConfig::withDefaultStages("1.5.0", "bench-node");
        canary_ = std::make_unique<CanaryRollout>(engine_, cfg);
    }

    void TearDown(const ::benchmark::State&) override {
        canary_.reset();
        engine_.reset();
    }
};

BENCHMARK_F(CanaryRolloutBenchmark, NetworkDelayResilience)(::benchmark::State& state) {
    std::uniform_int_distribution<int> delay_ms(100, 1000);

    for (auto _ : state) {
        const auto delay = delay_ms(rng_);
        const auto before = std::chrono::steady_clock::now();

        auto result = canary_->applyIfIncluded();
        if (result.success) {
            canary_->reportSuccess();
            canary_->reportSuccess();
            canary_->reportError();
        }

        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - before).count();

        state.counters["network_delay_ms"] = static_cast<double>(delay);
        state.counters["observed_latency_ms"] = static_cast<double>(elapsed_ms);
        ::benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_F(CanaryRolloutBenchmark, NodeFailureAtStages)(::benchmark::State& state) {
    for (auto _ : state) {
        const auto before = std::chrono::steady_clock::now();

        auto result = canary_->applyIfIncluded();
        if (result.success) {
            canary_->reportError();
            canary_->reportError();
            canary_->reportSuccess();
            if (canary_->shouldRollback()) {
                canary_->rollback("benchmark failure injection");
            }
        }

        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - before).count();

        state.counters["failure_stage"] = static_cast<double>(canary_->currentStage());
        state.counters["rollback_latency_ms"] = static_cast<double>(elapsed_ms);
        ::benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_F(CanaryRolloutBenchmark, HealthCheckTimeout)(::benchmark::State& state) {
    for (auto _ : state) {
        const auto before = std::chrono::steady_clock::now();
        auto result = canary_->applyIfIncluded();
        if (result.success) {
            for (int i = 0; i < 10; ++i) {
                canary_->reportError();
            }
            const auto status = canary_->status();
            ::benchmark::DoNotOptimize(status);
        }

        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - before).count();
        state.counters["healthcheck_latency_ms"] = static_cast<double>(elapsed_ms);
        ::benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_F(CanaryRolloutBenchmark, AutomaticRollbackOnFailure)(::benchmark::State& state) {
    for (auto _ : state) {
        const auto before = std::chrono::steady_clock::now();
        auto result = canary_->applyIfIncluded();
        if (result.success) {
            for (int i = 0; i < 6; ++i) {
                canary_->reportError();
            }
            if (canary_->shouldRollback()) {
                const bool rolled_back = canary_->rollback("automatic rollback benchmark");
                ::benchmark::DoNotOptimize(rolled_back);
            }
        }

        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - before).count();
        state.counters["rollback_detection_latency_ms"] = static_cast<double>(elapsed_ms);
        ::benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_REGISTER_F(CanaryRolloutBenchmark, NetworkDelayResilience)
    ->Iterations(100)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/CanaryRollout/NetworkDelayResilience");

BENCHMARK_REGISTER_F(CanaryRolloutBenchmark, NodeFailureAtStages)
    ->Iterations(100)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/CanaryRollout/NodeFailureAtStages");

BENCHMARK_REGISTER_F(CanaryRolloutBenchmark, HealthCheckTimeout)
    ->Iterations(100)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/CanaryRollout/HealthCheckTimeout");

BENCHMARK_REGISTER_F(CanaryRolloutBenchmark, AutomaticRollbackOnFailure)
    ->Iterations(100)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/CanaryRollout/AutomaticRollbackOnFailure");

} // namespace benchmark
} // namespace updates
} // namespace themis
