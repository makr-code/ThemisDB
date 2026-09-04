// Unit tests for lossless vector compression methods
// Tests roundtrip correctness, compression ratio, and edge cases

#include <gtest/gtest.h>
#include <vector>
#include <random>
#include <cmath>
#include <unordered_set>
#include <unordered_map>

// Mock implementations for testing (will be in utils/ in actual implementation)
namespace themis { namespace compression { 

// ============================================================================
// Sparse Vector CSR Implementation
// ============================================================================

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
    
    static float compute_sparsity(const std::vector<float>& vec, float epsilon = 1e-9f) {
        size_t zero_count = 0;
        for (float v : vec) {
            if (std::abs(v) < epsilon) {
              ++zero_count;
            }
        }
        return static_cast<float>(zero_count) / vec.size();
    }
    
    static float dot_product_sparse_dense(
        const SparseVectorCSR& sparse,
        const std::vector<float>& dense
    ) {
        float result = 0.0f;
        for (size_t i = 0; i < sparse.values.size(); ++i) {
            result += sparse.values[i] * dense[sparse.indices[i]];
        }
        return result;
    }
};

// ============================================================================
// VarInt Codec Implementation
// ============================================================================

class VarIntCodec {
public:
    static uint32_t zigzag_encode(int32_t n) {
        return (static_cast<uint32_t>(n) << 1) ^ (n >> 31);
    }
    
    static int32_t zigzag_decode(uint32_t n) {
        const int32_t sign = (n & 1u) != 0u ? -1 : 0;
        return static_cast<int32_t>((n >> 1) ^ static_cast<uint32_t>(sign));
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
            if ((byte & 0x80) == 0) {
              break;
            }
            shift += 7;
        }
        
        return result;
    }
    
    static std::vector<uint8_t> compress_delta(const std::vector<int32_t>& values) {
        std::vector<uint8_t> result;
        
        if (values.empty()) {
          return result;
        }
        
        encode(result, zigzag_encode(values[0]));
        
        for (size_t i = 1; i < values.size(); ++i) {
            int32_t delta = values[i] - values[i - 1];
            encode(result, zigzag_encode(delta));
        }
        
        return result;
    }
    
    static std::vector<int32_t> decompress_delta(const std::vector<uint8_t>& data) {
        std::vector<int32_t> result;
        
        if (data.empty()) {
          return result;
        }
        
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

// ============================================================================
// Dictionary Encoding Implementation
// ============================================================================

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
                if (result.dictionary.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
                    throw std::overflow_error("Dictionary size exceeds uint32_t index range");
                }
                uint32_t idx = static_cast<uint32_t>(result.dictionary.size());
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
} } // namespace themis::compression
using namespace themis::compression;

// ============================================================================
// Test: Sparse Vector CSR
// ============================================================================

TEST(SparseVectorTest, RoundtripEmpty) {
    std::vector<float> original;
    auto compressed = SparseVectorCodec::compress(original);
    auto decompressed = SparseVectorCodec::decompress(compressed);
    
    EXPECT_EQ(decompressed.size(), 0);
}

TEST(SparseVectorTest, RoundtripAllZeros) {
    std::vector<float> original(1000, 0.0f);
    auto compressed = SparseVectorCodec::compress(original);
    auto decompressed = SparseVectorCodec::decompress(compressed);
    
    EXPECT_EQ(decompressed.size(), 1000);
    for (float val : decompressed) {
        EXPECT_EQ(val, 0.0f);
    }
    
    EXPECT_EQ(compressed.values.size(), 0);
    EXPECT_EQ(compressed.indices.size(), 0);
}

TEST(SparseVectorTest, RoundtripSparseVector) {
    // 99% sparse vector
    std::vector<float> original(10000, 0.0f);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> idx_dist(0, original.size() - 1);
    std::uniform_real_distribution<float> val_dist(-10.0f, 10.0f);
    
    // Add 100 non-zero values
    for (int i = 0; i < 100; ++i) {
        original[idx_dist(rng)] = val_dist(rng);
    }
    
    auto compressed = SparseVectorCodec::compress(original);
    auto decompressed = SparseVectorCodec::decompress(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_NEAR(decompressed[i], original[i], 1e-6f);
    }
}

