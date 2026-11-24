/**
 * @file bench_llm_raid_pipeline.cpp
 * @brief Comprehensive Google Benchmark for RAID + LoRA + Inferencing pipeline
 * 
 * Measures technical effort (latency, throughput, resource usage) for:
 * - Data distribution across RAID shards
 * - LoRA loading and caching
 * - Inference performance with different LoRAs
 * - Cross-shard consistency overhead
 */

#include <benchmark/benchmark.h>
#include "benchmark_artifact_preflight.h"
#include "llm/llm_plugin_manager.h"
#include "llama_cpp/llama_cpp_plugin.h"
#include <filesystem>
#include <fstream>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <memory>
#include <exception>
#include <utility>

namespace fs = std::filesystem;
using themis::llm::InferenceRequest;
using themis::llm::LLMPluginManager;

namespace {

class RAIDLoRAPipelineFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        (void)state;
        const char* configured_model_path = std::getenv("THEMIS_LLM_MODEL_PATH");
        const char* configured_model_dir = std::getenv("THEMIS_MODEL_DIR");
        // CTest must not load an arbitrary model discovered in a user or repository directory.
        if ((configured_model_path && *configured_model_path != '\0') ||
            (configured_model_dir && *configured_model_dir != '\0')) {
            model_path_ = themis::bench::resolveModelPath();
            lora_path_ = themis::bench::resolveLoraPath();
        }
        std::error_code error;
        const auto temporary_dir = fs::temp_directory_path(error);
        if (!error) {
            test_dir_ = (temporary_dir /
                ("themis_bench_raid_lora_" +
                 std::to_string(reinterpret_cast<std::uintptr_t>(this)))).string();
        }
    }

    void TearDown(const ::benchmark::State& state) override {
        (void)state;
        if (!test_dir_.empty()) {
            std::error_code error;
            fs::remove_all(test_dir_, error);
        }
    }
    
    static bool ensureBackend(
        benchmark::State& state,
        LLMPluginManager& manager) {
#ifdef THEMIS_LLAMA_CPP_STUB_MODE
        state.SkipWithError(
            "LLM/LoRA performance benchmarks cannot run with synthetic llama.cpp stub mode");
        return false;
#else
        try {
            if (manager.getPlugin("benchmark_llamacpp") == nullptr) {
                manager.registerPlugin(
                    "benchmark_llamacpp",
                    std::make_unique<themis::llamacpp::LlamaCppPlugin>());
            }
            manager.setDefaultPlugin("benchmark_llamacpp");
        } catch (const std::exception& error) {
            state.SkipWithError(error.what());
            return false;
        }
        if (manager.getDefaultPlugin() == nullptr) {
            state.SkipWithError("Failed to register the llama.cpp benchmark backend");
            return false;
        }
        return true;
#endif
    }

    static bool requireRealBackend(
        benchmark::State& state,
        LLMPluginManager& manager) {
        auto* plugin = dynamic_cast<themis::llamacpp::LlamaCppPlugin*>(
            manager.getDefaultPlugin());
        if (plugin == nullptr || !plugin->hasRealRuntimeLoaded()) {
            state.SkipWithError(
                "Configured model did not initialize a real llama.cpp runtime");
            return false;
        }
        return true;
    }

    bool loadModel(benchmark::State& state, LLMPluginManager& manager,
                   const std::string& model_id) const {
        if (model_path_.empty()) {
            state.SkipWithMessage(
                "Optional model-backed RAID benchmark skipped: set "
                "THEMIS_LLM_MODEL_PATH or THEMIS_MODEL_DIR to opt in to a GGUF model.");
            return false;
        }
        if (!ensureBackend(state, manager)) {
            return false;
        }
        if (!manager.loadModel(model_id, model_path_)) {
            state.SkipWithError(
                "Configured GGUF model failed to load through the llama.cpp runtime");
            return false;
        }
        return requireRealBackend(state, manager);
    }

    bool loadLoRA(benchmark::State& state, LLMPluginManager& manager,
                  const std::string& adapter_id, const std::string& model_id) const {
        if (lora_path_.empty()) {
            state.SkipWithMessage(
                "Optional model-backed LoRA benchmark skipped: no external "
                "LoRA adapter was found at the configured adapter path.");
            return false;
        }
        if (!manager.loadLoRA(adapter_id, lora_path_, model_id)) {
            state.SkipWithError(
                "Configured LoRA adapter failed to load into the llama.cpp runtime");
            return false;
        }
        return true;
    }

    static bool requireInference(benchmark::State& state,
                                 const themis::llm::InferenceResponse& response) {
        if (!response.success || response.text.empty() ||
            response.tokens_generated == 0) {
            state.SkipWithError(response.error_message.empty()
                ? "Real-model inference returned no valid generated tokens"
                : response.error_message.c_str());
            return false;
        }
        return true;
    }

