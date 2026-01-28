#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {

/**
 * @brief Binary Quantization for Maximum Vector Compression
 * 
 * Implements binary quantization that represents each vector dimension as a single bit (±1),
 * achieving 32x compression for typical floating-point vectors. While accuracy is reduced
 * compared to Product Quantization, binary quantization is extremely fast and memory-efficient,
 * making it ideal for filtering, pre-ranking, and memory-constrained environments.
 * 
 * @sources
 * - Algorithm: Binary Quantization / Sign-based Hashing
 * - Paper: Gong, Y., & Lazebnik, S. (2011). 
 *          "Iterative Quantization: A Procrustean Approach to Learning Binary Codes"
 *          IEEE Conference on Computer Vision and Pattern Recognition (CVPR)
 * - DOI: 10.1109/CVPR.2011.5995432
 * - Paper: Joulin, A., et al. (2016).
 *          "FastText.zip: Compressing text classification models"
 *          arXiv:1612.03651
 * - Implementation: Optimized for ThemisDB with hardware acceleration
 * 
 * Features:
 * - 1 bit per dimension (32x compression vs float32)
 * - Hardware-accelerated Hamming distance (SIMD popcount)
 * - Optional centering for improved accuracy
 * - Fast encoding and distance computation
 * 
 * Part of ThemisDB v1.4.1 - Feature: Vector Compression Research (#914)
 */
class BinaryQuantizer {
public:
    struct Config {
        bool center_values;       // Center around mean before quantization
        bool normalize_input;     // Normalize input vectors to unit length
        float scale_factor;       // Scaling factor for reconstruction (auto-learned if 0)
        
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

    /**
     * @brief Train quantizer statistics from training vectors
     * Learns mean and scale for centering/reconstruction
     * @param training_vectors Training data (num_vectors x dimension)
     * @return Status indicating success or failure
     */
    Status train(const std::vector<std::vector<float>>& training_vectors);

    /**
     * @brief Encode a vector into binary codes
     * Each bit represents the sign of (value - mean)
     * @param vector Input vector (dimension floats)
     * @return Binary codes (dimension/8 bytes, packed)
     */
    std::vector<uint8_t> encode(const std::vector<float>& vector) const;

    /**
     * @brief Decode binary codes back to approximate vector
     * @param codes Binary codes (dimension/8 bytes)
     * @return Reconstructed vector (dimension floats, values are ±scale)
     */
    std::vector<float> decode(const std::vector<uint8_t>& codes) const;

    /**
     * @brief Compute Hamming distance between two binary code vectors
     * Fast hardware-accelerated distance using popcount
     * @param codes_a First binary codes
     * @param codes_b Second binary codes
     * @return Hamming distance (number of differing bits)
     */
    float hammingDistance(const std::vector<uint8_t>& codes_a,
                         const std::vector<uint8_t>& codes_b) const;

    /**
     * @brief Compute asymmetric distance between query and binary codes
     * Query is in full precision, database vector is binarized
     * @param query Query vector (dimension floats)
     * @param codes Binary codes (dimension/8 bytes)
     * @return Approximate L2 distance
     */
    float asymmetricDistance(const std::vector<float>& query,
                            const std::vector<uint8_t>& codes) const;

    /**
     * @brief Check if quantizer is trained
     */
    bool isTrained() const { return trained_; }

    /**
     * @brief Get compression ratio
     * @return Ratio of original size to compressed size (typically 32.0 for float32)
     */
    float getCompressionRatio() const {
        return (dimension_ * sizeof(float)) / static_cast<float>(getEncodedSize());
    }

    /**
     * @brief Get encoded size in bytes
     */
    size_t getEncodedSize() const {
        return (dimension_ + 7) / 8;  // Round up to nearest byte
    }

    /**
     * @brief Get memory usage in bytes for quantizer metadata
     */
    size_t getMemoryUsage() const {
        return sizeof(*this) + mean_values_.size() * sizeof(float);
    }

    // Getters
    int getDimension() const { return dimension_; }
    float getMean(int dim) const { return mean_values_[dim]; }
    float getScale() const { return scale_; }

private:
    int dimension_;
    Config config_;
    bool trained_ = false;
    
    // Per-dimension mean values (for centering)
    std::vector<float> mean_values_;
    
    // Global scale factor for reconstruction
    float scale_ = 1.0f;

    /**
     * @brief Compute mean value across all dimensions
     */
    float computeMean(const std::vector<float>& vector) const;

    /**
     * @brief Compute L2 norm of vector
     */
    float computeNorm(const std::vector<float>& vector) const;

    /**
     * @brief Count set bits in a byte (popcount)
     */
    static int popcount(uint8_t byte);
};

} // namespace themis
