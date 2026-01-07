// Comprehensive comparison benchmark: Lossy vs Lossless compression
// Compares compression ratio, speed, quality metrics, and use-case suitability

#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <type_traits>

// ============================================================================
// Lossy Compression Implementations (SQ8, PQ)
// ============================================================================

namespace themis::lossy {

// Scalar Quantization (int8)
struct QuantizedVector {
    std::vector<int8_t> quantized;
    float scale;
    float min_val;
    
    size_t compressed_bytes() const {
        return quantized.size() * sizeof(int8_t) + sizeof(float) + sizeof(float);
    }
};

class ScalarQuantizer {
public:
    static QuantizedVector compress(const std::vector<float>& vec) {
        QuantizedVector result;
        
        if (vec.empty()) return result;
        
        // Find min/max
        float min_val = *std::min_element(vec.begin(), vec.end());
        float max_val = *std::max_element(vec.begin(), vec.end());
        
        // Compute scale
        float range = max_val - min_val;
        float scale = range / 255.0f;
        
        result.scale = scale;
        result.min_val = min_val;
        result.quantized.reserve(vec.size());
        
        // Quantize
        for (float val : vec) {
            float normalized = (val - min_val) / scale;
            int8_t quantized = static_cast<int8_t>(std::round(normalized));
            result.quantized.push_back(quantized);
        }
        
        return result;
    }
    
    static std::vector<float> decompress(const QuantizedVector& qvec) {
        std::vector<float> result;
        result.reserve(qvec.quantized.size());
        
        for (int8_t q : qvec.quantized) {
            float val = qvec.min_val + q * qvec.scale;
            result.push_back(val);
        }
        
        return result;
    }
};

// Product Quantization (simplified)
struct PQCompressed {
    std::vector<uint8_t> codes; // Cluster IDs
    std::vector<std::vector<float>> codebooks; // Centroids per subspace
    size_t dimension;
    size_t num_subspaces;
    
    size_t compressed_bytes() const {
        size_t bytes = codes.size() * sizeof(uint8_t);
        bytes += sizeof(dimension) + sizeof(num_subspaces);
        for (const auto& codebook : codebooks) {
            bytes += codebook.size() * sizeof(float);
        }
        return bytes;
    }
};

class ProductQuantizer {
public:
    static PQCompressed compress(const std::vector<float>& vec, size_t num_subspaces = 8) {
        PQCompressed result;
        result.dimension = vec.size();
        result.num_subspaces = num_subspaces;
        
        size_t subspace_dim = vec.size() / num_subspaces;
        
        // Simplified PQ: just quantize each subspace independently
        for (size_t s = 0; s < num_subspaces; ++s) {
            size_t start = s * subspace_dim;
            size_t end = start + subspace_dim;
            
            // Create simple codebook (8 centroids for this example)
            std::vector<float> subvec(vec.begin() + start, vec.begin() + end);
            float min_val = *std::min_element(subvec.begin(), subvec.end());
            float max_val = *std::max_element(subvec.begin(), subvec.end());
            
            std::vector<float> codebook;
            for (int i = 0; i < 8; ++i) {
                codebook.push_back(min_val + i * (max_val - min_val) / 7.0f);
            }
            result.codebooks.push_back(codebook);
            
            // Find nearest centroid (simplified)
            uint8_t code = 0;
            float avg = std::accumulate(subvec.begin(), subvec.end(), 0.0f) / subvec.size();
            float min_dist = std::abs(avg - codebook[0]);
            for (size_t i = 1; i < codebook.size(); ++i) {
                float dist = std::abs(avg - codebook[i]);
                if (dist < min_dist) {
                    min_dist = dist;
                    code = static_cast<uint8_t>(i);
                }
            }
            result.codes.push_back(code);
        }
        
        return result;
    }
    
