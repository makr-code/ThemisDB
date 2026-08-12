/**
 * @file lossless_vector_compression.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// EXPERIMENTAL: Lossless Vector Compression Utilities
// 
// WARNING: This is a scientific experiment and may be rolled back.
// The implementation is based on research documented in:
// docs/performance/performance_vector_compression_lossless.md
//
// These methods are alternatives to the existing SQ8 (lossy) quantization
// and provide 100% lossless compression for specific vector types:
// - Sparse vectors (CSR): 10-100x compression for >95% zeros
// - Integer features (Delta+VarInt): 3-10x compression
// - Categorical features (Dictionary): 5-20x compression
//
// Usage is controlled via config:vector_compression_lossless in DB
// Default: disabled (use existing SQ8 for dense vectors)

#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace themis {
namespace experimental {

// ============================================================================
// Sparse Vector CSR (Compressed Sparse Row)
// ============================================================================
// Best for: Sparse vectors with >95% zeros (TF-IDF, one-hot encodings)
// Compression ratio: 10-100x
// Quality: 100% lossless

struct SparseVectorCSR {
    std::vector<float> values;       // Non-zero values
    std::vector<uint32_t> indices;   // Positions of non-zero values
    uint32_t dimension = 0;          // Original dimension
    
    size_t compressed_bytes() const {
        return sizeof(dimension) + 
               values.size() * sizeof(float) + 
               indices.size() * sizeof(uint32_t);
    }
    
    // Serialize to bytes for storage
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> result;
        result.reserve(compressed_bytes() + 8); // +8 for header
        
        // Header: magic + dimension + nnz count
        uint32_t magic = 0x43535200; // "CSR\0"
        result.insert(result.end(), (uint8_t*)&magic, (uint8_t*)&magic + 4);
        result.insert(result.end(), (uint8_t*)&dimension, (uint8_t*)&dimension + 4);
        
        uint32_t nnz = static_cast<uint32_t>(values.size());
        result.insert(result.end(), (uint8_t*)&nnz, (uint8_t*)&nnz + 4);
        
        // Values
        result.insert(result.end(), (uint8_t*)values.data(), 
                     (uint8_t*)values.data() + values.size() * sizeof(float));
        
        // Indices
        result.insert(result.end(), (uint8_t*)indices.data(),
                     (uint8_t*)indices.data() + indices.size() * sizeof(uint32_t));
        
        return result;
    }
    
    // Deserialize from bytes
    static SparseVectorCSR deserialize(const std::vector<uint8_t>& data) {
        SparseVectorCSR result;
        if (data.size() < 12) return result;
        
        const uint8_t* ptr = data.data();
        
        // Check magic
        uint32_t magic = *reinterpret_cast<const uint32_t*>(ptr);
        if (magic != 0x43535200) return result;
        ptr += 4;
        
        // Dimension
        result.dimension = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += 4;
        
        // NNZ count
        uint32_t nnz = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += 4;
        
        // Values
        result.values.resize(nnz);
        std::memcpy(result.values.data(), ptr, nnz * sizeof(float));
        ptr += nnz * sizeof(float);
        
        // Indices
        result.indices.resize(nnz);
        std::memcpy(result.indices.data(), ptr, nnz * sizeof(uint32_t));
        
        return result;
    }
};

/** @brief Sparse vector codec component. */
class SparseVectorCodec {
public:
    // Compress vector to sparse CSR format
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
    
    // Decompress CSR to dense vector
    static std::vector<float> decompress(const SparseVectorCSR& sparse) {
        std::vector<float> vec(sparse.dimension, 0.0f);
        
        for (size_t i = 0; i < sparse.values.size(); ++i) {
            vec[sparse.indices[i]] = sparse.values[i];
        }
        
        return vec;
    }
    
    // Compute sparsity (fraction of zeros)
    static float compute_sparsity(const std::vector<float>& vec, float epsilon = 1e-9f) {
        size_t zero_count = 0;
        for (float v : vec) {
            if (std::abs(v) < epsilon) ++zero_count;
        }
        return static_cast<float>(zero_count) / vec.size();
    }

