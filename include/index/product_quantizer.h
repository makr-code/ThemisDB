/**
 * @file product_quantizer.h
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

// Forward declare FAISS types
namespace faiss {
    class ProductQuantizer;
}

namespace themis {

/**
 * @brief Product Quantization for Vector Compression (FAISS-native with fallback)
 * 
 * v1.5.0 - Custom implementation with optional FAISS acceleration
 * 
 * Custom implementation of Product Quantization (PQ) for compressing high-dimensional vectors
 * from float32 (e.g., 1536D = 6KB) to 8-bit codes (e.g., 192 bytes).
 * 
 * NOTE: FAISS IndexIVFPQ doesn't expose standalone encode/decode methods needed by this interface.
 * This implementation provides those standalone operations. For integrated search with quantization,
 * consider using AdvancedVectorIndex which wraps FAISS IndexIVFPQ directly.
 * 
 * FAISS Integration: When THEMIS_HAS_FAISS is defined and prefer_faiss is true, uses FAISS
 * K-means clustering for training (20-30% faster with SIMD optimizations). Encoding/decoding
 * uses custom implementation since FAISS doesn't expose standalone methods for that.
 * FAISS-native implementation with fallback for Product Quantization (PQ) for compressing 
 * high-dimensional vectors from float32 (e.g., 1536D = 6KB) to 8-bit codes (e.g., 192 bytes).
 * 
 * This implementation uses FAISS's optimized ProductQuantizer when available (THEMIS_HAS_FAISS),
 * providing better performance through SIMD optimizations and potential GPU acceleration.
 * Falls back to custom K-means implementation when FAISS is not available.
 * 
 * Sources:
 * - Library: FAISS (Facebook AI Similarity Search)
 * - Repository: https://github.com/facebookresearch/faiss
 * - License: MIT
 * - FAISS Class: faiss::ProductQuantizer (faiss/impl/ProductQuantizer.h)
 * - Algorithm: Jégou, H., Douze, M., & Schmid, C. (2011). 
 *   "Product Quantization for Nearest Neighbor Search"
 *   IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI)
 * - DOI: 10.1109/TPAMI.2010.57
 * - URL: https://hal.inria.fr/inria-00514462
 * - Implementation: Custom ThemisDB implementation with optional FAISS K-means acceleration
 * - FAISS Alternative: faiss::IndexIVFPQ (integrated search, not standalone encode/decode)
 * 
 * Part of ThemisDB v1.5.0 - FAISS K-means Integration (#1079)
 * 
 * Part of ThemisDB v1.4.2 - Migration to FAISS native quantizers
 * Previous: Custom implementation (v1.3.0-v1.4.1)
 */
class ProductQuantizer {
public:
    struct Config {
        int num_subquantizers;      // Number of subquantizers (divides dimension)
        int num_centroids;        // Number of centroids per subquantizer (8-bit = 256)
        int max_iterations;        // K-means max iterations
        float convergence_threshold;  // K-means convergence threshold
        bool prefer_faiss;         // Prefer FAISS K-means acceleration if available (default: true)
        
        // Default constructor with default values
        Config() 
            : num_subquantizers(8)
            , num_centroids(256)
            , max_iterations(25)
            , convergence_threshold(0.001f)
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
     * @brief Construct a new Product Quantizer
     * @param dimension Vector dimension (must be divisible by num_subquantizers)
     * @param config Configuration parameters
     */
    explicit ProductQuantizer(int dimension, const Config& config);
    
    ~ProductQuantizer();
    
    // Prevent copying due to unique_ptr/FAISS internals
    ProductQuantizer(const ProductQuantizer&) = delete;
    ProductQuantizer& operator=(const ProductQuantizer&) = delete;
    
    // Allow moving
    ProductQuantizer(ProductQuantizer&&) noexcept;
    ProductQuantizer& operator=(ProductQuantizer&&) noexcept;

    /**
     * @brief Train quantizer using K-means on training vectors
     * @param training_vectors Training data (num_vectors x dimension)
     * @return Status indicating success or failure
     */
    Status train(const std::vector<std::vector<float>>& training_vectors);

    /**
     * @brief Encode a vector into quantized codes
     * @param vector Input vector (dimension floats)
     * @return Quantized codes (num_subquantizers bytes)
     */
    std::vector<uint8_t> encode(const std::vector<float>& vector) const;

    /**
     * @brief Decode quantized codes back to approximate vector
     * @param codes Quantized codes (num_subquantizers bytes)
     * @return Reconstructed vector (dimension floats)
     */
    std::vector<float> decode(const std::vector<uint8_t>& codes) const;

    /**
     * @brief Compute asymmetric distance between query and quantized codes
     * Faster than decode + distance, directly computes distance from codes
     * @param query Query vector (dimension floats)
     * @param codes Quantized codes (num_subquantizers bytes)
     * @return L2 distance
     */
    float computeAsymmetricDistance(const std::vector<float>& query, 
                                   const std::vector<uint8_t>& codes) const;

    /**
     * @brief Check if quantizer is trained
     */
    bool isTrained() const { return trained_; }

    /**
     * @brief Get compression ratio
     * @return Ratio of original size to compressed size (e.g., 32.0 for 6KB->192B)
     */
    float getCompressionRatio() const;

    /**
     * @brief Get memory usage in bytes
     */
    size_t getMemoryUsage() const;

    // Getters
    int getDimension() const { return dimension_; }
    int getNumSubquantizers() const { return config_.num_subquantizers; }
    int getSubvectorDim() const { return subvector_dim_; }
    
    /**
     * @brief Check which backend is being used for training
     * @return "faiss" if using FAISS K-means, "custom" if using custom implementation
     */
    const char* getBackend() const;

private:
    int dimension_;
    int subvector_dim_;  // dimension / num_subquantizers
    Config config_;
    bool trained_ = false;
    bool use_faiss_ = false;  // Track if using FAISS acceleration

#ifdef THEMIS_HAS_FAISS
    // FAISS ProductQuantizer (when FAISS is available)
    std::unique_ptr<faiss::ProductQuantizer> faiss_pq_;
#else
    // Fallback: Custom codebooks for non-FAISS builds
    // [subquantizer_idx][centroid_idx][subvector_dim]
    std::vector<std::vector<std::vector<float>>> codebooks_;
#endif

#ifndef THEMIS_HAS_FAISS
    // Fallback implementations used when FAISS is not available
    std::vector<std::vector<float>> runKMeans(
        const std::vector<std::vector<float>>& subvector_data) const;
    
    uint8_t findNearestCentroid(
        const std::vector<float>& subvector,
        const std::vector<std::vector<float>>& centroids) const;
    
    static float l2Distance(const std::vector<float>& a, const std::vector<float>& b);
#endif
};

} // namespace themis
