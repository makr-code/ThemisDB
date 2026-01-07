// Comprehensive benchmark suite for lossless vector compression
// Tests compression ratio, encode/decode speed, and hardware-specific optimizations
// Supports CPU SIMD (AVX2, AVX-512, NEON), GPU acceleration, and AI accelerators

#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <iostream>
#include <thread>
#include <immintrin.h> // AVX/AVX2/AVX-512

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

// ============================================================================
// Hardware Detection and Configuration
// ============================================================================

namespace themis::hw {

enum class SIMDLevel {
    NONE,
    SSE2,
    SSE4_2,
    AVX,
    AVX2,
    AVX512,
    NEON
};

struct HardwareCapabilities {
    SIMDLevel simd_level = SIMDLevel::NONE;
    bool has_gpu = false;
    bool has_cuda = false;
    bool has_vulkan = false;
    bool has_ai_accelerator = false; // TPU, NPU, etc.
    int cpu_cores = 1;
    size_t l1_cache_kb = 32;
    size_t l2_cache_kb = 256;
    size_t l3_cache_kb = 0;
    
    static HardwareCapabilities detect() {
        HardwareCapabilities caps;
        
        // Detect SIMD capabilities
        #if defined(__AVX512F__)
            caps.simd_level = SIMDLevel::AVX512;
        #elif defined(__AVX2__)
            caps.simd_level = SIMDLevel::AVX2;
        #elif defined(__AVX__)
            caps.simd_level = SIMDLevel::AVX;
        #elif defined(__SSE4_2__)
            caps.simd_level = SIMDLevel::SSE4_2;
        #elif defined(__SSE2__)
            caps.simd_level = SIMDLevel::SSE2;
        #elif defined(__ARM_NEON)
            caps.simd_level = SIMDLevel::NEON;
        #endif
        
        caps.cpu_cores = std::thread::hardware_concurrency();
        
        return caps;
    }
    
    std::string simd_level_string() const {
        switch (simd_level) {
            case SIMDLevel::AVX512: return "AVX-512";
            case SIMDLevel::AVX2: return "AVX2";
            case SIMDLevel::AVX: return "AVX";
            case SIMDLevel::SSE4_2: return "SSE4.2";
            case SIMDLevel::SSE2: return "SSE2";
            case SIMDLevel::NEON: return "NEON";
            default: return "NONE";
        }
    }
};

} // namespace themis::hw

// ============================================================================
// Test Data Generators
// ============================================================================

namespace themis::testdata {

enum class VectorType {
    SPARSE_TFIDF,        // 95-99% zeros, realistic TF-IDF
    SPARSE_ONEHOT,       // 99.9% zeros, one-hot encodings
    INTEGER_HISTOGRAM,   // Integer values, small deltas
    INTEGER_MONOTONIC,   // Monotonically increasing
    CATEGORICAL,         // Few unique values (5-20)
    DENSE_UNIFORM,       // Uniform random floats
    DENSE_NORMAL,        // Normal distribution floats
    SMOOTH_SIGNAL,       // Smooth waveform (physics simulation)
    CLUSTERED            // Clustered values (ML embeddings)
};

class VectorGenerator {
public:
    static std::vector<float> generate(VectorType type, size_t dimension, uint64_t seed = 42) {
        std::mt19937_64 rng(seed);
        std::vector<float> result;
        result.reserve(dimension);
        
        switch (type) {
            case VectorType::SPARSE_TFIDF:
                return generate_sparse_tfidf(dimension, 0.98f, rng);
            
            case VectorType::SPARSE_ONEHOT:
                return generate_sparse_onehot(dimension, 0.999f, rng);
            
            case VectorType::INTEGER_HISTOGRAM:
                return generate_integer_histogram(dimension, rng);
            
            case VectorType::INTEGER_MONOTONIC:
                return generate_integer_monotonic(dimension, rng);
            
            case VectorType::CATEGORICAL:
                return generate_categorical(dimension, 10, rng);
            
            case VectorType::DENSE_UNIFORM:
                return generate_dense_uniform(dimension, -1.0f, 1.0f, rng);
            
            case VectorType::DENSE_NORMAL:
                return generate_dense_normal(dimension, 0.0f, 1.0f, rng);
            
            case VectorType::SMOOTH_SIGNAL:
                return generate_smooth_signal(dimension, rng);
            
            case VectorType::CLUSTERED:
                return generate_clustered(dimension, 5, rng);
            
            default:
                return std::vector<float>(dimension, 0.0f);
        }
    }
    
