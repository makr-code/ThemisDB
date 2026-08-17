/**
 * @file learnable_rope.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/learnable_rope.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <random>
#include <fstream>
#include <sstream>

namespace themis {

// ============================================================================
// LearnableRotaryEmbedding Implementation
// ============================================================================

LearnableRotaryEmbedding::LearnableRotaryEmbedding(
    const RotationConfig& config, 
    bool trainable
)
    : RotaryEmbedding(config)
    , trainable_(trainable)
    , training_mode_(false)
    , adam_t_(0)
{
    // Initialize learnable theta from base configuration
    learnable_theta_ = config.theta_cache;
    theta_gradients_.resize(learnable_theta_.size(), 0.0);
}

void LearnableRotaryEmbedding::setTrainingMode(bool training) {
    training_mode_ = training;
}

void LearnableRotaryEmbedding::setLearnableTheta(const std::vector<double>& theta) {
    if (theta.size() != learnable_theta_.size()) {
        throw std::invalid_argument(
            "Theta size mismatch: expected " + std::to_string(learnable_theta_.size()) +
            ", got " + std::to_string(theta.size())
        );
    }
    learnable_theta_ = theta;
}

void LearnableRotaryEmbedding::resetToBase() {
    learnable_theta_ = getConfig().theta_cache;
}

// ============================================================================
// Rotation with Learnable Parameters
// ============================================================================

std::vector<float> LearnableRotaryEmbedding::rotate(
    const std::vector<float>& embedding,
    size_t position
) const {
    if (embedding.size() != getConfig().hidden_dim) {
        throw std::invalid_argument(
            "Embedding dimension mismatch: expected " + 
            std::to_string(getConfig().hidden_dim) + ", got " + 
            std::to_string(embedding.size())
        );
    }
    
    std::vector<float> rotated = embedding;
    
    // Apply rotation to each coordinate pair using learnable theta
    for (size_t pair_idx = 0; pair_idx < getConfig().num_rotation_pairs; ++pair_idx) {
        size_t idx_0 = pair_idx * 2;
        size_t idx_1 = pair_idx * 2 + 1;
        
        if (idx_1 >= rotated.size()) break;
        
        auto [cos_theta, sin_theta] = computeLearnableRotationAngles(position, pair_idx);
        
        // 2D rotation
        float x_new = static_cast<float>(rotated[idx_0] * cos_theta - rotated[idx_1] * sin_theta);
        float y_new = static_cast<float>(rotated[idx_0] * sin_theta + rotated[idx_1] * cos_theta);
        
        rotated[idx_0] = x_new;
        rotated[idx_1] = y_new;
    }
    
    // Optional L2 normalization
    if (getConfig().normalize_after) {
        // Normalize
        double norm_squared = 0.0;
        for (float val : rotated) {
            norm_squared += val * val;
        }
        
        if (norm_squared > 0.0) {
            double norm = std::sqrt(norm_squared);
            for (float& val : rotated) {
                val = static_cast<float>(val / norm);
            }
        }
    }
    
    return rotated;
}

std::pair<double, double> LearnableRotaryEmbedding::computeLearnableRotationAngles(
    size_t position, 
    size_t pair_idx
) const {
    if (pair_idx >= learnable_theta_.size()) {
        throw std::out_of_range(
            "Pair index out of range: " + std::to_string(pair_idx) +
            " >= " + std::to_string(learnable_theta_.size())
        );
    }
    
    // Use learnable theta instead of base theta
    double theta = learnable_theta_[pair_idx];
    double angle = static_cast<double>(position) * theta;
    
    return {std::cos(angle), std::sin(angle)};
}

// ============================================================================
// Gradient Computation
// ============================================================================

std::vector<double> LearnableRotaryEmbedding::computeGradients(
    const std::vector<float>& embedding,
    float target_similarity,
    size_t position
) {
    if (!trainable_) {
        throw std::logic_error("Cannot compute gradients: parameters are not trainable");
    }
    
    // Initialize gradients
    std::vector<double> gradients(learnable_theta_.size(), 0.0);
    
    // Simple gradient approximation using finite differences
    // For production, this should be replaced with analytical gradients
    const double epsilon = 1e-6;
    
    // Rotate with current parameters
    auto rotated = rotate(embedding, position);
    
    // Compute baseline loss based on the rotated embedding
    // Simple loss: mean squared deviation from target
    double base_loss = 0.0;
    for (size_t i = 0; i < rotated.size(); ++i) {
        double diff = rotated[i] - (embedding[i] * target_similarity);
        base_loss += diff * diff;
    }
    base_loss /= rotated.size();
    
    // Compute gradient for each theta parameter using finite differences
    for (size_t i = 0; i < learnable_theta_.size(); ++i) {
        // Perturb theta[i] by epsilon
        double original_theta = learnable_theta_[i];
        learnable_theta_[i] = original_theta + epsilon;
        
        // Recompute rotation with perturbed theta
        auto perturbed = rotate(embedding, position);
        
        // Compute perturbed loss
        double perturbed_loss = 0.0;
        for (size_t j = 0; j < perturbed.size(); ++j) {
            double diff = perturbed[j] - (embedding[j] * target_similarity);
            perturbed_loss += diff * diff;
        }
        perturbed_loss /= perturbed.size();
        
        // Gradient approximation: (loss' - loss) / epsilon
        gradients[i] = (perturbed_loss - base_loss) / epsilon;
        
        // Restore original theta (safe now that we're modifying non-const object)
        learnable_theta_[i] = original_theta;
    }
    
    return gradients;
}

// ============================================================================
// Parameter Updates
// ============================================================================

void LearnableRotaryEmbedding::updateParameters(
    const std::vector<double>& gradients,
    float learning_rate
) {
    if (!trainable_) {
        throw std::logic_error("Cannot update parameters: not trainable");
    }
    
    if (gradients.size() != learnable_theta_.size()) {
        throw std::invalid_argument("Gradient size mismatch");
    }
    
    // Simple SGD update: theta = theta - learning_rate * gradient
    for (size_t i = 0; i < learnable_theta_.size(); ++i) {
        learnable_theta_[i] -= learning_rate * gradients[i];
        
        // Ensure theta stays positive (it's a frequency parameter)
        if (learnable_theta_[i] < 1e-8) {
            learnable_theta_[i] = 1e-8;
        }
    }
}

void LearnableRotaryEmbedding::updateSGD(
    const std::vector<double>& gradients,
    float learning_rate
) {
    updateParameters(gradients, learning_rate);
}

void LearnableRotaryEmbedding::updateAdam(
    const std::vector<double>& gradients,
    float learning_rate,
    const TrainingConfig& config
) {
    if (adam_m_.empty()) {
        initializeOptimizer(config);
    }
    
    adam_t_++;
    
    // Adam update: https://arxiv.org/abs/1412.6980
    for (size_t i = 0; i < learnable_theta_.size(); ++i) {
        // Update biased first moment estimate
        adam_m_[i] = config.adam_beta1 * adam_m_[i] + (1.0f - config.adam_beta1) * gradients[i];
        
        // Update biased second moment estimate
        adam_v_[i] = config.adam_beta2 * adam_v_[i] + 
                     (1.0f - config.adam_beta2) * gradients[i] * gradients[i];
        
        // Compute bias-corrected moment estimates
        double m_hat = adam_m_[i] / (1.0 - std::pow(config.adam_beta1, adam_t_));
        double v_hat = adam_v_[i] / (1.0 - std::pow(config.adam_beta2, adam_t_));
        
        // Update parameters
        learnable_theta_[i] -= learning_rate * m_hat / (std::sqrt(v_hat) + config.adam_epsilon);
        
        // Ensure theta stays positive
        if (learnable_theta_[i] < 1e-8) {
            learnable_theta_[i] = 1e-8;
        }
    }
}

void LearnableRotaryEmbedding::initializeOptimizer(const TrainingConfig& config) {
    if (config.use_adam) {
        adam_m_.resize(learnable_theta_.size(), 0.0);
        adam_v_.resize(learnable_theta_.size(), 0.0);
        adam_t_ = 0;
    }
}

// ============================================================================
// Training
// ============================================================================

float LearnableRotaryEmbedding::computeContrastiveLoss(
    const std::vector<TrainingSample>& batch,
    float /*temperature*/
) const {
    if (batch.empty()) {
        return 0.0f;
    }
    
    // Improved contrastive loss computation
    // Computes loss based on actual rotated embeddings
    
    float total_loss = 0.0f;
    
    for (const auto& sample : batch) {
        // Rotate the embedding using learnable theta
        auto rotated = rotate(sample.embedding, sample.position);
        
        // Compute loss as mean squared difference between rotated and target-scaled embedding
        // This encourages the rotation to preserve similarity according to target
        float sample_loss = 0.0f;
        for (size_t i = 0; i < rotated.size(); ++i) {
            float target_val = sample.embedding[i] * sample.similarity_target;
            float diff = rotated[i] - target_val;
            sample_loss += diff * diff;
        }
        sample_loss /= static_cast<float>(rotated.size());
        
        total_loss += sample_loss;
    }
    
    return total_loss / static_cast<float>(batch.size());
}

