/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_llama_cpp_inference.cpp                      ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:43:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   73.0/100                                       ║
    • Total Lines:     252                                            ║
    • Open Issues:     TODOs: 0, Stubs: 6                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8332e5afa3  2026-04-13  Refactor and update various components for improved compa... ║
    • 4c802da514  2026-04-12  feat(benchmarks): add 9 missing benchmark suites for util... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bench_llama_cpp_inference.cpp
 * @brief Real llama.cpp inference benchmark via LlamaCppPlugin
 *
 * Validates:
 *   LLAMACPP-PHASE5: Track compatibility for high-throughput concurrent inference calls
 *
 * Scenarios:
 *   - LlamaCppPlugin::generate() — stub path (no model file), measures plugin overhead
 *   - LlamaCppPlugin::generateBatch() — throughput across varying request counts
 *   - LlamaCppPlugin::embed() — embedding vector latency
 *   - LlamaCppPlugin::generateStream() — streaming token dispatch overhead
 *   - Concurrent inference from multiple threads (mutex contention baseline)
 *   - getPerformanceStats() / getMemoryStats() query cost
 *
 * When THEMIS_ENABLE_LLAMA_CPP is defined and a GGUF model is available at
 * THEMIS_BENCH_LLAMA_MODEL_PATH, the benchmarks exercise the real inference
 * path.  Without a model the plugin runs in stub mode and the benchmarks
 * measure pure plugin / dispatch overhead — useful as a regression baseline.
 */

#include <benchmark/benchmark.h>
#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/llm_plugin_interface.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace themis::llamacpp;
using namespace themis::llm;

// ─── helpers ─────────────────────────────────────────────────────────────────

static const char* kModelPath =
#ifdef THEMIS_BENCH_LLAMA_MODEL_PATH
    THEMIS_BENCH_LLAMA_MODEL_PATH;
#else
    "";  // empty → stub mode
#endif

/// Build an InferenceRequest with a given prompt and max_tokens budget.
static InferenceRequest makeRequest(const std::string& prompt,
                                    int max_tokens   = 64,
                                    float temperature = 0.0f) {
    InferenceRequest req;
    req.prompt      = prompt;
    req.max_tokens  = max_tokens;
    req.temperature = temperature;
    req.top_p       = 1.0f;
    req.top_k       = 1;
    return req;
}

// ─── Fixture ─────────────────────────────────────────────────────────────────

class LlamaCppBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        plugin = std::make_unique<LlamaCppPlugin>();
        nlohmann::json cfg;
        cfg["model_path"] = kModelPath;
        cfg["n_threads"]  = 4;
        cfg["n_ctx"]      = 2048;
        plugin->loadModel(kModelPath, cfg);
        // Stub mode: loadModel returns false without a real model; that is
        // acceptable — plugin is still usable for dispatch overhead measurement.
    }

    void TearDown(const benchmark::State& /*s*/) override {
        plugin->unloadModel();
        plugin.reset();
    }

    std::unique_ptr<LlamaCppPlugin> plugin;
};

// ─── 1. generate() single request latency ────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, Generate_SingleRequest)(benchmark::State& state) {
    auto req = makeRequest("Summarise the concept of data gravity in one sentence.",
                           /*max_tokens=*/32);

    for (auto _ : state) {
        auto resp = plugin->generate(req);
        benchmark::DoNotOptimize(resp.text);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("generate() — stub or real model; max_tokens=32");
}

// ─── 2. generate() prompt-size sweep ─────────────────────────────────────────

