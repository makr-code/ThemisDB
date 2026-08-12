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
#include <cstdint>

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
    static TensorTrainCore makeSyntheticTrain(std::uint32_t seed) {
        TensorTrainCore core;
        core.train.mode_sizes = {8, 8, 8};
        core.cores.resize(3);

        core.cores[0].r_left = 1;
        core.cores[0].n = 8;
        core.cores[0].r_right = 2;

        core.cores[1].r_left = 2;
        core.cores[1].n = 8;
        core.cores[1].r_right = 2;

        core.cores[2].r_left = 2;
        core.cores[2].n = 8;
        core.cores[2].r_right = 1;

        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (auto& tt_core : core.cores) {
            tt_core.data.resize(tt_core.r_left * tt_core.n * tt_core.r_right);
            for (auto& value : tt_core.data) {
                value = dist(rng);
            }
        }
        return core;
    }

    void populateAdapters(int count) {
        for (int i = 0; i < count; ++i) {
            std::string key = "adapter:model:v1:" + std::to_string(i);
            TensorTrainCore core = makeSyntheticTrain(static_cast<std::uint32_t>(i + 1));

            (void)adapter_repo_->store("tenant1", "domain1", key, core);
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
    TensorTrainCore query_core = TensorPipelineFixture::makeSyntheticTrain(424242);
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
        (void)adapter_repo_->store("tenant1", "domain1", key, core);
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

static void BM_CompressionStrategy_TTSVDDecomposition(benchmark::State& state) {
    TTDecompositionStrategy strategy;
    storage::TTTrain train;
    train.cores.resize(3);

    CompressionConfig config;
    config.algorithm = CompressionAlgorithm::TT_SVD;
    config.target_rank = 2;
    config.tt_epsilon = 1e-6f;

    for (auto _ : state) {
        auto result = strategy.compressTTTrain(train, config);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_CompressionStrategy_TTSVDDecomposition)->Unit(benchmark::kMicrosecond);

static void BM_CompressionStrategy_Quantization(benchmark::State& state) {
    QuantizationStrategy strategy(8);
    storage::TTTrain train;
    train.cores.resize(4);

    CompressionConfig config;
    config.quantization_bits = 8;

    for (auto _ : state) {
        auto result = strategy.compressTTTrain(train, config);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_CompressionStrategy_Quantization)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BENCH-007: Routing Strategy Selection
// ============================================================================

static void BM_RoutingStrategy_QualityBased(benchmark::State& state) {
    QualityBasedRouting strategy;

    std::vector<BaseTensorSummary> summaries;
    summaries.reserve(100);
    for (int i = 0; i < 100; ++i) {
        BaseTensorSummary s;
        s.id = "adapter:" + std::to_string(i);
        s.similarity_score = 0.5f + static_cast<float>(i) * 0.004f;
        summaries.push_back(std::move(s));
    }

    index::AnnQueryContext ctx;
    ctx.scope_id = "domain1";
    ctx.recall_target = 0.95;

    for (auto _ : state) {
        auto decision = strategy.route(summaries, summaries.size(), 1.0f, ctx);
        benchmark::DoNotOptimize(decision);
    }
}
BENCHMARK(BM_RoutingStrategy_QualityBased)->Unit(benchmark::kMicrosecond);

static void BM_RoutingStrategy_ShardAware(benchmark::State& state) {
    ShardAwareRouting strategy;

    std::vector<BaseTensorSummary> summaries;
    summaries.reserve(100);
    for (int i = 0; i < 100; ++i) {
        BaseTensorSummary s;
        s.id = "shard:" + std::to_string(i % 5) + ":adapter:" + std::to_string(i);
        s.similarity_score = 0.5f;
        summaries.push_back(std::move(s));
    }

    index::AnnQueryContext ctx;
    ctx.shard_aware = true;
    ctx.recall_target = 0.95;

    for (auto _ : state) {
        auto decision = strategy.route(summaries, summaries.size(), 1.0f, ctx);
        benchmark::DoNotOptimize(decision);
    }
}
BENCHMARK(BM_RoutingStrategy_ShardAware)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BENCH-008: Redundancy Detection
// ============================================================================

static void BM_RedundancyDetection_SimilarityBased(benchmark::State& state) {
    SimilarityBasedRedundancyDetector detector;

    std::vector<BaseTensorSummary> summaries;
    summaries.reserve(50);
    for (int i = 0; i < 50; ++i) {
        BaseTensorSummary s;
        s.id = "adapter:" + std::to_string(i);
        s.similarity_score = 0.5f + static_cast<float>(i) * 0.01f;
        summaries.push_back(std::move(s));
    }
    std::vector<const BaseTensorSummary*> ptrs;
    ptrs.reserve(summaries.size());
    for (const auto& s : summaries) {
        ptrs.push_back(&s);
    }

    for (auto _ : state) {
        auto metrics = detector.detect(ptrs, 0.9f);
        benchmark::DoNotOptimize(metrics);
    }
}
BENCHMARK(BM_RedundancyDetection_SimilarityBased)->Unit(benchmark::kMicrosecond);

}  // namespace themis::tensor::bench

BENCHMARK_MAIN();