    static std::string type_name(VectorType type) {
        switch (type) {
            case VectorType::SPARSE_TFIDF: return "SPARSE_TFIDF";
            case VectorType::SPARSE_ONEHOT: return "SPARSE_ONEHOT";
            case VectorType::INTEGER_HISTOGRAM: return "INTEGER_HISTOGRAM";
            case VectorType::INTEGER_MONOTONIC: return "INTEGER_MONOTONIC";
            case VectorType::CATEGORICAL: return "CATEGORICAL";
            case VectorType::DENSE_UNIFORM: return "DENSE_UNIFORM";
            case VectorType::DENSE_NORMAL: return "DENSE_NORMAL";
            case VectorType::SMOOTH_SIGNAL: return "SMOOTH_SIGNAL";
            case VectorType::CLUSTERED: return "CLUSTERED";
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
    
    static std::vector<float> generate_sparse_onehot(size_t dim, float sparsity, std::mt19937_64& rng) {
        std::vector<float> vec(dim, 0.0f);
        size_t non_zero_count = static_cast<size_t>(dim * (1.0f - sparsity));
        std::uniform_int_distribution<size_t> idx_dist(0, dim - 1);
        
        for (size_t i = 0; i < non_zero_count; ++i) {
            vec[idx_dist(rng)] = 1.0f;
        }
        return vec;
    }
    
    static std::vector<float> generate_integer_histogram(size_t dim, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_int_distribution<int> dist(0, 255);
        
        for (size_t i = 0; i < dim; ++i) {
            vec.push_back(static_cast<float>(dist(rng)));
        }
        return vec;
    }
    
    static std::vector<float> generate_integer_monotonic(size_t dim, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_int_distribution<int> delta_dist(0, 5);
        
        int current = 1000;
        for (size_t i = 0; i < dim; ++i) {
            vec.push_back(static_cast<float>(current));
            current += delta_dist(rng);
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
    
    static std::vector<float> generate_dense_uniform(size_t dim, float min, float max, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_real_distribution<float> dist(min, max);
        
        for (size_t i = 0; i < dim; ++i) {
            vec.push_back(dist(rng));
        }
        return vec;
    }
    
    static std::vector<float> generate_dense_normal(size_t dim, float mean, float stddev, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::normal_distribution<float> dist(mean, stddev);
        
        for (size_t i = 0; i < dim; ++i) {
            vec.push_back(dist(rng));
        }
        return vec;
    }
    
    static std::vector<float> generate_smooth_signal(size_t dim, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_real_distribution<float> noise_dist(-0.01f, 0.01f);
        
        for (size_t i = 0; i < dim; ++i) {
            float signal = std::sin(i * 0.01f) + 0.5f * std::cos(i * 0.05f);
            vec.push_back(signal + noise_dist(rng));
        }
        return vec;
    }
    
    static std::vector<float> generate_clustered(size_t dim, size_t num_clusters, std::mt19937_64& rng) {
        std::vector<float> vec;
        vec.reserve(dim);
        std::uniform_int_distribution<size_t> cluster_dist(0, num_clusters - 1);
        std::normal_distribution<float> noise_dist(0.0f, 0.1f);
        
        // Cluster centers
        std::vector<float> centers;
        std::uniform_real_distribution<float> center_dist(-10.0f, 10.0f);
        for (size_t i = 0; i < num_clusters; ++i) {
            centers.push_back(center_dist(rng));
        }
        
        for (size_t i = 0; i < dim; ++i) {
            size_t cluster = cluster_dist(rng);
            vec.push_back(centers[cluster] + noise_dist(rng));
        }
        return vec;
    }
};

} // namespace themis::testdata

// ============================================================================
// Compression Implementations with SIMD Optimizations
// ============================================================================

namespace themis::compression {

// ----------------------------------------------------------------------------
// Sparse Vector CSR (with SIMD-optimized sparsity detection)
// ----------------------------------------------------------------------------

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
    // Scalar version
    static SparseVectorCSR compress_scalar(const std::vector<float>& vec, float epsilon = 1e-9f) {
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
    
    // AVX2-optimized version
    #ifdef __AVX2__
    static SparseVectorCSR compress_avx2(const std::vector<float>& vec, float epsilon = 1e-9f) {
        SparseVectorCSR result;
        result.dimension = static_cast<uint32_t>(vec.size());
        
        const size_t simd_width = 8; // AVX2 processes 8 floats
        const size_t aligned_size = (vec.size() / simd_width) * simd_width;
        
        __m256 eps = _mm256_set1_ps(epsilon);
        __m256 neg_eps = _mm256_set1_ps(-epsilon);
        
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256 vals = _mm256_loadu_ps(&vec[i]);
            
            // Check if abs(vals) > epsilon
            __m256 gt_eps = _mm256_cmp_ps(vals, eps, _CMP_GT_OQ);
            __m256 lt_neg_eps = _mm256_cmp_ps(vals, neg_eps, _CMP_LT_OQ);
            __m256 non_zero = _mm256_or_ps(gt_eps, lt_neg_eps);
            
            int mask = _mm256_movemask_ps(non_zero);
            
            // Process each non-zero element
            for (int j = 0; j < simd_width; ++j) {
                if (mask & (1 << j)) {
                    result.values.push_back(vec[i + j]);
                    result.indices.push_back(static_cast<uint32_t>(i + j));
                }
            }
        }
        
        // Handle remaining elements
        for (size_t i = aligned_size; i < vec.size(); ++i) {
            if (std::abs(vec[i]) > epsilon) {
                result.values.push_back(vec[i]);
                result.indices.push_back(static_cast<uint32_t>(i));
            }
        }
        
        return result;
    }
    #endif
    
    // NEON-optimized version (ARM)
    #ifdef __ARM_NEON
    static SparseVectorCSR compress_neon(const std::vector<float>& vec, float epsilon = 1e-9f) {
        SparseVectorCSR result;
        result.dimension = static_cast<uint32_t>(vec.size());
        
        const size_t simd_width = 4; // NEON processes 4 floats
        const size_t aligned_size = (vec.size() / simd_width) * simd_width;
        
        float32x4_t eps = vdupq_n_f32(epsilon);
        float32x4_t neg_eps = vdupq_n_f32(-epsilon);
        
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            float32x4_t vals = vld1q_f32(&vec[i]);
            
            // Check if abs(vals) > epsilon
            uint32x4_t gt_eps = vcgtq_f32(vals, eps);
            uint32x4_t lt_neg_eps = vcltq_f32(vals, neg_eps);
            uint32x4_t non_zero = vorrq_u32(gt_eps, lt_neg_eps);
            
            // Process each non-zero element
            uint32_t mask[4];
            vst1q_u32(mask, non_zero);
            
            for (int j = 0; j < simd_width; ++j) {
                if (mask[j]) {
                    result.values.push_back(vec[i + j]);
                    result.indices.push_back(static_cast<uint32_t>(i + j));
                }
            }
        }
        
        // Handle remaining elements
        for (size_t i = aligned_size; i < vec.size(); ++i) {
            if (std::abs(vec[i]) > epsilon) {
                result.values.push_back(vec[i]);
                result.indices.push_back(static_cast<uint32_t>(i));
            }
        }
        
        return result;
    }
    #endif
    
    static std::vector<float> decompress(const SparseVectorCSR& sparse) {
        std::vector<float> vec(sparse.dimension, 0.0f);
        
        for (size_t i = 0; i < sparse.values.size(); ++i) {
            vec[sparse.indices[i]] = sparse.values[i];
        }
        
        return vec;
    }
};

// ----------------------------------------------------------------------------
// VarInt Delta Encoding
// ----------------------------------------------------------------------------

class VarIntCodec {
public:
    static uint32_t zigzag_encode(int32_t n) {
        return (static_cast<uint32_t>(n) << 1) ^ (n >> 31);
    }
    
    static int32_t zigzag_decode(uint32_t n) {
        return static_cast<int32_t>((n >> 1) ^ -(n & 1));
    }
    
    static void encode(std::vector<uint8_t>& output, uint32_t value) {
        while (value >= 0x80) {
            output.push_back(static_cast<uint8_t>(value | 0x80));
            value >>= 7;
        }
        output.push_back(static_cast<uint8_t>(value));
    }
    
    static uint32_t decode(const uint8_t*& ptr) {
        uint32_t result = 0;
        int shift = 0;
        
        while (true) {
            uint8_t byte = *ptr++;
            result |= static_cast<uint32_t>(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        
        return result;
    }
    
    static std::vector<uint8_t> compress_delta(const std::vector<int32_t>& values) {
        std::vector<uint8_t> result;
        
        if (values.empty()) return result;
        
        encode(result, zigzag_encode(values[0]));
        
        for (size_t i = 1; i < values.size(); ++i) {
            int32_t delta = values[i] - values[i - 1];
            encode(result, zigzag_encode(delta));
        }
        
        return result;
    }
    
    static std::vector<int32_t> decompress_delta(const std::vector<uint8_t>& data) {
        std::vector<int32_t> result;
        
        if (data.empty()) return result;
        
        const uint8_t* ptr = data.data();
        const uint8_t* end = ptr + data.size();
        
        int32_t current = zigzag_decode(decode(ptr));
        result.push_back(current);
        
        while (ptr < end) {
            int32_t delta = zigzag_decode(decode(ptr));
            current += delta;
            result.push_back(current);
        }
        
        return result;
    }
};

// ----------------------------------------------------------------------------
// Dictionary Encoding
// ----------------------------------------------------------------------------

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

} // namespace themis::compression

// ============================================================================
// Benchmark Results Tracking
// ============================================================================

struct BenchmarkResult {
    std::string compression_method;
    std::string vector_type;
    std::string hardware;
    size_t dimension;
    size_t original_bytes;
    size_t compressed_bytes;
    double compression_ratio;
    double encode_time_ms;
    double decode_time_ms;
    double encode_throughput_mbps;
    double decode_throughput_mbps;
    
    void print() const {
        std::cout << "----------------------------------------\n";
        std::cout << "Method: " << compression_method << "\n";
        std::cout << "Vector Type: " << vector_type << "\n";
        std::cout << "Hardware: " << hardware << "\n";
        std::cout << "Dimension: " << dimension << "\n";
        std::cout << "Compression Ratio: " << compression_ratio << "x\n";
        std::cout << "Encode: " << encode_time_ms << " ms (" << encode_throughput_mbps << " MB/s)\n";
        std::cout << "Decode: " << decode_time_ms << " ms (" << decode_throughput_mbps << " MB/s)\n";
        std::cout << "Original: " << original_bytes << " bytes\n";
        std::cout << "Compressed: " << compressed_bytes << " bytes\n";
        std::cout << "----------------------------------------\n";
    }
};

std::vector<BenchmarkResult> g_results;

// ============================================================================
// Benchmark Functions
// ============================================================================

using namespace themis::compression;
using namespace themis::testdata;
using namespace themis::hw;

// Sparse CSR Benchmarks

static void BM_SparseCSR_Encode_Scalar(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    size_t total_bytes = 0;
    for (auto _ : state) {
        auto compressed = SparseVectorCodec::compress_scalar(vec);
        total_bytes = compressed.compressed_bytes();
        benchmark::DoNotOptimize(compressed);
    }
    
    size_t original_bytes = vec.size() * sizeof(float);
    double ratio = static_cast<double>(original_bytes) / total_bytes;
    
    state.SetBytesProcessed(state.iterations() * original_bytes);
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = total_bytes;
}

#ifdef __AVX2__
static void BM_SparseCSR_Encode_AVX2(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    size_t total_bytes = 0;
    for (auto _ : state) {
        auto compressed = SparseVectorCodec::compress_avx2(vec);
        total_bytes = compressed.compressed_bytes();
        benchmark::DoNotOptimize(compressed);
    }
    
    size_t original_bytes = vec.size() * sizeof(float);
    double ratio = static_cast<double>(original_bytes) / total_bytes;
    
    state.SetBytesProcessed(state.iterations() * original_bytes);
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = total_bytes;
}
#endif

#ifdef __ARM_NEON
static void BM_SparseCSR_Encode_NEON(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    size_t total_bytes = 0;
    for (auto _ : state) {
        auto compressed = SparseVectorCodec::compress_neon(vec);
        total_bytes = compressed.compressed_bytes();
        benchmark::DoNotOptimize(compressed);
    }
    
    size_t original_bytes = vec.size() * sizeof(float);
    double ratio = static_cast<double>(original_bytes) / total_bytes;
    
    state.SetBytesProcessed(state.iterations() * original_bytes);
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = total_bytes;
}
#endif

static void BM_SparseCSR_Decode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    auto compressed = SparseVectorCodec::compress_scalar(vec);
    
    for (auto _ : state) {
        auto decompressed = SparseVectorCodec::decompress(compressed);
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(float));
}

// Delta+VarInt Benchmarks

static void BM_DeltaVarInt_Encode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec_float = VectorGenerator::generate(vector_type, dimension);
    std::vector<int32_t> vec;
    for (float f : vec_float) {
        vec.push_back(static_cast<int32_t>(std::round(f)));
    }
    
    size_t compressed_size = 0;
    for (auto _ : state) {
        auto compressed = VarIntCodec::compress_delta(vec);
        compressed_size = compressed.size();
        benchmark::DoNotOptimize(compressed);
    }
    
    size_t original_bytes = vec.size() * sizeof(int32_t);
    double ratio = static_cast<double>(original_bytes) / compressed_size;
    
    state.SetBytesProcessed(state.iterations() * original_bytes);
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = compressed_size;
}

static void BM_DeltaVarInt_Decode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec_float = VectorGenerator::generate(vector_type, dimension);
    std::vector<int32_t> vec;
    for (float f : vec_float) {
        vec.push_back(static_cast<int32_t>(std::round(f)));
    }
    
