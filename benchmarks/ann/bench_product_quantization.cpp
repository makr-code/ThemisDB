/**
 * @file bench_product_quantization.cpp
 * @brief Google Benchmark for Product Quantization (Feature #7)
 * 
 * Benchmarks vector compression using Product Quantization:
 * - Training performance with various dataset sizes
 * - Encode/decode throughput
 * - Asymmetric distance computation speed
 * - Memory compression ratio validation
 * - End-to-end compression pipeline
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <vector>
#include <cmath>

#include "index/product_quantizer.h"

using themis::ProductQuantizer;

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
        
        // L2 normalize for stable distances
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
struct QuantizationEnv {
    int dimension = 1536;  // OpenAI ada-002 dimension
    int num_subquantizers = 8;
    std::vector<std::vector<float>> training_data;
    std::shared_ptr<ProductQuantizer> quantizer;
    bool initialized = false;
    
    static QuantizationEnv& instance() {
        static QuantizationEnv env;
        return env;
    }
    
    void initOnce(int num_training_vectors = 10000) {
        if (initialized) {
          return;
        }
        
        VectorGenerator gen(42);
        training_data = gen.generateDataset(num_training_vectors, dimension);
        
        ProductQuantizer::Config config;
        config.num_subquantizers = num_subquantizers;
        config.max_iterations = 25;
        
        quantizer = std::make_shared<ProductQuantizer>(dimension, config);
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
 * Args: {dimension, num_training_vectors, num_subquantizers}
 */
static void BM_PQ_Training(benchmark::State& state) {
    const int dim = static_cast<int>(state.range(0));
    const int num_training = static_cast<int>(state.range(1));
    const int num_subq = static_cast<int>(state.range(2));
    
    // Pre-generate training data outside the timing loop
    VectorGenerator gen(42);
    auto training_data = gen.generateDataset(num_training, dim);
    
    ProductQuantizer::Config config;
    config.num_subquantizers = num_subq;
    config.max_iterations = 25;
    
    for (auto _ : state) {
        // Pause timing for setup
        state.PauseTiming();
        ProductQuantizer pq(dim, config);
        state.ResumeTiming();
        
        // Measure only the training operation
        auto status = pq.train(training_data);
        
        if (!status.ok) {
            state.SkipWithError(status.message.c_str());
            break;
        }
    }
    
    state.counters["dimension"] = dim;
    state.counters["training_vectors"] = num_training;
    state.counters["subquantizers"] = num_subq;
    state.counters["compression_ratio"] = (dim * sizeof(float)) / 
                                          (static_cast<float>(num_subq) * sizeof(uint8_t));
}

// =============================================================================
// Encode Benchmarks
// =============================================================================

/**
 * @brief Benchmark vector encoding throughput
 * Measures vectors encoded per second
 */
static void BM_PQ_Encode(benchmark::State& state) {
    auto& env = QuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(123);
    std::vector<std::vector<float>> test_vectors = gen.generateDataset(1000, env.dimension);
    
    size_t encoded_count = 0;
    size_t test_idx = 0;
    
    for (auto _ : state) {
        const auto& vec = test_vectors[test_idx % test_vectors.size()];
        auto codes = env.quantizer->encode(vec);
        
        benchmark::DoNotOptimize(codes);
        
        if (codes.empty()) {
            state.SkipWithError("Encoding failed");
            break;
        }
        
        ++encoded_count;
        ++test_idx;
    }
    
    state.counters["vectors_encoded"] = static_cast<double>(encoded_count);
    state.counters["vectors_per_sec"] = benchmark::Counter(
        static_cast<double>(encoded_count),
        benchmark::Counter::kIsRate
    );
}

/**
 * @brief Benchmark batch encoding
 * Args: {batch_size}
 */
static void BM_PQ_EncodeBatch(benchmark::State& state) {
    auto& env = QuantizationEnv::instance();
    env.initOnce();
    
    const int batch_size = static_cast<int>(state.range(0));
    
    VectorGenerator gen(123);
    std::vector<std::vector<float>> test_vectors = gen.generateDataset(batch_size * 10, env.dimension);
    
    size_t encoded_count = 0;
    size_t batch_idx = 0;
    
    for (auto _ : state) {
        std::vector<std::vector<uint8_t>> batch_codes;
        batch_codes.reserve(batch_size);
        
        for (int i = 0; i < batch_size; ++i) {
            const auto& vec = test_vectors[(batch_idx * batch_size + i) % test_vectors.size()];
            auto codes = env.quantizer->encode(vec);
            batch_codes.push_back(std::move(codes));
        }
        
        benchmark::DoNotOptimize(batch_codes);
        
        encoded_count += batch_size;
        ++batch_idx;
    }
    
    state.counters["batch_size"] = batch_size;
    state.counters["vectors_per_sec"] = benchmark::Counter(
        static_cast<double>(encoded_count),
        benchmark::Counter::kIsRate
    );
}

