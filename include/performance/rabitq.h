/**
 * @file rabitq.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// RaBitQ: Quantization for High-Dimensional Vector Search
// Paper: "RaBitQ: Quantizing High-Dimensional Vectors with a Theoretical Error Bound for Approximate Nearest Neighbor Search" (SIGMOD'24)
// Authors: Jianyang Gao, Cheng Long (NTU Singapore)
//
// Key idea: 2-bit product quantization with theoretical error bounds
// Expected gain: 16x memory reduction, +50-80% throughput
// Reference: https://dl.acm.org/doi/10.1145/3626246.3653368

#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <cmath>
#include <memory>

namespace themis {
namespace performance {

/// 2-bit quantized vector representation
/// Reduces float32 (4 bytes) to 2 bits = 16x compression
class RaBitQVector {
public:
    // Pack 4 2-bit values into 1 byte
    static constexpr int BITS_PER_VALUE = 2;
    static constexpr int VALUES_PER_BYTE = 8 / BITS_PER_VALUE; // 4
    
    RaBitQVector() : dimension_(0) {}
    
    explicit RaBitQVector(size_t dimension) 
        : dimension_(dimension),
          quantized_data_((dimension + VALUES_PER_BYTE - 1) / VALUES_PER_BYTE) {}
    
    size_t dimension() const { return dimension_; }
    size_t compressed_size() const { return quantized_data_.size(); }
    const std::vector<uint8_t>& data() const { return quantized_data_; }
    
    // Get 2-bit value at index
    uint8_t get(size_t index) const {
        size_t byte_idx = index / VALUES_PER_BYTE;
        size_t bit_offset = (index % VALUES_PER_BYTE) * BITS_PER_VALUE;
        return (quantized_data_[byte_idx] >> bit_offset) & 0x3; // Mask 2 bits
    }
    
    // Set 2-bit value at index
    void set(size_t index, uint8_t value) {
        size_t byte_idx = index / VALUES_PER_BYTE;
        size_t bit_offset = (index % VALUES_PER_BYTE) * BITS_PER_VALUE;
        quantized_data_[byte_idx] &= ~(0x3 << bit_offset); // Clear 2 bits
        quantized_data_[byte_idx] |= ((value & 0x3) << bit_offset); // Set 2 bits
    }

private:
    size_t dimension_;
    std::vector<uint8_t> quantized_data_;
};

/// RaBitQ encoder with learned quantization parameters
class RaBitQEncoder {
public:
    explicit RaBitQEncoder(size_t dimension);
    
    // Train quantization parameters from dataset
    void train(const std::vector<std::vector<float>>& training_data);
    
    // Encode float32 vector to 2-bit representation
    RaBitQVector encode(const std::vector<float>& vec) const;
    
    // Decode 2-bit vector back to float32 (for verification)
    std::vector<float> decode(const RaBitQVector& quantized) const;
    
    // Compute quantized distance (faster than decoding + computing distance)
    float compute_distance(const RaBitQVector& a, const RaBitQVector& b) const;
    
    // Asymmetric distance: query (full precision) vs database vector (quantized)
    float asymmetric_distance(const std::vector<float>& query, const RaBitQVector& db_vector) const;

private:
    size_t dimension_;
    std::vector<float> mean_;       // Per-dimension mean
    std::vector<float> scale_;      // Per-dimension scale
    
    // Quantization thresholds for 2-bit encoding
    std::vector<std::array<float, 3>> thresholds_; // 3 thresholds for 4 bins
    
    // Helper: Quantize single float value to 2 bits
    uint8_t quantize_value(float value, size_t dim) const;
    
    // Helper: Dequantize 2-bit value to float
    float dequantize_value(uint8_t quantized, size_t dim) const;
};

/// RaBitQ-accelerated vector index
class RaBitQIndex {
public:
    RaBitQIndex(size_t dimension, size_t max_capacity = 1000000);
    
    // Train encoder with initial vectors
    void train(const std::vector<std::vector<float>>& training_vectors);
    
    // Add vector to index
    void add(uint64_t id, const std::vector<float>& vector);
    
    // Search for k nearest neighbors
    struct SearchResult {
        uint64_t id = 0;
        float distance;
    };
    std::vector<SearchResult> search(const std::vector<float>& query, int k) const;
    
    // Get memory usage statistics
    struct MemoryStats {
        size_t uncompressed_bytes = 0;  // Original float32 size
        size_t compressed_bytes;    // 2-bit quantized size
        double compression_ratio;
    };
    MemoryStats get_memory_stats() const;
    
    size_t size() const { return vectors_.size(); }

private:
    size_t dimension_;
    std::unique_ptr<RaBitQEncoder> encoder_;
    
    // Storage: ID -> quantized vector
    std::vector<uint64_t> ids_;
    std::vector<RaBitQVector> vectors_;
    
    // Helper: Linear scan with quantized distance
    std::vector<SearchResult> linear_scan(const std::vector<float>& query, int k) const;
};

/// Product Quantization helper (for sub-vector encoding)
/// Used internally by RaBitQ for better compression
class ProductQuantizer {
public:
    ProductQuantizer(size_t dimension, size_t num_subvectors);
    
    // Split vector into subvectors
    std::vector<std::vector<float>> split_vector(const std::vector<float>& vec) const;
    
    // Train codebooks for each subvector
    void train(const std::vector<std::vector<float>>& training_data);
    
    // Encode vector using product quantization
    std::vector<uint8_t> encode(const std::vector<float>& vec) const;

private:
    size_t dimension_;
    size_t num_subvectors_;
    size_t subvector_dimension_;
    
    // Codebooks for each subvector (k-means centroids)
    std::vector<std::vector<std::vector<float>>> codebooks_;
};

} // namespace performance
} // namespace themis
