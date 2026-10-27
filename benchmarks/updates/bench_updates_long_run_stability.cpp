/**
 * @file bench_updates_long_run_stability.cpp
 * @brief Benchmark suite for sustained update workload using the public API.
 */

#include <benchmark/benchmark.h>

#include "updates/canary_rollout.h"
#include "updates/delta_update_engine.h"
#include "updates/hot_reload_engine.h"
#include "updates/tenant_update_scheduler.h"

#include <chrono>
#include <memory>
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

static void LongRunStability_MixedUpdateWorkload(::benchmark::State& state) {
    for (auto _ : state) {
        const auto before = std::chrono::steady_clock::now();

        TenantUpdateScheduler scheduler;
        StubHotReloadEngine engine;
        DeltaUpdateEngine delta_engine{".", "/tmp/themis_bench_updates"};

        const std::string tenant = "tenant-bench";
        const std::string version = "1.5.0";

        MaintenanceWindow window{{"Daily"}, "02:00", "04:00", "UTC"};
        scheduler.setMaintenanceWindow(tenant, window);

        UpdatePolicy policy;
        policy.auto_update = false;
        policy.notification_lead_time = std::chrono::hours{0};
        scheduler.setUpdatePolicy(tenant, policy);

        scheduler.recordNotification(tenant, version);
        scheduler.grantConsent(tenant, version);

        const bool can_update_now = scheduler.canUpdateNow(tenant, UpdatePriority::NORMAL);
        const auto apply_result = scheduler.applyUpdate(tenant, version, engine, UpdatePriority::NORMAL);
        const auto delta = delta_engine.findDelta("1.4.0", "1.5.0");

        auto cfg = CanaryConfig::withDefaultStages(version, tenant);
        CanaryRollout rollout(std::make_shared<StubHotReloadEngine>(), cfg);
        const auto rollout_result = rollout.applyIfIncluded();
        if (rollout_result.success) {
            rollout.reportSuccess();
            rollout.reportError();
        }

        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - before).count();

        state.counters["can_update_now"] = static_cast<double>(can_update_now ? 1.0 : 0.0);
        state.counters["apply_success"] = static_cast<double>(apply_result.success ? 1.0 : 0.0);
        state.counters["delta_available"] = static_cast<double>(delta.has_value() ? 1.0 : 0.0);
        state.counters["rollout_applied"] = static_cast<double>(rollout_result.success ? 1.0 : 0.0);
        state.counters["latency_ms"] = static_cast<double>(elapsed_ms);

        ::benchmark::DoNotOptimize(apply_result);
        ::benchmark::DoNotOptimize(delta);
        ::benchmark::DoNotOptimize(rollout_result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(LongRunStability_MixedUpdateWorkload)
    ->Iterations(50)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/LongRunStability/MixedUpdateWorkload");

} // namespace benchmark
} // namespace updates
} // namespace themis