    static std::vector<float> decompress(const PQCompressed& pq) {
        std::vector<float> result;
        result.reserve(pq.dimension);
        
        size_t subspace_dim = pq.dimension / pq.num_subspaces;
        
        for (size_t s = 0; s < pq.num_subspaces; ++s) {
            uint8_t code = pq.codes[s];
            float centroid = pq.codebooks[s][code];
            
            // Repeat centroid for subspace (simplified)
            for (size_t i = 0; i < subspace_dim; ++i) {
                result.push_back(centroid);
            }
        }
        
        return result;
    }
};

} // namespace themis::lossy

// ============================================================================
// Lossless Compression Implementations (from previous file)
// ============================================================================

namespace themis::lossless {

struct SparseVectorCSR {
    std::vector<float> values;
    std::vector<uint32_t> indices;
    uint32_t dimension;
    
    size_t compressed_bytes() const {
        return sizeof(dimension) + 
               values.size() * sizeof(float) + 
               indices.size() * sizeof(uint32_t);
    }
};

class SparseVectorCodec {
public:
    static SparseVectorCSR compress(const std::vector<float>& vec, float epsilon = 1e-9f) {
        SparseVectorCSR result;
        result.dimension = static_cast<uint32_t>(vec.size());
        
        for (size_t i = 0; i < vec.size(); ++i) {
            if (std::abs(vec[i]) > epsilon) {
                result.values.push_back(vec[i]);
                result.indices.push_back(static_cast<uint32_t>(i));
            }
        }
        
        return result;
    }
    
    static std::vector<float> decompress(const SparseVectorCSR& sparse) {
        std::vector<float> vec(sparse.dimension, 0.0f);
        for (size_t i = 0; i < sparse.values.size(); ++i) {
            vec[sparse.indices[i]] = sparse.values[i];
        }
        return vec;
    }
};

template<typename T>
struct DictionaryCompressed {
    std::vector<T> dictionary;
    std::vector<uint32_t> indices;
    size_t original_size;
    
    size_t compressed_bytes() const {
        return dictionary.size() * sizeof(T) + 
               indices.size() * sizeof(uint32_t) +
               sizeof(size_t);
    }
};

template<typename T>
class DictionaryCodec {
public:
    static DictionaryCompressed<T> compress(const std::vector<T>& vec) {
        DictionaryCompressed<T> result;
        result.original_size = vec.size();
        
        std::unordered_map<T, uint32_t> value_to_index;
        
        for (const auto& val : vec) {
            auto it = value_to_index.find(val);
            if (it == value_to_index.end()) {
                uint32_t idx = result.dictionary.size();
                result.dictionary.push_back(val);
                value_to_index[val] = idx;
                result.indices.push_back(idx);
            } else {
                result.indices.push_back(it->second);
            }
        }
        
        return result;
    }
    
    static std::vector<T> decompress(const DictionaryCompressed<T>& compressed) {
        std::vector<T> result;
        result.reserve(compressed.original_size);
        
        for (uint32_t idx : compressed.indices) {
            result.push_back(compressed.dictionary[idx]);
        }
        
        return result;
    }
};

} // namespace themis::lossless

// ============================================================================
// Quality Metrics
// ============================================================================

namespace themis::metrics {

struct QualityMetrics {
    double mse;           // Mean Squared Error
    double rmse;          // Root Mean Squared Error
    double mae;           // Mean Absolute Error
    double max_error;     // Maximum absolute error
    double psnr;          // Peak Signal-to-Noise Ratio
    double cosine_sim;    // Cosine similarity
    double l2_distance;   // L2 distance
    
    void print() const {
        std::cout << "  MSE: " << mse << "\n";
        std::cout << "  RMSE: " << rmse << "\n";
        std::cout << "  MAE: " << mae << "\n";
        std::cout << "  Max Error: " << max_error << "\n";
        std::cout << "  PSNR: " << psnr << " dB\n";
        std::cout << "  Cosine Similarity: " << cosine_sim << "\n";
        std::cout << "  L2 Distance: " << l2_distance << "\n";
    }
};

QualityMetrics compute_quality(const std::vector<float>& original, 
                                const std::vector<float>& reconstructed) {
    QualityMetrics metrics;
    
    size_t n = original.size();
    if (n == 0 || n != reconstructed.size()) {
        return metrics;
    }
    
    // MSE, RMSE, MAE, Max Error
    double sum_sq_error = 0.0;
    double sum_abs_error = 0.0;
    metrics.max_error = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double error = original[i] - reconstructed[i];
        double abs_error = std::abs(error);
        sum_sq_error += error * error;
        sum_abs_error += abs_error;
        metrics.max_error = std::max(metrics.max_error, abs_error);
    }
    
