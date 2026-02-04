#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace themis {

/**
 * @brief Binary Quantization for Maximum Vector Compression
 * 
 * SIMPLIFIED IMPLEMENTATION - FAISS-Compatible (v1.4.1)
 * Uses binary hashing approach compatible with FAISS IndexBinaryFlat principles.
 * 
 * Binary quantization compresses float32 vectors to binary (1 bit per dimension),
 * achieving 32x compression ratio. Uses sign of (value - mean) for binarization.
 * 
 * @deprecated NOT USED IN PRODUCTION CODE. Consider using FAISS IndexBinaryFlat directly.
 * 
 * @sources
 * - Algorithm: Locality Sensitive Hashing (LSH) / Binary Quantization
 * - Implementation: Simplified, compatible with FAISS IndexBinaryFlat principles
 * - For production use: Prefer FAISS IndexBinaryFlat directly
 * 
 * Part of ThemisDB v1.4.1 - Feature: Vector Compression Research (#914)
 * Simplified: Reduced from 231 to ~120 lines, marked as deprecated
 */
class BinaryQuantizer {
public:
    struct Config {
        bool center_values;       // Center vectors before binarization
        bool normalize_input;     // Normalize input vectors
        float scale_factor;       // Manual scale factor (0 = auto-learn)
        
        Config() 
            : center_values(true)
            , normalize_input(false)
            , scale_factor(0.0f)
        {}
    };

    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /**
     * @brief Construct a new Binary Quantizer
     * @param dimension Vector dimension
     * @param config Configuration parameters
     */
    explicit BinaryQuantizer(int dimension, const Config& config = Config());
    
    ~BinaryQuantizer();

    /**
     * @brief Train quantizer to learn centering and scaling parameters
     * @param training_vectors Training data
     * @return Status indicating success or failure
     */
    Status train(const std::vector<std::vector<float>>& training_vectors);

    /**
     * @brief Encode vector to binary representation
     * @param vector Input vector (dimension floats)
     * @return Binary codes (dimension/8 bytes, packed)
     */
    std::vector<uint8_t> encode(const std::vector<float>& vector) const;

    /**
     * @brief Decode binary codes back to approximate vector
     * @param codes Binary codes
     * @return Reconstructed vector
     */
    std::vector<float> decode(const std::vector<uint8_t>& codes) const;

    /**
     * @brief Compute Hamming distance between two binary codes
     * @param codes_a First binary codes
     * @param codes_b Second binary codes
     * @return Hamming distance (number of differing bits)
     */
    float hammingDistance(const std::vector<uint8_t>& codes_a,
                         const std::vector<uint8_t>& codes_b) const;

    /**
     * @brief Compute asymmetric distance: full-precision query vs binary database vector
     * @param query Query vector (full precision)
     * @param codes Binary codes
     * @return Approximate distance
     */
    float asymmetricDistance(const std::vector<float>& query,
                            const std::vector<uint8_t>& codes) const;

    bool isTrained() const { return trained_; }
    float getCompressionRatio() const { return 32.0f; }  // float32 -> 1 bit = 32x
    size_t getMemoryUsage() const { return sizeof(BinaryQuantizer) + mean_values_.capacity() * sizeof(float); }
    int getDimension() const { return dimension_; }

private:
    int dimension_;
    Config config_;
    bool trained_ = false;
    float scale_ = 1.0f;
    std::vector<float> mean_values_;

    // Helper methods
    float computeNorm(const std::vector<float>& vector) const;
    int popcount(uint8_t byte) const;
};

} // namespace themis