protected:
    std::string test_dir_;
    std::string model_path_;
    std::string lora_path_;
};

// ═══════════════════════════════════════════════════════════
// Benchmark: Model Loading Overhead
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_ModelLoadSingleShard)
(benchmark::State& state) {
    if (model_path_.empty()) {
        state.SkipWithMessage(
            "Optional model-backed RAID benchmark skipped: set "
            "THEMIS_LLM_MODEL_PATH or THEMIS_MODEL_DIR to opt in to a GGUF model.");
        return;
    }

    LLMPluginManager manager;
    if (!ensureBackend(state, manager)) {
        return;
    }

    for (auto _ : state) {
        if (!manager.loadModel("benchmark_model", model_path_)) {
            state.SkipWithError(
                "Configured GGUF model failed to load through the llama.cpp runtime");
            return;
        }
        if (!requireRealBackend(state, manager)) {
            return;
        }
        manager.unloadModel("benchmark_model");
    }

    state.counters["model_loads"] = static_cast<double>(state.iterations());
}

// ═══════════════════════════════════════════════════════════
// Benchmark: LoRA Loading (Single vs Multiple)
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_LoRA_LoadSingleAdapter)
(benchmark::State& state) {
    LLMPluginManager manager;
    state.PauseTiming();
    const bool model_ready = loadModel(state, manager, "benchmark_model");
    state.ResumeTiming();
    if (!model_ready) {
        return;
    }
    
    for (auto _ : state) {
        if (!loadLoRA(state, manager, "configured_adapter", "benchmark_model")) {
            return;
        }
        benchmark::DoNotOptimize(manager.getDefaultPlugin()->listLoRAs().size());
        if (!manager.unloadLoRA("configured_adapter")) {
            state.SkipWithError("Failed to unload the configured LoRA adapter");
            return;
        }
    }
    
    state.counters["operations"] = 2;  // load + unload
}

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_LoRA_LoadMultipleAdapters)
(benchmark::State& state) {
    LLMPluginManager manager;
    state.PauseTiming();
    const bool model_ready = loadModel(state, manager, "benchmark_model");
    state.ResumeTiming();
    if (!model_ready) {
        return;
    }
    constexpr size_t adapter_count = 3;
    const std::array<std::string, adapter_count> adapter_ids = {
        "configured_adapter_1", "configured_adapter_2", "configured_adapter_3"};
    
    for (auto _ : state) {
        for (const auto& adapter_id : adapter_ids) {
            if (!loadLoRA(state, manager, adapter_id, "benchmark_model")) {
                return;
            }
        }

        benchmark::DoNotOptimize(manager.getDefaultPlugin()->listLoRAs().size());

        for (const auto& adapter_id : adapter_ids) {
            if (!manager.unloadLoRA(adapter_id)) {
                state.SkipWithError("Failed to unload a configured LoRA adapter");
                return;
            }
        }
    }
    
    state.counters["num_adapter_slots"] = static_cast<double>(adapter_count);
    state.SetLabel("same configured adapter loaded into distinct runtime slots");
}

// ═══════════════════════════════════════════════════════════
// Benchmark: LoRA Switching Latency
// Uses the production plugin/runtime path so each case binds the adapter to
// llama.cpp's live inference context instead of calling applyLoRA with null.
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_LoRA_SwitchingLatency)
(benchmark::State& state) {
    auto& runtime = LLMPluginManager::instance();
    if (model_path_.empty()) {
        state.SkipWithMessage(
            "Optional model-backed LoRA benchmark skipped: set "
            "THEMIS_LLM_MODEL_PATH or THEMIS_MODEL_DIR to opt in to a GGUF model.");
        return;
    }
    if (lora_path_.empty()) {
        state.SkipWithMessage(
            "Optional model-backed LoRA benchmark skipped: no external LoRA "
            "adapter was found at the configured adapter path.");
        return;
    }

    if (!ensureBackend(state, runtime)) {
        return;
    }
    state.PauseTiming();
    auto* plugin = runtime.getDefaultPlugin();
    if (plugin != nullptr && plugin->isModelLoaded()) {
        plugin->unloadModel();
    }
    if (!runtime.loadModel("benchmark_base", model_path_)) {
        state.ResumeTiming();
        state.SkipWithError("Failed to load the configured GGUF model for LoRA benchmark");
        return;
    }
    if (!requireRealBackend(state, runtime)) {
        state.ResumeTiming();
        return;
    }
    if (!runtime.loadLoRA("configured_adapter", lora_path_, "benchmark_base")) {
        state.ResumeTiming();
        state.SkipWithError(
            "Configured LoRA adapter failed to load into the llama.cpp runtime");
        return;
    }
    state.ResumeTiming();

    bool use_adapter = false;
    
    for (auto _ : state) {
        InferenceRequest request;
        request.prompt = "Measure production adapter activation and deactivation";
        request.max_tokens = 1;
        request.lora_adapter_id = use_adapter ? "configured_adapter" : "";
        use_adapter = !use_adapter;
        const auto response = runtime.generate(request);
        if (!response.success || response.tokens_generated == 0) {
            state.SkipWithError(response.error_message.empty()
                ? "Production LoRA inference returned no valid output"
                : response.error_message.c_str());
            return;
        }
        benchmark::DoNotOptimize(response);
    }
    
    state.counters["adapter_state_switches"] = static_cast<double>(state.iterations());
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("configured adapter on/off");
}

