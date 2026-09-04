/**
 * @file rotary_embeddings.h
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
#include <memory>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <cstdint>
#include <mutex>

namespace themis {

/// Configuration for Rotary Position Embeddings (RoPE)
/// Based on: Su, J., et al. (2021). "RoFormer: Enhanced Transformer with Rotary Position Embedding"
/// arXiv preprint arXiv:2104.09864
struct RotationConfig {
    size_t hidden_dim = 0;              // Embedding dimension (must be even)
    size_t num_rotation_pairs;      // Number of 2D rotation pairs (must be <= hidden_dim/2)
    double base_theta = 10000.0;    // Base frequency (from RoPE paper)
    bool normalize_after = false;   // L2 normalize after rotation
    
    // Precomputed theta values for efficiency
    // θ_i = base^(-2i/d) where i ∈ [0, d/2)
    std::vector<double> theta_cache;
    
    /// Compute theta cache for all rotation pairs
    void computeThetaCache();
    
    /// Validation
    bool isValid() const {
        return hidden_dim > 0 && 
               hidden_dim % 2 == 0 && 
               num_rotation_pairs > 0 &&
               num_rotation_pairs <= hidden_dim / 2;
    }
};

/// Rotary Position Embeddings Implementation
/// 
/// Implements rotation-based positional encoding for transformer models,
/// adapted for use in ThemisDB's vector storage layer. Enables:
/// - Positional awareness for sequential entities
/// - Relational embeddings for knowledge graph operations
/// - Temporal encoding for time-series data
/// - Multi-relational vector search
///
/// Sources:
/// - RoFormer Paper: Su, J., et al. (2021). arXiv:2104.09864
/// - Original Concept: Rotary Position Embedding (RoPE)
/// - ThemisDB Integration: ThemisDB Core Team
class RotaryEmbedding {
public:
    struct RotationStats {
        uint64_t total_rotated_entities = 0;
        uint64_t total_relational_rotations = 0;
        double avg_rotation_time_us = 0.0;
    };

    explicit RotaryEmbedding(const RotationConfig& config);
    
    // ===== Core rotation operations =====
    
    /// Rotate embedding by position
    /// Applies 2D rotations to coordinate pairs: R(x, θ) = [x₀ cos(θ) - x₁ sin(θ), x₀ sin(θ) + x₁ cos(θ)]
    /// For position m: f(x_m) = R(x_m, mθ₀) ⊕ R(x_m, mθ₁) ⊕ ... ⊕ R(x_m, mθ_{d/2-1})
    std::vector<float> rotate(
        const std::vector<float>& embedding,
        size_t position
    ) const;
    
    /// Apply inverse rotation (for decoding/reconstruction)
    std::vector<float> rotateInverse(
        const std::vector<float>& embedding,
        size_t position
    ) const;
    
    // ===== Batch operations for efficiency =====
    
    /// Rotate multiple embeddings with corresponding positions
    std::vector<std::vector<float>> rotateBatch(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions
    ) const;
    
    // ===== Relational rotation (for Knowledge Graph edges) =====
    
    /// Apply relational rotation based on relation type
    /// Uses hash of relation_type to determine rotation angle
    /// Enables TransE-like translational embeddings in vector space
    std::vector<float> rotateRelational(
        const std::vector<float>& embedding,
        const std::string& relation_type
    ) const;
    
    // ===== Configuration =====
    
    const RotationConfig& getConfig() const { return config_; }
    RotationStats getStats() const;
    
private:
    RotationConfig config_;
    
    // Cached relation type to rotation index mapping
    mutable std::unordered_map<std::string, size_t> relation_cache_;
    mutable std::mutex stats_mutex_;
    mutable uint64_t total_rotated_entities_ = 0;
    mutable uint64_t total_relational_rotations_ = 0;
    mutable double total_rotation_time_us_ = 0.0;
    
    // ===== Internal helpers =====
    
    /// Rotate a single coordinate pair in 2D
    void rotateCoordinatePair(
        float& x, float& y,
        double cos_theta, double sin_theta
    ) const;
    
    /// Compute cos and sin for rotation at given position and pair index
    std::pair<double, double> computeRotationAngles(
        size_t position, size_t pair_idx
    ) const;
    
    /// Hash relation type to rotation index (for relational embeddings)
    size_t hashRelationType(const std::string& relation_type) const;
    
    /// Normalize vector to unit length (L2 normalization)
    void normalizeL2(std::vector<float>& vec) const;

    std::vector<float> rotateImpl(
        const std::vector<float>& embedding,
        size_t position,
        bool is_relational
    ) const;
};

} // namespace themis
