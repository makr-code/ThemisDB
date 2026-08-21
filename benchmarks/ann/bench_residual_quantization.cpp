/**
 * @file bench_residual_quantization.cpp
 * @brief Google Benchmark for Residual Quantization (Issue #914)
 * 
 * Benchmarks multi-stage residual quantization for high-accuracy compression
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <vector>
#include <cmath>

#include "index/residual_quantizer.h"

using themis::ResidualQuantizer;

namespace {

struct VectorGenerator {
    std::mt19937 rng;
    std::normal_distribution<float> dist;
    
    VectorGenerator(uint32_t seed = 42) : rng(seed), dist(0.0f, 1.0f) {}
    
    std::vector<float> generate(int dimension) {
        std::vector<float> vec(dimension);
        for (int i = 0; i < dimension; ++i) {
            vec[i] = dist(rng);
        }
        
        float norm = 0.0f;
        for (float v : vec) norm += v * v;
        norm = std::sqrt(std::max(norm, 1e-12f));
        for (float& v : vec) v /= norm;
        
        return vec;
    }
    
    std::vector<std::vector<float>> generateDataset(int num_vectors, int dimension) {
        std::vector<std::vector<float>> dataset;
        dataset.reserve(num_vectors);
        for (int i = 0; i < num_vectors; ++i) {
            dataset.push_back(generate(dimension));
        }
        return dataset;
    }
};

struct ResidualQuantizationEnv {
    int dimension = 1536;
    std::vector<std::vector<float>> training_data;
    std::shared_ptr<ResidualQuantizer> quantizer_2stage;
    std::shared_ptr<ResidualQuantizer> quantizer_3stage;
    bool initialized = false;
    
    static ResidualQuantizationEnv& instance() {
        static ResidualQuantizationEnv env;
        return env;
    }
    
    void initOnce(int num_training_vectors = 10000) {
        if (initialized) return;
        
        VectorGenerator gen(42);
        training_data = gen.generateDataset(num_training_vectors, dimension);
        
        // 2-stage quantizer
        ResidualQuantizer::Config config2;
        config2.num_stages = 2;
        config2.num_subquantizers = 8;
        quantizer_2stage = std::make_shared<ResidualQuantizer>(dimension, config2);
        quantizer_2stage->train(training_data);
        
        // 3-stage quantizer
        ResidualQuantizer::Config config3;
        config3.num_stages = 3;
        config3.num_subquantizers = 8;
        quantizer_3stage = std::make_shared<ResidualQuantizer>(dimension, config3);
        quantizer_3stage->train(training_data);
        
        initialized = true;
    }
};

} // namespace

// =============================================================================
// Training Benchmarks
// =============================================================================

static void BM_ResidualQuant_Training(benchmark::State& state) {
    const int dim = static_cast<int>(state.range(0));
    const int num_training = static_cast<int>(state.range(1));
    const int num_stages = static_cast<int>(state.range(2));
    
    VectorGenerator gen(42);
    auto training_data = gen.generateDataset(num_training, dim);
    
    for (auto _ : state) {
        ResidualQuantizer::Config config;
        config.num_stages = num_stages;
        config.num_subquantizers = 8;
        ResidualQuantizer rq(dim, config);
        
        benchmark::DoNotOptimize(rq.train(training_data));
    }
    
    state.SetItemsProcessed(state.iterations() * num_training);
}

BENCHMARK(BM_ResidualQuant_Training)
    ->Args({128, 1000, 2})
    ->Args({512, 1000, 2})
    ->Args({1536, 1000, 2})
    ->Args({1536, 10000, 2})
    ->Args({1536, 1000, 3})
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Encoding/Decoding Benchmarks
// =============================================================================

static void BM_ResidualQuant_Encode_2stage(benchmark::State& state) {
    auto& env = ResidualQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    
    for (auto _ : state) {
        auto codes = env.quantizer_2stage->encode(query);
        benchmark::DoNotOptimize(codes);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ResidualQuant_Encode_2stage)->Unit(benchmark::kMicrosecond);

static void BM_ResidualQuant_Encode_3stage(benchmark::State& state) {
    auto& env = ResidualQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    
    for (auto _ : state) {
        auto codes = env.quantizer_3stage->encode(query);
        benchmark::DoNotOptimize(codes);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ResidualQuant_Encode_3stage)->Unit(benchmark::kMicrosecond);

static void BM_ResidualQuant_Decode_2stage(benchmark::State& state) {
    auto& env = ResidualQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto codes = env.quantizer_2stage->encode(query);
    
    for (auto _ : state) {
        auto decoded = env.quantizer_2stage->decode(codes);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ResidualQuant_Decode_2stage)->Unit(benchmark::kMicrosecond);

static void BM_ResidualQuant_Decode_3stage(benchmark::State& state) {
    auto& env = ResidualQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto codes = env.quantizer_3stage->encode(query);
    
    for (auto _ : state) {
        auto decoded = env.quantizer_3stage->decode(codes);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ResidualQuant_Decode_3stage)->Unit(benchmark::kMicrosecond);

// =============================================================================
// Distance Computation
// =============================================================================

static void BM_ResidualQuant_AsymmetricDistance_2stage(benchmark::State& state) {
    auto& env = ResidualQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto db_vec = gen.generate(env.dimension);
    auto db_codes = env.quantizer_2stage->encode(db_vec);
    
    for (auto _ : state) {
        float dist = env.quantizer_2stage->asymmetricDistance(query, db_codes);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ResidualQuant_AsymmetricDistance_2stage)->Unit(benchmark::kMicrosecond);

static void BM_ResidualQuant_AsymmetricDistance_3stage(benchmark::State& state) {
    auto& env = ResidualQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto db_vec = gen.generate(env.dimension);
    auto db_codes = env.quantizer_3stage->encode(db_vec);
    
    for (auto _ : state) {
        float dist = env.quantizer_3stage->asymmetricDistance(query, db_codes);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ResidualQuant_AsymmetricDistance_3stage)->Unit(benchmark::kMicrosecond);

// =============================================================================
// End-to-End Search
// =============================================================================

static void BM_ResidualQuant_EndToEnd(benchmark::State& state) {
    const int db_size = static_cast<int>(state.range(0));
    const int k = static_cast<int>(state.range(1));
    const int num_stages = static_cast<int>(state.range(2));
    
    auto& env = ResidualQuantizationEnv::instance();
    env.initOnce();
    
    auto quantizer = (num_stages == 2) ? env.quantizer_2stage : env.quantizer_3stage;
    
    VectorGenerator gen(42);
    
    // Pre-encode database
    std::vector<std::vector<uint8_t>> db_codes;
    db_codes.reserve(db_size);
    for (int i = 0; i < db_size; ++i) {
        auto vec = gen.generate(env.dimension);
        db_codes.push_back(quantizer->encode(vec));
    }
    
    for (auto _ : state) {
        auto query = gen.generate(env.dimension);
        
        std::vector<std::pair<float, int>> distances;
        distances.reserve(db_size);
        
        for (int i = 0; i < db_size; ++i) {
            float dist = quantizer->asymmetricDistance(query, db_codes[i]);
            distances.push_back({dist, i});
        }
        
        std::partial_sort(distances.begin(), 
                         distances.begin() + std::min(k, db_size),
                         distances.end());
        
        benchmark::DoNotOptimize(distances);
    }
    
    state.SetItemsProcessed(state.iterations() * db_size);
}

BENCHMARK(BM_ResidualQuant_EndToEnd)
    ->Args({1000, 10, 2})
    ->Args({10000, 10, 2})
    ->Args({10000, 10, 3})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