    auto compressed = VarIntCodec::compress_delta(vec);
    
    for (auto _ : state) {
        auto decompressed = VarIntCodec::decompress_delta(compressed);
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * vec.size() * sizeof(int32_t));
}

// Dictionary Encoding Benchmarks

static void BM_Dictionary_Encode(benchmark::State& state) {
    auto vector_type = static_cast<VectorType>(state.range(0));
    size_t dimension = state.range(1);
    
    auto vec = VectorGenerator::generate(vector_type, dimension);
    
    size_t compressed_bytes = 0;
    for (auto _ : state) {
        auto compressed = DictionaryCodec<float>::compress(vec);
        compressed_bytes = compressed.compressed_bytes();
        benchmark::DoNotOptimize(compressed);
    }
    
    size_t original_bytes = vec.size() * sizeof(float);
    double ratio = static_cast<double>(original_bytes) / compressed_bytes;
    
    state.SetBytesProcessed(state.iterations() * original_bytes);
    state.counters["CompressionRatio"] = ratio;
    state.counters["CompressedBytes"] = compressed_bytes;
}

static void BM_Dictionary_Decode(benchmark::State& state) {
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
// Benchmark Registration
// ============================================================================

// Register Sparse CSR benchmarks for different vector types and dimensions
BENCHMARK(BM_SparseCSR_Encode_Scalar)
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 1000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 10000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_ONEHOT), 1000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_ONEHOT), 10000})
    ->Unit(benchmark::kMicrosecond);

