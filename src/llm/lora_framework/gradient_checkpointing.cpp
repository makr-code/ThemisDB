/**
 * @file gradient_checkpointing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gradient_checkpointing.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

GradientCheckpointer::GradientCheckpointer(const CheckpointConfig& config)
    : config_(config) {
    
    if (config_.total_layers <= 0 && config_.strategy == CheckpointStrategy::SQRT_N) {
        spdlog::warn("SQRT_N strategy requires total_layers > 0, falling back to NONE");
        config_.strategy = CheckpointStrategy::NONE;
    }
    
    stats_.total_layers = config_.total_layers;
    
    spdlog::info("GradientCheckpointer initialized:");
    spdlog::info("  Strategy: {}", static_cast<int>(config_.strategy));
    spdlog::info("  Total layers: {}", config_.total_layers);
    
    if (config_.strategy == CheckpointStrategy::UNIFORM) {
        spdlog::info("  Checkpoint frequency: {}", config_.checkpoint_frequency);
    } else if (config_.strategy == CheckpointStrategy::SQRT_N) {
        int interval = calculateSqrtNInterval();
        spdlog::info("  SQRT_N interval: {} (checkpoint every {} layers)", interval, interval);
    }
}

bool GradientCheckpointer::shouldCheckpoint(int layer_id, LayerType layer_type) const {
    if (layer_id < 0) {
        return false;
    }
    
    switch (config_.strategy) {
        case CheckpointStrategy::NONE:
            return false;
            
        case CheckpointStrategy::SQRT_N: {
            // Checkpoint every √n layers (optimal for deep networks)
            // For 32 layers: checkpoint at 0, 6, 12, 18, 24, 30
            int interval = calculateSqrtNInterval();
            if (interval <= 0) {
              return false;
            }
            return (layer_id % interval) == 0;
        }
        
        case CheckpointStrategy::UNIFORM:
            // Checkpoint every N layers
            if (config_.checkpoint_frequency <= 0) {
              return false;
            }
            return (layer_id % config_.checkpoint_frequency) == 0;
        
        case CheckpointStrategy::ATTENTION_ONLY: {
            // Only checkpoint attention layers (most memory-intensive)
            LayerType type = layer_type;
            if (type == LayerType::UNKNOWN) {
                auto it = layer_types_.find(layer_id);
                if (it != layer_types_.end()) {
                    type = it->second;
                }
            }
            return type == LayerType::ATTENTION;
        }
        
        case CheckpointStrategy::CUSTOM:
            // User-defined checkpoints
            return custom_checkpoints_.count(layer_id) > 0;
        
        default:
            return false;
    }
}

void GradientCheckpointer::saveCheckpoint(int layer_id, const GPUTensor& input, 
                                          ForwardFunction forward_fn) {
    if (!forward_fn) {
        throw std::invalid_argument("Forward function cannot be null");
    }
    
    CheckpointData data;
    data.input = input.clone();  // Make a copy of the input
    data.forward_fn = forward_fn;
    data.activation_size_bytes = input.size() * sizeof(float);  // Assuming float32
    
    checkpoints_[layer_id] = std::move(data);
    stats_.num_checkpoints = checkpoints_.size();
    
    spdlog::debug("Saved checkpoint for layer {} ({} bytes)", 
                 layer_id, data.activation_size_bytes);
}

bool GradientCheckpointer::hasCheckpoint([[maybe_unused]] int layer_id) const {
    return checkpoints_.find(layer_id) != checkpoints_.end();
}

GPUTensor GradientCheckpointer::recomputeActivation([[maybe_unused]] int layer_id) {
    auto it = checkpoints_.find(layer_id);
    if (it == checkpoints_.end()) {
        throw std::runtime_error("No checkpoint found for layer " + std::to_string(layer_id));
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Recompute forward pass
    const CheckpointData& checkpoint = it->second;
    GPUTensor recomputed = checkpoint.forward_fn(checkpoint.input);
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    
    // Update statistics
    updateRecomputeTime(duration.count());
    
    spdlog::debug("Recomputed activation for layer {} in {}ms", 
                 layer_id, duration.count());
    
    return recomputed;
}

void GradientCheckpointer::clearCheckpoint([[maybe_unused]] int layer_id) {
    auto it = checkpoints_.find(layer_id);
    if (it != checkpoints_.end()) {
        checkpoints_.erase(it);
        stats_.num_checkpoints = checkpoints_.size();
        spdlog::debug("Cleared checkpoint for layer {}", layer_id);
    }
}

void GradientCheckpointer::clearAll() {
    checkpoints_.clear();
    stats_.num_checkpoints = 0;
    stats_.memory_saved_bytes = 0;
    stats_.recomputation_time_ms = 0;
    spdlog::debug("Cleared all checkpoints");
}

void GradientCheckpointer::addCustomCheckpoint([[maybe_unused]] int layer_id) {
    custom_checkpoints_.insert(layer_id);
    spdlog::debug("Added custom checkpoint for layer {}", layer_id);
}

void GradientCheckpointer::setLayerType(int layer_id, LayerType type) {
    layer_types_[layer_id] = type;
}

CheckpointStats GradientCheckpointer::getStats() const {
    CheckpointStats stats = stats_;
    
    // Calculate memory reduction percentage
    if (stats_.total_layers > 0) {
        size_t avg_activation_size = 4 * 1024 * 1024;  // Assume 4MB per layer
        size_t total_memory = stats_.total_layers * avg_activation_size;
        size_t checkpointed_memory = stats_.num_checkpoints * avg_activation_size;
        
        stats.memory_saved_bytes = total_memory - checkpointed_memory;
        
        if (total_memory > 0) {
            stats.memory_reduction_pct = 100.0f * 
                static_cast<float>(stats.memory_saved_bytes) / 
                static_cast<float>(total_memory);
        }
    }
    
    // Calculate compute overhead
    stats.compute_overhead_pct = estimateComputeOverhead();
    
    return stats;
}

size_t GradientCheckpointer::estimateMemorySavings([[maybe_unused]] size_t avg_activation_size) const {
    if (stats_.total_layers == 0 || avg_activation_size == 0) {
        return 0;
    }
    
    // Memory saved = (total layers - checkpointed layers) × activation size
    size_t total_activation_memory = stats_.total_layers * avg_activation_size;
    size_t checkpointed_memory = stats_.num_checkpoints * avg_activation_size;
    
    return total_activation_memory > checkpointed_memory ? 
           total_activation_memory - checkpointed_memory : 0;
}

float GradientCheckpointer::estimateComputeOverhead() const {
    if (stats_.total_layers == 0) {
        return 0.0f;
    }
    
    // Compute overhead = (recomputation fraction) × typical overhead
    // Research shows ~30% overhead per recomputation
    float recompute_fraction = static_cast<float>(stats_.num_checkpoints) / 
                               static_cast<float>(stats_.total_layers);
    
    // For SQRT_N: overhead ≈ 20-25% (optimal)
    // For UNIFORM with high frequency: overhead can be higher
    float overhead_per_recompute = 0.30f;  // 30% per recomputation
    
    if (config_.strategy == CheckpointStrategy::SQRT_N) {
        // SQRT_N is more efficient due to optimal placement
        overhead_per_recompute = 0.25f;  // 25% per recomputation
    }
    
    return recompute_fraction * overhead_per_recompute * 100.0f;
}

void GradientCheckpointer::updateRecomputeTime([[maybe_unused]] size_t recompute_time_ms) {
    stats_.recomputation_time_ms += recompute_time_ms;
}

int GradientCheckpointer::calculateSqrtNInterval() const {
    if (config_.total_layers <= 0) {
        return 1;
    }
    
    // Checkpoint every √n layers
    int interval = static_cast<int>(std::sqrt(config_.total_layers));
    
    // Ensure at least 1 (avoid divide by zero)
    return std::max(1, interval);
}

} // namespace lora
} // namespace llm
} // namespace themis

