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
#include "llm/llm_plugin_manager.h"
#include "llm/multi_lora_manager.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <memory>

namespace fs = std::filesystem;
using themis::llm::InferenceRequest;
using themis::llm::LLMPluginManager;
using themis::llm::MultiLoRAManager;

namespace {

class RAIDLoRAPipelineFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        (void)state;
        
        test_dir_ = "./bench_raid_lora";
        model_dir_ = test_dir_ + "/models";
        lora_dir_ = test_dir_ + "/loras";
        
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(model_dir_);
        fs::create_directories(lora_dir_);
        
        // Create base model (100MB)
        createDummyModel("base.gguf", 100);
        
        // Create LoRA adapters (20MB each)
        createDummyLoRA("legal.bin", 20);
        createDummyLoRA("medical.bin", 20);
        createDummyLoRA("finance.bin", 20);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        (void)state;
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    void createDummyModel(const std::string& filename, size_t size_mb) {
        std::string path = model_dir_ + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        file.write("GGUF", 4);
        std::vector<char> data(size_mb * 1024 * 1024, 0xAB);
        file.write(data.data(), data.size());
        file.close();
    }
    
    void createDummyLoRA(const std::string& filename, size_t size_mb) {
        std::string path = lora_dir_ + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        file.write("LORA", 4);
        std::vector<char> data(size_mb * 1024 * 1024, 0xCD);
        file.write(data.data(), data.size());
        file.close();
    }
    
protected:
    std::string test_dir_;
    std::string model_dir_;
    std::string lora_dir_;
};

// ═══════════════════════════════════════════════════════════
// Benchmark: Model Loading Overhead
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_ModelLoadSingleShard)
(benchmark::State& state) {
    for (auto _ : state) {
        LLMPluginManager mgr_iter;
        // Simulate model load
        benchmark::DoNotOptimize(model_dir_);
    }
    
    state.counters["model_size_mb"] = 100;
}

// ═══════════════════════════════════════════════════════════
// Benchmark: LoRA Loading (Single vs Multiple)
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_LoRA_LoadSingleAdapter)
(benchmark::State& state) {
    MultiLoRAManager::Config cfg;
    cfg.max_lora_slots = 8;
    cfg.max_lora_vram_mb = 512;
    
    MultiLoRAManager mgr(cfg);
    const std::string base_model = "base";
    
    for (auto _ : state) {
        mgr.loadLoRA("legal", lora_dir_ + "/legal.bin", base_model, 1.0f);
        benchmark::DoNotOptimize(mgr.listLoRAs().size());
        mgr.unloadLoRA("legal", true);
    }
    
    state.counters["lora_size_mb"] = 20;
    state.counters["operations"] = 2;  // load + unload
}

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_LoRA_LoadMultipleAdapters)
(benchmark::State& state) {
    MultiLoRAManager::Config cfg;
    cfg.max_lora_slots = 8;
    cfg.max_lora_vram_mb = 512;
    
    MultiLoRAManager mgr(cfg);
    const std::string base_model = "base";
    
    for (auto _ : state) {
        mgr.loadLoRA("legal", lora_dir_ + "/legal.bin", base_model, 1.0f);
        mgr.loadLoRA("medical", lora_dir_ + "/medical.bin", base_model, 1.0f);
        mgr.loadLoRA("finance", lora_dir_ + "/finance.bin", base_model, 1.0f);
        
        benchmark::DoNotOptimize(mgr.listLoRAs().size());
        
        mgr.unloadLoRA("legal", true);
        mgr.unloadLoRA("medical", true);
        mgr.unloadLoRA("finance", true);
    }
    
    state.counters["num_loras"] = 3;
    state.counters["total_lora_size_mb"] = 60;
}

// ═══════════════════════════════════════════════════════════
// Benchmark: LoRA Switching Latency
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_LoRA_SwitchingLatency)
(benchmark::State& state) {
    MultiLoRAManager::Config cfg;
    cfg.max_lora_slots = 8;
    cfg.max_lora_vram_mb = 512;
    
    MultiLoRAManager mgr(cfg);
    const std::string base_model = "base";
    
    mgr.loadLoRA("legal", lora_dir_ + "/legal.bin", base_model, 1.0f);
    mgr.loadLoRA("medical", lora_dir_ + "/medical.bin", base_model, 1.0f);
    mgr.loadLoRA("finance", lora_dir_ + "/finance.bin", base_model, 1.0f);
    
    std::vector<std::string> adapters = {"legal", "medical", "finance"};
    size_t idx = 0;
    
    for (auto _ : state) {
        const auto& adapter = adapters[idx++ % adapters.size()];
        mgr.applyLoRA(adapter, nullptr);
        benchmark::ClobberMemory();
    }
    
    state.counters["num_adapters"] = 3;
    state.counters["switches"] = static_cast<double>(state.iterations());
}

