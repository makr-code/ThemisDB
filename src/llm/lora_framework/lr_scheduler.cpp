/**
 * @file lr_scheduler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lr_scheduler.h"
#include "utils/type_conversion.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

// M_PI is not part of standard C++ but commonly available
// Define it if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using themis::utils::conversion::safe_double_to_float;

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// LinearLR Implementation
// ============================================================================

float LinearLR::get_lr([[maybe_unused]] int step) const {
    if (step >= total_steps_) {
        return end_lr_;
    }
    
    float progress = static_cast<float>(step) / static_cast<float>(total_steps_);
    return start_lr_ + (end_lr_ - start_lr_) * progress;
}

LRSchedulerConfig LinearLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::LINEAR;
    cfg.base_lr = start_lr_;
    cfg.min_lr = end_lr_;
    cfg.total_steps = total_steps_;
    return cfg;
}

// ============================================================================
// CosineAnnealingLR Implementation
// ============================================================================

float CosineAnnealingLR::get_lr([[maybe_unused]] int step) const {
    if (step >= total_steps_) {
        return min_lr_;
    }
    
    float progress = static_cast<float>(step) / static_cast<float>(total_steps_);
    float cosine_decay = 0.5f * (1.0f + static_cast<float>(std::cos(M_PI * progress)));
    return min_lr_ + (max_lr_ - min_lr_) * cosine_decay;
}

LRSchedulerConfig CosineAnnealingLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::COSINE;
    cfg.max_lr = max_lr_;
    cfg.min_lr = min_lr_;
    cfg.total_steps = total_steps_;
    return cfg;
}

// ============================================================================
// CosineAnnealingWarmRestartsLR Implementation
// ============================================================================

float CosineAnnealingWarmRestartsLR::get_lr([[maybe_unused]] int step) const {
    // Find which cycle we're in
    int cycle = step / period_;
    if (cycle >= num_cycles_) {
        return min_lr_;
    }
    
    // Progress within current cycle
    int step_in_cycle = step % period_;
    float progress = static_cast<float>(step_in_cycle) / static_cast<float>(period_);
    float cosine_decay = 0.5f * (1.0f + static_cast<float>(std::cos(M_PI * progress)));

    return min_lr_ + (max_lr_ - min_lr_) * cosine_decay;
}

LRSchedulerConfig CosineAnnealingWarmRestartsLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::COSINE_WITH_RESTARTS;
    cfg.max_lr = max_lr_;
    cfg.min_lr = min_lr_;
    cfg.step_size = static_cast<float>(period_);
    cfg.num_cycles = num_cycles_;
    return cfg;
}

// ============================================================================
// PolynomialLR Implementation
// ============================================================================

float PolynomialLR::get_lr([[maybe_unused]] int step) const {
    if (step >= total_steps_) {
        return end_lr_;
    }
    
    float progress = static_cast<float>(step) / static_cast<float>(total_steps_);
    float decay = std::pow(1.0f - progress, safe_double_to_float(power_, true));
    return end_lr_ + (start_lr_ - end_lr_) * decay;
}

LRSchedulerConfig PolynomialLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::POLYNOMIAL;
    cfg.base_lr = start_lr_;
    cfg.min_lr = end_lr_;
    cfg.total_steps = total_steps_;
    cfg.decay_power = power_;
    return cfg;
}

// ============================================================================
// StepLR Implementation
// ============================================================================

float StepLR::get_lr([[maybe_unused]] int step) const {
    int num_steps = step / step_size_;
    return static_cast<float>(initial_lr_ * std::pow(safe_double_to_float(gamma_, true), num_steps));
}

LRSchedulerConfig StepLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::STEP;
    cfg.base_lr = initial_lr_;
    cfg.step_size = static_cast<float>(step_size_);
    cfg.gamma = gamma_;
    return cfg;
}

// ============================================================================
// ExponentialLR Implementation
// ============================================================================

float ExponentialLR::get_lr([[maybe_unused]] int step) const {
    return static_cast<float>(initial_lr_ * std::pow(safe_double_to_float(gamma_, true), step));
}

LRSchedulerConfig ExponentialLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::EXPONENTIAL;
    cfg.base_lr = initial_lr_;
    cfg.gamma = gamma_;
    return cfg;
}

// ============================================================================
// WarmupConstantLR Implementation
// ============================================================================

float WarmupConstantLR::get_lr([[maybe_unused]] int step) const {
    if (step >= warmup_steps_) {
        return target_lr_;
    }
    
    // Linear warmup
    float progress = static_cast<float>(step) / static_cast<float>(warmup_steps_);
    return target_lr_ * progress;
}

LRSchedulerConfig WarmupConstantLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::WARMUP_CONSTANT;
    cfg.base_lr = target_lr_;
    cfg.warmup_steps = warmup_steps_;
    return cfg;
}

// ============================================================================
// WarmupCosineLR Implementation
// ============================================================================

float WarmupCosineLR::get_lr([[maybe_unused]] int step) const {
    // Warmup phase
    if (step < warmup_steps_) {
        float progress = static_cast<float>(step) / static_cast<float>(warmup_steps_);
        return max_lr_ * progress;
    }
    
    // Cosine annealing phase
    int adjusted_step = step - warmup_steps_;
    int adjusted_total = total_steps_ - warmup_steps_;
    
    if (adjusted_step >= adjusted_total) {
        return min_lr_;
    }
    
    float progress = static_cast<float>(adjusted_step) / static_cast<float>(adjusted_total);
    float cosine_decay = 0.5f * (1.0f + static_cast<float>(std::cos(M_PI * progress)));
    return min_lr_ + (max_lr_ - min_lr_) * cosine_decay;
}

LRSchedulerConfig WarmupCosineLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::WARMUP_COSINE;
    cfg.max_lr = max_lr_;
    cfg.min_lr = min_lr_;
    cfg.warmup_steps = warmup_steps_;
    cfg.total_steps = total_steps_;
    return cfg;
}

// ============================================================================
// WarmupLinearLR Implementation
// ============================================================================

float WarmupLinearLR::get_lr([[maybe_unused]] int step) const {
    // Warmup phase
    if (step < warmup_steps_) {
        float progress = static_cast<float>(step) / static_cast<float>(warmup_steps_);
        return max_lr_ * progress;
    }
    
    // Linear decay phase
    int adjusted_step = step - warmup_steps_;
    int adjusted_total = total_steps_ - warmup_steps_;
    
    if (adjusted_step >= adjusted_total) {
        return min_lr_;
    }
    
    float progress = static_cast<float>(adjusted_step) / static_cast<float>(adjusted_total);
    return max_lr_ + (min_lr_ - max_lr_) * progress;
}

LRSchedulerConfig WarmupLinearLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::WARMUP_LINEAR;
    cfg.max_lr = max_lr_;
    cfg.min_lr = min_lr_;
    cfg.warmup_steps = warmup_steps_;
    cfg.total_steps = total_steps_;
    return cfg;
}

// ============================================================================
// CyclicLR Implementation (Triangular)
// ============================================================================

float CyclicLR::get_lr([[maybe_unused]] int step) const {
    int cycle_length = step_size_up_ + step_size_down_;
    if (cycle_length <= 0) {
        return base_lr_;
    }
    int cycle_pos = step % cycle_length;

    if (cycle_pos <= step_size_up_) {
        float progress = static_cast<float>(cycle_pos) / static_cast<float>(std::max(1, step_size_up_));
        return base_lr_ + (max_lr_ - base_lr_) * progress;
    }

    int down_pos = cycle_pos - step_size_up_;
    float progress = static_cast<float>(down_pos) / static_cast<float>(std::max(1, step_size_down_));
    return max_lr_ - (max_lr_ - base_lr_) * progress;
}

LRSchedulerConfig CyclicLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::CYCLIC;
    cfg.base_lr = base_lr_;
    cfg.max_lr = max_lr_;
    cfg.step_size_up = step_size_up_;
    cfg.step_size_down = step_size_down_;
    return cfg;
}

// ============================================================================
// OneCycleLR Implementation
// ============================================================================

float OneCycleLR::get_lr([[maybe_unused]] int step) const {
    int warmup_steps = static_cast<int>(pct_start_ * total_steps_);
    warmup_steps = std::max(1, warmup_steps);
    int decay_steps = total_steps_ - warmup_steps;
    if (decay_steps < 1) {
      decay_steps = 1;
    }

    if (step < warmup_steps) {
        float progress = static_cast<float>(step) / static_cast<float>(warmup_steps);
        return base_lr_ + (max_lr_ - base_lr_) * progress;
    }

    int decay_step = step - warmup_steps;
    if (decay_step >= decay_steps) {
        return max_lr_ / final_div_factor_;
    }

    float progress = static_cast<float>(decay_step) / static_cast<float>(decay_steps);
    float min_lr = max_lr_ / final_div_factor_;
    float cosine_decay = 0.5f * (1.0f + static_cast<float>(std::cos(M_PI * progress)));
    return min_lr + (max_lr_ - min_lr) * cosine_decay;
}

LRSchedulerConfig OneCycleLR::config() const {
    LRSchedulerConfig cfg;
    cfg.type = SchedulerType::ONE_CYCLE;
    cfg.max_lr = max_lr_;
    cfg.base_lr = base_lr_;
    cfg.final_div_factor = final_div_factor_;
    cfg.total_steps = total_steps_;
    cfg.pct_start = pct_start_;
    return cfg;
}

// ============================================================================
// LRSchedulerFactory Implementation
// ============================================================================

std::unique_ptr<LRScheduler> LRSchedulerFactory::create(const LRSchedulerConfig& config) {
    spdlog::debug("Creating LR scheduler: type={}", static_cast<int>(config.type));
    
    switch (config.type) {
        case SchedulerType::CONSTANT:
            return std::make_unique<ConstantLR>(config.base_lr);
        
        case SchedulerType::LINEAR:
            return std::make_unique<LinearLR>(config.base_lr, config.min_lr, config.total_steps);
        
        case SchedulerType::COSINE:
            return std::make_unique<CosineAnnealingLR>(config.max_lr, config.min_lr, config.total_steps);
        
        case SchedulerType::COSINE_WITH_RESTARTS:
            return std::make_unique<CosineAnnealingWarmRestartsLR>(
                config.max_lr, config.min_lr, 
                static_cast<int>(config.step_size), config.num_cycles
            );
        
        case SchedulerType::POLYNOMIAL:
            return std::make_unique<PolynomialLR>(
                config.base_lr, config.min_lr, config.total_steps, config.decay_power
            );
        
        case SchedulerType::STEP:
            return std::make_unique<StepLR>(
                config.base_lr, static_cast<int>(config.step_size), config.gamma
            );
        
        case SchedulerType::EXPONENTIAL:
            return std::make_unique<ExponentialLR>(config.base_lr, config.gamma);
        
        case SchedulerType::WARMUP_CONSTANT:
            return std::make_unique<WarmupConstantLR>(config.base_lr, config.warmup_steps);
        
        case SchedulerType::WARMUP_COSINE:
            return std::make_unique<WarmupCosineLR>(
                config.max_lr, config.min_lr, config.warmup_steps, config.total_steps
            );
        
        case SchedulerType::WARMUP_LINEAR:
            // Warmup + linear decay
            return std::make_unique<WarmupLinearLR>(
                config.max_lr, config.min_lr, config.warmup_steps, config.total_steps
            );
        
        case SchedulerType::CYCLIC:
            return std::make_unique<CyclicLR>(
                config.base_lr, config.max_lr,
                static_cast<int>(config.step_size_up), static_cast<int>(config.step_size_down)
            );
        
        case SchedulerType::ONE_CYCLE:
            return std::make_unique<OneCycleLR>(
                config.max_lr, config.base_lr, config.final_div_factor,
                config.total_steps, config.pct_start
            );
        
        default:
            spdlog::warn("Unknown scheduler type, using constant");
            return std::make_unique<ConstantLR>(config.base_lr);
    }
}

std::unique_ptr<LRScheduler> LRSchedulerFactory::createConstant([[maybe_unused]] float lr) {
    return std::make_unique<ConstantLR>(lr);
}

std::unique_ptr<LRScheduler> LRSchedulerFactory::createLinearDecay(
    float start_lr, float end_lr, int steps
) {
    return std::make_unique<LinearLR>(start_lr, end_lr, steps);
}

std::unique_ptr<LRScheduler> LRSchedulerFactory::createCosineAnnealing(
    float max_lr, float min_lr, int steps
) {
    return std::make_unique<CosineAnnealingLR>(max_lr, min_lr, steps);
}

std::unique_ptr<LRScheduler> LRSchedulerFactory::createWarmupCosine(
    float max_lr, float min_lr, int warmup_steps, int total_steps
) {
    return std::make_unique<WarmupCosineLR>(max_lr, min_lr, warmup_steps, total_steps);
}

} // namespace lora
} // namespace llm
} // namespace themis