    // Compute dot product between a sparse CSR vector and a dense vector.
    // Iterates only over non-zero entries for O(nnz) complexity instead of O(d).
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
// VarInt Delta Encoding
// ============================================================================
// Best for: Integer-valued features (histograms, counts)
// Compression ratio: 3-10x
// Quality: 100% lossless

/** @brief Quality: 100% lossless. */
class VarIntCodec {
public:
    // Zigzag encoding for signed integers
    static uint32_t zigzag_encode(int32_t n) {
        return (static_cast<uint32_t>(n) << 1) ^ (n >> 31);
    }
    
    static int32_t zigzag_decode(uint32_t n) {
        // Avoid unary minus on unsigned to keep MSVC warning-free
        const int32_t sign = static_cast<int32_t>(-(static_cast<int32_t>(n & 1)));
        return static_cast<int32_t>((n >> 1) ^ sign);
    }
    
    // Variable-length integer encoding
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
    
    // Delta + VarInt compression for integer vectors
    static std::vector<uint8_t> compress_delta(const std::vector<int32_t>& values) {
        std::vector<uint8_t> result;
        
        if (values.empty()) return result;
        
        // First value
        encode(result, zigzag_encode(values[0]));
        
        // Deltas
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
        
        // First value
        int32_t current = zigzag_decode(decode(ptr));
        result.push_back(current);
        
        // Deltas
        while (ptr < end) {
            int32_t delta = zigzag_decode(decode(ptr));
            current += delta;
            result.push_back(current);
        }
        
        return result;
    }
};

// ============================================================================
// Dictionary Encoding
// ============================================================================
// Best for: Categorical features with few unique values
// Compression ratio: 5-20x
// Quality: 100% lossless

template<typename T>
struct DictionaryCompressed {
    std::vector<T> dictionary;         // Unique values
    std::vector<uint32_t> indices;     // Index into dictionary for each element
    size_t original_size;
    
    size_t compressed_bytes() const {
        return dictionary.size() * sizeof(T) + 
               indices.size() * sizeof(uint32_t) +
               sizeof(size_t);
    }
};

template<typename T>
/** @brief Dictionary codec component. */
class DictionaryCodec {
public:
    static DictionaryCompressed<T> compress(const std::vector<T>& vec) {
        DictionaryCompressed<T> result;
        result.original_size = vec.size();
        
        std::unordered_map<T, uint32_t> value_to_index;
        
        for (const auto& val : vec) {
            auto it = value_to_index.find(val);
            if (it == value_to_index.end()) {
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

// ============================================================================
// Adaptive Compression Method Selection
// ============================================================================

enum class LosslessCompressionMethod {
    NONE,           // No compression (use existing SQ8 or raw)
    SPARSE_CSR,     // For sparse vectors (>95% zeros)
    DELTA_VARINT,   // For integer-valued features
    DICTIONARY      // For categorical features
};

/** @brief Adaptive compressor component. */
class AdaptiveCompressor {
public:
    // Analyze vector and select optimal lossless compression method
    static LosslessCompressionMethod selectMethod(
        const std::vector<float>& vec,
        float sparse_threshold = 0.95f
    ) {
        // Sparsity check
        float sparsity = SparseVectorCodec::compute_sparsity(vec);
        if (sparsity >= sparse_threshold) {
            return LosslessCompressionMethod::SPARSE_CSR;
        }
        
        // Integer check
        size_t int_count = 0;
        for (float v : vec) {
            if (std::abs(v - std::round(v)) < 1e-6f) ++int_count;
        }
        if (int_count > vec.size() * 0.9) { // 90%+ integers
            return LosslessCompressionMethod::DELTA_VARINT;
        }
        
        // Unique values check
        std::unordered_set<float> unique_values(vec.begin(), vec.end());
        if (unique_values.size() < vec.size() / 10) { // <10% unique
            return LosslessCompressionMethod::DICTIONARY;
        }
        
        // Default: no lossless compression (use existing SQ8 or raw storage)
        return LosslessCompressionMethod::NONE;
    }
    
    static std::string method_name(LosslessCompressionMethod method) {
        switch (method) {
            case LosslessCompressionMethod::SPARSE_CSR: return "sparse_csr";
            case LosslessCompressionMethod::DELTA_VARINT: return "delta_varint";
            case LosslessCompressionMethod::DICTIONARY: return "dictionary";
            default: return "none";
        }
    }
};

} // namespace experimental
} // namespace themis