static void BM_Generate_PromptSize(benchmark::State& state) {
    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["model_path"] = kModelPath;
    plugin.loadModel(kModelPath, cfg);

    const int prompt_tokens = static_cast<int>(state.range(0));
    // Approximate token count: 1 word ≈ 1.3 tokens
    std::string prompt(static_cast<size_t>(prompt_tokens) * 5, 'A');
    auto req = makeRequest(prompt, /*max_tokens=*/16);

    for (auto _ : state) {
        auto resp = plugin.generate(req);
        benchmark::DoNotOptimize(resp.text);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("prompt ~" + std::to_string(prompt_tokens) + " tokens");
    plugin.unloadModel();
}
BENCHMARK(BM_Generate_PromptSize)->Arg(16)->Arg(64)->Arg(256)->Arg(512);

// ─── 3. generateBatch() throughput ───────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, GenerateBatch_Throughput)(benchmark::State& state) {
    const int kBatchSize = static_cast<int>(state.range(0));
    std::vector<InferenceRequest> requests;
    requests.reserve(static_cast<size_t>(kBatchSize));
    for (int i = 0; i < kBatchSize; ++i) {
        requests.push_back(makeRequest("What is " + std::to_string(i) + "?",
                                       /*max_tokens=*/8));
    }

    for (auto _ : state) {
        auto responses = plugin->generateBatch(requests);
        benchmark::DoNotOptimize(responses.size());
    }

    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("generateBatch size=" + std::to_string(kBatchSize));
}
BENCHMARK_REGISTER_F(LlamaCppBenchFixture, GenerateBatch_Throughput)
    ->Arg(1)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->Unit(benchmark::kMillisecond);

// ─── 4. embed() latency ──────────────────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, Embed_Latency)(benchmark::State& state) {
    const std::string text = "ThemisDB is a hybrid database system for enterprise workloads.";

    for (auto _ : state) {
        auto emb = plugin->embed(text);
        benchmark::DoNotOptimize(emb.size());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("embed() one sentence");
}

// ─── 5. generateStream() overhead ────────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, GenerateStream_Overhead)(benchmark::State& state) {
    auto req = makeRequest("List three benefits of column stores.", /*max_tokens=*/32);

    for (auto _ : state) {
        std::atomic<int> token_count{0};
        auto resp = plugin->generateStream(req, [&token_count](const std::string& /*tok*/) {
            token_count.fetch_add(1, std::memory_order_relaxed);
        });
        benchmark::DoNotOptimize(resp.text);
        benchmark::DoNotOptimize(token_count.load());
    }

    state.SetLabel("generateStream(); stub emits ≥1 callback");
}

// ─── 6. Concurrent inference — mutex contention baseline ─────────────────────

static void BM_ConcurrentInference(benchmark::State& state) {
    const int kThreads = static_cast<int>(state.range(0));

    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["model_path"] = kModelPath;
    plugin.loadModel(kModelPath, cfg);

    auto req = makeRequest("ping", /*max_tokens=*/4);

    for (auto _ : state) {
        std::atomic<uint64_t> completed{0};
        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(kThreads));

        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&plugin, &req, &completed]() {
                auto resp = plugin.generate(req);
                benchmark::DoNotOptimize(resp.text);
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& th : threads) th.join();
        benchmark::DoNotOptimize(completed.load());
    }

    state.SetItemsProcessed(state.iterations() * kThreads);
    state.SetLabel("concurrent inference threads=" + std::to_string(kThreads));
    plugin.unloadModel();
}
BENCHMARK(BM_ConcurrentInference)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

// ─── 7. getPerformanceStats() / getMemoryStats() query cost ──────────────────

BENCHMARK_F(LlamaCppBenchFixture, StatsQuery)(benchmark::State& state) {
    for (auto _ : state) {
        auto perf = plugin->getPerformanceStats();
        auto mem  = plugin->getMemoryStats();
        benchmark::DoNotOptimize(perf);
        benchmark::DoNotOptimize(mem);
    }
    state.SetLabel("getPerformanceStats() + getMemoryStats()");
}

// ─── 8. getCapabilities() overhead ───────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, GetCapabilities)(benchmark::State& state) {
    for (auto _ : state) {
        auto caps = plugin->getCapabilities();
        benchmark::DoNotOptimize(caps.supports_streaming);
    }
    state.SetLabel("getCapabilities()");
}
