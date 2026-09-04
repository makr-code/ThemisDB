/**
 * @file bench_llm_response_cache.cpp
 * @brief Benchmark for LLM Response Cache with semantic similarity
 * 
 * Tests:
 * - Cache put/get operations
 * - Semantic similarity matching
 * - Cache hit rate
 * - Performance vs stub implementation
 */

#include <benchmark/benchmark.h>
#include "llm/llm_response_cache.h"
#include <random>
#include <sstream>
#include <mutex>

using namespace themis::llm;

// Helper function to generate random prompts
std::string generatePrompt(int id, int variation = 0) {
    std::ostringstream oss = {};
    oss << "What is the capital of France? (variation " << variation << " - id " << id << ")";
    return oss.str();
}

// Helper function to create response
InferenceResponse createResponse(const std::string& text) {
    InferenceResponse response;
    response.text = text;
    response.tokens_generated = 50;
    response.inference_time_ms = 150.0f;
    response.model_id = "test-model";
    response.model_used = "test-model";
    return response;
}

// Benchmark: Cache Put Operation
static void BM_CachePut(benchmark::State& state) {
    LLMResponseCache::Config config;
    config.similarity_threshold = 0.90f;
    config.ttl_seconds = 3600;
    config.max_entries = 10000;
    config.cache_dir = "/tmp/bench_llm_cache_put";
    
    LLMResponseCache cache("bench_cache", config);
    
    int id = 0;
    for (auto _ : state) {
        auto prompt = generatePrompt(id++);
        auto response = createResponse("Paris is the capital of France.");
        cache.put(prompt, response);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CachePut);

// Benchmark: Cache Get - Hit (Exact Match)
static void BM_CacheGetExactHit(benchmark::State& state) {
    LLMResponseCache::Config config;
    config.similarity_threshold = 0.90f;
    config.cache_dir = "/tmp/bench_llm_cache_get_exact";
    
    LLMResponseCache cache("bench_cache", config);
    
    // Populate cache
    for (int i = 0; i < 100; ++i) {
        auto prompt = generatePrompt(i);
        auto response = createResponse("Paris is the capital of France.");
        cache.put(prompt, response);
    }
    
    int id = 0;
    for (auto _ : state) {
        auto prompt = generatePrompt(id++ % 100);
        auto result = cache.get(prompt);
        benchmark::DoNotOptimize(result);
    }
    
    auto stats = cache.getStatistics();
    state.counters["hit_rate"] = stats.getHitRate();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CacheGetExactHit);

// Benchmark: Cache Get - Semantic Match
static void BM_CacheGetSemanticMatch(benchmark::State& state) {
    LLMResponseCache::Config config;
    config.similarity_threshold = 0.85f;  // Lower threshold for semantic matching
    config.cache_dir = "/tmp/bench_llm_cache_get_semantic";
    
    LLMResponseCache cache("bench_cache", config);
    
    // Populate cache with base prompts
    for (int i = 0; i < 100; ++i) {
        auto prompt = generatePrompt(i, 0);
        auto response = createResponse("Paris is the capital of France.");
        cache.put(prompt, response);
    }
    
    int id = 0;
    for (auto _ : state) {
        // Query with variations that should match semantically
        auto prompt = generatePrompt(id++ % 100, (id / 100) + 1);
        auto result = cache.get(prompt);
        benchmark::DoNotOptimize(result);
    }
    
    auto stats = cache.getStatistics();
    state.counters["hit_rate"] = stats.getHitRate();
    state.counters["hits"] = static_cast<double>(stats.hits.load(std::memory_order_relaxed));
    state.counters["misses"] = static_cast<double>(stats.misses.load(std::memory_order_relaxed));
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CacheGetSemanticMatch);

// Benchmark: Cache Get - Miss
static void BM_CacheGetMiss(benchmark::State& state) {
    LLMResponseCache::Config config;
    config.cache_dir = "/tmp/bench_llm_cache_get_miss";
    
    LLMResponseCache cache("bench_cache", config);
    
    // Populate cache with different prompts
    for (int i = 0; i < 100; ++i) {
        auto prompt = "Cached prompt " + std::to_string(i);
        auto response = createResponse("Response " + std::to_string(i));
        cache.put(prompt, response);
    }
    
    int id = 0;
    for (auto _ : state) {
        // Query with completely different prompts
        auto prompt = "Uncached prompt " + std::to_string(id++);
        auto result = cache.get(prompt);
        benchmark::DoNotOptimize(result);
    }
    
    auto stats = cache.getStatistics();
    state.counters["hit_rate"] = stats.getHitRate();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CacheGetMiss);

// Benchmark: Cache Mixed Operations (realistic workload)
static void BM_CacheMixedWorkload(benchmark::State& state) {
    LLMResponseCache::Config config;
    config.similarity_threshold = 0.90f;
    config.cache_dir = "/tmp/bench_llm_cache_mixed";
    
    LLMResponseCache cache("bench_cache", config);
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<> dist(0, 999);
    
    int operation = 0;
    for (auto _ : state) {
        int id = dist(rng);
        
        // 70% reads, 30% writes (typical cache pattern)
        if (operation++ % 10 < 7) {
            auto prompt = generatePrompt(id);
            auto result = cache.get(prompt);
            benchmark::DoNotOptimize(result);
        } else {
            auto prompt = generatePrompt(id);
            auto response = createResponse("Response for id " + std::to_string(id));
            cache.put(prompt, response);
        }
    }
    
    auto stats = cache.getStatistics();
    state.counters["hit_rate"] = stats.getHitRate();
    state.counters["total_entries"] = static_cast<double>(stats.total_entries.load(std::memory_order_relaxed));
    state.counters["avg_lookup_ms"] = stats.avg_lookup_time_ms.load(std::memory_order_relaxed);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CacheMixedWorkload);

// Benchmark: LRU Eviction Performance
static void BM_CacheLRUEviction(benchmark::State& state) {
    LLMResponseCache::Config config;
    config.max_entries = 100;  // Small cache to trigger evictions
    config.cache_dir = "/tmp/bench_llm_cache_eviction";
    
    LLMResponseCache cache("bench_cache", config);
    
    int id = 0;
    for (auto _ : state) {
        auto prompt = generatePrompt(id++);
        auto response = createResponse("Response");
        cache.put(prompt, response);
    }
    
    auto stats = cache.getStatistics();
    state.counters["final_entries"] = static_cast<double>(stats.total_entries.load(std::memory_order_relaxed));
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CacheLRUEviction);

// Benchmark: Concurrent Access (multithreaded)
static void BM_CacheConcurrentAccess(benchmark::State& state) {
    static LLMResponseCache::Config config;
    config.cache_dir = "/tmp/bench_llm_cache_concurrent";
    static LLMResponseCache cache("bench_cache", config);
    
    // Pre-populate
    static std::once_flag flag;
    std::call_once(flag, []() {
        for (int i = 0; i < 1000; ++i) {
            auto prompt = generatePrompt(i);
            auto response = createResponse("Response " + std::to_string(i));
            cache.put(prompt, response);
        }
    });
    
    std::mt19937 rng(state.thread_index());
    std::uniform_int_distribution<> dist(0, 999);
    
    for (auto _ : state) {
        int id = dist(rng);
        auto prompt = generatePrompt(id);
        auto result = cache.get(prompt);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CacheConcurrentAccess)->Threads(1)->Threads(2)->Threads(4)->Threads(8);

BENCHMARK_MAIN();