TEST(SparseVectorTest, CompressionRatio) {
    // 99% sparse vector
    std::vector<float> original(10000, 0.0f);
    for (size_t i = 0; i < 100; ++i) {
        original[i * 100] = static_cast<float>(i);
    }
    
    auto compressed = SparseVectorCodec::compress(original);
    
    size_t original_bytes = original.size() * sizeof(float);
    size_t compressed_bytes = compressed.compressed_bytes();
    float ratio = static_cast<float>(original_bytes) / compressed_bytes;
    
    // Expect at least 20x compression for 99% sparse
    EXPECT_GT(ratio, 20.0f);
    
    std::cout << "Sparse CSR Compression Ratio: " << ratio << "x\n";
    std::cout << "Original: " << original_bytes << " bytes, Compressed: " << compressed_bytes << " bytes\n";
}

TEST(SparseVectorTest, SparsityComputation) {
    std::vector<float> vec(1000, 0.0f);
    for (size_t i = 0; i < 10; ++i) {
        vec[i] = 1.0f;
    }
    
    float sparsity = SparseVectorCodec::compute_sparsity(vec);
    EXPECT_NEAR(sparsity, 0.99f, 0.01f);
}

TEST(SparseVectorTest, DotProductSparse) {
    std::vector<float> vec1(1000, 0.0f);
    std::vector<float> vec2(1000, 1.0f);
    
    vec1[10] = 5.0f;
    vec1[20] = 3.0f;
    
    auto sparse1 = SparseVectorCodec::compress(vec1);
    
    float expected_dot = 5.0f * 1.0f + 3.0f * 1.0f; // = 8.0
    float actual_dot = SparseVectorCodec::dot_product_sparse_dense(sparse1, vec2);
    
    EXPECT_NEAR(actual_dot, expected_dot, 1e-6f);
}

// ============================================================================
// Test: VarInt Delta Encoding
// ============================================================================

TEST(VarIntTest, ZigzagEncoding) {
    EXPECT_EQ(VarIntCodec::zigzag_encode(0), 0u);
    EXPECT_EQ(VarIntCodec::zigzag_encode(-1), 1u);
    EXPECT_EQ(VarIntCodec::zigzag_encode(1), 2u);
    EXPECT_EQ(VarIntCodec::zigzag_encode(-2), 3u);
    EXPECT_EQ(VarIntCodec::zigzag_encode(2), 4u);
}

TEST(VarIntTest, ZigzagRoundtrip) {
    std::vector<int32_t> test_values = {0, 1, -1, 100, -100, 12345, -12345, 
                                         std::numeric_limits<int32_t>::max(),
                                         std::numeric_limits<int32_t>::min()};
    
    for (int32_t val : test_values) {
        uint32_t encoded = VarIntCodec::zigzag_encode(val);
        int32_t decoded = VarIntCodec::zigzag_decode(encoded);
        EXPECT_EQ(decoded, val);
    }
}

TEST(VarIntTest, VarIntEncoding) {
    std::vector<uint8_t> output;
    
    // Small value (1 byte)
    VarIntCodec::encode(output, 127);
    EXPECT_EQ(output.size(), 1);
    
    output.clear();
    
    // Medium value (2 bytes)
    VarIntCodec::encode(output, 300);
    EXPECT_EQ(output.size(), 2);
    
    output.clear();
    
    // Large value (5 bytes max for uint32)
    VarIntCodec::encode(output, 0xFFFFFFFF);
    EXPECT_LE(output.size(), 5);
}