// ═══════════════════════════════════════════════════════════
// Benchmark: Inference Without LoRA (Baseline)
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_Inference_WithoutLoRA)
(benchmark::State& state) {
    LLMPluginManager manager;
    state.PauseTiming();
    const bool model_ready = loadModel(state, manager, "benchmark_model");
    state.ResumeTiming();
    if (!model_ready) {
        return;
    }
    
    size_t counter = 0;
    for (auto _ : state) {
        InferenceRequest req;
        req.prompt = "Test query " + std::to_string(counter++);
        req.max_tokens = 64;
        
        auto resp = manager.generate(req);
        if (!requireInference(state, resp)) {
            return;
        }
        benchmark::DoNotOptimize(resp);
    }
    
    state.counters["baseline"] = 1;
    state.counters["max_tokens"] = 64;
}

// ═══════════════════════════════════════════════════════════
// Benchmark: Inference With Single LoRA
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_Inference_WithSingleLoRA)
(benchmark::State& state) {
    LLMPluginManager manager;
    state.PauseTiming();
    const bool runtime_ready =
        loadModel(state, manager, "benchmark_model") &&
        loadLoRA(state, manager, "configured_adapter", "benchmark_model");
    state.ResumeTiming();
    if (!runtime_ready) {
        return;
    }
    
    size_t counter = 0;
    for (auto _ : state) {
        InferenceRequest req;
        req.prompt = "Legal query " + std::to_string(counter++);
        req.max_tokens = 64;
        req.lora_adapter_id = "configured_adapter";
        
        auto resp = manager.generate(req);
        if (!requireInference(state, resp)) {
            return;
        }
        benchmark::DoNotOptimize(resp);
    }
    
    state.counters["lora_loaded"] = 1;
    state.counters["max_tokens"] = 64;
}

// ═══════════════════════════════════════════════════════════
// Benchmark: Inference With LoRA Switching
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_Inference_WithLoRASwitching)
(benchmark::State& state) {
    LLMPluginManager manager;
    state.PauseTiming();
    const bool runtime_ready =
        loadModel(state, manager, "benchmark_model") &&
        loadLoRA(state, manager, "configured_adapter", "benchmark_model");
    state.ResumeTiming();
    if (!runtime_ready) {
        return;
    }

    bool use_adapter = false;
    size_t counter = 0;
    
    for (auto _ : state) {
        InferenceRequest req;
        req.prompt = "Adapter activation query " + std::to_string(counter++);
        req.max_tokens = 64;
        req.lora_adapter_id = use_adapter ? "configured_adapter" : "";
        use_adapter = !use_adapter;
        
        auto resp = manager.generate(req);
        if (!requireInference(state, resp)) {
            return;
        }
        benchmark::DoNotOptimize(resp);
    }
    
    state.counters["adapter_state_switches"] = static_cast<double>(state.iterations());
    state.counters["max_tokens"] = 64;
    state.SetLabel("configured adapter on/off");
}

// ═══════════════════════════════════════════════════════════
// Benchmark: Multi-Shard Concurrent Inference
// ═══════════════════════════════════════════════════════════

