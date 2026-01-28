/**
 * @file bench_learned_quantization.cpp
 * @brief Google Benchmark for Learned Quantization (Issue #914)
 * 
 * Benchmarks learned quantization with adaptive thresholds
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <vector>
#include <cmath>

#include "index/learned_quantizer.h"

using themis::LearnedQuantizer;

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

struct LearnedQuantizationEnv {
    int dimension = 1536;
    std::vector<std::vector<float>> training_data;
    std::shared_ptr<LearnedQuantizer> quantizer_4bit;
    std::shared_ptr<LearnedQuantizer> quantizer_8bit;
    bool initialized = false;
    
    static LearnedQuantizationEnv& instance() {
        static LearnedQuantizationEnv env;
        return env;
    }
    
    void initOnce(int num_training_vectors = 10000) {
        if (initialized) return;
        
        VectorGenerator gen(42);
        training_data = gen.generateDataset(num_training_vectors, dimension);
        
        // 4-bit quantizer
        LearnedQuantizer::Config config4;
        config4.bits_per_dimension = 4;
        config4.per_dimension = true;
        quantizer_4bit = std::make_shared<LearnedQuantizer>(dimension, config4);
        quantizer_4bit->train(training_data);
        
        // 8-bit quantizer
        LearnedQuantizer::Config config8;
        config8.bits_per_dimension = 8;
        config8.per_dimension = true;
        quantizer_8bit = std::make_shared<LearnedQuantizer>(dimension, config8);
        quantizer_8bit->train(training_data);
        
        initialized = true;
    }
};

} // namespace

// =============================================================================
// Training Benchmarks
// =============================================================================

static void BM_LearnedQuant_Training(benchmark::State& state) {
    const int dim = static_cast<int>(state.range(0));
    const int num_training = static_cast<int>(state.range(1));
    const int bits = static_cast<int>(state.range(2));
    
    VectorGenerator gen(42);
    auto training_data = gen.generateDataset(num_training, dim);
    
    for (auto _ : state) {
        LearnedQuantizer::Config config;
        config.bits_per_dimension = bits;
        config.per_dimension = true;
        LearnedQuantizer lq(dim, config);
        
        benchmark::DoNotOptimize(lq.train(training_data));
    }
    
    state.SetItemsProcessed(state.iterations() * num_training);
}

BENCHMARK(BM_LearnedQuant_Training)
    ->Args({128, 1000, 4})
    ->Args({512, 1000, 4})
    ->Args({1536, 1000, 4})
    ->Args({1536, 10000, 4})
    ->Args({1536, 1000, 8})
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Encoding/Decoding Benchmarks
// =============================================================================

static void BM_LearnedQuant_Encode_4bit(benchmark::State& state) {
    auto& env = LearnedQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    
    for (auto _ : state) {
        auto codes = env.quantizer_4bit->encode(query);
        benchmark::DoNotOptimize(codes);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LearnedQuant_Encode_4bit)->Unit(benchmark::kMicrosecond);

static void BM_LearnedQuant_Encode_8bit(benchmark::State& state) {
    auto& env = LearnedQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    
    for (auto _ : state) {
        auto codes = env.quantizer_8bit->encode(query);
        benchmark::DoNotOptimize(codes);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LearnedQuant_Encode_8bit)->Unit(benchmark::kMicrosecond);

static void BM_LearnedQuant_Decode_4bit(benchmark::State& state) {
    auto& env = LearnedQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto codes = env.quantizer_4bit->encode(query);
    
    for (auto _ : state) {
        auto decoded = env.quantizer_4bit->decode(codes);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LearnedQuant_Decode_4bit)->Unit(benchmark::kMicrosecond);

static void BM_LearnedQuant_Decode_8bit(benchmark::State& state) {
    auto& env = LearnedQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto codes = env.quantizer_8bit->encode(query);
    
    for (auto _ : state) {
        auto decoded = env.quantizer_8bit->decode(codes);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LearnedQuant_Decode_8bit)->Unit(benchmark::kMicrosecond);

// =============================================================================
// Distance Computation
// =============================================================================

static void BM_LearnedQuant_AsymmetricDistance_4bit(benchmark::State& state) {
    auto& env = LearnedQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto db_vec = gen.generate(env.dimension);
    auto db_codes = env.quantizer_4bit->encode(db_vec);
    
    for (auto _ : state) {
        float dist = env.quantizer_4bit->asymmetricDistance(query, db_codes);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LearnedQuant_AsymmetricDistance_4bit)->Unit(benchmark::kMicrosecond);

static void BM_LearnedQuant_AsymmetricDistance_8bit(benchmark::State& state) {
    auto& env = LearnedQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto db_vec = gen.generate(env.dimension);
    auto db_codes = env.quantizer_8bit->encode(db_vec);
    
    for (auto _ : state) {
        float dist = env.quantizer_8bit->asymmetricDistance(query, db_codes);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LearnedQuant_AsymmetricDistance_8bit)->Unit(benchmark::kMicrosecond);

// =============================================================================
// End-to-End Search
// =============================================================================

static void BM_LearnedQuant_EndToEnd(benchmark::State& state) {
    const int db_size = static_cast<int>(state.range(0));
    const int k = static_cast<int>(state.range(1));
    const int bits = static_cast<int>(state.range(2));
    
    auto& env = LearnedQuantizationEnv::instance();
    env.initOnce();
    
    auto quantizer = (bits == 4) ? env.quantizer_4bit : env.quantizer_8bit;
    
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

BENCHMARK(BM_LearnedQuant_EndToEnd)
    ->Args({1000, 10, 4})
    ->Args({10000, 10, 4})
    ->Args({10000, 10, 8})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
