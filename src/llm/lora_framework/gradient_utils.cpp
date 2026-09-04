/**
 * @file gradient_utils.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gradient_utils.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <limits>
#include <algorithm>
#include <numeric>

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// GradientUtils Implementation
// ============================================================================

float GradientUtils::compute_global_norm(const std::vector<Tensor*>& gradients) {
    float sum_of_squares = 0.0f;
    
    for (const auto* grad_ptr : gradients) {
        if (!grad_ptr) continue;
        
        const auto& data = grad_ptr->data();
        for (float val : data) {
            sum_of_squares += val * val;
        }
    }
    
    return std::sqrt(sum_of_squares);
}

bool GradientUtils::clip_by_norm(std::vector<Tensor*>& gradients, float max_norm) {
    float global_norm = compute_global_norm(gradients);
    
    if (global_norm <= max_norm) {
        return false;  // No clipping needed
    }
    
    // Scale all gradients by (max_norm / global_norm)
    float scale = max_norm / global_norm;
    
    for (auto* grad_ptr : gradients) {
        if (!grad_ptr) continue;
        
        for (size_t i = 0; i < grad_ptr->size(); ++i) {
            (*grad_ptr)[i] *= scale;
        }
    }
    
    spdlog::debug("Clipped gradients: norm={:.4f} -> {:.4f}", global_norm, max_norm);
    return true;
}

bool GradientUtils::clip_by_value(std::vector<Tensor*>& gradients, float clip_value) {
    bool clipped = false;
    
    for (auto* grad_ptr : gradients) {
        if (!grad_ptr) continue;
        
        for (size_t i = 0; i < grad_ptr->size(); ++i) {
            float val = (*grad_ptr)[i];
            if (std::abs(val) > clip_value) {
                (*grad_ptr)[i] = std::copysign(clip_value, val);
                clipped = true;
            }
        }
    }
    
    if (clipped) {
        spdlog::debug("Clipped gradients by value: max={:.4f}", clip_value);
    }
    
    return clipped;
}

GradientStats GradientUtils::apply_clipping(
    std::vector<Tensor*>& gradients,
    const GradientClippingConfig& config
) {
    GradientStats stats = compute_stats(gradients);
    
    bool clipped = false;
    switch (config.method) {
        case ClippingMethod::BY_NORM:
        [[fallthrough]];\n        case ClippingMethod::BY_GLOBAL_NORM:
            clipped = clip_by_norm(gradients, config.max_norm);
            break;
        
        case ClippingMethod::BY_VALUE:
            clipped = clip_by_value(gradients, config.clip_value);
            break;
        
        case ClippingMethod::NONE:
        [[fallthrough]];\n        default:
            break;
    }
    
    if (clipped) {
        stats.num_clipped++;
    }
    
    return stats;
}

void GradientUtils::accumulate_gradients(
    std::vector<Tensor>& accumulated,
    const std::vector<Tensor*>& new_gradients
) {
    // Initialize accumulated if empty
    if (accumulated.empty()) {
        for (const auto* grad_ptr : new_gradients) {
            if (grad_ptr) {
                accumulated.push_back(grad_ptr->clone());
            } else {
                accumulated.push_back(Tensor());
            }
        }
        return;
    }
    
    // Add new gradients to accumulated
    size_t num_grads = std::min(accumulated.size(), new_gradients.size());
    for (size_t i = 0; i < num_grads; ++i) {
        if (!new_gradients[i]) continue;
        
        const auto& new_data = new_gradients[i]->data();
        auto& acc_data = accumulated[i].data();
        
        if (acc_data.size() != new_data.size()) {
            spdlog::error("Gradient size mismatch at index {} (expected {}, got {})", 
                         i, acc_data.size(), new_data.size());
            throw std::runtime_error("Gradient accumulation failed: size mismatch");
        }
        
        for (size_t j = 0; j < acc_data.size(); ++j) {
            acc_data[j] += new_data[j];
        }
    }
}

void GradientUtils::normalize_gradients(
    std::vector<Tensor>& accumulated,
    int num_steps
) {
    if (num_steps <= 1) return;
    
    float scale = 1.0f / static_cast<float>(num_steps);
    
    for (auto& grad : accumulated) {
        auto& data = grad.data();
        for (float& val : data) {
            val *= scale;
        }
    }
}

GradientStats GradientUtils::compute_stats(const std::vector<Tensor*>& gradients) {
    GradientStats stats;
    
    if (gradients.empty()) {
        return stats;
    }
    
    // Compute global norm
    stats.global_norm = compute_global_norm(gradients);
    
    // Compute min, max, and mean
    float sum = 0.0f;
    size_t count = 0;
    stats.max_gradient = -std::numeric_limits<float>::infinity();
    stats.min_gradient = std::numeric_limits<float>::infinity();
    
    for (const auto* grad_ptr : gradients) {
        if (!grad_ptr) continue;
        
        const auto& data = grad_ptr->data();
        for (float val : data) {
            stats.max_gradient = std::max(stats.max_gradient, std::abs(val));
            stats.min_gradient = std::min(stats.min_gradient, val);
            sum += val;
            count++;
        }
    }
    
    if (count > 0) {
        stats.mean_gradient = sum / static_cast<float>(count);
    }
    
    // Check for invalid values
    if (has_invalid_gradients(gradients)) {
        stats.num_overflows++;
    }
    
    return stats;
}

bool GradientUtils::has_invalid_gradients(const std::vector<Tensor*>& gradients) {
    for (const auto* grad_ptr : gradients) {
        if (!grad_ptr) continue;
        
        const auto& data = grad_ptr->data();
        for (float val : data) {
            if (std::isnan(val) || std::isinf(val)) {
                return true;
            }
        }
    }
    return false;
}

void GradientUtils::zero_gradients(std::vector<Tensor*>& gradients) {
    for (auto* grad_ptr : gradients) {
        if (grad_ptr) {
            grad_ptr->zero();
        }
    }
}

// ============================================================================
// GradientAccumulator Implementation
// ============================================================================

GradientAccumulator::GradientAccumulator(const GradientAccumulationConfig& config)
    : config_(config), current_step_(0), initialized_(false) {
    
    spdlog::info("GradientAccumulator initialized:");
    spdlog::info("  Accumulation steps: {}", config_.accumulation_steps);
    spdlog::info("  Normalize: {}", config_.normalize);
}

void GradientAccumulator::accumulate(const std::vector<Tensor*>& gradients) {
    // Initialize on first accumulation
    if (!initialized_) {
        accumulated_gradients_.clear();
        for (const auto* grad_ptr : gradients) {
            if (grad_ptr) {
                accumulated_gradients_.push_back(Tensor(grad_ptr->shape(), 0.0f));
            } else {
                accumulated_gradients_.push_back(Tensor());
            }
        }
        initialized_ = true;
    }
    
    // Accumulate gradients
    GradientUtils::accumulate_gradients(accumulated_gradients_, gradients);
    current_step_++;
}

bool GradientAccumulator::should_step() const {
    return current_step_ >= config_.accumulation_steps;
}

std::vector<Tensor*> GradientAccumulator::get_accumulated_gradients() {
    // Normalize if configured
    if (config_.normalize && current_step_ > 0) {
        GradientUtils::normalize_gradients(accumulated_gradients_, current_step_);
    }
    
    // Return pointers to accumulated gradients
    std::vector<Tensor*> result;
    result.reserve(accumulated_gradients_.size());
    for (auto& grad : accumulated_gradients_) {
        result.push_back(&grad);
    }
    
    return result;
}

void GradientAccumulator::reset() {
    // Zero out accumulated gradients
    for (auto& grad : accumulated_gradients_) {
        grad.zero();
    }
    current_step_ = 0;
}

} // namespace lora
} // namespace llm
} // namespace themis
