/*
 * ThemisDB | File: bench_tensor_integration_baseline.cpp
 * Version: 1.0.0 | Maturity: Production-Ready
 * 
 * Tensor Mid-Layer Integration Benchmark Suite (Phase 4)
 * Purpose: Establish performance baselines for full pipeline execution
 */

#include <benchmark/benchmark.h>

#include "tensor/tensor_mid_layer.h"
#include "tensor/adapter_repository.h"
#include "tensor/tensor_fingerprint_graph.h"
#include "tensor/compression_strategy.h"
#include "tensor/tensor_routing_strategy.h"
#include "tensor/tensor_redundancy_detection.h"

#include <memory>
#include <vector>
#include <random>

namespace themis::tensor::bench {

// ============================================================================
// Setup Fixtures
// ============================================================================

class TensorPipelineFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        mid_layer_ = std::make_shared<TensorMidLayer>();
        adapter_repo_ = std::make_shared<AdapterRepository>();
        fingerprint_graph_ = std::make_shared<TensorFingerprintGraph>();
        
        mid_layer_->setAdapterRepository(adapter_repo_);
        mid_layer_->setFingerprintGraph(fingerprint_graph_);
        
        // Pre-populate adapter repository with test data
        populateAdapters(1000);
    }

    void TearDown(const ::benchmark::State&) override {
        // Cleanup if needed
    }

protected:
    void populateAdapters(int count) {
        for (int i = 0; i < count; ++i) {
            std::string key = "adapter:model:v1:" + std::to_string(i);
            TensorTrainCore core;
            core.order = 3;
            core.shape = {4, 8, 4};
            core.ranks = {1, 2, 2, 1};
            core.cores.resize(3);
            
            adapter_repo_->store("tenant1", "domain1", key, core);
            fingerprint_graph_->addAdapter(key, core, "tenant1", "domain1");
        }
    }

    std::shared_ptr<TensorMidLayer> mid_layer_;
    std::shared_ptr<AdapterRepository> adapter_repo_;
    std::shared_ptr<TensorFingerprintGraph> fingerprint_graph_;
};

// ============================================================================
// BENCH-001: Mid-Layer Planning
// ============================================================================

BENCHMARK_F(TensorPipelineFixture, MidLayerPlanning_AdapterScope)(benchmark::State& state) {
    TensorLayerContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.domain = "domain1";
    ctx.base_model_id = "model";
    ctx.top_k = 10;
    ctx.use_fingerprint_summary = true;
    
    for (auto _ : state) {
        auto plan = mid_layer_->plan(ctx);
        benchmark::DoNotOptimize(plan);
    }
}

BENCHMARK_F(TensorPipelineFixture, MidLayerPlanning_PackageScope)(benchmark::State& state) {
    TensorLayerContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.scope_id = "pkg:package1";
    ctx.use_fingerprint_summary = false;
    
    for (auto _ : state) {
        auto plan = mid_layer_->plan(ctx);
        benchmark::DoNotOptimize(plan);
    }
}

BENCHMARK_F(TensorPipelineFixture, MidLayerPlanning_ShardScope)(benchmark::State& state) {
    TensorLayerContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.scope_id = "shard:shard1";
    ctx.shard_aware = true;
    ctx.top_k = 50;
    
    for (auto _ : state) {
        auto plan = mid_layer_->plan(ctx);
        benchmark::DoNotOptimize(plan);
    }
}

// ============================================================================
// BENCH-002: Mid-Layer Summarization
// ============================================================================

BENCHMARK_F(TensorPipelineFixture, MidLayerSummarization_SingleScope)(benchmark::State& state) {
    TensorLayerContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.domain = "domain1";
    ctx.base_model_id = "model";
    ctx.top_k = 100;
    ctx.use_fingerprint_summary = true;
    
    for (auto _ : state) {
        auto summary = mid_layer_->summarize(ctx);
        benchmark::DoNotOptimize(summary);
    }
}

// ============================================================================
// BENCH-003: Fingerprint Graph Operations
// ============================================================================

BENCHMARK_F(TensorPipelineFixture, FingerprintGraphFindSimilar_Top10)(benchmark::State& state) {
    std::string query_key = "adapter:model:v1:query";
    TensorTrainCore query_core;
    fingerprint_graph_->addAdapter(query_key, query_core, "tenant1", "domain1");
    
    for (auto _ : state) {
        auto results = fingerprint_graph_->findSimilar(query_key, 10);
        benchmark::DoNotOptimize(results);
    }
}

BENCHMARK_F(TensorPipelineFixture, FingerprintGraphFindSimilar_Top100)(benchmark::State& state) {
    std::string query_key = "adapter:model:v1:query";
    TensorTrainCore query_core;
    fingerprint_graph_->addAdapter(query_key, query_core, "tenant1", "domain1");
    
    for (auto _ : state) {
        auto results = fingerprint_graph_->findSimilar(query_key, 100);
        benchmark::DoNotOptimize(results);
    }
}

BENCHMARK_F(TensorPipelineFixture, FingerprintGraphAddAdapter)(benchmark::State& state) {
    int counter = 0;
    for (auto _ : state) {
        std::string key = "bench_adapter:" + std::to_string(counter++);
        TensorTrainCore core;
        fingerprint_graph_->addAdapter(key, core, "tenant1", "domain1");
        benchmark::DoNotOptimize(key);
    }
}

// ============================================================================
// BENCH-004: Adapter Repository Operations
// ============================================================================