float LearnableRotaryEmbedding::computeValidationLoss(
    const std::vector<TrainingSample>& samples
) const {
    return computeContrastiveLoss(samples, 0.07f);
}

std::pair<std::vector<TrainingSample>, std::vector<TrainingSample>>
LearnableRotaryEmbedding::splitTrainValidation(
    const std::vector<TrainingSample>& samples,
    float validation_split
) const {
    if (validation_split <= 0.0f || validation_split >= 1.0f) {
        // No validation split
        return {samples, {}};
    }
    
    size_t val_size = static_cast<size_t>(samples.size() * validation_split);
    size_t train_size = samples.size() - val_size;
    
    // Simple split: take last val_size samples for validation
    std::vector<TrainingSample> train_samples(
        samples.begin(), 
        samples.begin() + train_size
    );
    std::vector<TrainingSample> val_samples(
        samples.begin() + train_size,
        samples.end()
    );
    
    return {train_samples, val_samples};
}

std::vector<float> LearnableRotaryEmbedding::train(
    const std::vector<TrainingSample>& samples,
    const TrainingConfig& config
) {
    if (!trainable_) {
        throw std::logic_error("Cannot train: parameters are not trainable");
    }
    
    if (samples.empty()) {
        throw std::invalid_argument("Cannot train on empty dataset");
    }
    
    // Initialize optimizer
    initializeOptimizer(config);
    
    // Split into train/validation
    auto [train_samples, val_samples] = splitTrainValidation(samples, config.validation_split);
    
    std::vector<float> loss_history;
    loss_history.reserve(config.max_epochs);
    
    float best_val_loss = std::numeric_limits<float>::max();
    size_t epochs_without_improvement = 0;
    // Training loop
    for (size_t epoch = 0; epoch < config.max_epochs; ++epoch) {
        setTrainingMode(true);

        // Shuffle training samples
        std::vector<TrainingSample> shuffled = train_samples;
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(shuffled.begin(), shuffled.end(), g);

        // Process mini-batches
        float epoch_loss = 0.0f;
        size_t num_batches = 0;

        for (size_t i = 0; i < shuffled.size(); i += config.batch_size) {
            size_t batch_end = std::min(i + config.batch_size, shuffled.size());
            std::vector<TrainingSample> batch(
                shuffled.begin() + i,
                shuffled.begin() + batch_end
            );

            // Compute batch loss
            float batch_loss = computeContrastiveLoss(batch, config.temperature);
            epoch_loss += batch_loss;
            num_batches++;

            // Compute and accumulate gradients for the batch
            std::vector<double> batch_gradients(learnable_theta_.size(), 0.0);

            for (const auto& sample : batch) {
                auto sample_grads = computeGradients(
                    sample.embedding,
                    sample.similarity_target,
                    sample.position
                );

                for (size_t j = 0; j < batch_gradients.size(); ++j) {
                    batch_gradients[j] += sample_grads[j];
                }
            }

            // Average gradients over batch
            for (auto& grad : batch_gradients) {
                grad /= static_cast<double>(batch.size());
            }

            // Update parameters
            if (config.use_adam) {
                updateAdam(batch_gradients, config.learning_rate, config);
            } else {
                updateSGD(batch_gradients, config.learning_rate);
            }
        }

        epoch_loss /= static_cast<float>(num_batches);
        loss_history.push_back(epoch_loss);

        // Validation
        if (!val_samples.empty()) {
            setTrainingMode(false);
            float val_loss = computeValidationLoss(val_samples);

            // Early stopping check
            if (val_loss < best_val_loss) {
                best_val_loss = val_loss;
                epochs_without_improvement = 0;
            } else {
                epochs_without_improvement++;

                if (epochs_without_improvement >= config.early_stop_patience) {
                    // Early stopping triggered
                    break;
                }
            }
        }
    }
    }
     
    setTrainingMode(false);
    return loss_history;
}

