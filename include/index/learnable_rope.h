/**
 * @file learnable_rope.h
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
#include <optional>

namespace themis {

/// Training configuration for learnable RoPE parameters
/// Defines hyperparameters for the training process
struct TrainingConfig {
    float learning_rate = 1e-4f;     // Learning rate for gradient descent
    size_t batch_size = 256;         // Mini-batch size
    size_t max_epochs = 100;         // Maximum training epochs
    float temperature = 0.07f;       // Temperature for contrastive loss
    float weight_decay = 1e-5f;      // L2 regularization strength
    float validation_split = 0.1f;   // Fraction of data for validation
    bool use_adam = false;           // Use Adam optimizer (if true), else SGD
    float early_stop_patience = 10;  // Stop if no improvement for N epochs
    
    // Adam optimizer parameters (only used if use_adam = true)
    float adam_beta1 = 0.9f;
    float adam_beta2 = 0.999f;
    float adam_epsilon = 1e-8f;
};

/// Training sample for contrastive learning
/// Represents a single data point for training
struct TrainingSample {
    std::vector<float> embedding;     // Input embedding vector
    size_t position;                  // Position index
    float similarity_target;          // Target similarity for contrastive loss
    
    TrainingSample() = default;
    TrainingSample(const std::vector<float>& emb, size_t pos, float sim)
        : embedding(emb), position(pos), similarity_target(sim) {}
};

/// Learnable Rotary Position Embeddings
/// 
/// Extends the base RotaryEmbedding class with trainable theta parameters.
/// Enables domain-specific adaptation of rotation frequencies through
/// contrastive learning or other training objectives.
///
/// Key features:
/// - Learnable theta values (initialized from base RoPE)
/// - Gradient computation and backpropagation
/// - Parameter updates with SGD or Adam optimizer
/// - Serialization/deserialization of trained parameters
/// - Training/inference mode switching
///
/// @see RotaryEmbedding for base functionality
/// @see TrainingConfig for hyperparameters
class LearnableRotaryEmbedding : public RotaryEmbedding {
public:
    /// Initialize with configuration and optional trainable mode
    /// @param config Rotation configuration (must have theta_cache computed)
    /// @param trainable If true, theta values are trainable; if false, frozen
    explicit LearnableRotaryEmbedding(const RotationConfig& config, bool trainable = true);
    
    // ===== Training Mode Control =====
    
    /// Set training mode (affects gradient computation)
    /// @param training If true, gradients are computed during rotation
    void setTrainingMode(bool training);
    
    /// Check if in training mode
    bool isTraining() const { return training_mode_; }
    
    /// Check if parameters are trainable
    bool isTrainable() const { return trainable_; }
    
    // ===== Training Interface =====
    
    /// Train the learnable theta parameters on a dataset
    /// Uses contrastive learning objective to optimize positional encoding
    /// @param samples Training samples with embeddings, positions, and targets
    /// @param config Training configuration (learning rate, batch size, etc.)
    /// @return Training loss history (one value per epoch)
    std::vector<float> train(
        const std::vector<TrainingSample>& samples,
        const TrainingConfig& config
    );
    
    /// Compute gradients for a single sample (used internally during training)
    /// @param embedding Input embedding
    /// @param target_similarity Target similarity for contrastive loss
    /// @param position Position index
    /// @return Gradients with respect to theta parameters
    std::vector<double> computeGradients(
        const std::vector<float>& embedding,
        float target_similarity,
        size_t position
    );
    
    /// Update theta parameters using computed gradients
    /// @param gradients Gradients for each theta value
    /// @param learning_rate Learning rate for the update
    void updateParameters(
        const std::vector<double>& gradients,
        float learning_rate
    );
    
    // ===== Parameter Access =====
    
    /// Get current learnable theta values
    const std::vector<double>& getLearnableTheta() const { return learnable_theta_; }
    
    /// Set learnable theta values (e.g., from loaded checkpoint)
    void setLearnableTheta(const std::vector<double>& theta);
    
    /// Reset learnable theta to base configuration values
    void resetToBase();
    
    // ===== Serialization =====
    
    /// Save trained parameters to file (JSON format)
    /// Stores theta values, configuration, and training metadata
    /// @param path File path for saving
    /// @return True if successful, false otherwise
    bool saveParameters(const std::string& path) const;
    
    /// Load trained parameters from file
    /// @param path File path for loading
    /// @return True if successful, false otherwise
    bool loadParameters(const std::string& path);
    
    // ===== Validation =====
    
    /// Compute validation loss on a held-out dataset
    /// @param samples Validation samples
    /// @return Average validation loss
    float computeValidationLoss(const std::vector<TrainingSample>& samples) const;
    
    // Rotate using learnable parameters (shadows base class method)
    // This provides the same interface but uses learnable theta values
    std::vector<float> rotate(
        const std::vector<float>& embedding,
        size_t position
    ) const;
    
private:
    // ===== Internal Rotation Helpers =====
    
    /// Compute rotation angles using learnable theta
    /// (Similar to base class but uses learnable parameters)
    std::pair<double, double> computeLearnableRotationAngles(
        size_t position, size_t pair_idx
    ) const;
    
    bool trainable_;                      // Are parameters trainable?
    bool training_mode_;                  // Is training mode active?
    std::vector<double> learnable_theta_; // Trainable theta values
    std::vector<double> theta_gradients_; // Accumulated gradients
    
    // Adam optimizer state (only used if use_adam = true)
    std::vector<double> adam_m_;          // First moment estimates
    std::vector<double> adam_v_;          // Second moment estimates
    size_t adam_t_;                       // Time step for Adam
    
    // ===== Internal Training Helpers =====
    
    /// Compute contrastive loss for a batch
    float computeContrastiveLoss(
        const std::vector<TrainingSample>& batch,
        float temperature
    ) const;
    
    /// Update parameters using SGD
    void updateSGD(const std::vector<double>& gradients, float learning_rate);
    
    /// Update parameters using Adam optimizer
    void updateAdam(
        const std::vector<double>& gradients,
        float learning_rate,
        const TrainingConfig& config
    );
    
    /// Initialize optimizer state (Adam)
    void initializeOptimizer(const TrainingConfig& config);
    
    /// Split samples into training and validation sets
    std::pair<std::vector<TrainingSample>, std::vector<TrainingSample>>
    splitTrainValidation(
        const std::vector<TrainingSample>& samples,
        float validation_split
    ) const;
};

} // namespace themis