TEST(VarIntTest, DeltaCompressionMonotonic) {
    // Monotonic increasing sequence
    std::vector<int32_t> original = {};

    for (int32_t i = 0; i < 1000; ++i) {
        original.push_back(1000 + i);
    }
    
    auto compressed = VarIntCodec::compress_delta(original);
    auto decompressed = VarIntCodec::decompress_delta(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(decompressed[i], original[i]);
    }
    
    size_t original_bytes = original.size() * sizeof(int32_t);
    size_t compressed_bytes = compressed.size();
    float ratio = static_cast<float>(original_bytes) / compressed_bytes;
    
    // Monotonic sequence with delta=1 should compress very well
    EXPECT_GT(ratio, 3.0f);
    
    std::cout << "Delta+VarInt Compression Ratio (monotonic): " << ratio << "x\n";
}

TEST(VarIntTest, DeltaCompressionHistogram) {
    // Simulate histogram data (small variations)
    std::vector<int32_t> original;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int32_t> dist(-5, 5);
    
    int32_t base = 100;
    for (int i = 0; i < 256; ++i) {
        base += dist(rng);
        original.push_back(base);
    }
    
    auto compressed = VarIntCodec::compress_delta(original);
    auto decompressed = VarIntCodec::decompress_delta(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(decompressed[i], original[i]);
    }
    
    size_t original_bytes = original.size() * sizeof(int32_t);
    size_t compressed_bytes = compressed.size();
    float ratio = static_cast<float>(original_bytes) / compressed_bytes;
    
    std::cout << "Delta+VarInt Compression Ratio (histogram): " << ratio << "x\n";
}

TEST(VarIntTest, DeltaCompressionRandom) {
    // Random data (worst case)
    std::vector<int32_t> original;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int32_t> dist(0, 1000000);
    
    for (int i = 0; i < 100; ++i) {
        original.push_back(dist(rng));
    }
    
    auto compressed = VarIntCodec::compress_delta(original);
    auto decompressed = VarIntCodec::decompress_delta(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(decompressed[i], original[i]);
    }
    
    size_t original_bytes = original.size() * sizeof(int32_t);
    size_t compressed_bytes = compressed.size();
    float ratio = static_cast<float>(original_bytes) / compressed_bytes;
    
    std::cout << "Delta+VarInt Compression Ratio (random): " << ratio << "x\n";
}

// ============================================================================
// Test: Dictionary Encoding
// ============================================================================

TEST(DictionaryTest, RoundtripUnique) {
    // All unique values (worst case)
    std::vector<float> original = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    auto compressed = DictionaryCodec<float>::compress(original);
    auto decompressed = DictionaryCodec<float>::decompress(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(decompressed[i], original[i]);
    }
    
    EXPECT_EQ(compressed.dictionary.size(), 5);
}

TEST(DictionaryTest, RoundtripRepeated) {
    // Many repeated values (best case)
    std::vector<float> original = {1.0f, 2.0f, 1.0f, 2.0f, 1.0f, 2.0f, 1.0f, 2.0f};
    auto compressed = DictionaryCodec<float>::compress(original);
    auto decompressed = DictionaryCodec<float>::decompress(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(decompressed[i], original[i]);
    }
    
    EXPECT_EQ(compressed.dictionary.size(), 2);
}

TEST(DictionaryTest, CompressionRatioCategorical) {
    // Categorical embeddings (10 unique values, 1000 total)
    std::vector<float> original;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 9);
    
    for (int i = 0; i < 1000; ++i) {
        original.push_back(static_cast<float>(dist(rng)));
    }
    
    auto compressed = DictionaryCodec<float>::compress(original);
    auto decompressed = DictionaryCodec<float>::decompress(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(decompressed[i], original[i]);
    }
    
    size_t original_bytes = original.size() * sizeof(float);
    size_t compressed_bytes = compressed.compressed_bytes();
    float ratio = static_cast<float>(original_bytes) / compressed_bytes;
    
    // Current dictionary format stores uint32 indices, so for float inputs
    // the ratio can be around 1.0x with slight metadata overhead.
    EXPECT_GT(ratio, 0.95f);
    
    std::cout << "Dictionary Compression Ratio (categorical): " << ratio << "x\n";
    std::cout << "Dictionary size: " << compressed.dictionary.size() << " unique values\n";
}