// ============================================================================
// Serialization
// ============================================================================

bool LearnableRotaryEmbedding::saveParameters(const std::string& path) const {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        // Write JSON format
        file << "{\n";
        file << "  \"version\": \"1.0\",\n";
        file << "  \"hidden_dim\": " << getConfig().hidden_dim << ",\n";
        file << "  \"num_rotation_pairs\": " << getConfig().num_rotation_pairs << ",\n";
        file << "  \"base_theta\": " << getConfig().base_theta << ",\n";
        file << "  \"learnable_theta\": [";
        
        for (size_t i = 0; i < learnable_theta_.size(); ++i) {
            if (i > 0) file << ", ";
            file << learnable_theta_[i];
        }
        
        file << "]\n";
        file << "}\n";
        
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool LearnableRotaryEmbedding::loadParameters(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        // Simple JSON parsing (production should use a proper JSON library)
        std::string line;
        std::vector<double> loaded_theta;
        bool reading_theta = false;
        
        while (std::getline(file, line)) {
            // Look for learnable_theta array
            if (line.find("\"learnable_theta\"") != std::string::npos) {
                // Find the array start
                size_t start = line.find('[');
                if (start != std::string::npos) {
                    reading_theta = true;
                    line = line.substr(start + 1);
                }
            }
            
            if (reading_theta) {
                // Parse numbers from the line
                std::stringstream ss(line);
                std::string token;
                
                while (std::getline(ss, token, ',')) {
                    // Remove whitespace and brackets
                    token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
                    token.erase(std::remove(token.begin(), token.end(), ']'), token.end());
                    
                    if (!token.empty() && token != "]") {
                        try {
                            double value = std::stod(token);
                            loaded_theta.push_back(value);
                        } catch (...) {
                            // Skip invalid values
                        }
                    }
                }
                
                if (line.find(']') != std::string::npos) {
                    reading_theta = false;
                    break;
                }
            }
        }
        
        file.close();
        
        // Validate and set loaded theta
        if (loaded_theta.size() == learnable_theta_.size()) {
            learnable_theta_ = loaded_theta;
            return true;
        }
        
        return false;
    } catch (...) {
        return false;
    }
}

} // namespace themis
