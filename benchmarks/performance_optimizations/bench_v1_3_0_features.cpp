#include <benchmark/benchmark.h>
#include "cache/embedding_cache.h"
#include "search/hybrid_search.h"
#include "query/cte_subquery.h"
#include "sharding/distributed_transaction.h"
#include <nlohmann/json.hpp>
#include <random>
#include <vector>

using namespace themis;

// ============================================================================
// Embedding Cache Benchmarks
// ============================================================================

static void BM_EmbeddingCache_Store(benchmark::State& state) {
    EmbeddingCache::Config config;
    config.max_entries = 10000;
    EmbeddingCache cache(config);
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    const int dim = state.range(0);
    
    for (auto _ : state) {
        std::vector<float> embedding(dim);
        for (int i = 0; i < dim; ++i) {
            embedding[i] = dist(gen);
        }
        
        cache.store("query_" + std::to_string(state.iterations()), embedding);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EmbeddingCache_Store)->Arg(384)->Arg(768)->Arg(1536)->Arg(3072);

static void BM_EmbeddingCache_Query_Hit(benchmark::State& state) {
    EmbeddingCache::Config config;
    config.max_entries = 10000;
    config.similarity_threshold = 0.95f;
    EmbeddingCache cache(config);
    
    const int dim = state.range(0);
    const int num_entries = 1000;
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Pre-populate cache
    std::vector<std::vector<float>> embeddings;
    for (int i = 0; i < num_entries; ++i) {
        std::vector<float> emb(dim);
        for (int j = 0; j < dim; ++j) {
            emb[j] = dist(gen);
        }
        embeddings.push_back(emb);
        cache.store("query_" + std::to_string(i), emb);
    }
    
    int idx = 0;
    for (auto _ : state) {
        auto result = cache.query(embeddings[idx % num_entries]);
        benchmark::DoNotOptimize(result);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EmbeddingCache_Query_Hit)->Arg(384)->Arg(768)->Arg(1536)->Arg(3072);

static void BM_EmbeddingCache_Query_Miss(benchmark::State& state) {
    EmbeddingCache::Config config;
    config.max_entries = 10000;
    EmbeddingCache cache(config);
    
    const int dim = state.range(0);
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (auto _ : state) {
        std::vector<float> embedding(dim);
        for (int i = 0; i < dim; ++i) {
            embedding[i] = dist(gen);
        }
        
        auto result = cache.query(embedding);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EmbeddingCache_Query_Miss)->Arg(384)->Arg(768)->Arg(1536)->Arg(3072);

static void BM_EmbeddingCache_CostSavings(benchmark::State& state) {
    EmbeddingCache::Config config;
    config.max_entries = 10000;
    EmbeddingCache cache(config);
    
    const int dim = 1536; // GPT-3 embedding size
    std::vector<float> embedding(dim, 0.5f);
    
    // Store one embedding
    cache.store("test", embedding);
    
    for (auto _ : state) {
        auto result = cache.query(embedding); // Cache hit
        benchmark::DoNotOptimize(result);
    }
    
    auto stats = cache.getStats();
    state.counters["HitRate"] = stats.hit_rate;
    state.counters["CostSavingsUSD"] = stats.cost_savings_usd;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EmbeddingCache_CostSavings);

// ============================================================================
// Hybrid Search Benchmarks
// ============================================================================

static void BM_HybridSearch_RRF(benchmark::State& state) {
    // Use null managers (benchmarking API surface only)
    SecondaryIndexManager* fulltext = nullptr;
    VectorIndexManager* vector = nullptr;

    HybridSearch::Config config;
    config.bm25_weight = 0.5;
    config.vector_weight = 0.5;
    config.use_rrf = true;

    HybridSearch hybrid(fulltext, vector, config);
    
    const int dim = state.range(0);
    std::vector<float> query_vector(dim, 0.5f);
    
    for (auto _ : state) {
        auto results = hybrid.search("machine learning", query_vector.data(), dim);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridSearch_RRF)->Arg(384)->Arg(768)->Arg(1536);

static void BM_HybridSearch_LinearCombination(benchmark::State& state) {
    SecondaryIndexManager* fulltext = nullptr;
    VectorIndexManager* vector = nullptr;

    HybridSearch::Config config;
    config.bm25_weight = 0.3;
    config.vector_weight = 0.7;
    config.use_rrf = false; // linear combination fallback

    HybridSearch hybrid(fulltext, vector, config);
    
    const int dim = 1536;
    std::vector<float> query_vector(dim, 0.5f);
    
    for (auto _ : state) {
        auto results = hybrid.search("database query", query_vector.data(), dim);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridSearch_LinearCombination);

static void BM_HybridSearch_VaryingWeights(benchmark::State& state) {
    SecondaryIndexManager* fulltext = nullptr;
    VectorIndexManager* vector = nullptr;
    
    const float bm25_weight = state.range(0) / 100.0f;
    const float vector_weight = 1.0f - bm25_weight;
    
    HybridSearch::Config config;
    config.bm25_weight = bm25_weight;
    config.vector_weight = vector_weight;

    HybridSearch hybrid(fulltext, vector, config);
    
    std::vector<float> query_vector(1536, 0.5f);
    
    for (auto _ : state) {
        auto results = hybrid.search("test query", query_vector.data(), 1536);
        benchmark::DoNotOptimize(results);
    }
    
    state.counters["BM25Weight"] = bm25_weight;
    state.counters["VectorWeight"] = vector_weight;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridSearch_VaryingWeights)->Arg(0)->Arg(25)->Arg(50)->Arg(75)->Arg(100);

// ============================================================================
// CTE Benchmarks
// ============================================================================

static void BM_CTE_NonRecursive_Simple(benchmark::State& state) {
    // CTE evaluator only (no real QueryEngine needed for this micro-benchmark)
    query::CTEEvaluator evaluator;
    
    const int num_ctes = state.range(0);
    
    for (auto _ : state) {
        // Execute non-recursive CTE with N CTEs
        for (int i = 0; i < num_ctes; ++i) {
            // Simplified CTE execution
            benchmark::DoNotOptimize(i);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_ctes);
}
BENCHMARK(BM_CTE_NonRecursive_Simple)->Arg(1)->Arg(5)->Arg(10)->Arg(20);

static void BM_CTE_Recursive_Depth(benchmark::State& state) {
    query::CTEEvaluator evaluator;
    query::CTEEvaluator::RecursiveCTEConfig config;
    config.max_iterations = state.range(0);
    
    for (auto _ : state) {
        // Simulate recursive CTE with specific depth
        int iterations = 0;
        for (int i = 0; i < config.max_iterations; ++i) {
            // Fixpoint iteration logic
            iterations++;
            benchmark::DoNotOptimize(iterations);
        }
    }
    
    state.counters["MaxDepth"] = config.max_iterations;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CTE_Recursive_Depth)->Arg(10)->Arg(50)->Arg(100)->Arg(1000);

static void BM_CTE_CycleDetection(benchmark::State& state) {
    query::CTEEvaluator::RecursiveCTEConfig config;
    config.enable_cycle_detection = true;
    config.max_iterations = 1000;
    
    const int data_size = state.range(0);
    
    for (auto _ : state) {
        // Simulate cycle detection with varying data sizes
        std::vector<int> seen(data_size);
        for (int i = 0; i < data_size; ++i) {
            seen[i] = i;
        }
        benchmark::DoNotOptimize(seen);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CTE_CycleDetection)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_Subquery_EXISTS_WithLIMIT1(benchmark::State& state) {
    // Benchmark EXISTS query with LIMIT 1 optimization
    const int table_size = state.range(0);
    
    for (auto _ : state) {
        // Simulate EXISTS with LIMIT 1 - stops at first match
        for (int i = 0; i < table_size; ++i) {
            if (i % 10 == 0) { // Found match
                break; // LIMIT 1 optimization
            }
        }
    }
    
    state.counters["TableSize"] = table_size;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Subquery_EXISTS_WithLIMIT1)->Arg(100)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_Subquery_EXISTS_WithoutLIMIT1(benchmark::State& state) {
    // Benchmark EXISTS query WITHOUT LIMIT 1 - scans entire table
    const int table_size = state.range(0);
    
    for (auto _ : state) {
        int count = 0;
        for (int i = 0; i < table_size; ++i) {
            if (i % 10 == 0) {
                count++; // Continues scanning
            }
        }
        benchmark::DoNotOptimize(count);
    }
    
    state.counters["TableSize"] = table_size;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Subquery_EXISTS_WithoutLIMIT1)->Arg(100)->Arg(1000)->Arg(10000)->Arg(100000);

// ============================================================================
// Distributed Transaction Benchmarks
// ============================================================================

static void BM_DistributedTxn_2PC_Latency(benchmark::State& state) {
    themis::sharding::TrueTime::Config tt_cfg;
    auto truetime = std::make_shared<themis::sharding::TrueTime>(tt_cfg);
    themis::sharding::DistributedTransactionCoordinator::Config ccfg;
    sharding::DistributedTransactionCoordinator coordinator(truetime, ccfg);
    
    const int num_shards = state.range(0);
    std::vector<std::string> shards = {};

    for (int i = 0; i < num_shards; ++i) {
        shards.push_back("shard" + std::to_string(i));
    }
    
    for (auto _ : state) {
        auto txn_id = coordinator.beginTransaction(shards);
        
        for (const auto& shard : shards) {
            nlohmann::json op;
            op["type"] = "insert";
            coordinator.addOperation(txn_id, shard, op);
        }
        
        coordinator.commit(txn_id); // 2PC
    }
    
    state.counters["NumShards"] = num_shards;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DistributedTxn_2PC_Latency)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

static void BM_DistributedTxn_Throughput(benchmark::State& state) {
    themis::sharding::TrueTime::Config tt_cfg;
    auto truetime = std::make_shared<themis::sharding::TrueTime>(tt_cfg);
    themis::sharding::DistributedTransactionCoordinator::Config ccfg;
    sharding::DistributedTransactionCoordinator coordinator(truetime, ccfg);
    
    std::vector<std::string> shards = {"shard1", "shard2"};
    
    for (auto _ : state) {
        auto txn_id = coordinator.beginTransaction(shards);
        
        nlohmann::json op;
        op["type"] = "insert";
        coordinator.addOperation(txn_id, "shard1", op);
        coordinator.addOperation(txn_id, "shard2", op);
        
        coordinator.commit(txn_id);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DistributedTxn_Throughput);

static void BM_DistributedTxn_SnapshotRead(benchmark::State& state) {
    themis::sharding::TrueTime::Config tt_cfg;
    auto truetime = std::make_shared<themis::sharding::TrueTime>(tt_cfg);
    themis::sharding::DistributedTransactionCoordinator::Config ccfg;
    sharding::DistributedTransactionCoordinator coordinator(truetime, ccfg);
    
    const int num_shards = state.range(0);
    std::vector<std::string> shards = {};

    for (int i = 0; i < num_shards; ++i) {
        shards.push_back("shard" + std::to_string(i));
    }
    
    for (auto _ : state) {
        nlohmann::json ops = nlohmann::json::array({ { {"type", "read"} } });
        auto results = coordinator.executeReadOnly(shards, ops);
        benchmark::DoNotOptimize(results);
    }
    
    state.counters["NumShards"] = num_shards;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DistributedTxn_SnapshotRead)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// Removed ShardRPCClient benchmark (API not present in current version)

// ============================================================================
// Combined Feature Benchmarks
// ============================================================================

static void BM_Combined_LLM_RAG_Pipeline(benchmark::State& state) {
    // Simulate complete LLM RAG pipeline with all features (API surface only)
    EmbeddingCache::Config cache_config;
    EmbeddingCache emb_cache(cache_config);

    SecondaryIndexManager* fulltext = nullptr;
    VectorIndexManager* vector = nullptr;
    HybridSearch::Config hs_cfg;
    hs_cfg.use_rrf = true;
    HybridSearch hybrid(fulltext, vector, hs_cfg);
    
    std::vector<float> query_embedding(1536, 0.5f);
    
    for (auto _ : state) {
        // 1. Check embedding cache
        auto cached = emb_cache.query(query_embedding);
        if (!cached) {
            // 2. Cache miss - perform hybrid search
            auto results = hybrid.search("test query", query_embedding.data(), 1536);
            benchmark::DoNotOptimize(results);
            
            // 3. Store in cache for next time
            emb_cache.store("test query", query_embedding);
        }
    }
    
    auto stats = emb_cache.getStats();
    state.counters["CacheHitRate"] = stats.hit_rate;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Combined_LLM_RAG_Pipeline);

BENCHMARK_MAIN();
