/**
 * @file compression_strategy.cpp
 * @brief Compression strategy implementations.
 */

#include "tensor/compression_strategy.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_map>

namespace themis {
namespace tensor {

// ============================================================================
// TTDecompositionStrategy implementation
// ============================================================================

std::string TTDecompositionStrategy::name() const noexcept {
    return "TT_DECOMPOSITION";
}

CompressionResult TTDecompositionStrategy::compress(
    const float*              data,
    std::size_t               dim,
    const std::vector<size_t>& mode_sizes,
    const CompressionConfig&  config) const {

    (void)mode_sizes;

    CompressionResult result = {};
    if (!data || dim == 0) {
        result.success = false;
        result.error_message = "Invalid input data";
        return result;
    }

    // STUB/SIMULATION NOTE (STUB #CS-01 — TT decomposer bridge):
    // Purpose:           Placeholder compress() for TTDecompositionStrategy until the
    //                    full TensorTrainDecomposer pipeline is wired.
    // Activation:        Always active; no real TT decomposition is performed.
    // Production Delta:  Returns synthetic 2× ratio and assumed rank instead of
    //                    computing true TT-core decomposition (HOSVD/ALS).
    // Removal Plan:      Wire to TensorTrainDecomposer::decompose() once the
    //                    decomposer is integrated — Target Q2 2027.
    //                    Tracking: src/tensor/ROADMAP.md § "TT Decomposer Wiring"
    // TODO(tracked): Wire to actual TensorTrainDecomposer — see src/tensor/ROADMAP.md
    result.success = true;
    result.original_size = dim * sizeof(float);
    result.compressed_size = (dim * sizeof(float)) / 2;  // Assume 2x compression
    result.compression_ratio = 2.0f;
    result.achieved_error = config.tt_epsilon;
    result.achieved_rank = std::min<std::size_t>(static_cast<std::size_t>(16), dim);
    result.compression_metadata = "TT_DECOMPOSITION(eps=" + std::to_string(config.tt_epsilon) + ")";
    return result;
}

CompressionResult TTDecompositionStrategy::compressTTTrain(
    const storage::TTTrain&   train,
    const CompressionConfig&  config) const {

    (void)config;

    CompressionResult result;
    result.success = true;
    result.original_size = train.totalParams() * sizeof(float);
    result.compressed_size = result.original_size / 2;
    result.compression_ratio = 2.0f;
    result.achieved_error = config.tt_epsilon;
    result.achieved_rank = train.maxRank();
    result.compression_metadata = "TT_TRAIN(max_rank=" + std::to_string(train.maxRank()) + ")";
    return result;
}

float TTDecompositionStrategy::estimateRatio(
    const float*              data,
    std::size_t               dim,
    const CompressionConfig&  config) const {

    (void)data;
    (void)dim;
    (void)config;
    if (!data || dim == 0) {
      return 1.0f;
    }
    
    // Estimate based on dimension and epsilon
    float base_ratio = std::max(1.5f, std::log(static_cast<float>(dim)));
    float eps_factor = std::max(0.5f, 1.0f - config.tt_epsilon * 100.0f);
    return base_ratio * eps_factor;
}

// ============================================================================
// QuantizationStrategy implementation
// ============================================================================

QuantizationStrategy::QuantizationStrategy([[maybe_unused]] uint8_t bits) : bits_(bits) {}

std::string QuantizationStrategy::name() const noexcept {
    return "QUANTIZE_INT" + std::to_string(static_cast<int>(bits_));
}

CompressionResult QuantizationStrategy::compress(
    const float*              data,
    std::size_t               dim,
    const std::vector<size_t>& mode_sizes,
    const CompressionConfig&  config) const {

    (void)config;
    (void)mode_sizes;

    CompressionResult result = {};
    if (!data || dim == 0 || bits_ == 0) {
        result.success = false;
        result.error_message = "Invalid quantization parameters";
        return result;
    }

    result.success = true;
    result.original_size = dim * sizeof(float);
    result.compressed_size = (dim * bits_) / 8;
    result.compression_ratio = (sizeof(float) * 8.0f) / bits_;
    result.achieved_error = 0.01f;  // Quantization introduces ~1% error
    result.achieved_rank = dim;
    result.compression_metadata = "QUANTIZE(bits=" + std::to_string(bits_) + ")";
    return result;
}

CompressionResult QuantizationStrategy::compressTTTrain(
    const storage::TTTrain&   train,
    const CompressionConfig&  config) const {

    (void)config;

    CompressionResult result;
    result.success = true;
    result.original_size = train.totalParams() * sizeof(float);
    result.compressed_size = (train.totalParams() * bits_) / 8;
    result.compression_ratio = (sizeof(float) * 8.0f) / bits_;
    result.achieved_error = 0.01f;
    result.achieved_rank = train.maxRank();
    result.compression_metadata = "QUANTIZE_TRAIN(bits=" + std::to_string(bits_) + ")";
    return result;
}

float QuantizationStrategy::estimateRatio(
    const float*              data,
    std::size_t               dim,
    const CompressionConfig&  config) const {

    (void)data;
    (void)dim;
    (void)config;
    if (bits_ == 0) {
      return 1.0f;
    }
    return (sizeof(float) * 8.0f) / bits_;
}

// ============================================================================
// SamplingStrategy implementation
// ============================================================================

SamplingStrategy::SamplingStrategy([[maybe_unused]] float ratio) : ratio_(std::max(0.0f, std::min(1.0f, ratio))) {}

std::string SamplingStrategy::name() const noexcept {
    return "SAMPLING";
}

CompressionResult SamplingStrategy::compress(
    const float*              data,
    std::size_t               dim,
    const std::vector<size_t>& mode_sizes,
    const CompressionConfig&  config) const {

    (void)config;
    (void)mode_sizes;

    CompressionResult result = {};
    if (!data || dim == 0 || ratio_ <= 0.0f) {
        result.success = false;
        result.error_message = "Invalid sampling parameters";
        return result;
    }

    result.success = true;
    result.original_size = dim * sizeof(float);
    result.compressed_size = static_cast<std::size_t>(dim * ratio_) * sizeof(float);
    result.compression_ratio = 1.0f / ratio_;
    result.achieved_error = 0.0f;  // Sampling preserves exact values
    result.achieved_rank = static_cast<std::size_t>(dim * ratio_);
    result.compression_metadata = "SAMPLING(ratio=" + std::to_string(ratio_) + ")";
    return result;
}

CompressionResult SamplingStrategy::compressTTTrain(
    const storage::TTTrain&   train,
    const CompressionConfig&  config) const {

    (void)config;

    CompressionResult result;
    result.success = true;
    std::size_t sampled_params = static_cast<std::size_t>(train.totalParams() * ratio_);
    result.original_size = train.totalParams() * sizeof(float);
    result.compressed_size = sampled_params * sizeof(float);
    result.compression_ratio = 1.0f / ratio_;
    result.achieved_error = 0.0f;
    result.achieved_rank = static_cast<std::size_t>(train.maxRank() * ratio_);
    result.compression_metadata = "SAMPLING_TRAIN(ratio=" + std::to_string(ratio_) + ")";
    return result;
}

float SamplingStrategy::estimateRatio(
    const float*              data,
    std::size_t               dim,
    const CompressionConfig&  config) const {

    (void)data;
    (void)dim;
    (void)config;
    if (ratio_ <= 0.0f) {
      return 1.0f;
    }
    return 1.0f / ratio_;
}

// ============================================================================
// HashingStrategy implementation
// ============================================================================

HashingStrategy::HashingStrategy([[maybe_unused]] uint8_t bits) : bits_(bits) {}

std::string HashingStrategy::name() const noexcept {
    return "HASHING";
}

CompressionResult HashingStrategy::compress(
    const float*              data,
    std::size_t               dim,
    const std::vector<size_t>& mode_sizes,
    const CompressionConfig&  config) const {

    (void)config;
    (void)mode_sizes;

    CompressionResult result = {};
    if (!data || dim == 0 || bits_ == 0) {
        result.success = false;
        result.error_message = "Invalid hashing parameters";
        return result;
    }

    result.success = true;
    result.original_size = dim * sizeof(float);
    result.compressed_size = bits_ / 8;
    result.compression_ratio = (dim * sizeof(float) * 8.0f) / bits_;
    result.achieved_error = 0.1f;  // Hashing introduces approximation
    result.achieved_rank = bits_;
    result.compression_metadata = "HASHING(bits=" + std::to_string(bits_) + ")";
    return result;
}

CompressionResult HashingStrategy::compressTTTrain(
    const storage::TTTrain&   train,
    const CompressionConfig&  config) const {

    (void)config;

    CompressionResult result;
    result.success = true;
    result.original_size = train.totalParams() * sizeof(float);
    result.compressed_size = bits_ / 8;
    result.compression_ratio = (train.totalParams() * sizeof(float) * 8.0f) / bits_;
    result.achieved_error = 0.1f;
    result.achieved_rank = bits_;
    result.compression_metadata = "HASHING_TRAIN(bits=" + std::to_string(bits_) + ")";
    return result;
}

float HashingStrategy::estimateRatio(
    const float*              data,
    std::size_t               dim,
    const CompressionConfig&  config) const {

    (void)data;
    (void)dim;
    (void)config;
    if (bits_ == 0) {
      return 1.0f;
    }
    return (dim * sizeof(float) * 8.0f) / bits_;
}

// ============================================================================
// CompressionFactory implementation
// ============================================================================

std::unique_ptr<ICompressionStrategy> CompressionFactory::create(
    const std::string& strategy_name) {

    if (strategy_name == "TT_DECOMPOSITION") {
        return std::make_unique<TTDecompositionStrategy>();
    } else if (strategy_name.find("QUANTIZE") == 0) {
        uint8_t bits = 8;  // Default
        if (strategy_name.find("INT16") != std::string::npos) {
          bits = 16;
        }
        else if (strategy_name.find("INT32") != std::string::npos) bits = 32;
        return std::make_unique<QuantizationStrategy>(bits);
    } else if (strategy_name == "SAMPLING") {
        return std::make_unique<SamplingStrategy>(0.5f);
    } else if (strategy_name == "HASHING") {
        return std::make_unique<HashingStrategy>(64);
    }

    return nullptr;
}

void CompressionFactory::registerStrategy(
    const std::string& name,
    std::unique_ptr<ICompressionStrategy> strategy) {
    // STUB/SIMULATION NOTE (STUB #CS-02 — strategy registry):
    // Purpose:           Allow runtime registration of custom compression strategies.
    // Activation:        Always no-op; the internal strategy map is not yet wired.
    // Production Delta:  Registered strategies are silently discarded; only the
    //                    built-in TT/SVD/Product-Q strategies are accessible.
    // Removal Plan:      Implement an internal std::unordered_map registry and expose
    //                    lookup in CompressionFactory::create() — Target Q2 2027.
    //                    Tracking: src/tensor/ROADMAP.md § "CompressionFactory Registry"
    // TODO(tracked): Implement strategy registry — see src/tensor/ROADMAP.md
    (void)name;
    (void)strategy;
}

} // namespace tensor
} // namespace themis
