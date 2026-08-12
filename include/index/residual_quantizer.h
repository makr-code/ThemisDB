/**
 * @file residual_quantizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/product_quantizer.h"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace themis {

/**
 * @brief Residual Quantization for High-Accuracy Vector Compression
 * 
 * v1.5.0 - Multi-stage quantization with FAISS-accelerated ProductQuantizer
 * 
 * Implements multi-stage residual quantization that iteratively quantizes the
 * residual (error) from the previous stage. This hierarchical approach achieves
 * better accuracy than single-stage quantization at the same compression ratio.
 * 
 * Each stage uses ProductQuantizer, which can leverage FAISS K-means clustering
 * when THEMIS_HAS_FAISS is defined and prefer_faiss is true, providing 20-30%
 * faster training per stage.
 * 
 * Sources:
 * - Algorithm: Residual Vector Quantization (RVQ)
 * - Paper: Chen, Y., Guan, T., & Wang, C. (2010).
 *          "Approximate Nearest Neighbor Search by Residual Vector Quantization"
 *          Sensors 10(12)
 * - DOI: 10.3390/s101211259
 * - Paper: Subramanya, S. J., et al. (2019).
 *          "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node"
 *          Neural Information Processing Systems (NeurIPS)
 * - Paper: Gao, J., & Long, C. (2024).
 *          "RaBitQ: Quantizing High-Dimensional Vectors with a Theoretical Error Bound"
 *          ACM SIGMOD (extensions for multi-stage)
 * - Implementation: Optimized for ThemisDB using ProductQuantization per stage
 * 
 * Features:
 * - Multi-stage iterative quantization (default 2 stages)
 * - Each stage uses ProductQuantization (with optional FAISS acceleration)
 * - Progressive refinement for higher accuracy
 * - 97-99% recall@10 vs 95-98% for single-stage PQ
 * - Used in production systems (DiskANN, FAISS)
 * 
 * Part of ThemisDB v1.5.0 - Complete FAISS Migration (#1079)
 */
class ResidualQuantizer {
public:
    struct Config {
        int num_stages;               // Number of residual stages (2-4 typical)
        int num_subquantizers;        // PQ subquantizers per stage
        int num_centroids;            // Centroids per subquantizer (8-bit = 256)
        int max_kmeans_iterations;    // K-means iterations per stage
        float convergence_threshold;  // K-means convergence threshold
        bool early_termination;       // Enable early termination for distance
        
        Config() 
            : num_stages(2)
            , num_subquantizers(8)
            , num_centroids(256)
            , max_kmeans_iterations(25)
            , convergence_threshold(0.001f)
            , early_termination(false)
        {}
    };

    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /**
     * @brief Construct a new Residual Quantizer
     * @param dimension Vector dimension (must be divisible by num_subquantizers)
     * @param config Configuration parameters
     */
    explicit ResidualQuantizer(int dimension, const Config& config);

    /**
     * @brief Train multi-stage quantizer
     * Trains each stage sequentially on the residuals from the previous stage
     * @param training_vectors Training data (num_vectors x dimension)
     * @return Status indicating success or failure
     */
    Status train(const std::vector<std::vector<float>>& training_vectors);

    /**
     * @brief Encode a vector through all stages
     * Returns concatenated codes from all stages
     * @param vector Input vector (dimension floats)
     * @return Quantized codes (num_stages * num_subquantizers bytes)
     */
    std::vector<uint8_t> encode(const std::vector<float>& vector) const;

    /**
     * @brief Decode quantized codes by summing all stage reconstructions
     * @param codes Quantized codes (num_stages * num_subquantizers bytes)
     * @return Reconstructed vector (dimension floats)
     */
    std::vector<float> decode(const std::vector<uint8_t>& codes) const;

    /**
     * @brief Compute asymmetric distance between query and quantized codes
     * Computes distance contribution from each stage and sums them
     * @param query Query vector (dimension floats)
     * @param codes Quantized codes (num_stages * num_subquantizers bytes)
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
     * @return Ratio of original size to compressed size
     */
    float getCompressionRatio() const;

    /**
     * @brief Get encoded size in bytes
     */
    size_t getEncodedSize() const {
        return config_.num_stages * config_.num_subquantizers;
    }

    /**
     * @brief Get memory usage for quantizer codebooks
     */
    size_t getMemoryUsage() const;

    // Getters
    int getDimension() const { return dimension_; }
    int getNumStages() const { return config_.num_stages; }
    int getCodesPerStage() const { return config_.num_subquantizers; }

    /**
     * @brief Get a specific stage quantizer (for inspection/testing)
     */
    const ProductQuantizer* getStageQuantizer(int stage) const {
        if (stage >= 0 && stage < static_cast<int>(stage_quantizers_.size())) {
            return stage_quantizers_[stage].get();
        }
        return nullptr;
    }

private:
    int dimension_;
    Config config_;
    bool trained_ = false;

    // One Product Quantizer per stage
    std::vector<std::unique_ptr<ProductQuantizer>> stage_quantizers_;

    /**
     * @brief Compute residuals after quantizing with a specific stage
     * @param vectors Input vectors
     * @param stage_quantizer Quantizer for this stage
     * @return Residual vectors
     */
    std::vector<std::vector<float>> computeResiduals(
        const std::vector<std::vector<float>>& vectors,
        const ProductQuantizer* stage_quantizer) const;
};

} // namespace themis