    metrics.mse = sum_sq_error / n;
    metrics.rmse = std::sqrt(metrics.mse);
    metrics.mae = sum_abs_error / n;
    
    // PSNR (assuming signal range is -10 to 10 for ML embeddings)
    double max_signal = 20.0; // Range of signal
    if (metrics.mse > 0) {
        metrics.psnr = 20.0 * std::log10(max_signal / metrics.rmse);
    } else {
        metrics.psnr = std::numeric_limits<double>::infinity();
    }
    
    // Cosine Similarity
    double dot_product = 0.0;
    double norm_original = 0.0;
    double norm_reconstructed = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        dot_product += original[i] * reconstructed[i];
        norm_original += original[i] * original[i];
        norm_reconstructed += reconstructed[i] * reconstructed[i];
    }
    
    norm_original = std::sqrt(norm_original);
    norm_reconstructed = std::sqrt(norm_reconstructed);
    
    if (norm_original > 0 && norm_reconstructed > 0) {
        metrics.cosine_sim = dot_product / (norm_original * norm_reconstructed);
    } else {
        metrics.cosine_sim = 0.0;
    }
    
    // L2 Distance
    metrics.l2_distance = std::sqrt(sum_sq_error);
    
    return metrics;
}

} // namespace themis::metrics

// ============================================================================
// Test Data Generators
// ============================================================================

namespace themis::testdata {

enum class VectorType {
    SPARSE_TFIDF,        // 95-99% zeros
    DENSE_EMBEDDING,     // ML embeddings (uniform)
    CATEGORICAL,         // Few unique values
    INTEGER_FEATURES     // Integer-valued features
};

class VectorGenerator {
public:
    static std::vector<float> generate(VectorType type, size_t dimension, uint64_t seed = 42) {
        std::mt19937_64 rng(seed);
        
        switch (type) {
            case VectorType::SPARSE_TFIDF:
                return generate_sparse_tfidf(dimension, 0.98f, rng);
            case VectorType::DENSE_EMBEDDING:
                return generate_dense_embedding(dimension, rng);
            case VectorType::CATEGORICAL:
                return generate_categorical(dimension, 10, rng);
            case VectorType::INTEGER_FEATURES:
                return generate_integer_features(dimension, rng);
            default:
                return std::vector<float>(dimension, 0.0f);
        }
    }
    
    static std::string type_name(VectorType type) {
        switch (type) {
            case VectorType::SPARSE_TFIDF: return "SPARSE_TFIDF";
            case VectorType::DENSE_EMBEDDING: return "DENSE_EMBEDDING";
            case VectorType::CATEGORICAL: return "CATEGORICAL";
            case VectorType::INTEGER_FEATURES: return "INTEGER_FEATURES";
            default: return "UNKNOWN";
        }
    }

private:
    static std::vector<float> generate_sparse_tfidf(size_t dim, float sparsity, std::mt19937_64& rng) {
        std::vector<float> vec(dim, 0.0f);
        size_t non_zero_count = static_cast<size_t>(dim * (1.0f - sparsity));
        std::uniform_int_distribution<size_t> idx_dist(0, dim - 1);
        std::uniform_real_distribution<float> val_dist(0.01f, 10.0f);
        
        for (size_t i = 0; i < non_zero_count; ++i) {
            vec[idx_dist(rng)] = val_dist(rng);
        }
        return vec;
    }
    