TEST(DictionaryTest, CompressionRatioOnehot) {
    // One-hot style (mostly 0.0, few 1.0)
    std::vector<float> original(1000, 0.0f);
    for (size_t i = 0; i < 10; ++i) {
        original[i * 100] = 1.0f;
    }
    
    auto compressed = DictionaryCodec<float>::compress(original);
    auto decompressed = DictionaryCodec<float>::decompress(compressed);
    
    ASSERT_EQ(decompressed.size(), original.size());
    
    size_t original_bytes = original.size() * sizeof(float);
    size_t compressed_bytes = compressed.compressed_bytes();
    float ratio = static_cast<float>(original_bytes) / compressed_bytes;
    
    std::cout << "Dictionary Compression Ratio (one-hot): " << ratio << "x\n";
    std::cout << "Dictionary size: " << compressed.dictionary.size() << " unique values\n";
}

// ============================================================================
// Integration Test: Compression Method Selection
// ============================================================================

enum class CompressionMethod {
    NONE,
    SPARSE_CSR,
    DELTA_VARINT,
    DICTIONARY
};

struct CompressionResult {
    CompressionMethod method;
    size_t original_bytes;
    size_t compressed_bytes;
    float compression_ratio;
    bool lossless;
};

CompressionMethod selectCompressionMethod(const std::vector<float>& vec, 
                                          float sparse_threshold = 0.95f) {
    // Sparsity check
    float sparsity = SparseVectorCodec::compute_sparsity(vec);
    if (sparsity >= sparse_threshold) {
        return CompressionMethod::SPARSE_CSR;
    }
    
    // Integer check
    size_t int_count = 0;
    for (float v : vec) {
        if (std::abs(v - std::round(v)) < 1e-6f) {
          ++int_count;
        }
    }
    if (int_count > vec.size() * 0.9) {
        return CompressionMethod::DELTA_VARINT;
    }
    
    // Unique values check
    std::unordered_set<float> unique_values(vec.begin(), vec.end());
    if (unique_values.size() < vec.size() / 10) {
        return CompressionMethod::DICTIONARY;
    }
    
    return CompressionMethod::NONE;
}

TEST(IntegrationTest, AdaptiveCompressionSelection) {
    // Test 1: Sparse vector
    {
        std::vector<float> sparse_vec(10000, 0.0f);
        for (size_t i = 0; i < 50; ++i) {
            sparse_vec[i * 200] = static_cast<float>(i);
        }
        
        auto method = selectCompressionMethod(sparse_vec);
        EXPECT_EQ(method, CompressionMethod::SPARSE_CSR);
        std::cout << "Sparse vector -> CSR\n";
    }
    
    // Test 2: Integer vector
    {
        std::vector<float> int_vec = {};

        for (int i = 0; i < 1000; ++i) {
            int_vec.push_back(static_cast<float>(100 + i));
        }
        
        auto method = selectCompressionMethod(int_vec);
        EXPECT_EQ(method, CompressionMethod::DELTA_VARINT);
        std::cout << "Integer vector -> Delta+VarInt\n";
    }
    
    // Test 3: Categorical vector
    {
        std::vector<float> cat_vec;
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(0.0f, 5.0f);
        for (int i = 0; i < 1000; ++i) {
            // Quantize to a small non-integer domain to avoid DELTA_VARINT path.
            float raw = dist(rng);
            cat_vec.push_back(std::floor(raw * 2.0f) / 2.0f);
        }
        
        auto method = selectCompressionMethod(cat_vec);
        EXPECT_EQ(method, CompressionMethod::DICTIONARY);
        std::cout << "Categorical vector -> Dictionary\n";
    }
    
    // Test 4: Dense random (should be NONE)
    {
        std::vector<float> dense_vec;
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < 1000; ++i) {
            dense_vec.push_back(dist(rng));
        }
        
        auto method = selectCompressionMethod(dense_vec);
        EXPECT_EQ(method, CompressionMethod::NONE);
        std::cout << "Dense random vector -> NONE (use lossy instead)\n";
    }
}
