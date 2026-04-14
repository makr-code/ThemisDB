/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_lora_inline.cpp                              ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:35:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     166                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <benchmark/benchmark.h>
#include "llm/multi_lora_manager.h"
#include <chrono>
#include <string>
#include <vector>

using themis::llm::InferenceRequest;
using themis::llm::InferenceResponse;
using themis::llm::MultiLoRAManager;

namespace {

MultiLoRAManager::Config benchConfig() {
    MultiLoRAManager::Config cfg;
    cfg.max_lora_vram_mb = 256;
    cfg.max_lora_slots = 4;
    cfg.enable_multi_lora_batch = true;
    cfg.lora_ttl = std::chrono::seconds(300);
    return cfg;
}

static void BM_LoRA_LoadUnload(benchmark::State& state) {
    MultiLoRAManager mgr(benchConfig());
    const std::string base_model = "bench-base";
    int counter = 0;

    for (auto _ : state) {
        auto id = "bench-lora-" + std::to_string(counter++);
        mgr.loadLoRA(id, "/loras/" + id + ".bin", base_model, 1.0f);
        benchmark::DoNotOptimize(mgr.listLoRAs().size());
        mgr.unloadLoRA(id, true);
    }

    state.counters["slots"] = static_cast<double>(mgr.listLoRAs().size());
}
BENCHMARK(BM_LoRA_LoadUnload);

static void BM_LoRA_Switching(benchmark::State& state) {
    MultiLoRAManager mgr(benchConfig());
    const std::string base_model = "bench-base";
    std::vector<std::string> ids = {"alpha", "beta", "gamma", "delta"};
    for (const auto& id : ids) {
        mgr.loadLoRA(id, "/loras/" + id + ".bin", base_model, 1.0f);
    }

    size_t idx = 0;
    for (auto _ : state) {
        const auto& id = ids[idx++ % ids.size()];
        mgr.applyLoRA(id, nullptr);
        mgr.removeLoRA(id, nullptr);
        benchmark::ClobberMemory();
    }

    auto cache = mgr.getCacheStats();
    state.counters["switches"] = cache.value("switches", 0.0);
    state.counters["hit_rate"] = cache.value("hit_rate", 0.0);
}
BENCHMARK(BM_LoRA_Switching);

static void BM_LoRA_ExportImport(benchmark::State& state) {
    MultiLoRAManager source(benchConfig());
    MultiLoRAManager sink(benchConfig());
    const std::string base_model = "bench-base";

    source.loadLoRA("export-src", "/loras/export-src.bin", base_model, 1.0f);
    auto payload = source.exportLoRA("export-src");

    for (auto _ : state) {
        sink.importLoRA("remote", payload, base_model);
        benchmark::DoNotOptimize(sink.listLoRAs().size());
        sink.unloadLoRA("remote", true);
    }

    auto stats = sink.getCacheStats();
    state.counters["evictions"] = stats.value("evictions", 0.0);
}
BENCHMARK(BM_LoRA_ExportImport);

static void BM_LoRA_InferenceWithoutAdapter(benchmark::State& state) {
    MultiLoRAManager mgr(benchConfig());
    const std::string base_model = "bench-base";

    size_t counter = 0;
    for (auto _ : state) {
        InferenceRequest req;
        req.prompt = "bench prompt " + std::to_string(counter++);
        req.max_tokens = 32;
        // NO lora_adapter_id - baseline

        benchmark::DoNotOptimize(req);
        benchmark::ClobberMemory();
    }

    state.counters["baseline"] = 1.0;
}
BENCHMARK(BM_LoRA_InferenceWithoutAdapter);

static void BM_LoRA_InferenceWithAdapter(benchmark::State& state) {
    MultiLoRAManager mgr(benchConfig());
    const std::string base_model = "bench-base";

    // Load adapter first
    mgr.loadLoRA("bench-adapter", "/loras/bench.bin", base_model, 1.0f);

    size_t counter = 0;
    for (auto _ : state) {
        InferenceRequest req;
        req.prompt = "bench prompt " + std::to_string(counter++);
        req.max_tokens = 32;
        req.lora_adapter_id = std::string("bench-adapter");

        benchmark::DoNotOptimize(req);
        benchmark::ClobberMemory();
    }

    auto stats = mgr.getCacheStats();
    state.counters["hit_rate"] = stats.value("hit_rate", 0.0);
}
BENCHMARK(BM_LoRA_InferenceWithAdapter);

static void BM_LoRA_ConcurrentAdapterSwitching(benchmark::State& state) {
    MultiLoRAManager mgr(benchConfig());
    const std::string base_model = "bench-base";
    
    std::vector<std::string> adapters = {"adapter-1", "adapter-2", "adapter-3"};
    for (const auto& id : adapters) {
        mgr.loadLoRA(id, "/loras/" + id + ".bin", base_model, 1.0f);
    }

    size_t idx = 0;
    for (auto _ : state) {
        auto adapter_id = adapters[idx++ % adapters.size()];
        mgr.applyLoRA(adapter_id, nullptr);
        benchmark::ClobberMemory();
    }

    state.counters["num_adapters"] = static_cast<double>(adapters.size());
}
BENCHMARK(BM_LoRA_ConcurrentAdapterSwitching);

} // namespace

BENCHMARK_MAIN();
