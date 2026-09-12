/**
 * @file adaptive_batcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/adaptive_batcher.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace themis {
namespace llm {
namespace lora {

AdaptiveBatcher::AdaptiveBatcher(const Config& config, ::themis::llm::GPUMemoryManager* mem_manager)
    : config_(config)
    , mem_manager_(mem_manager)
    , current_batch_size_(config.min_batch_size)
    , oom_count_(0) {
    
    if (!mem_manager_) {
        spdlog::warn("AdaptiveBatcher: No memory manager provided, dynamic batching may be limited");
    }
    
    // Reserve space for utilization history (last 100 values)
    recent_utilizations_.reserve(100);
    
    spdlog::info("AdaptiveBatcher initialized:");
    spdlog::info("  Batch size range: [{}, {}]", config_.min_batch_size, config_.max_batch_size);
    spdlog::info("  Target VRAM utilization: {}%", config_.target_vram_utilization_pct);
    spdlog::info("  VRAM safety margin: {:.1f}%", config_.vram_safety_margin * 100.0f);
}

size_t AdaptiveBatcher::computeOptimalBatchSize(size_t sequence_length) {
    if (!config_.enable_dynamic_batching) {
        return current_batch_size_;
    }
    
    if (!mem_manager_) {
        spdlog::debug("No memory manager, using current batch size: {}", current_batch_size_);
        return current_batch_size_;
    }
    
    // Get available VRAM
    size_t available_vram = mem_manager_->getFreeVRAM();
    size_t target_vram = static_cast<size_t>(available_vram * config_.vram_safety_margin);
    
    spdlog::debug("Computing optimal batch size:");
    spdlog::debug("  Available VRAM: {:.2f} GB", available_vram / (1024.0 * 1024.0 * 1024.0));
    spdlog::debug("  Target VRAM ({}% margin): {:.2f} GB", 
                  config_.vram_safety_margin * 100.0f,
                  target_vram / (1024.0 * 1024.0 * 1024.0));
    
    // Estimate memory per sample
    size_t per_sample_memory = estimateMemoryPerSample(sequence_length);
    
    // Shared memory (weights, optimizer)
    size_t shared_memory = estimateSharedMemory();
    
    spdlog::debug("  Per-sample memory: {:.2f} MB", per_sample_memory / (1024.0 * 1024.0));
    spdlog::debug("  Shared memory: {:.2f} MB", shared_memory / (1024.0 * 1024.0));
    
    // Compute max batch size
    if (target_vram <= shared_memory) {
        spdlog::warn("Insufficient VRAM for even shared memory, using min batch size");
        current_batch_size_ = config_.min_batch_size;
        return current_batch_size_;
    }
    
    size_t available_for_batches = target_vram - shared_memory;
    size_t max_batch = available_for_batches / per_sample_memory;
    
    // Clamp to configured limits
    size_t optimal_batch = std::clamp(
        max_batch,
        config_.min_batch_size,
        config_.max_batch_size
    );
    
    // If we had OOM events, be more conservative
    if (oom_count_ > 0) {
        optimal_batch = std::min(optimal_batch, current_batch_size_);
    }
    
    spdlog::debug("  Optimal batch size: {} (seq_len={})", optimal_batch, sequence_length);
    
    current_batch_size_ = optimal_batch;
    return optimal_batch;
}

void AdaptiveBatcher::handleOOMEvent() {
    // Reduce batch size by 25% on OOM
    size_t new_batch_size = static_cast<size_t>(current_batch_size_ * 0.75);
    current_batch_size_ = std::max(new_batch_size, config_.min_batch_size);
    
    oom_count_++;
    
    spdlog::warn("OOM detected (event #{}), reducing batch size to {}", 
                 oom_count_, current_batch_size_);
}

void AdaptiveBatcher::increaseBatchSizeIfPossible() {
    if (!config_.enable_dynamic_batching) {
        return;
    }
    
    // Increase batch size by 10% if utilization < 75%
    float avg_utilization = computeAverageUtilization();
    
    if (avg_utilization < 0.75f && oom_count_ == 0) {
        size_t new_batch_size = static_cast<size_t>(current_batch_size_ * 1.1);
        new_batch_size = std::min(new_batch_size, config_.max_batch_size);
        
        if (new_batch_size > current_batch_size_) {
            spdlog::info("Low GPU utilization ({:.1f}%), increasing batch size from {} to {}",
                        avg_utilization * 100.0f, current_batch_size_, new_batch_size);
            current_batch_size_ = new_batch_size;
        }
    } else if (avg_utilization >= 0.75f) {
        spdlog::debug("GPU utilization is good ({:.1f}%), keeping batch size at {}",
                     avg_utilization * 100.0f, current_batch_size_);
    }
}

void AdaptiveBatcher::updateUtilization(float utilization) {
    recent_utilizations_.push_back(utilization);
    
    // Keep only last 100 values
    if (recent_utilizations_.size() > 100) {
        recent_utilizations_.erase(recent_utilizations_.begin());
    }
}

AdaptiveBatcher::Stats AdaptiveBatcher::getStats() const {
    Stats stats;
    stats.current_batch_size = current_batch_size_;
    stats.oom_events = oom_count_;
    stats.avg_gpu_utilization = computeAverageUtilization();
    
    if (mem_manager_) {
        size_t total_vram = mem_manager_->getTotalVRAM();
        size_t used_vram = total_vram - mem_manager_->getFreeVRAM();
        stats.vram_utilization_pct = total_vram > 0 
            ? (100.0f * used_vram / total_vram) 
            : 0.0f;
    } else {
        stats.vram_utilization_pct = 0.0f;
    }
    
    return stats;
}

float AdaptiveBatcher::computeAverageUtilization() const {
    if (recent_utilizations_.empty()) {
        return 0.5f;  // Default to 50% if no data
    }
    
    float sum = std::accumulate(recent_utilizations_.begin(), 
                                recent_utilizations_.end(), 
                                0.0f);
    return sum / recent_utilizations_.size();
}

size_t AdaptiveBatcher::estimateMemoryPerSample(size_t sequence_length) const {
    // Memory breakdown per sample:
    // - Input embeddings: seq_len × hidden_dim × 4 bytes
    // - LoRA activations: seq_len × rank × 4 bytes
    // - Gradients: seq_len × hidden_dim × 4 bytes
    
    size_t input_memory = sequence_length * config_.hidden_dim * 4;
    size_t activation_memory = sequence_length * config_.lora_rank * 4;
    size_t gradient_memory = sequence_length * config_.hidden_dim * 4;
    
    size_t base_estimate = input_memory + activation_memory + gradient_memory;
    
    // Apply calibration multiplier if calibrated
    return static_cast<size_t>(base_estimate * memory_estimation_multiplier_);
}

size_t AdaptiveBatcher::estimateSharedMemory() const {
    // Shared memory (not per-sample):
    // - LoRA weights: (hidden_dim × rank + rank × hidden_dim) × 4 bytes
    // - Optimizer state (Adam): 2 × parameters × 4 bytes
    
    size_t lora_params = config_.hidden_dim * config_.lora_rank + 
                        config_.lora_rank * config_.hidden_dim;
    size_t weight_memory = lora_params * 4;
    size_t optimizer_memory = lora_params * 2 * 4;  // Adam: momentum + variance
    
    size_t base_estimate = weight_memory + optimizer_memory;
    
    // Apply calibration multiplier if calibrated
    return static_cast<size_t>(base_estimate * memory_estimation_multiplier_);
}

// Helper to get base estimates without calibration multiplier
size_t estimateMemoryPerSampleBase(size_t sequence_length, const AdaptiveBatcher::Config& config) {
    size_t input_memory = sequence_length * config.hidden_dim * 4;
    size_t activation_memory = sequence_length * config.lora_rank * 4;
    size_t gradient_memory = sequence_length * config.hidden_dim * 4;
    return input_memory + activation_memory + gradient_memory;
}

size_t estimateSharedMemoryBase(const AdaptiveBatcher::Config& config) {
    size_t lora_params = config.hidden_dim * config.lora_rank + 
                        config.lora_rank * config.hidden_dim;
    size_t weight_memory = lora_params * 4;
    size_t optimizer_memory = lora_params * 2 * 4;
    return weight_memory + optimizer_memory;
}

void AdaptiveBatcher::calibrateMemoryEstimation(
    size_t actual_memory_used,
    size_t sequence_length,
    size_t batch_size
) {
    if (batch_size == 0 || sequence_length == 0) {
        return;
    }
    
    // Calculate base prediction WITHOUT applying current multiplier
    // This prevents feedback loop
    size_t per_sample_base = estimateMemoryPerSampleBase(sequence_length, config_);
    size_t shared_base = estimateSharedMemoryBase(config_);
    size_t predicted = shared_base + (per_sample_base * batch_size);
    
    if (predicted == 0) {
        return;
    }
    
    // Calculate ratio between actual and predicted (base)
    float ratio = static_cast<float>(actual_memory_used) / predicted;
    
    // Use exponential moving average to smooth calibration
    const float alpha = 0.1f;  // Smoothing factor
    if (!is_calibrated_) {
        memory_estimation_multiplier_ = ratio;
        is_calibrated_ = true;
        spdlog::info("Initial memory calibration: multiplier = {:.2f}", ratio);
    } else {
        memory_estimation_multiplier_ = 
            alpha * ratio + (1.0f - alpha) * memory_estimation_multiplier_;
        spdlog::debug("Updated memory calibration: multiplier = {:.2f} (ratio = {:.2f})",
                     memory_estimation_multiplier_, ratio);
    }
    
    // Log significant deviations
    if (std::abs(ratio - 1.0f) > 0.3f) {
        spdlog::warn("Memory estimation deviation: predicted {:.2f} GB, actual {:.2f} GB",
                    predicted / (1024.0 * 1024.0 * 1024.0),
                    actual_memory_used / (1024.0 * 1024.0 * 1024.0));
    }
}

} // namespace lora
} // namespace llm
} // namespace themis
