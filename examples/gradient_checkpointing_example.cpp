/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gradient_checkpointing_example.cpp                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file gradient_checkpointing_example.cpp
 * @brief Example demonstrating gradient checkpointing for memory-efficient LoRA training
 * 
 * This example shows how to enable and configure gradient checkpointing to reduce
 * memory usage during GPU training by up to 80%.
 */

#include "llm/lora_framework/gpu_training_loop.h"
#include "llm/lora_framework/gpu_data_loader.h"
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gradient_checkpointing.h"
#include <spdlog/spdlog.h>
#include <memory>

using namespace themis::llm::lora;

/**
 * @brief Example 1: Basic gradient checkpointing with SQRT_N strategy
 * 
 * This is the recommended approach for most use cases. The SQRT_N strategy
 * provides optimal memory-compute trade-off, checkpointing every √n layers.
 */
void example_basic_checkpointing() {
    spdlog::info("=== Example 1: Basic Gradient Checkpointing ===");
    
    // Configure training with gradient checkpointing enabled
    GPUTrainingConfig config;
    config.num_epochs = 3;
    config.learning_rate = 1e-4f;
    config.device = Device::cuda();
    
    // Enable gradient checkpointing with SQRT_N strategy
    config.enable_gradient_checkpointing = true;
    config.checkpoint_strategy = CheckpointStrategy::SQRT_N;
    
    spdlog::info("Training configuration:");
    spdlog::info("  Gradient checkpointing: ENABLED");
    spdlog::info("  Strategy: SQRT_N (optimal)");
    spdlog::info("  Expected memory reduction: 70-80%");
    spdlog::info("  Expected compute overhead: 20-25%");
    
    // Create training loop
    GPUTrainingLoop trainer(config);
    
    // Note: In a real application, you would:
    // 1. Set up data loader
    // 2. Add LoRA layers
    // 3. Call trainer.train()
    
    spdlog::info("Training loop created with checkpointing enabled");
}

/**
 * @brief Example 2: Custom checkpoint frequency
 * 
 * Use UNIFORM strategy when you want to control the checkpoint frequency
 * explicitly. Lower frequency = more memory, less compute overhead.
 */
void example_custom_frequency() {
    spdlog::info("=== Example 2: Custom Checkpoint Frequency ===");
    
    GPUTrainingConfig config;
    config.num_epochs = 3;
    config.learning_rate = 1e-4f;
    config.device = Device::cuda();
    
    // Enable checkpointing with custom frequency
    config.enable_gradient_checkpointing = true;
    config.checkpoint_strategy = CheckpointStrategy::UNIFORM;
    config.checkpoint_frequency = 4;  // Checkpoint every 4th layer
    
    spdlog::info("Checkpoint frequency: every {} layers", config.checkpoint_frequency);
    
    GPUTrainingLoop trainer(config);
}

/**
 * @brief Example 3: Estimating memory savings before training
 * 
 * Calculate expected memory savings and compute overhead before starting
 * training to validate configuration.
 */
void example_estimate_savings() {
    spdlog::info("=== Example 3: Estimate Memory Savings ===");
    
    // Configure checkpointing
    CheckpointConfig checkpoint_config;
    checkpoint_config.strategy = CheckpointStrategy::SQRT_N;
    checkpoint_config.total_layers = 32;  // Example: 32-layer model
    
    GradientCheckpointer checkpointer(checkpoint_config);
    
    // Estimate savings with assumed activation size
    // For a typical LoRA layer: batch_size × seq_len × hidden_dim × 4 bytes
    // Example: 4 × 512 × 768 × 4 = 6.3 MB per layer
    size_t avg_activation_size = 4 * 512 * 768 * 4;
    
    size_t memory_savings = checkpointer.estimateMemorySavings(avg_activation_size);
    float compute_overhead = checkpointer.estimateComputeOverhead();
    
    // Log estimates
    spdlog::info("Memory estimation for 32-layer model:");
    spdlog::info("  Avg activation size per layer: {:.2f} MB", 
                 avg_activation_size / (1024.0 * 1024.0));
    spdlog::info("  Total without checkpointing: {:.2f} MB",
                 (32 * avg_activation_size) / (1024.0 * 1024.0));
    spdlog::info("  Memory savings: {:.2f} MB ({:.1f}%)",
                 memory_savings / (1024.0 * 1024.0),
                 100.0f * memory_savings / (32 * avg_activation_size));
    spdlog::info("  Compute overhead: {:.1f}%", compute_overhead);
}

/**
 * @brief Example 4: Layer-level checkpointing control
 * 
 * Shows how to enable/disable checkpointing for individual layers.
 */
void example_layer_level_control() {
    spdlog::info("=== Example 4: Layer-Level Checkpointing ===");
    
    Device device = Device::cuda();
    
    // Create LoRA layers
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;
    
    GPULoRALayer layer1(in_dim, out_dim, rank, 1.0f, device);
    GPULoRALayer layer2(in_dim, out_dim, rank, 1.0f, device);
    GPULoRALayer layer3(in_dim, out_dim, rank, 1.0f, device);
    
    // Enable checkpointing for layer 1 and 3, but not layer 2
    layer1.set_checkpointing(true);
    layer1.set_layer_id(0);
    spdlog::info("Layer 0: Checkpointing ENABLED");
    
    layer2.set_checkpointing(false);
    layer2.set_layer_id(1);
    spdlog::info("Layer 1: Checkpointing DISABLED (normal caching)");
    
    layer3.set_checkpointing(true);
    layer3.set_layer_id(2);
    spdlog::info("Layer 2: Checkpointing ENABLED");
    
    // During training:
    // - Layer 1: Will recompute activations during backward pass
    // - Layer 2: Will use cached activations (faster but more memory)
    // - Layer 3: Will recompute activations during backward pass
}