    static std::vector<float> generate_dense_embedding(size_t dim, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (size_t i = 0; i < dim; ++i) {
            vec.push_back(dist(rng));
        }
        return vec;
    }
    
    static std::vector<float> generate_categorical(size_t dim, size_t num_categories, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_int_distribution<size_t> dist(0, num_categories - 1);
        
        for (size_t i = 0; i < dim; ++i) {
            vec.push_back(static_cast<float>(dist(rng)));
        }
        return vec;
    }
    
    static std::vector<float> generate_integer_features(size_t dim, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_int_distribution<int> dist(0, 255);
        
        for (size_t i = 0; i < dim; ++i) {
            vec.push_back(static_cast<float>(dist(rng)));
        }
        return vec;
    }
};

} // namespace themis::testdata

// ============================================================================
// Comprehensive Comparison Results
// ============================================================================

struct CompressionComparison {
    std::string method;
    std::string vector_type;
    bool is_lossy;
    size_t dimension;
    size_t original_bytes;
    size_t compressed_bytes;
    double compression_ratio;
    double encode_time_us;
    double decode_time_us;
    themis::metrics::QualityMetrics quality;
    
    void print() const {
        std::cout << "\n========================================\n";
        std::cout << "Method: " << method << (is_lossy ? " (LOSSY)" : " (LOSSLESS)") << "\n";
        std::cout << "Vector Type: " << vector_type << "\n";
        std::cout << "Dimension: " << dimension << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Compression:\n";
        std::cout << "  Original: " << original_bytes << " bytes\n";
        std::cout << "  Compressed: " << compressed_bytes << " bytes\n";
        std::cout << "  Ratio: " << std::fixed << std::setprecision(2) << compression_ratio << "x\n";
        std::cout << "Performance:\n";
        std::cout << "  Encode: " << std::fixed << std::setprecision(2) << encode_time_us << " µs\n";
        std::cout << "  Decode: " << std::fixed << std::setprecision(2) << decode_time_us << " µs\n";
        std::cout << "Quality:\n";
        quality.print();
        std::cout << "========================================\n";
    }
};

std::vector<CompressionComparison> g_comparison_results;

// ============================================================================
// Benchmark Helper Functions
// ============================================================================

template<typename CompressFunc, typename DecompressFunc>
CompressionComparison benchmark_compression(
    const std::string& method,
    bool is_lossy,
    const std::vector<float>& vec,
    const std::string& vector_type,
    CompressFunc compress_fn,
    DecompressFunc decompress_fn
) {
    CompressionComparison result;
    result.method = method;
    result.vector_type = vector_type;
    result.is_lossy = is_lossy;
    result.dimension = vec.size();
    result.original_bytes = vec.size() * sizeof(float);
    
    // Encode
    auto encode_start = std::chrono::high_resolution_clock::now();
    auto compressed = compress_fn(vec);
    auto encode_end = std::chrono::high_resolution_clock::now();
    result.encode_time_us = std::chrono::duration<double, std::micro>(encode_end - encode_start).count();
    
    // Get compressed size
    result.compressed_bytes = compressed.compressed_bytes();
    
    result.compression_ratio = static_cast<double>(result.original_bytes) / result.compressed_bytes;
    
    // Decode
    auto decode_start = std::chrono::high_resolution_clock::now();
    auto decompressed = decompress_fn(compressed);
    auto decode_end = std::chrono::high_resolution_clock::now();
    result.decode_time_us = std::chrono::duration<double, std::micro>(decode_end - decode_start).count();
    
    // Quality metrics
    result.quality = themis::metrics::compute_quality(vec, decompressed);
    
    return result;
}

// ============================================================================
// Google Benchmark Functions
// ============================================================================

using namespace themis::lossy;
using namespace themis::lossless;
using namespace themis::testdata;

// Lossy: Scalar Quantization (SQ8)
static void BM_Lossy_SQ8_Encode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    for (auto _ : state) {
        auto compressed = ScalarQuantizer::compress(vec);
        benchmark::DoNotOptimize(compressed);
    }
    
    auto compressed = ScalarQuantizer::compress(vec);
    double ratio = static_cast<double>(vec.size() * sizeof(float)) / compressed.compressed_bytes();
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = compressed.compressed_bytes();
}

