/**
 * @file bench_embedding_cache_performance.cpp
 * @brief Real Google Benchmark performance tests for embedding cache
 * 
 * Tests embedding cache performance with:
 * - Cache hit/miss performance
 * - Vector similarity search (HNSW)
 * - Various batch sizes (1, 10, 100, 1000)
 * - Cache eviction strategies
 * - Memory usage tracking
 * - Cost savings analysis
 * 
 * Output: JSON format for CI regression tracking
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "cache/embedding_cache.h"
#include <vector>
#include <string>
#include <random>
#include <cmath>

using namespace themis;

namespace {

// ═══════════════════════════════════════════════════════════
// Test Data Generation
// ═══════════════════════════════════════════════════════════

std::vector<float> generateRandomEmbedding(size_t dim, std::mt19937& rng) {
    std::vector<float> embedding(dim);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    for (size_t i = 0; i < dim; ++i) {
        embedding[i] = dist(rng);
    }
    
    // Normalize to unit length
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 0.0f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}

std::vector<std::vector<float>> generateEmbeddingDataset(size_t count, size_t dim) {
    std::vector<std::vector<float>> dataset;
    dataset.reserve(count);
    
    std::mt19937 rng(42);
    for (size_t i = 0; i < count; ++i) {
        dataset.push_back(generateRandomEmbedding(dim, rng));
    }
    
    return dataset;
}

// ═══════════════════════════════════════════════════════════
// Cache Configuration Helpers
// ═══════════════════════════════════════════════════════════

EmbeddingCache::Config createCacheConfig(size_t max_entries, size_t dim, bool use_index) {
    EmbeddingCache::Config cfg;
    cfg.max_entries = max_entries;
    cfg.embedding_dim = dim;
    cfg.ttl_seconds = 3600;
    cfg.similarity_threshold = 0.95f;
    cfg.use_vector_index = use_index;
    cfg.cache_dir = "/tmp/themis_bench_cache";
    return cfg;
}

// ═══════════════════════════════════════════════════════════
// Cache Store Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: Store embeddings without vector index
 * Target: >10K stores/sec
 */