BENCHMARK_DEFINE_F(RAIDLoRAPipelineFixture, BM_MultiShard_ConcurrentInference)
(benchmark::State& state) {
    const int num_shards = state.range(0);
    std::vector<LLMPluginManager> shard_mgrs(num_shards);
    
    // Initialize all shards
    state.PauseTiming();
    bool shards_ready = true;
    for (int i = 0; i < num_shards; i++) {
        const auto model_id = "shard_model_" + std::to_string(i);
        if (!loadModel(state, shard_mgrs[i], model_id) ||
            !loadLoRA(state, shard_mgrs[i], "configured_adapter", model_id)) {
            shards_ready = false;
            break;
        }
    }
    state.ResumeTiming();
    if (!shards_ready) {
        return;
    }
    
    size_t counter = 0;
    for (auto _ : state) {
        std::vector<std::future<themis::llm::InferenceResponse>> responses;
        responses.reserve(shard_mgrs.size());
        for (int i = 0; i < num_shards; i++) {
            InferenceRequest req;
            req.prompt = "Shard " + std::to_string(i) + " query " + std::to_string(counter);
            req.max_tokens = 32;
            req.lora_adapter_id = "configured_adapter";
            responses.push_back(std::async(
                std::launch::async,
                [&manager = shard_mgrs[i], request = std::move(req)]() {
                    return manager.generate(request);
                }));
        }
        for (auto& pending : responses) {
            const auto resp = pending.get();
            if (!requireInference(state, resp)) {
                return;
            }
            benchmark::DoNotOptimize(resp);
        }
        counter++;
    }
    
    state.counters["num_shards"] = num_shards;
    state.SetItemsProcessed(state.iterations() * num_shards);
}
BENCHMARK_REGISTER_F(RAIDLoRAPipelineFixture, BM_MultiShard_ConcurrentInference)
    ->Arg(1)
    ->Arg(3)
    ->Arg(5)
    ->Arg(9);

// ═══════════════════════════════════════════════════════════
// Benchmark: Cross-Shard Data Distribution
// ═══════════════════════════════════════════════════════════

BENCHMARK_DEFINE_F(RAIDLoRAPipelineFixture, BM_DataDistribution_RAIDStriping)
(benchmark::State& state) {
    const int num_shards = state.range(0);
    const int num_records = state.range(1);
    if (test_dir_.empty()) {
        state.SkipWithError("Unable to resolve a temporary directory for RAID data benchmark");
        return;
    }
    
    std::vector<std::string> shard_paths(num_shards);
    for (int i = 0; i < num_shards; i++) {
        shard_paths[i] = test_dir_ + "/shard_" + std::to_string(i);
        if (!fs::exists(shard_paths[i])) {
            fs::create_directories(shard_paths[i]);
        }
    }
    
    for (auto _ : state) {
        // Simulate RAID0 striping distribution
        for (int record_id = 0; record_id < num_records; record_id++) {
            int shard_idx = record_id % num_shards;
            std::string record = "record_" + std::to_string(record_id);
            
            // Write to shard file
            std::ofstream out(shard_paths[shard_idx] + "/data.txt", std::ios::app);
            out << record << "\n";
            out.close();
            
            benchmark::DoNotOptimize(record);
        }
        
        // Clean up for next iteration
        for (const auto& path : shard_paths) {
            if (fs::exists(path + "/data.txt")) {
                fs::remove(path + "/data.txt");
            }
        }
    }
    
    state.counters["num_shards"] = num_shards;
    state.counters["num_records"] = num_records;
    state.counters["records_per_sec"] = num_records / state.iterations();
}
BENCHMARK_REGISTER_F(RAIDLoRAPipelineFixture, BM_DataDistribution_RAIDStriping)
    ->Args({1, 1000})
    ->Args({3, 3000})
    ->Args({5, 5000})
    ->Args({9, 9000});

