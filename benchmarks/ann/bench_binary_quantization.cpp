/**
 * @file bench_binary_quantization.cpp
 * @brief Google Benchmark for Binary Quantization (Issue #914)
 * 
 * Benchmarks binary quantization for maximum vector compression:
 * - Training performance with various dataset sizes
 * - Encode/decode throughput
 * - Hamming distance computation speed
 * - Asymmetric distance computation
 * - Memory compression ratio validation
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <vector>
#include <cmath>

#include "index/binary_quantizer.h"

using themis::BinaryQuantizer;

namespace {

/**
 * @brief Utility for generating random normalized vectors
 */
struct VectorGenerator {
    std::mt19937 rng;
    std::normal_distribution<float> dist;
    
    VectorGenerator(uint32_t seed = 42) : rng(seed), dist(0.0f, 1.0f) {}
    
    std::vector<float> generate(int dimension) {
        std::vector<float> vec(dimension);
        for (int i = 0; i < dimension; ++i) {
            vec[i] = dist(rng);
        }
        
        // L2 normalize
        float norm = 0.0f;
        for (float v : vec) {
            norm += v * v;
        }
        norm = std::sqrt(std::max(norm, 1e-12f));
        
        for (float& v : vec) {
            v /= norm;
        }
        
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

/**
 * @brief Shared environment for benchmarks
 */
struct BinaryQuantizationEnv {
    int dimension = 1536;  // OpenAI ada-002 dimension
    std::vector<std::vector<float>> training_data;
    std::shared_ptr<BinaryQuantizer> quantizer;
    bool initialized = false;
    
    static BinaryQuantizationEnv& instance() {
        static BinaryQuantizationEnv env;
        return env;
    }
    
    void initOnce(int num_training_vectors = 10000) {
        if (initialized) {
          return;
        }
        
        VectorGenerator gen(42);
        training_data = gen.generateDataset(num_training_vectors, dimension);
        
        BinaryQuantizer::Config config;
        config.center_values = true;
        
        quantizer = std::make_shared<BinaryQuantizer>(dimension, config);
        auto status = quantizer->train(training_data);
        
        if (!status.ok) {
            throw std::runtime_error("Training failed: " + status.message);
        }
        
        initialized = true;
    }
};

} // namespace

// =============================================================================
// Training Benchmarks
// =============================================================================

/**
 * @brief Benchmark quantizer training with different dataset sizes
 * Args: {dimension, num_training_vectors}
 */
static void BM_BinaryQuant_Training(benchmark::State& state) {
    const int dim = static_cast<int>(state.range(0));
    const int num_training = static_cast<int>(state.range(1));
    
    // Pre-generate training data
    VectorGenerator gen(42);
    auto training_data = gen.generateDataset(num_training, dim);
    
    for (auto _ : state) {
        BinaryQuantizer::Config config;
        config.center_values = true;
        BinaryQuantizer bq(dim, config);
        
        benchmark::DoNotOptimize(bq.train(training_data));
    }
    
    state.SetItemsProcessed(state.iterations() * num_training);
    state.SetBytesProcessed(state.iterations() * num_training * dim * sizeof(float));
}

BENCHMARK(BM_BinaryQuant_Training)
    ->Args({128, 1000})
    ->Args({512, 1000})
    ->Args({1536, 1000})
    ->Args({1536, 10000})
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Encoding Benchmarks
// =============================================================================

/**
 * @brief Benchmark encoding performance (single vector)
 */
static void BM_BinaryQuant_Encode(benchmark::State& state) {
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    
    for (auto _ : state) {
        auto codes = env.quantizer->encode(query);
        benchmark::DoNotOptimize(codes);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * env.dimension * sizeof(float));
}

BENCHMARK(BM_BinaryQuant_Encode)
    ->Unit(benchmark::kMicrosecond);

/**
 * @brief Benchmark batch encoding
 */
static void BM_BinaryQuant_BatchEncode(benchmark::State& state) {
    const int batch_size = static_cast<int>(state.range(0));
    
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto queries = gen.generateDataset(batch_size, env.dimension);
    
    for (auto _ : state) {
        for (const auto& query : queries) {
            auto codes = env.quantizer->encode(query);
            benchmark::DoNotOptimize(codes);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * batch_size * env.dimension * sizeof(float));
}

BENCHMARK(BM_BinaryQuant_BatchEncode)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Decoding Benchmarks
// =============================================================================

/**
 * @brief Benchmark decoding performance (single vector)
 */
static void BM_BinaryQuant_Decode(benchmark::State& state) {
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto codes = env.quantizer->encode(query);
    
    for (auto _ : state) {
        auto decoded = env.quantizer->decode(codes);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_BinaryQuant_Decode)
    ->Unit(benchmark::kMicrosecond);

// =============================================================================
// Distance Computation Benchmarks
// =============================================================================

/**
 * @brief Benchmark Hamming distance computation
 */
static void BM_BinaryQuant_HammingDistance(benchmark::State& state) {
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto vec_a = gen.generate(env.dimension);
    auto vec_b = gen.generate(env.dimension);
    
    auto codes_a = env.quantizer->encode(vec_a);
    auto codes_b = env.quantizer->encode(vec_b);
    
    for (auto _ : state) {
        float dist = env.quantizer->hammingDistance(codes_a, codes_b);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_BinaryQuant_HammingDistance)
    ->Unit(benchmark::kNanosecond);

/**
 * @brief Benchmark batch Hamming distance (1 query vs N database vectors)
 */
static void BM_BinaryQuant_BatchHammingDistance(benchmark::State& state) {
    const int db_size = static_cast<int>(state.range(0));
    
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query_vec = gen.generate(env.dimension);
    auto query_codes = env.quantizer->encode(query_vec);
    
    // Pre-encode database
    std::vector<std::vector<uint8_t>> db_codes;
    db_codes.reserve(db_size);
    for (int i = 0; i < db_size; ++i) {
        auto vec = gen.generate(env.dimension);
        db_codes.push_back(env.quantizer->encode(vec));
    }
    
    for (auto _ : state) {
        for (const auto& db_code : db_codes) {
            float dist = env.quantizer->hammingDistance(query_codes, db_code);
            benchmark::DoNotOptimize(dist);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * db_size);
}

BENCHMARK(BM_BinaryQuant_BatchHammingDistance)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

/**
 * @brief Benchmark asymmetric distance computation
 */
static void BM_BinaryQuant_AsymmetricDistance(benchmark::State& state) {
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    auto query = gen.generate(env.dimension);
    auto db_vec = gen.generate(env.dimension);
    auto db_codes = env.quantizer->encode(db_vec);
    
    for (auto _ : state) {
        float dist = env.quantizer->asymmetricDistance(query, db_codes);
        benchmark::DoNotOptimize(dist);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_BinaryQuant_AsymmetricDistance)
    ->Unit(benchmark::kMicrosecond);

// =============================================================================
// Compression Benchmarks
// =============================================================================

/**
 * @brief Benchmark compression ratio measurement
 */
static void BM_BinaryQuant_CompressionRatio(benchmark::State& state) {
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    for (auto _ : state) {
        float ratio = env.quantizer->getCompressionRatio();
        benchmark::DoNotOptimize(ratio);
    }
}

BENCHMARK(BM_BinaryQuant_CompressionRatio);

/**
 * @brief End-to-end: encode database, search, decode top-k
 */
static void BM_BinaryQuant_EndToEnd(benchmark::State& state) {
    const int db_size = static_cast<int>(state.range(0));
    const int k = static_cast<int>(state.range(1));
    
    auto& env = BinaryQuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(42);
    
    // Pre-encode database
    std::vector<std::vector<uint8_t>> db_codes;
    db_codes.reserve(db_size);
    for (int i = 0; i < db_size; ++i) {
        auto vec = gen.generate(env.dimension);
        db_codes.push_back(env.quantizer->encode(vec));
    }
    
    for (auto _ : state) {
        // Generate query
        auto query = gen.generate(env.dimension);
        auto query_codes = env.quantizer->encode(query);
        
        // Compute distances
        std::vector<std::pair<float, int>> distances;
        distances.reserve(db_size);
        
        for (int i = 0; i < db_size; ++i) {
            float dist = env.quantizer->hammingDistance(query_codes, db_codes[i]);
            distances.push_back({dist, i});
        }
        
        // Partial sort to get top-k
        std::partial_sort(distances.begin(), 
                         distances.begin() + std::min(k, db_size),
                         distances.end());
        
        benchmark::DoNotOptimize(distances);
    }
    
    state.SetItemsProcessed(state.iterations() * db_size);
}

BENCHMARK(BM_BinaryQuant_EndToEnd)
    ->Args({1000, 10})
    ->Args({10000, 10})
    ->Args({10000, 100})
    ->Unit(benchmark::kMillisecond);

// Main
BENCHMARK_MAIN();