// ═══════════════════════════════════════════════════════════
// Benchmark: Inference Without LoRA (Baseline)
// ═══════════════════════════════════════════════════════════

BENCHMARK_F(RAIDLoRAPipelineFixture, BM_Inference_WithoutLoRA)
(benchmark::State& state) {
    LLMPluginManager mgr;
    mgr.loadModel("base", model_dir_ + "/base.gguf");
    
    size_t counter = 0;
    for (auto _ : state) {
        InferenceRequest req;
        req.prompt = "Test query " + std::to_string(counter++);
        req.max_tokens = 64;
        
        auto resp = mgr.generate(req);
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
    LLMPluginManager mgr;
    mgr.loadModel("base", model_dir_ + "/base.gguf");
    mgr.loadLoRA("legal", lora_dir_ + "/legal.bin", "base");
    
    size_t counter = 0;
    for (auto _ : state) {
        InferenceRequest req;
        req.prompt = "Legal query " + std::to_string(counter++);
        req.max_tokens = 64;
        req.lora_adapter_id = "legal";
        
        auto resp = mgr.generate(req);
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
    LLMPluginManager mgr;
    mgr.loadModel("base", model_dir_ + "/base.gguf");
    mgr.loadLoRA("legal", lora_dir_ + "/legal.bin", "base");
    mgr.loadLoRA("medical", lora_dir_ + "/medical.bin", "base");
    mgr.loadLoRA("finance", lora_dir_ + "/finance.bin", "base");
    
    std::vector<std::string> adapters = {"legal", "medical", "finance"};
    size_t idx = 0;
    size_t counter = 0;
    
    for (auto _ : state) {
        const auto& adapter = adapters[idx++ % adapters.size()];
        
        InferenceRequest req;
        req.prompt = adapter + " query " + std::to_string(counter++);
        req.max_tokens = 64;
        req.lora_adapter_id = adapter;
        
        auto resp = mgr.generate(req);
        benchmark::DoNotOptimize(resp);
    }
    
    state.counters["num_loras"] = 3;
    state.counters["max_tokens"] = 64;
}

// ═══════════════════════════════════════════════════════════
// Benchmark: Multi-Shard Concurrent Inference
// ═══════════════════════════════════════════════════════════

BENCHMARK_DEFINE_F(RAIDLoRAPipelineFixture, BM_MultiShard_ConcurrentInference)
(benchmark::State& state) {
    const int num_shards = state.range(0);
    std::vector<LLMPluginManager> shard_mgrs(num_shards);
    
    // Initialize all shards
    for (int i = 0; i < num_shards; i++) {
        shard_mgrs[i].loadModel("base", model_dir_ + "/base.gguf");
        shard_mgrs[i].loadLoRA("legal", lora_dir_ + "/legal.bin", "base");
    }
    
    size_t counter = 0;
    for (auto _ : state) {
        for (int i = 0; i < num_shards; i++) {
            InferenceRequest req;
            req.prompt = "Shard " + std::to_string(i) + " query " + std::to_string(counter);
            req.max_tokens = 32;
            req.lora_adapter_id = "legal";
            
            auto resp = shard_mgrs[i].generate(req);
            benchmark::DoNotOptimize(resp);
        }
        counter++;
    }
    
    state.counters["num_shards"] = num_shards;
    state.counters["total_requests"] = num_shards;
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
        for (auto& shard : shards) {
            shard.loadModel("base", model_dir_ + "/base.gguf");
        }
        
        // Load LoRAs on all shards
        for (auto& shard : shards) {
            shard.loadLoRA("legal", lora_dir_ + "/legal.bin", "base");
            shard.loadLoRA("medical", lora_dir_ + "/medical.bin", "base");
        }
        
        // Run inferences
        std::vector<std::string> adapters = {"legal", "medical"};
        for (auto& shard : shards) {
            for (int i = 0; i < inferences_per_shard; i++) {
                const auto& adapter = adapters[i % adapters.size()];
                
                InferenceRequest req;
                req.prompt = "Query " + std::to_string(i);
                req.max_tokens = 32;
                req.lora_adapter_id = adapter;
                
                auto resp = shard.generate(req);
                benchmark::DoNotOptimize(resp);
            }
        }
    }
    
    state.counters["num_shards"] = 3;
    state.counters["num_loras"] = 2;
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
        std::vector<std::string> results;
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
