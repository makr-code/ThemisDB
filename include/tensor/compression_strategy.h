/**
 * @file compression_strategy.h
 * @brief Compression strategy abstractions for the tensor mid-layer.
 * 
 * Defines the interface and concrete strategies for candidate compression,
 * including Tensor-Train decomposition, quantization, sampling, and hashing.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "tensor/tensor_compat.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// CompressionConfig — configuration for compression strategies
// ============================================================================

/**
 * @brief Configuration for tensor compression operations.
 */
enum class CompressionAlgorithm : uint8_t {
    NONE = 0,
    TT_SVD = 1,
    AUTO = 2,
};

struct CompressionConfig {
    /// Tensor-Train reconstruction error tolerance (epsilon).
    float tt_epsilon = 0.01f;

    /// Maximum TT-rank (-1 = no limit).
    int32_t max_tt_rank = -1;

    /// Quantization bit-depth (0 = disabled, 8/16/32 = bits).
    uint8_t quantization_bits = 0;

    /// Sampling ratio for candidate reduction (0.0-1.0, 0 = no sampling).
    float sampling_ratio = 0.0f;

    /// Hash function output bits for fingerprinting (0 = disabled).
    uint8_t fingerprint_bits = 0;

    /// Whether to preserve exact values for critical dimensions.
    bool preserve_critical_dims = true;

    // Backwards-compatible fields (legacy tests expect these names)
    CompressionAlgorithm algorithm = CompressionAlgorithm::NONE;
    int32_t target_rank = -1;
};

// ============================================================================
// CompressionResult — output of compression operation
// ============================================================================

/**
 * @brief Result of a compression operation.
 */
struct CompressionResult {
    /// Whether compression succeeded.
    bool success = false;

    /// Human-readable error message if compression failed.
    std::string error_message;

    /// Original size in bytes (before compression).
    std::size_t original_size = 0;

    /// Compressed size in bytes (after compression).
    std::size_t compressed_size = 0;

    /// Compression ratio: original_size / compressed_size (> 1 = compressed).
    float compression_ratio = 1.0f;

    /// Estimated reconstruction error (L2 norm).
    float achieved_error = 0.0f;

    /// Actual rank achieved (for TT compression).
    std::size_t achieved_rank = 0;

    /// Optional compressed representation (format depends on strategy).
    std::string compressed_data;

    /// Metadata about the compression (strategy, parameters, etc.).
    std::string compression_metadata;
};

// ============================================================================
// ICompressionStrategy — interface for compression strategies
// ============================================================================

/**
 * @brief Abstract interface for candidate compression strategies.
 * 
 * Implementations provide specific compression algorithms (TT, quantization,
 * sampling, etc.) with standardized configuration and result reporting.
 */
class ICompressionStrategy {
public:
    virtual ~ICompressionStrategy() = default;

    /**
     * @brief Get the name of this compression strategy.
     * @return Human-readable strategy name (e.g., "TT_DECOMPOSITION", "QUANTIZE_INT8").
     */
    [[nodiscard]] virtual std::string name() const noexcept = 0;

    /**
     * @brief Compress a candidate represented as a flat vector.
     * 
     * @param data        Flat vector data (dim elements of float).
     * @param dim         Vector dimension.
     * @param mode_sizes  Tensor mode sizes (for reshape operations).
     * @param config      Compression configuration parameters.
     * @return Compression result with success flag and metrics.
     */
    [[nodiscard]] virtual CompressionResult compress(
        const float*              data,
        std::size_t               dim,
        const std::vector<size_t>& mode_sizes,
        const CompressionConfig&  config) const = 0;

    /**
     * @brief Compress a Tensor-Train representation.
     * 
     * @param train  TT-format candidate.
     * @param config Compression configuration parameters.
     * @return Compression result.
     */
    [[nodiscard]] virtual CompressionResult compressTTTrain(
        const storage::TTTrain&   train,
        const CompressionConfig&  config) const = 0;

    /**
     * @brief Estimate compression ratio without performing actual compression.
     * 
     * @param data       Sample data (may be partial).
     * @param dim        Vector dimension.
     * @param config     Compression configuration parameters.
     * @return Estimated compression ratio.
     */
    [[nodiscard]] virtual float estimateRatio(
        const float*              data,
        std::size_t               dim,
        const CompressionConfig&  config) const = 0;
};