static void BM_Lossy_SQ8_Decode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    auto compressed = ScalarQuantizer::compress(vec);
    
    for (auto _ : state) {
        auto decompressed = ScalarQuantizer::decompress(compressed);
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
}

// Lossy: Product Quantization
static void BM_Lossy_PQ_Encode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    for (auto _ : state) {
        auto compressed = ProductQuantizer::compress(vec, 8);
        benchmark::DoNotOptimize(compressed);
    }
    
    auto compressed = ProductQuantizer::compress(vec, 8);
    double ratio = static_cast<double>(vec.size() * sizeof(float)) / compressed.compressed_bytes();
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = compressed.compressed_bytes();
}

static void BM_Lossy_PQ_Decode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    auto compressed = ProductQuantizer::compress(vec, 8);
    
    for (auto _ : state) {
        auto decompressed = ProductQuantizer::decompress(compressed);
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
}

// Lossless: Sparse CSR
static void BM_Lossless_SparseCSR_Encode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    for (auto _ : state) {
        auto compressed = SparseVectorCodec::compress(vec);
        benchmark::DoNotOptimize(compressed);
    }
    
    auto compressed = SparseVectorCodec::compress(vec);
    double ratio = static_cast<double>(vec.size() * sizeof(float)) / compressed.compressed_bytes();
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = compressed.compressed_bytes();
}

static void BM_Lossless_SparseCSR_Decode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    auto compressed = SparseVectorCodec::compress(vec);
    
    for (auto _ : state) {
        auto decompressed = SparseVectorCodec::decompress(compressed);
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
}

// Lossless: Dictionary
static void BM_Lossless_Dictionary_Encode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    for (auto _ : state) {
        auto compressed = DictionaryCodec<float>::compress(vec);
        benchmark::DoNotOptimize(compressed);
    }
    
    auto compressed = DictionaryCodec<float>::compress(vec);
    double ratio = static_cast<double>(vec.size() * sizeof(float)) / compressed.compressed_bytes();
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = compressed.compressed_bytes();
}

static void BM_Lossless_Dictionary_Decode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    auto compressed = DictionaryCodec<float>::compress(vec);
    
    for (auto _ : state) {
        auto decompressed = DictionaryCodec<float>::decompress(compressed);
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
}

// ============================================================================
// Register Benchmarks
// ============================================================================

// Dense embeddings: Lossy vs Lossless
BENCHMARK(BM_Lossy_SQ8_Encode)
    ->Args({static_cast<int64_t>(VectorType::DENSE_EMBEDDING), 768})
    ->Args({static_cast<int64_t>(VectorType::DENSE_EMBEDDING), 1536})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Lossy_SQ8_Decode)
    ->Args({static_cast<int64_t>(VectorType::DENSE_EMBEDDING), 768})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Lossy_PQ_Encode)
    ->Args({static_cast<int64_t>(VectorType::DENSE_EMBEDDING), 768})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Lossy_PQ_Decode)
    ->Args({static_cast<int64_t>(VectorType::DENSE_EMBEDDING), 768})
    ->Unit(benchmark::kMicrosecond);

// Sparse vectors: Lossless shines here
BENCHMARK(BM_Lossless_SparseCSR_Encode)
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 10000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 50000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Lossless_SparseCSR_Decode)
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 10000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Lossy_SQ8_Encode)
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 10000})
    ->Unit(benchmark::kMicrosecond);

// Categorical: Lossless Dictionary vs Lossy
BENCHMARK(BM_Lossless_Dictionary_Encode)
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 1000})
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 10000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Lossless_Dictionary_Decode)
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 1000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Lossy_SQ8_Encode)
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 1000})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Custom Main with Comprehensive Comparison
// ============================================================================

