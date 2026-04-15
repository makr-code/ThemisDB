/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mixed_precision.cpp                                ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:42:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     210                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/lora_framework/mixed_precision.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

MixedPrecisionTrainer::MixedPrecisionTrainer(const MixedPrecisionConfig& config)
    : config_(config),
      current_loss_scale_(config.loss_scale),
      steps_since_overflow_(0),
      total_overflows_(0),
      total_steps_(0) {
    
    spdlog::info("MixedPrecisionTrainer initialized:");
    spdlog::info("  Mode: {}", static_cast<int>(config_.mode));
    spdlog::info("  Initial loss scale: {}", current_loss_scale_);
    spdlog::info("  Dynamic scaling: {}", config_.dynamic_loss_scaling);
}

Tensor MixedPrecisionTrainer::to_lower_precision(const Tensor& input) const {
    if (!is_enabled()) {
        return input.clone();
    }
    
    // Simulate FP16/BF16 conversion on CPU
    // In production with GPU, this would use native half-precision types
    Tensor output = input.clone();
    
    if (config_.mode == PrecisionMode::FP16 || config_.mode == PrecisionMode::AMP) {
        // Simulate FP16 precision loss
        for (size_t i = 0; i < output.size(); ++i) {
            output[i] = fp16_to_fp32(fp32_to_fp16(output[i]));
        }
    } else if (config_.mode == PrecisionMode::BF16) {
        // BF16 has same exponent range as FP32, just reduced mantissa
        for (size_t i = 0; i < output.size(); ++i) {
            // Truncate mantissa bits (simplified)
            float val = output[i];
            uint32_t bits;
            std::memcpy(&bits, &val, sizeof(float));
            bits &= 0xFFFF0000;  // Keep only upper 16 bits
            std::memcpy(&val, &bits, sizeof(float));
            output[i] = val;
        }
    }
    
    return output;
}

Tensor MixedPrecisionTrainer::to_fp32(const Tensor& input) const {
    // If already in FP32 or no conversion needed
    return input.clone();
}

float MixedPrecisionTrainer::scale_loss(float loss) {
    if (!is_enabled()) {
        return loss;
    }
    
    total_steps_++;
    return loss * current_loss_scale_;
}

bool MixedPrecisionTrainer::unscale_gradients(std::vector<Tensor*>& gradients) {
    if (!is_enabled() || gradients.empty()) {
        return true;  // No overflow
    }
    
    // Check for overflow before unscaling
    bool overflow = has_overflow(gradients);
    
    if (!overflow) {
        // Unscale all gradients
        float scale = 1.0f / current_loss_scale_;
        for (auto* grad_ptr : gradients) {
            if (grad_ptr && grad_ptr->data().size() > 0) {
                for (size_t i = 0; i < grad_ptr->size(); ++i) {
                    (*grad_ptr)[i] *= scale;
                }
            }
        }
    }
    
    return !overflow;
}

bool MixedPrecisionTrainer::has_overflow(const std::vector<Tensor*>& gradients) {
    for (const auto* grad_ptr : gradients) {
        if (!grad_ptr) continue;
        
        const auto& data = grad_ptr->data();
        for (float val : data) {
            if (std::isnan(val) || std::isinf(val)) {
                return true;
            }
            // Check for very large values that might cause issues
            if (std::abs(val) > 1e10f) {
                return true;
            }
        }
    }
    return false;
}

void MixedPrecisionTrainer::update_loss_scale(bool had_overflow) {
    if (!config_.dynamic_loss_scaling || !is_enabled()) {
        return;
    }
    
    if (had_overflow) {
        // Reduce loss scale on overflow
        current_loss_scale_ = std::max(
            config_.min_loss_scale,
            current_loss_scale_ / config_.loss_scale_factor
        );
        steps_since_overflow_ = 0;
        total_overflows_++;
        
        spdlog::warn("Gradient overflow detected. Reducing loss scale to {}", current_loss_scale_);
    } else {
        steps_since_overflow_++;
        
        // Increase loss scale after stable period
        if (steps_since_overflow_ >= config_.loss_scale_window) {
            current_loss_scale_ = std::min(
                config_.max_loss_scale,
                current_loss_scale_ * config_.loss_scale_factor
            );
            steps_since_overflow_ = 0;
            
            spdlog::debug("Increasing loss scale to {}", current_loss_scale_);
        }
    }
}

json MixedPrecisionTrainer::get_stats() const {
    return json{
        {"precision_mode", static_cast<int>(config_.mode)},
        {"current_loss_scale", current_loss_scale_},
        {"total_steps", total_steps_},
        {"total_overflows", total_overflows_},
        {"steps_since_overflow", steps_since_overflow_},
        {"overflow_rate", total_steps_ > 0 ? 
            static_cast<float>(total_overflows_) / total_steps_ : 0.0f}
    };
}

void MixedPrecisionTrainer::reset_stats() {
    steps_since_overflow_ = 0;
    total_overflows_ = 0;
    total_steps_ = 0;
    current_loss_scale_ = config_.loss_scale;
}

// Simplified FP16 conversion for CPU (IEEE 754 half precision)
// NOTE: This is a simplified implementation for Phase 1 (CPU-only)
// Real GPU implementation would use native __half type from cuda_fp16.h
// and perform proper mantissa/exponent conversion
float MixedPrecisionTrainer::fp32_to_fp16(float value) {
    // Clamp to FP16 range
    const float fp16_max = 65504.0f;
    const float fp16_min = -65504.0f;
    
    if (value > fp16_max) return fp16_max;
    if (value < fp16_min) return fp16_min;
    if (std::isnan(value)) return std::numeric_limits<float>::quiet_NaN();
    
    // For CPU simulation, we just clamp to FP16 range
    // TODO: In GPU implementation, convert to actual FP16 format:
    //   - Extract sign, exponent, mantissa bits
    //   - Convert to 16-bit representation (1 sign + 5 exp + 10 mantissa)
    //   - Store as __half type for GPU operations
    return value;
}

float MixedPrecisionTrainer::fp16_to_fp32(float value) {
    // No conversion needed for CPU simulation
    return value;
}

} // namespace lora
} // namespace llm
} // namespace themis
