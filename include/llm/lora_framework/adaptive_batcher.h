/**
 * @file adaptive_batcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/gpu_memory_manager.h"
#include <cstddef>
#include <vector>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

class AdaptiveBatcher {
public:
    /**
     * @brief Adaptive Batcher.
     * @return Return value.
     */
    virtual ~AdaptiveBatcher() = default;
    struct Config {
        size_t min_batch_size = 1;
        size_t max_batch_size = 32;
        size_t target_vram_utilization_pct = 85;  // Target 85% VRAM usage
        bool enable_dynamic_batching = true;
        float vram_safety_margin = 0.9f;  // Leave 10% headroom
        
        // Memory estimation parameters
        size_t hidden_dim = 768;  // Model hidden dimension
        size_t lora_rank = 8;     // LoRA rank
    };
    
    explicit AdaptiveBatcher(const Config& config, ::themis::llm::GPUMemoryManager* mem_manager);
    
    /**
     * @brief Compute Optimal Batch Size.
     * @param[in] sequence_length Input parameter.
     * @return Return value.
     */
    size_t computeOptimalBatchSize(size_t sequence_length);
    
    /**
     * @brief Handle OOMEvent.
     */
    void handleOOMEvent();
    
    /**
     * @brief Increase Batch Size If Possible.
     */
    void increaseBatchSizeIfPossible();
    
    /**
     * @brief Update Utilization.
     * @param[in] utilization Input parameter.
     */
    void updateUtilization(float utilization);
    
    struct Stats {
        size_t current_batch_size = 0;
        float vram_utilization_pct = 0.0f;
        int oom_events = 0;
        float avg_gpu_utilization = 0.0f;
    };
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;
    
    size_t getCurrentBatchSize() const { return current_batch_size_; }
    
    /**
     * @brief Reset OOMCounter.
     * @details Implements resetOOMCounter without additional internal calls.
     */
    void resetOOMCounter() { oom_count_ = 0; }
    
    /**
     * @brief Calibrate Memory Estimation.
     * @param[in] actual_memory_used Input parameter.
     * @param[in] sequence_length Input parameter.
     * @param[in] batch_size Input parameter.
     */
    void calibrateMemoryEstimation(size_t actual_memory_used, 
                                    size_t sequence_length, 
                                    size_t batch_size);
    
private:
    Config config_;
    ::themis::llm::GPUMemoryManager* mem_manager_;
    size_t current_batch_size_ = 0;
    int oom_count_ = 0;
    std::vector<float> recent_utilizations_;
    
    // Calibration state
    bool is_calibrated_ = false;
    float memory_estimation_multiplier_ = 1.0f;  // Adjust estimates based on actual usage
    
    /**
     * @brief Compute Average Utilization.
     * @return Return value.
     */
    float computeAverageUtilization() const;
    
    /**
     * @brief Estimate Memory Per Sample.
     * @param[in] sequence_length Input parameter.
     * @return Return value.
     */
    size_t estimateMemoryPerSample(size_t sequence_length) const;
    
    /**
     * @brief Estimate Shared Memory.
     * @return Return value.
     */
    size_t estimateSharedMemory() const;
};

} // namespace lora
} // namespace llm
} // namespace themis
