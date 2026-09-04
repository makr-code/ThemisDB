/**
 * @file residual_quantizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/residual_quantizer.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace themis {

ResidualQuantizer::ResidualQuantizer(int dimension, const Config& config)
    : dimension_(dimension), config_(config) {
    
    if (dimension_ <= 0) {
        throw std::invalid_argument("Dimension must be positive");
    }
    
    if (dimension_ % config_.num_subquantizers != 0) {
        throw std::invalid_argument(
            "Dimension must be divisible by num_subquantizers");
    }
    
    if (config_.num_stages <= 0 || config_.num_stages > 10) {
        throw std::invalid_argument("Number of stages must be between 1 and 10");
    }
    
    // Pre-allocate stage quantizers
    stage_quantizers_.reserve(config_.num_stages);
}

ResidualQuantizer::Status ResidualQuantizer::train(
    const std::vector<std::vector<float>>& training_vectors) {
    
    if (training_vectors.empty()) {
        return Status::Error("No training vectors provided");
    }
    
    if (training_vectors[0].size() != static_cast<size_t>(dimension_)) {
        return Status::Error("Training vector dimension mismatch");
    }
    
    THEMIS_INFO("ResidualQuantizer::train - Training with {} vectors, dim={}, stages={}",
                training_vectors.size(), dimension_, config_.num_stages);
    
    // Initialize residuals with original data
    auto residuals = training_vectors;
    stage_quantizers_.reserve(static_cast<int>(stage_quantizers_.size()) + static_cast<size_t>(config_.num_stages));
    
    // Train each stage
    for (int stage = 0; stage < config_.num_stages; stage++) {
        THEMIS_INFO("ResidualQuantizer::train - Training stage {}/{}",
                    stage + 1, config_.num_stages);
        
        // Configure PQ for this stage
        ProductQuantizer::Config pq_config;
        pq_config.num_subquantizers = config_.num_subquantizers;
        pq_config.num_centroids = config_.num_centroids;
        pq_config.max_iterations = config_.max_kmeans_iterations;
        pq_config.convergence_threshold = config_.convergence_threshold;
        
        // Create and train PQ quantizer for this stage
        auto pq = std::make_unique<ProductQuantizer>(dimension_, pq_config);
        auto status = pq->train(residuals);
        
        if (!status.ok) {
            return Status::Error("Stage " + std::to_string(stage) + 
                                 " training failed: " + status.message);
        }
        
        // Store trained quantizer
        stage_quantizers_.push_back(std::move(pq));
        
        // Compute residuals for next stage
        if (stage < config_.num_stages - 1) {
            THEMIS_DEBUG("ResidualQuantizer::train - Computing residuals for stage {}",
                        stage + 2);
            residuals = computeResiduals(residuals, stage_quantizers_[stage].get());
        }
    }
    
    trained_ = true;
    THEMIS_INFO("ResidualQuantizer::train - Training complete. Stages: {}, Compression ratio: {:.1f}x",
                config_.num_stages, getCompressionRatio());
    
    return Status::OK();
}

std::vector<std::vector<float>> ResidualQuantizer::computeResiduals(
    const std::vector<std::vector<float>>& vectors,
    const ProductQuantizer* stage_quantizer) const {
    
    std::vector<std::vector<float>> residuals;
    residuals.reserve(vectors.size());
    
    for (const auto& vec : vectors) {
        // Encode and decode to get approximation
        auto codes = stage_quantizer->encode(vec);
        auto approx = stage_quantizer->decode(codes);
        
        // Compute residual: original - approximation
        std::vector<float> residual(dimension_);
        for (int d = 0; d < dimension_; d++) {
            residual[d] = vec[d] - approx[d];
        }
        
        residuals.push_back(std::move(residual));
    }
    
    return residuals;
}

std::vector<uint8_t> ResidualQuantizer::encode(const std::vector<float>& vector) const {
    if (!trained_) {
        THEMIS_ERROR("ResidualQuantizer::encode - Quantizer not trained");
        return {};
    }
    
    if (static_cast<int>(vector.size()) != static_cast<size_t>(dimension_)) {
        THEMIS_ERROR("ResidualQuantizer::encode - Dimension mismatch: {} vs {}",
                     vector.size(), dimension_);
        return {};
    }
    
    std::vector<uint8_t> all_codes;
    all_codes.reserve(config_.num_stages * config_.num_subquantizers);
    
    std::vector<float> residual = vector;
    
    // Encode through all stages
    for (int stage = 0; stage < config_.num_stages; stage++) {
        // Encode residual with this stage's quantizer
        auto stage_codes = stage_quantizers_[stage]->encode(residual);
        all_codes.insert(all_codes.end(), stage_codes.begin(), stage_codes.end());
        
        // Compute residual for next stage
        if (stage < config_.num_stages - 1) {
            auto approx = stage_quantizers_[stage]->decode(stage_codes);
            for (int d = 0; d < dimension_; d++) {
                residual[d] -= approx[d];
            }
        }
    }
    
    return all_codes;
}

std::vector<float> ResidualQuantizer::decode(const std::vector<uint8_t>& codes) const {
    if (!trained_) {
        THEMIS_ERROR("ResidualQuantizer::decode - Quantizer not trained");
        return {};
    }
    
    size_t expected_size = getEncodedSize();
    if (static_cast<int>(codes.size()) != expected_size) {
        THEMIS_ERROR("ResidualQuantizer::decode - Code size mismatch: {} vs {}",
                     codes.size(), expected_size);
        return {};
    }
    
    std::vector<float> result(dimension_, 0.0f);
    
    int code_offset = 0;
    int codes_per_stage = config_.num_subquantizers;
    
    // Decode and sum all stages
    for (int stage = 0; stage < config_.num_stages; stage++) {
        // Extract codes for this stage
        std::vector<uint8_t> stage_codes(
            codes.begin() + code_offset,
            codes.begin() + code_offset + codes_per_stage
        );
        
        // Decode and add to result
        auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);
        
        for (int d = 0; d < dimension_; d++) {
            result[d] += stage_approx[d];
        }
        
        code_offset += codes_per_stage;
    }
    
    return result;
}

float ResidualQuantizer::asymmetricDistance(const std::vector<float>& query,
                                           const std::vector<uint8_t>& codes) const {
    if (!trained_) {
        THEMIS_ERROR("ResidualQuantizer::asymmetricDistance - Quantizer not trained");
        return std::numeric_limits<float>::max();
    }
    
    if (static_cast<int>(query.size()) != static_cast<size_t>(dimension_)) {
        THEMIS_ERROR("ResidualQuantizer::asymmetricDistance - Query dimension mismatch");
        return std::numeric_limits<float>::max();
    }
    
    size_t expected_size = getEncodedSize();
    if (static_cast<int>(codes.size()) != expected_size) {
        THEMIS_ERROR("ResidualQuantizer::asymmetricDistance - Code size mismatch");
        return std::numeric_limits<float>::max();
    }
    
    // Compute L2 distance between query and decoded vector
    // Process: For each stage, compute distance between current residual and stage approximation,
    // then update residual for next stage. This accumulates squared errors across all stages.
    // Final result: ||query - (approx_stage0 + approx_stage1 + ... + approx_stageN)||²
    std::vector<float> query_residual = query;
    float total_distance_sq = 0.0f;
    
    int code_offset = 0;
    int codes_per_stage = config_.num_subquantizers;
    
    for (int stage = 0; stage < config_.num_stages; stage++) {
        // Extract codes for this stage
        std::vector<uint8_t> stage_codes(
            codes.begin() + code_offset,
            codes.begin() + code_offset + codes_per_stage
        );
        
        // Decode stage approximation
        auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);
        
        // Accumulate squared distance and update residual
        for (int d = 0; d < dimension_; d++) {
            float diff = query_residual[d] - stage_approx[d];
            total_distance_sq += diff * diff;
            
            // Update query residual for next stage
            if (stage < config_.num_stages - 1) {
                query_residual[d] -= stage_approx[d];
            }
        }
        
        code_offset += codes_per_stage;
        
        // Early termination if enabled and distance already too large
        if (config_.early_termination && stage < config_.num_stages - 1) {
            // This would require a distance threshold parameter
            // For now, we compute all stages
        }
    }
    
    return std::sqrt(total_distance_sq);
}

float ResidualQuantizer::getCompressionRatio() const {
    float original_bytes = static_cast<float>(dimension_ * sizeof(float));
    float compressed_bytes = static_cast<float>(getEncodedSize());
    return original_bytes / compressed_bytes;
}

size_t ResidualQuantizer::getMemoryUsage() const {
    size_t memory = sizeof(*this);
    
    for (const auto& quantizer : stage_quantizers_) {
        if (quantizer) {
            memory += quantizer->getMemoryUsage();
        }
    }
    
    return memory;
}

} // namespace themis
