/**
 * @file binary_quantizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace themis {

/**
 * @brief Binary Quantization for Maximum Vector Compression
 * 
 * v1.5.0 - FAISS-optimized Binary Quantizer with Fallback
 * 
 * Binary quantization compresses float32 vectors to binary (1 bit per dimension),
 * achieving 32x compression ratio. Uses sign of (value - mean) for binarization.
 * 
 * FAISS Integration: When THEMIS_HAS_FAISS is defined and prefer_faiss is true,
 * uses compiler intrinsics (same as FAISS uses internally) for optimized Hamming
 * distance computation with SIMD instructions.
 * 
 * Sources:
 * - Algorithm: Locality Sensitive Hashing (LSH) / Binary Quantization
 * - Implementation: Custom ThemisDB with optional FAISS-style optimizations
 * - Library: https://github.com/facebookresearch/faiss
 * - For production use: Consider FAISS IndexBinaryFlat directly or AdvancedVectorIndex
 * 
 * Part of ThemisDB v1.5.0 - FAISS Integration (#1079)
 */
class BinaryQuantizer {
public:
    struct Config {
        bool center_values = 0;       // Center vectors before binarization
        bool normalize_input;     // Normalize input vectors
        float scale_factor;       // Manual scale factor (0 = auto-learn)
        bool prefer_faiss;        // Prefer FAISS-style optimizations if available (default: true)
        
        Config() 
            : center_values(true)
            , normalize_input(false)
            , scale_factor(0.0f)
            , prefer_faiss(true)
        {}
    };

    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /**
     * @brief Construct a new Binary Quantizer.
     * @param dimension Vector dimension.
     */
    explicit BinaryQuantizer(int dimension);
    /**
     * @brief Construct a new Binary Quantizer.
     * @param dimension Vector dimension.
     * @param config Configuration parameters.
     */
    explicit BinaryQuantizer(int dimension, const Config& config);
    
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

    /**
     * @brief Check if the quantizer has been trained.
     * @return True if trained, false otherwise.
     */
    bool isTrained() const { return trained_; }

    /**
     * @brief Get the compression ratio achieved by this quantizer.
     * @return Compression ratio (32.0 for binary quantization: float32 -> 1 bit).
     */
    float getCompressionRatio() const { return 32.0f; }  // float32 -> 1 bit = 32x

    /**
     * @brief Get total memory usage of the quantizer.
     * @return Memory usage in bytes.
     */
    size_t getMemoryUsage() const;

    /**
     * @brief Get the dimensionality of vectors handled by this quantizer.
     * @return Vector dimension.
     */
    int getDimension() const { return dimension_; }

    /**
     * @brief Get the learned scale factor used during quantization.
     * @return Scale factor.
     */
    float getScale() const { return scale_; }  // Get learned scale factor
    
    /**
     * @brief Get encoded size in bytes.
     * @return Number of bytes needed to store one encoded vector.
     */
    size_t getEncodedSize() const {
        return (dimension_ + 7) / 8;  // Ceiling division for bit packing
    }
    
    /**
     * @brief Check which backend is being used.
     * @return Backend name: "faiss" or "custom".
     */
    const char* getBackend() const;

private:
    int dimension_;
    Config config_;
    bool trained_ = false;
    float scale_ = 1.0f;
    std::vector<float> mean_values_;

    // Helper methods
    float computeNorm(const std::vector<float>& vector) const;
    int popcount(uint8_t byte) const;
    
    // Backend tracking
    bool use_faiss_ = false;
};

} // namespace themis
