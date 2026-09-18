/**
 * @file binary_quantizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace themis {

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
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK() { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /**
     * @brief Binary Quantizer.
     * @param[in] dimension Input parameter.
     * @return Return value.
     */
    explicit BinaryQuantizer(int dimension);
    /**
     * @brief Binary Quantizer.
     * @param[in] dimension Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit BinaryQuantizer(int dimension, const Config& config);
    
    ~BinaryQuantizer();

    /**
     * @brief Train.
     * @param[in] training_vectors Input parameter.
     * @return Return value.
     */
    Status train(const std::vector<std::vector<float>>& training_vectors);

    /**
     * @brief Encode.
     * @param[in] vector Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> encode(const std::vector<float>& vector) const;

    /**
     * @brief Decode.
     * @param[in] codes Input parameter.
     * @return Return value.
     */
    std::vector<float> decode(const std::vector<uint8_t>& codes) const;

    /**
     * @brief Hamming Distance.
     * @param[in] codes_a Input parameter.
     * @param[in] codes_b Input parameter.
     * @return Return value.
     */
    float hammingDistance(const std::vector<uint8_t>& codes_a,
                         const std::vector<uint8_t>& codes_b) const;

    /**
     * @brief Asymmetric Distance.
     * @param[in] query Input parameter.
     * @param[in] codes Input parameter.
     * @return Return value.
     */
    float asymmetricDistance(const std::vector<float>& query,
                            const std::vector<uint8_t>& codes) const;

    bool isTrained() const { return trained_; }

    float getCompressionRatio() const { return 32.0f; }  // float32 -> 1 bit = 32x

    /**
     * @brief Get Memory Usage.
     * @return Return value.
     */
    size_t getMemoryUsage() const;

    int getDimension() const { return dimension_; }

    float getScale() const { return scale_; }  // Get learned scale factor
    
    size_t getEncodedSize() const {
        return (dimension_ + 7) / 8;  // Ceiling division for bit packing
    }
    
    /**
     * @brief Get Backend.
     * @return Pointer to the result.
     */
    const char* getBackend() const;

private:
    int dimension_;
    Config config_;
    bool trained_ = false;
    float scale_ = 1.0f;
    std::vector<float> mean_values_;

    // Helper methods
    /**
     * @brief Compute Norm.
     * @param[in] vector Input parameter.
     * @return Return value.
     */
    float computeNorm(const std::vector<float>& vector) const;
    /**
     * @brief Popcount.
     * @param[in] byte Input parameter.
     * @return Return value.
     */
    int popcount(uint8_t byte) const;
    
    // Backend tracking
    bool use_faiss_ = false;
};

} // namespace themis