// ═══════════════════════════════════════════════════════════
// Benchmark: Complete Pipeline (measure end-to-end overhead)
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_CompletePipeline_EndToEnd)
(benchmark::State& state) {
    const int num_shards = 3;
    const int inferences_per_shard = 5;
    
    for (auto _ : state) {
        state.PauseTiming();
        
        std::vector<LLMPluginManager> shards(num_shards);
        
        state.ResumeTiming();
        
        // Load model on all shards
        for (size_t shard_idx = 0; shard_idx < shards.size(); ++shard_idx) {
            const auto model_id = "pipeline_model_" + std::to_string(shard_idx);
            if (!loadModel(state, shards[shard_idx], model_id) ||
                !loadLoRA(state, shards[shard_idx], "configured_adapter", model_id)) {
                return;
            }
        }
        
        // Run inferences
        for (auto& shard : shards) {
            for (int i = 0; i < inferences_per_shard; i++) {
                InferenceRequest req;
                req.prompt = "Query " + std::to_string(i);
                req.max_tokens = 32;
                req.lora_adapter_id = (i % 2 == 0) ? "configured_adapter" : "";
                
                auto resp = shard.generate(req);
                if (!requireInference(state, resp)) {
                    return;
                }
                benchmark::DoNotOptimize(resp);
            }
        }
    }
    
    state.counters["num_shards"] = 3;
    state.counters["num_loras"] = 1;
    state.counters["inferences_per_shard"] = inferences_per_shard;
    state.counters["total_inferences"] = num_shards * inferences_per_shard;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Phase 6 — Distributed Inference Cases
// ─────────────────────────────────────────────────────────────────────────
// These benchmarks measure the overhead introduced by domain routing
// decisions and batch fan-out across multiple shards.  They run entirely
// in-process (no real network) to give stable, reproducible numbers.
// ═══════════════════════════════════════════════════════════════════════════

namespace {

// ─────────────────────────────────────────────────────────────────────────
// BM_DomainRouting_OverheadPerRequest
//
// Measures the latency of a single domain-routing decision made by
// AdaptiveShardRouter::routeByDomain().  This is the overhead paid per
// request to select the most capable shard for a given domain.
//
// Target (Phase 6 doc): routing decision ≤ 5 µs p99.
// ─────────────────────────────────────────────────────────────────────────

} // anonymous namespace

#include "sharding/adaptive_shard_router.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "distributed_knowledge/adapter_capability_announcement.h"

using namespace themis::sharding;
using namespace themis::distributed_knowledge;

namespace {

static std::unique_ptr<AdaptiveShardRouter> makeThreeShardRouter()
{
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    ShardRouter::Config cfg;
    auto r = std::make_unique<AdaptiveShardRouter>(resolver, nullptr, topology, cfg);

    for (const auto& [shard, delta] :
         std::initializer_list<std::pair<const char*, double>>{
             {"shard-legal",   0.85},
             {"shard-medical", 0.20},
             {"shard-general", 0.10}}) {
        AdapterCapabilityAnnouncement cap;
        cap.domain_type    = AdapterDomainType::LEGAL;
        cap.accuracy_delta = delta;
        cap.adapter_version = "v1";
        r->updateAdapterCapability(shard, cap);
    }
    return r;
}

} // anonymous namespace

static void BM_DomainRouting_OverheadPerRequest(benchmark::State& state)
{
    auto router = makeThreeShardRouter();

    for (auto _ : state) {
        const auto shard = router->routeByDomain(AdapterDomainType::LEGAL);
        benchmark::DoNotOptimize(shard);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("target: ≤5 µs p99");
}

BENCHMARK(BM_DomainRouting_OverheadPerRequest)
    ->Iterations(100'000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────
// BM_BatchFanOut_LatencyScaling
//
// Measures how batch-fanout latency grows as batch size N increases from 1
// to 64 when requests are spread across a simulated shard-group.  Uses
// std::async fan-out (the same mechanism as executeBatchInfer) with an
// in-process mock that just hashes the prompt string.
//
// Args: batch_size (1, 8, 16, 32, 64)
// Expected: near-linear throughput, super-linear wall time (up to thread pool saturation).
// Target (Phase 6 doc): p99 latency for batch-64 ≤ 4× single-request latency.
// ─────────────────────────────────────────────────────────────────────────

static void BM_BatchFanOut_LatencyScaling(benchmark::State& state)
{
    const int batch_size = static_cast<int>(state.range(0));

    // Simulate a "domain shard" that processes one request synchronously.
    auto mock_infer = [](const std::string& prompt) -> std::string {
        // Busy-work proportional to prompt length (simulates token generation).
        volatile std::size_t h = 0;
        for (char c : prompt) { h = h * 31u + static_cast<unsigned char>(c); }
        return "result-" + std::to_string(h);
    };

    std::vector<std::string> prompts;
    prompts.reserve(static_cast<std::size_t>(batch_size));
    for (int i = 0; i < batch_size; ++i) {
        prompts.push_back("System prompt for request " + std::to_string(i));
    }

    for (auto _ : state) {
        // Fan out: one async task per request (mirrors executeBatchInfer fan-out).
        std::vector<std::future<std::string>> futures;
        futures.reserve(prompts.size());
        for (const auto& p : prompts) {
            futures.push_back(std::async(std::launch::async, mock_infer, p));
        }
        std::vector<std::string> results = {};

        results.reserve(prompts.size());
        for (auto& f : futures) {
            results.push_back(f.get());
        }
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.SetLabel("target: batch-64 ≤ 4× single latency");
}

BENCHMARK(BM_BatchFanOut_LatencyScaling)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
