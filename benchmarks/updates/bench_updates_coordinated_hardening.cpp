/**
 * @file bench_updates_coordinated_hardening.cpp
 * @brief Benchmark suite for coordinated multi-node updates using the public API.
 */

#include <benchmark/benchmark.h>

#include "updates/coordinated_update_manager.h"
#include "updates/hot_reload_engine.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

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

static void CoordinatedUpdate_AllNodesSuccessPath(::benchmark::State& state) {
    for (auto _ : state) {
        auto engine = std::make_shared<StubHotReloadEngine>();
        CoordinatedUpdateConfig config;
        config.version = "1.5.0";
        config.local_node_id = "node-1";
        config.nodes = {
            {"node-1", 0, false},
            {"node-2", 1, false},
            {"node-3", 2, true}
        };

        CoordinatedUpdateManager manager(engine, config);
        manager.setWaitForPreviousFunc([](const std::string&, std::chrono::milliseconds) { return true; });
        manager.setSignalReadyFunc([](const std::string&, bool) {});

        const auto result = manager.applyLocalUpdate();
        if (!result.success) {
            state.SkipWithError("Coordinated update failed");
        }

        state.counters["nodes_updated"] = static_cast<double>(result.nodes_updated);
        ::benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(CoordinatedUpdate_AllNodesSuccessPath)
    ->Iterations(50)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/CoordinatedUpdate/AllNodesSuccessPath");

} // namespace benchmark
} // namespace updates
} // namespace themis