#ifdef __AVX2__
BENCHMARK(BM_SparseCSR_Encode_AVX2)
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 1000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 10000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_ONEHOT), 1000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_ONEHOT), 10000})
    ->Unit(benchmark::kMicrosecond);
#endif

#ifdef __ARM_NEON
BENCHMARK(BM_SparseCSR_Encode_NEON)
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 1000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 10000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_ONEHOT), 1000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_ONEHOT), 10000})
    ->Unit(benchmark::kMicrosecond);
#endif

BENCHMARK(BM_SparseCSR_Decode)
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 1000})
    ->Args({static_cast<int64_t>(VectorType::SPARSE_TFIDF), 10000})
    ->Unit(benchmark::kMicrosecond);

// Register Delta+VarInt benchmarks
BENCHMARK(BM_DeltaVarInt_Encode)
    ->Args({static_cast<int64_t>(VectorType::INTEGER_HISTOGRAM), 256})
    ->Args({static_cast<int64_t>(VectorType::INTEGER_HISTOGRAM), 1000})
    ->Args({static_cast<int64_t>(VectorType::INTEGER_MONOTONIC), 1000})
    ->Args({static_cast<int64_t>(VectorType::INTEGER_MONOTONIC), 10000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_DeltaVarInt_Decode)
    ->Args({static_cast<int64_t>(VectorType::INTEGER_HISTOGRAM), 256})
    ->Args({static_cast<int64_t>(VectorType::INTEGER_HISTOGRAM), 1000})
    ->Args({static_cast<int64_t>(VectorType::INTEGER_MONOTONIC), 1000})
    ->Unit(benchmark::kMicrosecond);

// Register Dictionary benchmarks
BENCHMARK(BM_Dictionary_Encode)
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 1000})
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 10000})
    ->Args({static_cast<int64_t>(VectorType::CLUSTERED), 1000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Dictionary_Decode)
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 1000})
    ->Args({static_cast<int64_t>(VectorType::CATEGORICAL), 10000})
    ->Unit(benchmark::kMicrosecond);

// Custom main to print hardware capabilities
int main(int argc, char** argv) {
    auto hw_caps = HardwareCapabilities::detect();
    
    std::cout << "=== Hardware Capabilities ===\n";
    std::cout << "SIMD Level: " << hw_caps.simd_level_string() << "\n";
    std::cout << "CPU Cores: " << hw_caps.cpu_cores << "\n";
    std::cout << "L1 Cache: " << hw_caps.l1_cache_kb << " KB\n";
    std::cout << "L2 Cache: " << hw_caps.l2_cache_kb << " KB\n";
    std::cout << "L3 Cache: " << hw_caps.l3_cache_kb << " KB\n";
    std::cout << "=============================\n\n";
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    return 0;
}
