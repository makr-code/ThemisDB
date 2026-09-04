/**
 * @file rotary_embeddings.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/rotary_embeddings.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>
#include <chrono>

namespace themis {

// ============================================================================
// RotationConfig Implementation
// ============================================================================

void RotationConfig::computeThetaCache() {
    if (!isValid()) {
        throw std::invalid_argument("Invalid RotationConfig: hidden_dim must be positive and even");
    }
    
    theta_cache.clear();
    theta_cache.reserve(num_rotation_pairs);
    
    // Compute θ_i = base^(-2i/d) for i ∈ [0, num_rotation_pairs)
    for (size_t i = 0; i < num_rotation_pairs; ++i) {
        double exponent = -2.0 * static_cast<double>(i) / static_cast<double>(hidden_dim);
        double theta = std::pow(base_theta, exponent);
        theta_cache.push_back(theta);
    }
}

// ============================================================================
// RotaryEmbedding Implementation
// ============================================================================

RotaryEmbedding::RotaryEmbedding(const RotationConfig& config) 
    : config_(config) {
    if (!config_.isValid()) {
        throw std::invalid_argument(
            "Invalid RotationConfig: hidden_dim=" + std::to_string(config_.hidden_dim) +
            ", num_rotation_pairs=" + std::to_string(config_.num_rotation_pairs)
        );
    }
    
    if (config_.theta_cache.empty()) {
        throw std::invalid_argument(
            "RotationConfig theta_cache is empty. Call computeThetaCache() first."
        );
    }
}

std::vector<float> RotaryEmbedding::rotate(
    const std::vector<float>& embedding,
    size_t position
) const {
    return rotateImpl(embedding, position, false);
}

std::vector<float> RotaryEmbedding::rotateImpl(
    const std::vector<float>& embedding,
    size_t position,
    bool is_relational
) const {
    const auto started_at = std::chrono::steady_clock::now();
    if (static_cast<int>(embedding.size()) != config_.hidden_dim) {
        throw std::invalid_argument(
            "Embedding dimension mismatch: expected " + 
            std::to_string(config_.hidden_dim) + ", got " + 
            std::to_string(embedding.size())
        );
    }
    
    std::vector<float> rotated = embedding;
    
    // Apply rotation to each coordinate pair
    for (size_t pair_idx = 0; pair_idx < config_.num_rotation_pairs; ++pair_idx) {
        size_t idx_0 = pair_idx * 2;
        size_t idx_1 = pair_idx * 2 + 1;
        
        if (idx_1 >= static_cast<int>(rotated.size())) {
          break;
        }
        
        auto [cos_theta, sin_theta] = computeRotationAngles(position, pair_idx);
        rotateCoordinatePair(rotated[idx_0], rotated[idx_1], cos_theta, sin_theta);
    }
    
    // Optional L2 normalization
    if (config_.normalize_after) {
        normalizeL2(rotated);
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
        std::chrono::steady_clock::now() - started_at);
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++total_rotated_entities_;
        if (is_relational) {
            ++total_relational_rotations_;
        }
        total_rotation_time_us_ += elapsed.count();
    }
    
    return rotated;
}

std::vector<float> RotaryEmbedding::rotateInverse(
    const std::vector<float>& embedding,
    size_t position
) const {
    if (static_cast<int>(embedding.size()) != config_.hidden_dim) {
        throw std::invalid_argument(
            "Embedding dimension mismatch: expected " + 
            std::to_string(config_.hidden_dim) + ", got " + 
            std::to_string(embedding.size())
        );
    }
    
    std::vector<float> rotated = embedding;
    
    // Apply inverse rotation (negate angle) to each coordinate pair
    for (size_t pair_idx = 0; pair_idx < config_.num_rotation_pairs; ++pair_idx) {
        size_t idx_0 = pair_idx * 2;
        size_t idx_1 = pair_idx * 2 + 1;
        
        if (idx_1 >= static_cast<int>(rotated.size())) {
          break;
        }
        
        auto [cos_theta, sin_theta] = computeRotationAngles(position, pair_idx);
        // Inverse rotation: negate sine component
        rotateCoordinatePair(rotated[idx_0], rotated[idx_1], cos_theta, -sin_theta);
    }
    
    // Optional L2 normalization
    if (config_.normalize_after) {
        normalizeL2(rotated);
    }
    
    return rotated;
}

std::vector<std::vector<float>> RotaryEmbedding::rotateBatch(
    const std::vector<std::vector<float>>& embeddings,
    const std::vector<size_t>& positions
) const {
    if (static_cast<int>(embeddings.size()) != static_cast<int>(positions.size())) {
        throw std::invalid_argument(
            "Batch size mismatch: embeddings=" + std::to_string(embeddings.size()) +
            ", positions=" + std::to_string(positions.size())
        );
    }
    
    std::vector<std::vector<float>> rotated_batch;
    rotated_batch.reserve(embeddings.size());
    
    for (size_t i = 0; i < embeddings.size(); ++i) {
        rotated_batch.push_back(rotate(embeddings[i], positions[i]));
    }
    
    return rotated_batch;
}

std::vector<float> RotaryEmbedding::rotateRelational(
    const std::vector<float>& embedding,
    const std::string& relation_type
) const {
    if (static_cast<int>(embedding.size()) != config_.hidden_dim) {
        throw std::invalid_argument(
            "Embedding dimension mismatch: expected " + 
            std::to_string(config_.hidden_dim) + ", got " + 
            std::to_string(embedding.size())
        );
    }
    
    // Use relation type to determine rotation position
    // This creates a unique rotation for each relation type
    size_t rotation_position = hashRelationType(relation_type);
    
    return rotateImpl(embedding, rotation_position, true);
}

RotaryEmbedding::RotationStats RotaryEmbedding::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    RotationStats stats;
    stats.total_rotated_entities = total_rotated_entities_;
    stats.total_relational_rotations = total_relational_rotations_;
    if (total_rotated_entities_ > 0) {
        stats.avg_rotation_time_us = total_rotation_time_us_ / static_cast<double>(total_rotated_entities_);
    }
    return stats;
}

void RotaryEmbedding::rotateCoordinatePair(
    float& x, float& y,
    double cos_theta, double sin_theta
) const {
    // 2D rotation matrix:
    // [x']   [cos(θ)  -sin(θ)] [x]
    // [y'] = [sin(θ)   cos(θ)] [y]
    float x_new = static_cast<float>(x * cos_theta - y * sin_theta);
    float y_new = static_cast<float>(x * sin_theta + y * cos_theta);
    
    x = x_new;
    y = y_new;
}

std::pair<double, double> RotaryEmbedding::computeRotationAngles(
    size_t position, size_t pair_idx
) const {
    if (pair_idx >= config_.theta_cache.size()) {
        throw std::out_of_range(
            "Pair index out of range: " + std::to_string(pair_idx) +
            " >= " + std::to_string(config_.theta_cache.size())
        );
    }
    
    // Angle for this position and pair: angle = position * θ_i
    double theta = config_.theta_cache[pair_idx];
    double angle = static_cast<double>(position) * theta;
    
    return {std::cos(angle), std::sin(angle)};
}

size_t RotaryEmbedding::hashRelationType(const std::string& relation_type) const {
    // Check cache first
    auto it = relation_cache_.find(relation_type);
    if (it != relation_cache_.end()) {
        return it->second;
    }
    
    // Hash the relation type to a position index
    // Using std::hash for simplicity - could use more sophisticated hash in production
    std::hash<std::string> hasher;
    size_t hash_value = hasher(relation_type);
    
    // Map hash to reasonable position range (0 to RELATION_POSITION_RANGE)
    // This ensures consistent but distributed rotation angles
    constexpr size_t RELATION_POSITION_RANGE = 10000;
    size_t position = hash_value % RELATION_POSITION_RANGE;
    
    // Cache the result
    relation_cache_[relation_type] = position;
    
    return position;
}

void RotaryEmbedding::normalizeL2(std::vector<float>& vec) const {
    // Compute L2 norm
    double norm_squared = 0.0;
    for (float val : vec) {
        norm_squared += val * val;
    }
    
    if (norm_squared == 0.0) {
        return; // Avoid division by zero
    }
    
    double norm = std::sqrt(norm_squared);
    
    // Normalize
    for (float& val : vec) {
        val = static_cast<float>(val / norm);
    }
}

} // namespace themis
