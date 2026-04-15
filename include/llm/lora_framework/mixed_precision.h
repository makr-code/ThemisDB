/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mixed_precision.h                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     200                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "lora_layers.h"

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Precision mode for training
 */
enum class PrecisionMode {
    FP32,   // Full precision (32-bit float)
    FP16,   // Half precision (16-bit float)
    BF16,   // Brain Float 16
    AMP     // Automatic Mixed Precision
};

/**
 * @brief Configuration for mixed precision training
 */
struct MixedPrecisionConfig {
    PrecisionMode mode = PrecisionMode::FP32;
    float loss_scale = 1024.0f;           // Initial loss scaling factor
    float loss_scale_factor = 2.0f;       // Factor for dynamic loss scaling
    int loss_scale_window = 2000;         // Steps before increasing loss scale
    float max_loss_scale = 65536.0f;      // Maximum loss scale
    float min_loss_scale = 1.0f;          // Minimum loss scale
    bool dynamic_loss_scaling = true;     // Enable dynamic loss scaling
    int overflow_check_interval = 100;    // Check for overflow every N steps
    
    // Convert to JSON
    json toJSON() const {
        return json{
            {"mode", static_cast<int>(mode)},
            {"loss_scale", loss_scale},
            {"loss_scale_factor", loss_scale_factor},
            {"loss_scale_window", loss_scale_window},
            {"max_loss_scale", max_loss_scale},
            {"min_loss_scale", min_loss_scale},
            {"dynamic_loss_scaling", dynamic_loss_scaling},
            {"overflow_check_interval", overflow_check_interval}
        };
    }
};

/**
 * @brief Mixed precision trainer for LoRA
 * 
 * Features:
 * - FP16/BF16 forward and backward passes
 * - FP32 master weights and optimizer state
 * - Loss scaling to prevent underflow
 * - Dynamic loss scaling with overflow detection
 * - Memory efficiency (2x reduction)
 * - Speed improvement (up to 2x on compatible hardware)
 */
class MixedPrecisionTrainer {
public:
    explicit MixedPrecisionTrainer(const MixedPrecisionConfig& config = MixedPrecisionConfig{});
    ~MixedPrecisionTrainer() = default;
    
    /**
     * @brief Convert tensor to lower precision for forward pass
     * @param input FP32 input tensor
     * @return Lower precision tensor
     */
    Tensor to_lower_precision(const Tensor& input) const;
    
    /**
     * @brief Convert tensor back to FP32
     * @param input Lower precision tensor
     * @return FP32 tensor
     */
    Tensor to_fp32(const Tensor& input) const;
    
    /**
     * @brief Scale loss for backward pass (prevents underflow)
     * @param loss Original loss value
     * @return Scaled loss
     */
    float scale_loss(float loss);
    
    /**
     * @brief Unscale gradients after backward pass
     * @param gradients Vector of gradient tensors
     * @return true if no overflow detected, false otherwise
     */
    bool unscale_gradients(std::vector<Tensor*>& gradients);
    
    /**
     * @brief Check for NaN or Inf in gradients
     * @param gradients Vector of gradient tensors
     * @return true if overflow/underflow detected
     */
    static bool has_overflow(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Update loss scale based on overflow history
     * @param had_overflow Whether overflow occurred in this step
     */
    void update_loss_scale(bool had_overflow);
    
    /**
     * @brief Get current loss scale
     * @return Current loss scaling factor
     */
    float get_loss_scale() const { return current_loss_scale_; }
    
    /**
     * @brief Get precision mode
     * @return Current precision mode
     */
    PrecisionMode get_precision_mode() const { return config_.mode; }
    
    /**
     * @brief Check if mixed precision is enabled
     * @return true if mode is not FP32
     */
    bool is_enabled() const { return config_.mode != PrecisionMode::FP32; }
    
    /**
     * @brief Get statistics
     * @return JSON with training statistics
     */
    json get_stats() const;
    
    /**
     * @brief Reset statistics
     */
    void reset_stats();

private:
    MixedPrecisionConfig config_;
    float current_loss_scale_;
    int steps_since_overflow_;
    int total_overflows_;
    int total_steps_;
    
    // Helper for FP16/BF16 conversion (simplified for CPU)
    static float fp32_to_fp16(float value);
    static float fp16_to_fp32(float value);
};

/**
 * @brief RAII wrapper for mixed precision training scope
 * 
 * Usage:
 * ```cpp
 * MixedPrecisionTrainer trainer(config);
 * {
 *     auto scope = trainer.train_scope();
 *     // Training code here with mixed precision
 * }
 * ```
 */
class MixedPrecisionScope {
public:
    explicit MixedPrecisionScope(MixedPrecisionTrainer* trainer)
        : trainer_(trainer) {}
    
    ~MixedPrecisionScope() = default;
    
    MixedPrecisionTrainer* trainer() const { return trainer_; }

private:
    MixedPrecisionTrainer* trainer_;
};

} // namespace lora
} // namespace llm
} // namespace themis