// =============================================================================
// Decode Benchmarks
// =============================================================================

/**
 * @brief Benchmark vector decoding throughput
 */
static void BM_PQ_Decode(benchmark::State& state) {
    auto& env = QuantizationEnv::instance();
    env.initOnce();
    
    // Pre-encode test vectors
    VectorGenerator gen(123);
    std::vector<std::vector<float>> test_vectors = gen.generateDataset(1000, env.dimension);
    std::vector<std::vector<uint8_t>> encoded_vectors;
    encoded_vectors.reserve(test_vectors.size());
    
    for (const auto& vec : test_vectors) {
        encoded_vectors.push_back(env.quantizer->encode(vec));
    }
    
    size_t decoded_count = 0;
    size_t test_idx = 0;
    
    for (auto _ : state) {
        const auto& codes = encoded_vectors[test_idx % encoded_vectors.size()];
        auto decoded = env.quantizer->decode(codes);
        
        benchmark::DoNotOptimize(decoded);
        
        if (decoded.empty()) {
            state.SkipWithError("Decoding failed");
            break;
        }
        
        ++decoded_count;
        ++test_idx;
    }
    
    state.counters["vectors_decoded"] = static_cast<double>(decoded_count);
    state.counters["vectors_per_sec"] = benchmark::Counter(
        static_cast<double>(decoded_count),
        benchmark::Counter::kIsRate
    );
}

// =============================================================================
// Distance Computation Benchmarks
// =============================================================================

/**
 * @brief Benchmark asymmetric distance computation
 * This is the key operation for search without full reconstruction
 */
static void BM_PQ_AsymmetricDistance(benchmark::State& state) {
    auto& env = QuantizationEnv::instance();
    env.initOnce();
    
    // Pre-encode database vectors
    VectorGenerator gen(123);
    std::vector<std::vector<float>> db_vectors = gen.generateDataset(1000, env.dimension);
    std::vector<std::vector<uint8_t>> encoded_db;
    encoded_db.reserve(db_vectors.size());
    
    for (const auto& vec : db_vectors) {
        encoded_db.push_back(env.quantizer->encode(vec));
    }
    
    // Generate query vectors
    std::vector<std::vector<float>> query_vectors = gen.generateDataset(100, env.dimension);
    
    size_t distance_count = 0;
    size_t query_idx = 0;
    size_t db_idx = 0;
    
    for (auto _ : state) {
        const auto& query = query_vectors[query_idx % query_vectors.size()];
        const auto& codes = encoded_db[db_idx % encoded_db.size()];
        
        float dist = env.quantizer->computeAsymmetricDistance(query, codes);
        
        benchmark::DoNotOptimize(dist);
        
        ++distance_count;
        ++db_idx;
        
        if (db_idx % encoded_db.size() == 0) {
            ++query_idx;
        }
    }
    
    state.counters["distances_computed"] = static_cast<double>(distance_count);
    state.counters["distances_per_sec"] = benchmark::Counter(
        static_cast<double>(distance_count),
        benchmark::Counter::kIsRate
    );
}

/**
 * @brief Compare asymmetric distance vs full decode + distance
 */
static void BM_PQ_DistanceComparison(benchmark::State& state) {
    auto& env = QuantizationEnv::instance();
    env.initOnce();
    
    const bool use_asymmetric = state.range(0) != 0;
    
    VectorGenerator gen(123);
    std::vector<std::vector<float>> db_vectors = gen.generateDataset(100, env.dimension);
    std::vector<std::vector<uint8_t>> encoded_db;
    encoded_db.reserve(db_vectors.size());
    
    for (const auto& vec : db_vectors) {
        encoded_db.push_back(env.quantizer->encode(vec));
    }
    
    std::vector<std::vector<float>> query_vectors = gen.generateDataset(10, env.dimension);
    
    size_t distance_count = 0;
    size_t query_idx = 0;
    size_t db_idx = 0;
    
    for (auto _ : state) {
        const auto& query = query_vectors[query_idx % query_vectors.size()];
        const auto& codes = encoded_db[db_idx % encoded_db.size()];
        
        float dist = 0;
        
        if (use_asymmetric) {
            // Fast asymmetric distance
            dist = env.quantizer->computeAsymmetricDistance(query, codes);
        } else {
            // Decode + full L2 distance
            auto decoded = env.quantizer->decode(codes);
            float sum = 0.0f;
            for (size_t i = 0; i < query.size(); ++i) {
                float diff = query[i] - decoded[i];
                sum += diff * diff;
            }
            dist = std::sqrt(sum);
        }
        
        benchmark::DoNotOptimize(dist);
        
        ++distance_count;
        ++db_idx;
        
        if (db_idx % encoded_db.size() == 0) {
            ++query_idx;
        }
    }
    
    state.counters["method"] = use_asymmetric ? 1 : 0;
    state.counters["distances_per_sec"] = benchmark::Counter(
        static_cast<double>(distance_count),
        benchmark::Counter::kIsRate
    );
}

