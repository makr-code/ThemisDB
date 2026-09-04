/**
 * @file lora_rope.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/rotary_embeddings.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include <mutex>

namespace themis {

/// LoRA adapter for RoPE (Rotary Position Embeddings)
/// Provides low-rank adaptation matrices for modifying rotation behavior
struct LoRARopeAdapter {
    std::string name;                           // Adapter identifier
    std::string domain;                         // Target domain (e.g., "medical", "legal", "technical")
    size_t rank;                                // LoRA rank (r)
    float alpha;                                // LoRA scaling factor (α)
    
    // LoRA matrices: rotation modification = B @ A
    // B: (num_rotation_pairs, rank)
    // A: (rank, num_rotation_pairs)
    std::vector<std::vector<double>> matrix_B;  // Down-projection matrix
    std::vector<std::vector<double>> matrix_A;  // Up-projection matrix
    
    // Optional: learnable base theta modifications
    std::vector<double> theta_delta;            // Additive adjustments to base theta values
    
    // Metadata
    bool enabled = true;                        // Whether adapter is active
    float scaling = 1.0f;                       // Global scaling factor for this adapter
    
    /// Validate adapter dimensions
    bool isValid(size_t num_rotation_pairs) const {
        if (matrix_B.empty() || matrix_A.empty()) {
          return false;
        }
        if (matrix_B.size() != num_rotation_pairs) {
          return false;
        }
        if (matrix_A.size() != rank) {
          return false;
        }
        
        // Check B dimensions
        for (const auto& row : matrix_B) {
            if (row.size() != rank) {
              return false;
            }
        }
        
        // Check A dimensions
        for (const auto& row : matrix_A) {
            if (row.size() != num_rotation_pairs) {
              return false;
            }
        }
        
        // Check theta_delta if present
        if (!theta_delta.empty() && theta_delta.size() != num_rotation_pairs) {
            return false;
        }
        
        return true;
    }
    
    /// Initialize with random small values (for training)
    static LoRARopeAdapter createRandom(
        const std::string& name,
        const std::string& domain,
        size_t num_rotation_pairs,
        size_t rank,
        float alpha = 1.0f
    );
    
    /// Initialize with zeros (no modification initially)
    static LoRARopeAdapter createZero(
        const std::string& name,
        const std::string& domain,
        size_t num_rotation_pairs,
        size_t rank,
        float alpha = 1.0f
    );
};

/// Registry for managing multiple LoRA adapters for RoPE
class LoRARopeAdapterRegistry {
public:
    LoRARopeAdapterRegistry() = default;
    
    /// Register a new adapter
    bool registerAdapter(const LoRARopeAdapter& adapter);
    
    /// Unregister an adapter by name
    bool unregisterAdapter(const std::string& name);
    
    /// Get adapter by name
    std::optional<LoRARopeAdapter> getAdapter(const std::string& name) const;
    
    /// Check if adapter exists
    bool hasAdapter(const std::string& name) const;
    
    /// List all registered adapter names
    std::vector<std::string> listAdapters() const;
    
    /// Enable/disable an adapter
    bool setAdapterEnabled(const std::string& name, bool enabled);
    
    /// Clear all adapters
    void clear();
    
    /// Get number of registered adapters
    size_t size() const;
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, LoRARopeAdapter> adapters_;
};

/// LoRA-Enhanced Rotary Position Embeddings
/// 
/// Extends the base RotaryEmbedding with LoRA adapter support for dynamic,
/// context-aware rotation patterns. Enables different rotation behaviors
/// for specific domains or tasks without retraining the entire model.
///
/// Key features:
/// - Multiple LoRA adapters can be registered and switched dynamically
/// - Low-rank adaptation minimizes memory overhead
/// - Compatible with existing RoPE API (can be used as drop-in replacement)
/// - Supports adapter composition and blending
///
/// @see RotaryEmbedding for base functionality
/// @see LoRARopeAdapter for adapter structure
class LoRARotaryEmbedding : public RotaryEmbedding {
public:
    /// Initialize with configuration and optional adapter registry
    /// @param config Rotation configuration
    /// @param adapter_registry Optional shared adapter registry (creates new if not provided)
    explicit LoRARotaryEmbedding(
        const RotationConfig& config,
        std::shared_ptr<LoRARopeAdapterRegistry> adapter_registry = nullptr
    );
    
    // ===== Core rotation operations with LoRA =====
    
    /// Rotate embedding using a specific LoRA adapter
    /// Applies base rotation followed by additional LoRA-modified rotation
    /// The LoRA adapter modifies the rotation angles through low-rank transformation
    /// @param embedding Input embedding vector
    /// @param position Position index for rotation
    /// @param adapter_name Name of registered adapter to use
    /// @return Rotated embedding with LoRA modification
    std::vector<float> rotateWithAdapter(
        const std::vector<float>& embedding,
        size_t position,
        const std::string& adapter_name
    ) const;
    
    /// Rotate batch of embeddings with adapter
    /// @param embeddings Input embedding vectors
    /// @param positions Position indices for each embedding
    /// @param adapter_name Name of registered adapter to use
    /// @return Rotated embeddings with LoRA modification
    std::vector<std::vector<float>> rotateBatchWithAdapter(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions,
        const std::string& adapter_name
    ) const;
    
    // ===== Adapter Management =====
    
    /// Register a LoRA adapter for RoPE
    /// @param name Unique adapter name
    /// @param adapter LoRA adapter configuration
    /// @return True if registered successfully
    bool registerAdapter(
        const std::string& name,
        const LoRARopeAdapter& adapter
    );
    
    /// Unregister an adapter
    /// @param name Adapter name
    /// @return True if unregistered successfully
    bool unregisterAdapter(const std::string& name);
    
    /// Get list of registered adapters
    std::vector<std::string> listAdapters() const;
    
    /// Check if adapter is registered
    bool hasAdapter(const std::string& adapter_name) const;
    
    /// Enable/disable an adapter
    bool setAdapterEnabled(const std::string& name, bool enabled);
    
    /// Get the adapter registry
    std::shared_ptr<LoRARopeAdapterRegistry> getAdapterRegistry() const {
        return adapter_registry_;
    }
    
    // ===== Adapter Composition =====
    
    /// Rotate with multiple adapters (weighted combination)
    /// Applies multiple adapters with specified weights and combines results
    /// @param embedding Input embedding vector
    /// @param position Position index for rotation
    /// @param adapter_names Names of adapters to apply
    /// @param weights Weights for each adapter (must sum to 1.0 or will be normalized)
    /// @return Rotated embedding with combined LoRA modifications
    std::vector<float> rotateWithAdapterBlend(
        const std::vector<float>& embedding,
        size_t position,
        const std::vector<std::string>& adapter_names,
        const std::vector<float>& weights
    ) const;
    
private:
    std::shared_ptr<LoRARopeAdapterRegistry> adapter_registry_;
    
    // ===== Internal helpers =====
    
    /// Extract rotation features for LoRA input
    /// Creates a feature vector from position that can be transformed by LoRA matrices
    /// @param position Position index
    /// @return Feature vector (size = num_rotation_pairs)
    std::vector<double> extractRotationFeatures(size_t position) const;
};

} // namespace themis
