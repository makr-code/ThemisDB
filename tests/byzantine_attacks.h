/**
 * @file byzantine_attacks.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/distributed_training_coordinator.h"
#include <random>
#include <algorithm>
#include <cstring>  // for std::memcpy

namespace themis {
namespace llm {
namespace byzantine_attacks {

// ============================================================================
// Attack Simulation Utilities
// ============================================================================

// Scale attack: multiply gradients by large factor
inline void scaleAttack(std::vector<GradientTensor>& gradients, float scale) {
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            val *= scale;
        }
    }
}

// Sign flip attack: negate gradients
inline void signFlipAttack(std::vector<GradientTensor>& gradients) {
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            val = -val;
        }
    }
}

// Gaussian noise attack: add random noise
inline void noiseAttack(
    std::vector<GradientTensor>& gradients, 
    float stddev,
    unsigned int seed = 42
) {
    std::mt19937 gen(seed);
    std::normal_distribution<float> dist(0.0f, stddev);
    
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            val += dist(gen);
        }
    }
}

// Zero gradients attack: send all zeros
inline void zeroAttack(std::vector<GradientTensor>& gradients) {
    for (auto& tensor : gradients) {
        std::fill(tensor.data.begin(), tensor.data.end(), 0.0f);
    }
}

// Random gradients attack: replace with random values
inline void randomAttack(
    std::vector<GradientTensor>& gradients,
    float min_val = -1.0f,
    float max_val = 1.0f,
    unsigned int seed = 42
) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(min_val, max_val);
    
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            val = dist(gen);
        }
    }
}

// Constant value attack: set all gradients to a constant
inline void constantAttack(std::vector<GradientTensor>& gradients, float constant) {
    for (auto& tensor : gradients) {
        std::fill(tensor.data.begin(), tensor.data.end(), constant);
    }
}

// Bit flip attack: introduce random bit flips (simulating hardware errors)
inline void bitFlipAttack(
    std::vector<GradientTensor>& gradients,
    float flip_probability = 0.001f,
    unsigned int seed = 42
) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            if (dist(gen) < flip_probability) {
                // Flip a random bit in the float representation
                uint32_t bits;
                std::memcpy(&bits, &val, sizeof(float));
                
                // Flip a random bit (0-31)
                int bit_pos = static_cast<int>(dist(gen) * 32);
                bits ^= (1u << bit_pos);
                
                std::memcpy(&val, &bits, sizeof(float));
            }
        }
    }
}

// Label flipping attack: simulate poisoned data by inverting gradient signs selectively
inline void labelFlipAttack(
    std::vector<GradientTensor>& gradients,
    float flip_rate = 0.1f,
    unsigned int seed = 42
) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            if (dist(gen) < flip_rate) {
                val = -val;
            }
        }
    }
}

// Delayed gradient attack: use gradients from previous iteration (stale gradients)
inline void delayedGradientAttack(
    std::vector<GradientTensor>& current_gradients,
    const std::vector<GradientTensor>& previous_gradients
) {
    if (previous_gradients.empty() || 
        current_gradients.size() != previous_gradients.size()) {
        return;  // Cannot apply attack
    }
    
    for (size_t i = 0; i < current_gradients.size(); ++i) {
        if (current_gradients[i].data.size() == previous_gradients[i].data.size()) {
            current_gradients[i].data = previous_gradients[i].data;
        }
    }
}

// Backdoor attack: inject specific pattern into gradients
struct BackdoorPattern {
    std::vector<size_t> target_indices;  // Which gradient elements to modify
    std::vector<float> target_values;    // Values to inject
};

inline void backdoorAttack(
    std::vector<GradientTensor>& gradients,
    const BackdoorPattern& pattern
) {
    if (gradients.empty()) return;
    
    // Apply pattern to first tensor for simplicity
    auto& tensor = gradients[0];
    
    for (size_t i = 0; i < pattern.target_indices.size(); ++i) {
        size_t idx = pattern.target_indices[i];
        if (idx < tensor.data.size()) {
            tensor.data[idx] = pattern.target_values[i];
        }
    }
}

// Clipping attack: clip gradients to small range (gradient hiding)
inline void clippingAttack(
    std::vector<GradientTensor>& gradients,
    float clip_value = 0.001f
) {
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            if (val > clip_value) val = clip_value;
            if (val < -clip_value) val = -clip_value;
        }
    }
}

// Min-max attack: send minimum or maximum representative gradients
inline void minMaxAttack(
    std::vector<GradientTensor>& gradients,
    const std::vector<std::vector<GradientTensor>>& all_shard_gradients,
    bool use_min = true
) {
    if (all_shard_gradients.size() < 2 || gradients.empty()) return;
    
    for (size_t layer_idx = 0; layer_idx < gradients.size(); ++layer_idx) {
        auto& tensor = gradients[layer_idx];
        
        for (size_t coord = 0; coord < tensor.data.size(); ++coord) {
            std::vector<float> values;
            for (const auto& shard_grads : all_shard_gradients) {
                if (layer_idx < shard_grads.size() && 
                    coord < shard_grads[layer_idx].data.size()) {
                    values.push_back(shard_grads[layer_idx].data[coord]);
                }
            }
            
            if (!values.empty()) {
                if (use_min) {
                    tensor.data[coord] = *std::min_element(values.begin(), values.end());
                } else {
                    tensor.data[coord] = *std::max_element(values.begin(), values.end());
                }
            }
        }
    }
}

// Gaussian attack with outlier: add large Gaussian noise to create outliers
inline void gaussianOutlierAttack(
    std::vector<GradientTensor>& gradients,
    float mean_multiplier = 10.0f,
    float stddev_multiplier = 5.0f,
    unsigned int seed = 42
) {
    // First compute mean and stddev of current gradients
    float sum = 0.0f;
    int count = 0;
    for (const auto& tensor : gradients) {
        for (float val : tensor.data) {
            sum += val;
            count++;
        }
    }
    float mean = count > 0 ? sum / count : 0.0f;
    
    float sum_sq_diff = 0.0f;
    for (const auto& tensor : gradients) {
        for (float val : tensor.data) {
            float diff = val - mean;
            sum_sq_diff += diff * diff;
        }
    }
    float stddev = count > 0 ? std::sqrt(sum_sq_diff / count) : 1.0f;
    
    // Add outlier noise
    std::mt19937 gen(seed);
    std::normal_distribution<float> dist(
        mean * mean_multiplier, 
        stddev * stddev_multiplier
    );
    
    for (auto& tensor : gradients) {
        for (auto& val : tensor.data) {
            val = dist(gen);
        }
    }
}

// Helper: Create benign gradients for testing
inline std::vector<GradientTensor> createBenignGradients(
    int num_layers = 3,
    int layer_size = 64,
    float mean = 0.0f,
    float stddev = 0.01f,
    unsigned int seed = 42
) {
    std::mt19937 gen(seed);
    std::normal_distribution<float> dist(mean, stddev);
    
    std::vector<GradientTensor> gradients;
    
    for (int layer = 0; layer < num_layers; ++layer) {
        GradientTensor tensor;
        tensor.layer_name = "layer_" + std::to_string(layer);
        tensor.shape = {layer_size};
        tensor.data.resize(layer_size);
        
        for (auto& val : tensor.data) {
            val = dist(gen);
        }
        
        gradients.push_back(tensor);
    }
    
    return gradients;
}

} // namespace byzantine_attacks
} // namespace llm
} // namespace themis