BENCHMARK_F(TensorPipelineFixture, AdapterRepositoryStore)(benchmark::State& state) {
    int counter = 0;
    for (auto _ : state) {
        std::string key = "store_adapter:" + std::to_string(counter++);
        TensorTrainCore core;
        adapter_repo_->store("tenant1", "domain1", key, core);
        benchmark::DoNotOptimize(key);
    }
}

BENCHMARK_F(TensorPipelineFixture, AdapterRepositoryFindSimilar)(benchmark::State& state) {
    for (auto _ : state) {
        auto results = adapter_repo_->findSimilarAdapters("domain1", "model", 50);
        benchmark::DoNotOptimize(results);
    }
}

// ============================================================================
// BENCH-005: Federated Shard Summarization
// ============================================================================

BENCHMARK_F(TensorPipelineFixture, FederatedShardSummarization_3Shards)(benchmark::State& state) {
    TensorLayerContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.domain = "domain1";
    ctx.base_model_id = "model";
    ctx.top_k = 50;
    ctx.shard_scope_ids = {"shard:shard0", "shard:shard1", "shard:shard2"};
    ctx.shard_aware = true;
    
    for (auto _ : state) {
        auto federated = mid_layer_->summarizeFederatedShards(ctx);
        benchmark::DoNotOptimize(federated);
    }
}

BENCHMARK_F(TensorPipelineFixture, FederatedShardSummarization_10Shards)(benchmark::State& state) {
    std::vector<std::string> shard_ids;
    for (int i = 0; i < 10; ++i) {
        shard_ids.push_back("shard:shard" + std::to_string(i));
    }
    
    TensorLayerContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.domain = "domain1";
    ctx.base_model_id = "model";
    ctx.top_k = 25;
    ctx.shard_scope_ids = shard_ids;
    ctx.shard_aware = true;
    
    for (auto _ : state) {
        auto federated = mid_layer_->summarizeFederatedShards(ctx);
        benchmark::DoNotOptimize(federated);
    }
}

// ============================================================================
// BENCH-006: Compression Strategy Performance
// ============================================================================

BENCHMARK(CompressionStrategy_TTSVDDecomposition) {
    auto strategy = std::make_shared<CompressionStrategy>();
    TensorTrain train;
    train.order = 3;
    train.shape = {64, 64, 64};
    train.ranks = {1, 4, 4, 1};
    train.cores.resize(3);
    
    CompressionConfig config;
    config.algorithm = CompressionAlgorithm::TT_SVD;
    config.target_rank = 2;
    config.tolerance = 1e-6;
    
    for (auto _ : benchmark::State_) {
        auto result = strategy->compress(train, config);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(CompressionStrategy_Quantization) {
    auto strategy = std::make_shared<CompressionStrategy>();
    TensorTrain train;
    train.order = 4;
    train.shape = {32, 32, 32, 32};
    train.ranks = {1, 2, 2, 2, 1};
    train.cores.resize(4);
    
    CompressionConfig config;
    config.algorithm = CompressionAlgorithm::QUANTIZATION;
    config.bit_width = 8;
    
    for (auto _ : benchmark::State_) {
        auto result = strategy->compress(train, config);
        benchmark::DoNotOptimize(result);
    }
}

// ============================================================================
// BENCH-007: Routing Strategy Selection
// ============================================================================

BENCHMARK(RoutingStrategy_QualityBased) {
    auto strategy = std::make_shared<QualityBasedRoutingStrategy>();
    
    std::vector<SimilarityResult> candidates;
    for (int i = 0; i < 100; ++i) {
        candidates.push_back({
            .adapter_key = "adapter:" + std::to_string(i),
            .score = 0.5f + (i * 0.004f),
            .metadata = {}
        });
    }
    
    RoutingContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.top_k = 10;
    
    for (auto _ : benchmark::State_) {
        auto routes = strategy->route(candidates, ctx);
        benchmark::DoNotOptimize(routes);
    }
}

BENCHMARK(RoutingStrategy_ShardAware) {
    auto strategy = std::make_shared<ShardAwareRoutingStrategy>();
    
    std::vector<SimilarityResult> candidates;
    for (int i = 0; i < 100; ++i) {
        candidates.push_back({
            .adapter_key = "shard:" + std::to_string(i % 5) + ":adapter:" + 
                          std::to_string(i),
            .score = 0.5f,
            .metadata = {}
        });
    }
    
    RoutingContext ctx;
    ctx.tenant_id = "tenant1";
    ctx.top_k = 10;
    ctx.shard_aware = true;
    
    for (auto _ : benchmark::State_) {
        auto routes = strategy->route(candidates, ctx);
        benchmark::DoNotOptimize(routes);
    }
}

// ============================================================================
// BENCH-008: Redundancy Detection
// ============================================================================

BENCHMARK(RedundancyDetection_SimilarityBased) {
    auto detector = std::make_shared<SimilarityBasedRedundancyDetector>();
    
    std::vector<TensorTrainCore> batch;
    for (int i = 0; i < 50; ++i) {
        TensorTrainCore core;
        core.order = 3;
        core.shape = {16, 16, 16};
        core.ranks = {1, 2, 2, 1};
        core.cores.resize(3);
        batch.push_back(core);
    }
    
    for (auto _ : benchmark::State_) {
        auto redundant_pairs = detector->detectRedundancy(batch);
        benchmark::DoNotOptimize(redundant_pairs);
    }
}

}  // namespace themis::tensor::bench

BENCHMARK_MAIN();
