/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            learnable_rope_example.cpp                         ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Example: Training Learnable RoPE Parameters for Domain-Specific Optimization
// 
// This example demonstrates how to use the LearnableRotaryEmbedding class
// to train and optimize RoPE parameters for domain-specific data.

#include "index/learnable_rope.h"
#include <iostream>
#include <vector>
#include <numeric>

using namespace themis;

// Helper function to create synthetic training samples
std::vector<TrainingSample> createDomainSamples(size_t num_samples, size_t dim) {
    std::vector<TrainingSample> samples;
    samples.reserve(num_samples);
    
    for (size_t i = 0; i < num_samples; ++i) {
        std::vector<float> embedding(dim);
        
        // Create embeddings with domain-specific patterns
        // For example: medical documents might have specific sequential patterns
        for (size_t j = 0; j < dim; ++j) {
            embedding[j] = static_cast<float>(std::sin(i * 0.1 + j * 0.01));
        }
        
        // Position in sequence (e.g., sentence position in document)
        size_t position = i;
        
        // Target similarity (higher for nearby positions)
        float similarity_target = 0.9f - (i % 10) * 0.05f;
        
        samples.emplace_back(embedding, position, similarity_target);
    }
    
    return samples;
}

int main() {
    std::cout << "=== Learnable RoPE Training Example ===\n\n";
    
    // Step 1: Configure rotation parameters
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    std::cout << "Configuration:\n";
    std::cout << "  Hidden dimension: " << config.hidden_dim << "\n";
    std::cout << "  Rotation pairs: " << config.num_rotation_pairs << "\n";
    std::cout << "  Base theta: " << config.base_theta << "\n\n";
    
    // Step 2: Initialize learnable RoPE
    LearnableRotaryEmbedding learnable_rope(config, /*trainable=*/true);
    
    std::cout << "Learnable RoPE initialized\n";
    std::cout << "  Trainable: " << (learnable_rope.isTrainable() ? "Yes" : "No") << "\n";
    std::cout << "  Initial theta[0]: " << learnable_rope.getLearnableTheta()[0] << "\n\n";
    
    // Step 3: Create training data (e.g., from domain-specific documents)
    std::cout << "Generating training samples...\n";
    auto training_samples = createDomainSamples(200, 128);
    std::cout << "  Generated " << training_samples.size() << " samples\n\n";
    
    // Step 4: Configure training
    TrainingConfig train_config;
    train_config.learning_rate = 1e-3f;
    train_config.batch_size = 32;
    train_config.max_epochs = 20;
    train_config.validation_split = 0.2f;  // 20% for validation
    train_config.use_adam = true;          // Use Adam optimizer
    train_config.early_stop_patience = 5;
    
    std::cout << "Training configuration:\n";
    std::cout << "  Learning rate: " << train_config.learning_rate << "\n";
    std::cout << "  Batch size: " << train_config.batch_size << "\n";
    std::cout << "  Max epochs: " << train_config.max_epochs << "\n";
    std::cout << "  Optimizer: " << (train_config.use_adam ? "Adam" : "SGD") << "\n\n";
    
    // Step 5: Train the model
    std::cout << "Starting training...\n";
    auto loss_history = learnable_rope.train(training_samples, train_config);
    
    std::cout << "\nTraining complete!\n";
    std::cout << "  Epochs trained: " << loss_history.size() << "\n";
    std::cout << "  Initial loss: " << loss_history.front() << "\n";
    std::cout << "  Final loss: " << loss_history.back() << "\n";
    
    // Print loss progression
    std::cout << "\nLoss history:\n";
    for (size_t i = 0; i < loss_history.size(); ++i) {
        std::cout << "  Epoch " << (i + 1) << ": " << loss_history[i] << "\n";
    }
    std::cout << "\n";
    
    // Step 6: Compare learned parameters with base parameters
    std::cout << "Parameter comparison (first 5 theta values):\n";
    const auto& learned_theta = learnable_rope.getLearnableTheta();
    for (size_t i = 0; i < 5; ++i) {
        std::cout << "  theta[" << i << "]: "
                  << "base=" << config.theta_cache[i] << ", "
                  << "learned=" << learned_theta[i] << "\n";
    }
    std::cout << "\n";
    
    // Step 7: Save trained parameters
    std::string save_path = "/tmp/learned_rope_medical.json";
    bool saved = learnable_rope.saveParameters(save_path);
    
    if (saved) {
        std::cout << "Trained parameters saved to: " << save_path << "\n\n";
    } else {
        std::cout << "Failed to save parameters\n\n";
    }
    
    // Step 8: Demonstrate inference with trained parameters
    std::cout << "Testing inference with trained parameters:\n";
    learnable_rope.setTrainingMode(false);
    
    std::vector<float> test_embedding(128);
    std::iota(test_embedding.begin(), test_embedding.end(), 1.0f);
    
    auto rotated = learnable_rope.rotate(test_embedding, 42);
    std::cout << "  Rotated embedding at position 42 (first 5 values):\n";
    for (size_t i = 0; i < 5; ++i) {
        std::cout << "    [" << i << "]: " << rotated[i] << "\n";
    }
    std::cout << "\n";
    
    // Step 9: Load parameters in a new instance
    std::cout << "Loading parameters in a new instance...\n";
    LearnableRotaryEmbedding loaded_rope(config, true);
    
    if (loaded_rope.loadParameters(save_path)) {
        std::cout << "  Parameters loaded successfully\n";
        
        const auto& loaded_theta = loaded_rope.getLearnableTheta();
        bool params_match = true;
        for (size_t i = 0; i < learned_theta.size(); ++i) {
            if (std::abs(learned_theta[i] - loaded_theta[i]) > 1e-6) {
                params_match = false;
                break;
            }
        }
        
        std::cout << "  Parameter verification: " 
                  << (params_match ? "PASSED" : "FAILED") << "\n";
    } else {
        std::cout << "  Failed to load parameters\n";
    }
    
    std::cout << "\n=== Example Complete ===\n";
    
    return 0;
}
