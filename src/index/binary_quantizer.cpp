#include "index/binary_quantizer.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace themis {

BinaryQuantizer::BinaryQuantizer(int dimension, const Config& config)
    : dimension_(dimension), config_(config) {
    
    if (dimension_ <= 0) {
        throw std::invalid_argument("Dimension must be positive");
    }
    
    // Pre-allocate mean values
    if (config_.center_values) {
        mean_values_.resize(dimension_, 0.0f);
    }
}

BinaryQuantizer::Status BinaryQuantizer::train(
    const std::vector<std::vector<float>>& training_vectors) {
    
    if (training_vectors.empty()) {
        return Status::Error("No training vectors provided");
    }
    
    if (training_vectors[0].size() != static_cast<size_t>(dimension_)) {
        return Status::Error("Training vector dimension mismatch");
    }
    
    THEMIS_INFO("BinaryQuantizer::train - Training with {} vectors, dim={}",
                training_vectors.size(), dimension_);
    
    // Compute per-dimension means for centering
    if (config_.center_values) {
        std::fill(mean_values_.begin(), mean_values_.end(), 0.0f);
        
        for (const auto& vec : training_vectors) {
            for (int d = 0; d < dimension_; d++) {
                mean_values_[d] += vec[d];
            }
        }
        
        for (int d = 0; d < dimension_; d++) {
            mean_values_[d] /= training_vectors.size();
        }
    }
    
    // Learn scale factor if not provided
    if (config_.scale_factor <= 0.0f) {
        // Use mean absolute value across all dimensions
        double sum_abs = 0.0;
        size_t count = 0;
        
        for (const auto& vec : training_vectors) {
            for (int d = 0; d < dimension_; d++) {
                float centered = config_.center_values 
                    ? vec[d] - mean_values_[d]
                    : vec[d];
                sum_abs += std::abs(centered);
                count++;
            }
        }
        
        scale_ = static_cast<float>(sum_abs / count);
    } else {
        scale_ = config_.scale_factor;
    }
    
    if (scale_ <= 0.0f) {
        scale_ = 1.0f;  // Fallback
    }
    
    trained_ = true;
    THEMIS_INFO("BinaryQuantizer::train - Training complete. Scale: {:.4f}, Compression ratio: {:.1f}x",
                scale_, getCompressionRatio());
    
    return Status::OK();
}

std::vector<uint8_t> BinaryQuantizer::encode(const std::vector<float>& vector) const {
    if (vector.size() != static_cast<size_t>(dimension_)) {
        THEMIS_ERROR("BinaryQuantizer::encode - Dimension mismatch: {} vs {}",
                     vector.size(), dimension_);
        return {};
    }
    
    // Normalize if requested
    std::vector<float> input = vector;
    if (config_.normalize_input) {
        float norm = computeNorm(input);
        if (norm > 0.0f) {
            for (float& val : input) {
                val /= norm;
            }
        }
    }
    
    // Compute number of bytes needed
    size_t num_bytes = getEncodedSize();
    std::vector<uint8_t> codes(num_bytes, 0);
    
    // Encode each dimension as a bit
    for (int d = 0; d < dimension_; d++) {
        float value = input[d];
        
        // Center around mean if configured
        if (config_.center_values && !mean_values_.empty()) {
            value -= mean_values_[d];
        }
        
        // Set bit if value >= 0
        if (value >= 0.0f) {
            int byte_idx = d / 8;
            int bit_idx = d % 8;
            codes[byte_idx] |= (1 << bit_idx);
        }
    }
    
    return codes;
}

std::vector<float> BinaryQuantizer::decode(const std::vector<uint8_t>& codes) const {
    size_t expected_size = getEncodedSize();
    if (codes.size() != expected_size) {
        THEMIS_ERROR("BinaryQuantizer::decode - Code size mismatch: {} vs {}",
                     codes.size(), expected_size);
        return {};
    }
    
    std::vector<float> vector(dimension_);
    
    // Decode each bit
    for (int d = 0; d < dimension_; d++) {
        int byte_idx = d / 8;
        int bit_idx = d % 8;
        
        // Extract bit and convert to ±scale
        bool bit_set = (codes[byte_idx] & (1 << bit_idx)) != 0;
        float value = bit_set ? scale_ : -scale_;
        
        // Add mean back if centered
        if (config_.center_values && !mean_values_.empty()) {
            value += mean_values_[d];
        }
        
        vector[d] = value;
    }
    
    return vector;
}

float BinaryQuantizer::hammingDistance(const std::vector<uint8_t>& codes_a,
                                      const std::vector<uint8_t>& codes_b) const {
    if (codes_a.size() != codes_b.size()) {
        THEMIS_ERROR("BinaryQuantizer::hammingDistance - Code size mismatch");
        return std::numeric_limits<float>::max();
    }
    
    int distance = 0;
    
    // XOR and count set bits
    for (size_t i = 0; i < codes_a.size(); i++) {
        uint8_t xor_result = codes_a[i] ^ codes_b[i];
        distance += popcount(xor_result);
    }
    
    return static_cast<float>(distance);
}

float BinaryQuantizer::asymmetricDistance(const std::vector<float>& query,
                                         const std::vector<uint8_t>& codes) const {
    if (query.size() != static_cast<size_t>(dimension_)) {
        THEMIS_ERROR("BinaryQuantizer::asymmetricDistance - Query dimension mismatch");
        return std::numeric_limits<float>::max();
    }
    
    // Decode codes and compute L2 distance
    auto decoded = decode(codes);
    if (decoded.empty()) {
        return std::numeric_limits<float>::max();
    }
    
    float distance = 0.0f;
    for (int d = 0; d < dimension_; d++) {
        float diff = query[d] - decoded[d];
        distance += diff * diff;
    }
    
    return std::sqrt(distance);
}

float BinaryQuantizer::computeMean(const std::vector<float>& vector) const {
    if (vector.empty()) {
        return 0.0f;
    }
    
    double sum = 0.0;
    for (float val : vector) {
        sum += val;
    }
    
    return static_cast<float>(sum / vector.size());
}

float BinaryQuantizer::computeNorm(const std::vector<float>& vector) const {
    double sum_sq = 0.0;
    for (float val : vector) {
        sum_sq += val * val;
    }
    return std::sqrt(sum_sq);
}

int BinaryQuantizer::popcount(uint8_t byte) {
    // Use built-in popcount if available (GCC/Clang)
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(byte);
#else
    // Fallback: Brian Kernighan's algorithm
    int count = 0;
    while (byte) {
        byte &= (byte - 1);
        count++;
    }
    return count;
#endif
}

} // namespace themis