// =============================================================================
// End-to-End Pipeline Benchmarks
// =============================================================================

/**
 * @brief Benchmark complete compression pipeline
 * Measures encode -> store codes -> retrieve codes -> distance computation
 */
static void BM_PQ_E2E_Pipeline(benchmark::State& state) {
    auto& env = QuantizationEnv::instance();
    env.initOnce();
    
    VectorGenerator gen(123);
    
    size_t pipeline_count = 0;
    
    for (auto _ : state) {
        // 1. Generate vector
        auto vec = gen.generate(env.dimension);
        
        // 2. Encode
        auto codes = env.quantizer->encode(vec);
        
        // 3. Simulate storage (copy)
        std::vector<uint8_t> stored_codes = codes;
        
        // 4. Generate query
        auto query = gen.generate(env.dimension);
        
        // 5. Compute distance
        float dist = env.quantizer->computeAsymmetricDistance(query, stored_codes);
        
        benchmark::DoNotOptimize(dist);
        
        ++pipeline_count;
    }
    
    state.counters["pipelines_per_sec"] = benchmark::Counter(
        static_cast<double>(pipeline_count),
        benchmark::Counter::kIsRate
    );
}

// =============================================================================
// Memory Benchmark
// =============================================================================

/**
 * @brief Validate memory compression ratio
 */
static void BM_PQ_MemoryCompression(benchmark::State& state) {
    const int dim = static_cast<int>(state.range(0));
    const int num_subq = static_cast<int>(state.range(1));
    const int num_vectors = 10000;
    
    VectorGenerator gen(42);
    auto training_data = gen.generateDataset(1000, dim);
    
    ProductQuantizer::Config config;
    config.num_subquantizers = num_subq;
    ProductQuantizer pq(dim, config);
    
    auto status = pq.train(training_data);
    if (!status.ok) {
        state.SkipWithError(status.message.c_str());
        return;
    }
    
    size_t original_size = 0;
    size_t compressed_size = 0;
    
    for (auto _ : state) {
        auto test_vectors = gen.generateDataset(num_vectors, dim);
        
        original_size = num_vectors * dim * sizeof(float);
        compressed_size = num_vectors * num_subq * sizeof(uint8_t);
        
        // Encode all vectors
        std::vector<std::vector<uint8_t>> encoded;
        encoded.reserve(num_vectors);
        
        for (const auto& vec : test_vectors) {
            encoded.push_back(pq.encode(vec));
        }
        
        benchmark::DoNotOptimize(encoded);
    }
    
    float compression_ratio = static_cast<float>(original_size) / static_cast<float>(compressed_size);
    
    state.counters["dimension"] = dim;
    state.counters["subquantizers"] = num_subq;
    state.counters["original_mb"] = static_cast<double>(original_size) / (1024.0 * 1024.0);
    state.counters["compressed_mb"] = static_cast<double>(compressed_size) / (1024.0 * 1024.0);
    state.counters["compression_ratio"] = compression_ratio;
}

// =============================================================================
// Register Benchmarks
// =============================================================================

// Training benchmarks
BENCHMARK(BM_PQ_Training)
    ->Args({384, 1000, 8})    // Small: 384D, 1K training, 8 subq
    ->Args({768, 5000, 8})    // Medium: 768D, 5K training, 8 subq
    ->Args({1536, 10000, 8})  // Large: 1536D (OpenAI), 10K training, 8 subq
    ->Args({1536, 10000, 16}) // Large: 1536D, 10K training, 16 subq (higher compression)
    ->Unit(benchmark::kMillisecond);

// Encode benchmarks
BENCHMARK(BM_PQ_Encode)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_PQ_EncodeBatch)
    ->Args({10})
    ->Args({100})
    ->Args({1000})
    ->Unit(benchmark::kMillisecond);

// Decode benchmarks
BENCHMARK(BM_PQ_Decode)
    ->Unit(benchmark::kMicrosecond);

// Distance benchmarks
BENCHMARK(BM_PQ_AsymmetricDistance)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_PQ_DistanceComparison)
    ->Args({0})  // Decode + distance
    ->Args({1})  // Asymmetric distance
    ->Unit(benchmark::kNanosecond);

// Pipeline benchmark
BENCHMARK(BM_PQ_E2E_Pipeline)
    ->Unit(benchmark::kMicrosecond);

// Memory compression validation
BENCHMARK(BM_PQ_MemoryCompression)
    ->Args({384, 8})
    ->Args({768, 8})
    ->Args({1536, 8})
    ->Args({1536, 16})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