static void BM_EmbeddingCache_Store_NoIndex(benchmark::State& state) {
    size_t dim = 1536; // OpenAI ada-002
    auto config = createCacheConfig(100000, dim, false);
    EmbeddingCache cache(config);
    
    std::mt19937 rng(42);
    int counter = 0;
    
    for (auto _ : state) {
        auto embedding = generateRandomEmbedding(dim, rng);
        std::string query = "query_" + std::to_string(counter++);
        
        bool success = cache.store(query, embedding, "{}");
        benchmark::DoNotOptimize(success);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("store_no_index");
}
BENCHMARK(BM_EmbeddingCache_Store_NoIndex);

/**
 * Optimized: Store embeddings with HNSW index
 * Target: >5K stores/sec (acceptable trade-off for fast queries)
 */
static void BM_EmbeddingCache_Store_WithIndex(benchmark::State& state) {
    size_t dim = 1536; // OpenAI ada-002
    auto config = createCacheConfig(100000, dim, true);
    EmbeddingCache cache(config);
    
    std::mt19937 rng(42);
    int counter = 0;
    
    for (auto _ : state) {
        auto embedding = generateRandomEmbedding(dim, rng);
        std::string query = "query_" + std::to_string(counter++);
        
        bool success = cache.store(query, embedding, "{}");
        benchmark::DoNotOptimize(success);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("store_with_index");
}
BENCHMARK(BM_EmbeddingCache_Store_WithIndex);

// ═══════════════════════════════════════════════════════════
// Cache Query Benchmarks - Hit Rate
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: Query with brute-force similarity (no index)
 * Target: >1K queries/sec for 10K cache entries
 */
static void BM_EmbeddingCache_Query_NoIndex(benchmark::State& state) {
    size_t dim = 1536;
    size_t cache_size = state.range(0);
    
    auto config = createCacheConfig(cache_size * 2, dim, false);
    EmbeddingCache cache(config);
    
    // Populate cache
    auto embeddings = generateEmbeddingDataset(cache_size, dim);
    for (size_t i = 0; i < embeddings.size(); ++i) {
        cache.store("query_" + std::to_string(i), embeddings[i], "{}");
    }
    
    // Query with similar embeddings
    std::mt19937 rng(123);
    size_t query_idx = 0;
    
    for (auto _ : state) {
        // Query a cached embedding (should hit)
        auto result = cache.query(embeddings[query_idx % embeddings.size()]);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
        query_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("query_no_index");
}
BENCHMARK(BM_EmbeddingCache_Query_NoIndex)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000);

/**
 * Optimized: Query with HNSW index
 * Target: >100K queries/sec (100x improvement)
 */
static void BM_EmbeddingCache_Query_WithIndex(benchmark::State& state) {
    size_t dim = 1536;
    size_t cache_size = state.range(0);
    
    auto config = createCacheConfig(cache_size * 2, dim, true);
    EmbeddingCache cache(config);
    
    // Populate cache
    auto embeddings = generateEmbeddingDataset(cache_size, dim);
    for (size_t i = 0; i < embeddings.size(); ++i) {
        cache.store("query_" + std::to_string(i), embeddings[i], "{}");
    }
    
    // Query with similar embeddings
    size_t query_idx = 0;
    
    for (auto _ : state) {
        // Query a cached embedding (should hit)
        auto result = cache.query(embeddings[query_idx % embeddings.size()]);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
        query_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("query_with_index");
}
BENCHMARK(BM_EmbeddingCache_Query_WithIndex)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000)
    ->Arg(100000);

// ═══════════════════════════════════════════════════════════
// Cache Miss Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Query miss rate (querying non-cached embeddings)
 * Target: Similar to hit rate but with negative result
 */
static void BM_EmbeddingCache_Query_Miss(benchmark::State& state) {
    size_t dim = 1536;
    size_t cache_size = 10000;
    
    auto config = createCacheConfig(cache_size * 2, dim, true);
    EmbeddingCache cache(config);
    
    // Populate cache
    auto cached_embeddings = generateEmbeddingDataset(cache_size, dim);
    for (size_t i = 0; i < cached_embeddings.size(); ++i) {
        cache.store("query_" + std::to_string(i), cached_embeddings[i], "{}");
    }
    
    // Generate different embeddings for queries (will miss)
    std::mt19937 rng(999);
    
    for (auto _ : state) {
        auto query_embedding = generateRandomEmbedding(dim, rng);
        auto result = cache.query(query_embedding);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("query_miss");
}
BENCHMARK(BM_EmbeddingCache_Query_Miss);

// ═══════════════════════════════════════════════════════════
// Batch Operations
// ═══════════════════════════════════════════════════════════

/**
 * Batch store operations
 * Target: Linear scalability with batch size
 */
static void BM_EmbeddingCache_BatchStore(benchmark::State& state) {
    size_t dim = 1536;
    size_t batch_size = state.range(0);
    
    auto config = createCacheConfig(100000, dim, true);
    
    for (auto _ : state) {
        state.PauseTiming();
        EmbeddingCache cache(config);
        auto embeddings = generateEmbeddingDataset(batch_size, dim);
        state.ResumeTiming();
        
        for (size_t i = 0; i < embeddings.size(); ++i) {
            cache.store("batch_" + std::to_string(i), embeddings[i], "{}");
        }
        
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetLabel("batch_store");
}
BENCHMARK(BM_EmbeddingCache_BatchStore)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

/**
 * Batch query operations
 * Target: Linear scalability with batch size
 */
static void BM_EmbeddingCache_BatchQuery(benchmark::State& state) {
    size_t dim = 1536;
    size_t cache_size = 10000;
    size_t batch_size = state.range(0);
    
    auto config = createCacheConfig(cache_size * 2, dim, true);
    EmbeddingCache cache(config);
    
    // Populate cache
    auto cached_embeddings = generateEmbeddingDataset(cache_size, dim);
    for (size_t i = 0; i < cached_embeddings.size(); ++i) {
        cache.store("query_" + std::to_string(i), cached_embeddings[i], "{}");
    }
    
    // Generate query batch
    auto query_batch = generateEmbeddingDataset(batch_size, dim);
    
    for (auto _ : state) {
        for (const auto& query_emb : query_batch) {
            auto result = cache.query(query_emb);
            benchmark::DoNotOptimize(result.has_value());
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetLabel("batch_query");
}
BENCHMARK(BM_EmbeddingCache_BatchQuery)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

// ═══════════════════════════════════════════════════════════
// Cache Eviction Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Cache eviction when full (LRU)
 * Target: Minimal performance degradation
 */
static void BM_EmbeddingCache_Eviction(benchmark::State& state) {
    size_t dim = 1536;
    size_t max_entries = 1000;
    
    auto config = createCacheConfig(max_entries, dim, true);
    EmbeddingCache cache(config);
    
    // Fill cache to capacity
    auto embeddings = generateEmbeddingDataset(max_entries, dim);
    for (size_t i = 0; i < embeddings.size(); ++i) {
        cache.store("init_" + std::to_string(i), embeddings[i], "{}");
    }
    
    std::mt19937 rng(789);
    int counter = max_entries;
    
    for (auto _ : state) {
        // Store new embedding (will trigger eviction)
        auto new_embedding = generateRandomEmbedding(dim, rng);
        bool success = cache.store("new_" + std::to_string(counter++), new_embedding, "{}");
        benchmark::DoNotOptimize(success);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("eviction");
}
BENCHMARK(BM_EmbeddingCache_Eviction);

// ═══════════════════════════════════════════════════════════
// Similarity Threshold Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Query with different similarity thresholds
 * Target: Threshold doesn't significantly affect query time
 */
static void BM_EmbeddingCache_SimilarityThreshold(benchmark::State& state) {
    size_t dim = 1536;
    size_t cache_size = 10000;
    float threshold = static_cast<float>(state.range(0)) / 100.0f;
    
    auto config = createCacheConfig(cache_size * 2, dim, true);
    config.similarity_threshold = threshold;
    EmbeddingCache cache(config);
    
    // Populate cache
    auto embeddings = generateEmbeddingDataset(cache_size, dim);
    for (size_t i = 0; i < embeddings.size(); ++i) {
        cache.store("query_" + std::to_string(i), embeddings[i], "{}");
    }
    
    size_t query_idx = 0;
    
    for (auto _ : state) {
        auto result = cache.query(embeddings[query_idx % embeddings.size()]);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
        query_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
    std::string label = "threshold_" + std::to_string(state.range(0));
    state.SetLabel(label);
}
BENCHMARK(BM_EmbeddingCache_SimilarityThreshold)
    ->Arg(90)  // 0.90 threshold
    ->Arg(95)  // 0.95 threshold
    ->Arg(99); // 0.99 threshold

// ═══════════════════════════════════════════════════════════
// Memory Usage Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Measure memory usage per entry
 * Target: Track memory overhead
 */
static void BM_EmbeddingCache_MemoryUsage(benchmark::State& state) {
    size_t dim = 1536;
    size_t num_entries = state.range(0);
    
    auto config = createCacheConfig(num_entries * 2, dim, true);
    
    for (auto _ : state) {
        state.PauseTiming();
        EmbeddingCache cache(config);
        auto embeddings = generateEmbeddingDataset(num_entries, dim);
        state.ResumeTiming();
        
        for (size_t i = 0; i < embeddings.size(); ++i) {
            cache.store("mem_" + std::to_string(i), embeddings[i], "{}");
        }
        
        benchmark::ClobberMemory();
    }
    
    // Estimate memory per entry: embedding (dim * 4 bytes) + overhead
    size_t bytes_per_entry = dim * sizeof(float) + 256; // ~256 bytes overhead
    state.SetBytesProcessed(state.iterations() * num_entries * bytes_per_entry);
    state.SetLabel("memory_usage");
}
BENCHMARK(BM_EmbeddingCache_MemoryUsage)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

// ═══════════════════════════════════════════════════════════
// Cost Savings Analysis
// ═══════════════════════════════════════════════════════════

/**
 * Simulated cost savings calculation
 * Target: Demonstrate cache effectiveness
 */
static void BM_EmbeddingCache_CostSavings(benchmark::State& state) {
    size_t dim = 1536;
    size_t cache_size = 10000;
    double hit_rate = static_cast<double>(state.range(0)) / 100.0;
    
    auto config = createCacheConfig(cache_size * 2, dim, true);
    EmbeddingCache cache(config);
    
    // Populate cache
    auto embeddings = generateEmbeddingDataset(cache_size, dim);
    for (size_t i = 0; i < embeddings.size(); ++i) {
        cache.store("query_" + std::to_string(i), embeddings[i], "{}");
    }
    
    // Simulate queries with target hit rate
    std::mt19937 rng(555);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    size_t hits = 0;
    size_t misses = 0;
    
    for (auto _ : state) {
        if (dist(rng) < hit_rate) {
            // Cache hit
            auto result = cache.query(embeddings[hits % embeddings.size()]);
            benchmark::DoNotOptimize(result.has_value());
            hits++;
        } else {
            // Cache miss
            auto new_emb = generateRandomEmbedding(dim, rng);
            auto result = cache.query(new_emb);
            benchmark::DoNotOptimize(result.has_value());
            misses++;
        }
        benchmark::ClobberMemory();
    }
    
    // Cost per OpenAI API call: ~$0.0001 per embedding
    double cost_per_call = 0.0001;
    double savings = hits * cost_per_call;
    
    state.SetItemsProcessed(state.iterations());
    std::string label = "hit_rate_" + std::to_string(state.range(0)) + 
                        "_savings_$" + std::to_string(savings);
    state.SetLabel(label);
}
BENCHMARK(BM_EmbeddingCache_CostSavings)
    ->Arg(50)  // 50% hit rate
    ->Arg(70)  // 70% hit rate
    ->Arg(90); // 90% hit rate

} // namespace

// ═══════════════════════════════════════════════════════════
// Main - Configure JSON output for CI
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();