/**
 * @brief Example 5: Different strategies comparison
 * 
 * Compare different checkpointing strategies for the same model.
 */
void example_strategy_comparison() {
    spdlog::info("=== Example 5: Strategy Comparison ===");
    
    int total_layers = 36;
    size_t avg_activation_size = 6 * 1024 * 1024;  // 6MB per layer
    
    struct StrategyResult {
        std::string name;
        size_t memory_saved;
        float compute_overhead;
    };
    
    std::vector<StrategyResult> results;
    
    // Test different strategies
    std::vector<std::pair<CheckpointStrategy, std::string>> strategies = {
        {CheckpointStrategy::NONE, "NONE"},
        {CheckpointStrategy::SQRT_N, "SQRT_N"},
        {CheckpointStrategy::UNIFORM, "UNIFORM (freq=4)"},
    };
    
    for (const auto& [strategy, name] : strategies) {
        CheckpointConfig config;
        config.strategy = strategy;
        config.total_layers = total_layers;
        config.checkpoint_frequency = 4;  // For UNIFORM
        
        GradientCheckpointer checkpointer(config);
        
        size_t memory_saved = checkpointer.estimateMemorySavings(avg_activation_size);
        float overhead = checkpointer.estimateComputeOverhead();
        
        results.push_back({name, memory_saved, overhead});
    }
    
    // Print comparison table
    spdlog::info("Strategy Comparison (36 layers, 6MB per layer):");
    spdlog::info("┌─────────────────┬────────────────┬─────────────────┐");
    spdlog::info("│ Strategy        │ Memory Saved   │ Compute Overhead│");
    spdlog::info("├─────────────────┼────────────────┼─────────────────┤");
    
    for (const auto& result : results) {
        spdlog::info("│ {:15} │ {:9.1f} MB   │ {:9.1f}%      │",
                     result.name,
                     result.memory_saved / (1024.0 * 1024.0),
                     result.compute_overhead);
    }
    
    spdlog::info("└─────────────────┴────────────────┴─────────────────┘");
}

/**
 * @brief Example 6: Complete training example with checkpointing
 * 
 * Shows a complete training setup with gradient checkpointing,
 * including data loading and layer configuration.
 */
void example_complete_training() {
    spdlog::info("=== Example 6: Complete Training with Checkpointing ===");
    
    // 1. Configure GPU training with checkpointing
    GPUTrainingConfig config;
    config.num_epochs = 5;
    config.learning_rate = 1e-4f;
    config.momentum = 0.9f;
    config.weight_decay = 0.01f;
    config.device = Device::cuda();
    config.use_mixed_precision = true;  // Combine with mixed precision for max efficiency
    
    // Enable gradient checkpointing
    config.enable_gradient_checkpointing = true;
    config.checkpoint_strategy = CheckpointStrategy::SQRT_N;
    
    spdlog::info("Configuration:");
    spdlog::info("  Epochs: {}", config.num_epochs);
    spdlog::info("  Learning rate: {}", config.learning_rate);
    spdlog::info("  Mixed precision: {}", config.use_mixed_precision);
    spdlog::info("  Gradient checkpointing: ENABLED (SQRT_N)");
    
    // 2. Create training loop
    GPUTrainingLoop trainer(config);
    
    // 3. Set up data loader (pseudo-code)
    spdlog::info("Note: In a real application, you would:");
    spdlog::info("  - Create and configure GPUDataLoader");
    spdlog::info("  - Load training data");
    spdlog::info("  - Add LoRA layers to trainer");
    spdlog::info("  - Call trainer.train()");
    
    // Expected output during training:
    spdlog::info("\nExpected training output:");
    spdlog::info("  [info] Gradient checkpointing initialized:");
    spdlog::info("  [info]   Strategy: 2 (SQRT_N)");
    spdlog::info("  [info]   Total layers: 32");
    spdlog::info("  [info]   Checkpointed layers: 6");
    spdlog::info("  [info]   Estimated memory savings: 2.35 GB");
    spdlog::info("  [info]   Estimated compute overhead: 24.8%");
    spdlog::info("  ...");
    spdlog::info("  [info] Checkpoint stats: 75.2% memory reduction, 24.8% compute overhead, 1250ms recompute time");
}

int main() {
    // Set log level
    spdlog::set_level(spdlog::level::info);
    
    spdlog::info("Gradient Checkpointing Examples");
    spdlog::info("================================\n");
    
    // Run examples
    example_basic_checkpointing();
    spdlog::info("");
    
    example_custom_frequency();
    spdlog::info("");
    
    example_estimate_savings();
    spdlog::info("");
    
    example_layer_level_control();
    spdlog::info("");
    
    example_strategy_comparison();
    spdlog::info("");
    
    example_complete_training();
    
    spdlog::info("\n================================");
    spdlog::info("Examples completed!");
    spdlog::info("See docs/gradient_checkpointing_guide.md for more details.");
    
    return 0;
}