// ============================================================================
// TTDecompositionStrategy — Tensor-Train decomposition compression
// ============================================================================

/**
 * @brief Tensor-Train decomposition compression strategy.
 * 
 * Converts flat vectors or structured tensors into low-rank TT representations,
 * achieving compression through rank reduction while preserving key information.
 */
class TTDecompositionStrategy : public ICompressionStrategy {
public:
    TTDecompositionStrategy() = default;

    std::string name() const noexcept override;

    CompressionResult compress(
        const float*              data,
        std::size_t               dim,
        const std::vector<size_t>& mode_sizes,
        const CompressionConfig&  config) const override;

    CompressionResult compressTTTrain(
        const storage::TTTrain&   train,
        const CompressionConfig&  config) const override;

    float estimateRatio(
        const float*              data,
        std::size_t               dim,
        const CompressionConfig&  config) const override;
};

// ============================================================================
// QuantizationStrategy — Bit-depth reduction compression
// ============================================================================

/**
 * @brief Quantization-based compression strategy.
 * 
 * Reduces bit-depth of floating-point values to 8, 16, or 32 bits,
 * with optional per-channel or per-layer scaling for better accuracy.
 */
class QuantizationStrategy : public ICompressionStrategy {
public:
    QuantizationStrategy(uint8_t bits = 8);

    std::string name() const noexcept override;

    CompressionResult compress(
        const float*              data,
        std::size_t               dim,
        const std::vector<size_t>& mode_sizes,
        const CompressionConfig&  config) const override;

    CompressionResult compressTTTrain(
        const storage::TTTrain&   train,
        const CompressionConfig&  config) const override;

    float estimateRatio(
        const float*              data,
        std::size_t               dim,
        const CompressionConfig&  config) const override;

private:
    uint8_t bits_;
};

// ============================================================================
// SamplingStrategy — Candidate sampling compression
// ============================================================================

/**
 * @brief Sampling-based compression strategy.
 * 
 * Reduces candidate count by probabilistic or deterministic sampling,
 * useful for handling large result sets from ANN retrieval.
 */
class SamplingStrategy : public ICompressionStrategy {
public:
    SamplingStrategy(float ratio = 0.5f);

    std::string name() const noexcept override;

    CompressionResult compress(
        const float*              data,
        std::size_t               dim,
        const std::vector<size_t>& mode_sizes,
        const CompressionConfig&  config) const override;

    CompressionResult compressTTTrain(
        const storage::TTTrain&   train,
        const CompressionConfig&  config) const override;

    float estimateRatio(
        const float*              data,
        std::size_t               dim,
        const CompressionConfig&  config) const override;

private:
    float ratio_;
};

// ============================================================================
// HashingStrategy — Fingerprint-based compression
// ============================================================================

/**
 * @brief Hash-based compression strategy.
 * 
 * Converts candidates into fixed-size fingerprints using LSH or cryptographic
 * hashing, enabling efficient similarity matching and deduplication.
 */
class HashingStrategy : public ICompressionStrategy {
public:
    HashingStrategy(uint8_t bits = 64);

    std::string name() const noexcept override;

    CompressionResult compress(
        const float*              data,
        std::size_t               dim,
        const std::vector<size_t>& mode_sizes,
        const CompressionConfig&  config) const override;

    CompressionResult compressTTTrain(
        const storage::TTTrain&   train,
        const CompressionConfig&  config) const override;

    float estimateRatio(
        const float*              data,
        std::size_t               dim,
        const CompressionConfig&  config) const override;

private:
    uint8_t bits_;
};

// ============================================================================
// CompressionFactory — factory for creating strategies
// ============================================================================

/**
 * @brief Factory for creating compression strategy instances.
 */
class CompressionFactory {
public:
    /**
     * @brief Create a compression strategy by name.
     * 
     * @param strategy_name Strategy name (e.g., "TT_DECOMPOSITION", "QUANTIZE_INT8").
     * @return Pointer to strategy, or nullptr if name not recognized.
     */
    [[nodiscard]] static std::unique_ptr<ICompressionStrategy> create(
        const std::string& strategy_name);

    /**
     * @brief Register a custom compression strategy.
     * 
     * @param name     Strategy name for lookup.
     * @param strategy Strategy implementation (ownership transferred).
     */
    static void registerStrategy(
        const std::string& name,
        std::unique_ptr<ICompressionStrategy> strategy);
};

} // namespace tensor
} // namespace themis