void run_comprehensive_comparison() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     COMPREHENSIVE LOSSY vs LOSSLESS COMPARISON               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    std::vector<VectorType> vector_types = {
        VectorType::SPARSE_TFIDF,
        VectorType::DENSE_EMBEDDING,
        VectorType::CATEGORICAL
    };
    
    std::vector<size_t> dimensions = {768, 1536, 10000};
    
    for (auto vtype : vector_types) {
        for (auto dim : dimensions) {
            // Skip invalid combinations
            if (vtype == VectorType::SPARSE_TFIDF && dim < 10000) continue;
            if (vtype != VectorType::SPARSE_TFIDF && dim >= 10000) continue;
            
            auto vec = VectorGenerator::generate(vtype, dim);
            std::string type_name = VectorGenerator::type_name(vtype);
            
            std::cout << "\n\n### Vector Type: " << type_name << " (dim=" << dim << ") ###\n";
            
            // Lossy: SQ8
            {
                auto result = benchmark_compression(
                    "Scalar Quantization (SQ8)",
                    true,
                    vec,
                    type_name,
                    [](const auto& v) { return ScalarQuantizer::compress(v); },
                    [](const auto& c) { return ScalarQuantizer::decompress(c); }
                );
                result.print();
                g_comparison_results.push_back(result);
            }
            
            // Lossy: PQ (only for dense embeddings)
            if (vtype == VectorType::DENSE_EMBEDDING && dim == 768) {
                auto result = benchmark_compression(
                    "Product Quantization (PQ)",
                    true,
                    vec,
                    type_name,
                    [](const auto& v) { return ProductQuantizer::compress(v, 8); },
                    [](const auto& c) { return ProductQuantizer::decompress(c); }
                );
                result.print();
                g_comparison_results.push_back(result);
            }
            
            // Lossless: Sparse CSR (for sparse vectors)
            if (vtype == VectorType::SPARSE_TFIDF) {
                auto result = benchmark_compression(
                    "Sparse CSR (Lossless)",
                    false,
                    vec,
                    type_name,
                    [](const auto& v) { return SparseVectorCodec::compress(v); },
                    [](const auto& c) { return SparseVectorCodec::decompress(c); }
                );
                result.print();
                g_comparison_results.push_back(result);
            }
            
            // Lossless: Dictionary (for categorical)
            if (vtype == VectorType::CATEGORICAL) {
                auto result = benchmark_compression(
                    "Dictionary (Lossless)",
                    false,
                    vec,
                    type_name,
                    [](const auto& v) { return DictionaryCodec<float>::compress(v); },
                    [](const auto& c) { return DictionaryCodec<float>::decompress(c); }
                );
                result.print();
                g_comparison_results.push_back(result);
            }
        }
    }
    
    // Summary comparison table
    std::cout << "\n\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    SUMMARY COMPARISON                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << std::setw(30) << "Method" 
              << std::setw(12) << "Ratio" 
              << std::setw(12) << "Enc(µs)" 
              << std::setw(12) << "Dec(µs)"
              << std::setw(10) << "MAE"
              << std::setw(12) << "CosineSim\n";
    std::cout << std::string(88, '-') << "\n";
    
    for (const auto& result : g_comparison_results) {
        std::cout << std::setw(30) << (result.method + " (" + result.vector_type + ")")
                  << std::setw(12) << std::fixed << std::setprecision(2) << result.compression_ratio
                  << std::setw(12) << std::fixed << std::setprecision(1) << result.encode_time_us
                  << std::setw(12) << std::fixed << std::setprecision(1) << result.decode_time_us
                  << std::setw(10) << std::scientific << std::setprecision(2) << result.quality.mae
                  << std::setw(12) << std::fixed << std::setprecision(6) << result.quality.cosine_sim
                  << "\n";
    }
    
    std::cout << "\n";
}

int main(int argc, char** argv) {
    // Run comprehensive comparison first
    run_comprehensive_comparison();
    
    std::cout << "\n\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              RUNNING GOOGLE BENCHMARK SUITE                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    return 0;
}
