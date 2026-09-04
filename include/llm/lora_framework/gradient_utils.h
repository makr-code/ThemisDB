/**
 * @file gradient_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_layers.h"
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Gradient clipping method
 */
enum class ClippingMethod {
    NONE,           // No clipping
    BY_NORM,        // Clip by global norm
    BY_VALUE,       // Clip by value
    BY_GLOBAL_NORM  // Clip by global norm (all gradients)
};

/**
 * @brief Configuration for gradient clipping
 */
struct GradientClippingConfig {
    ClippingMethod method = ClippingMethod::NONE;
    float max_norm = 1.0f;          // Maximum gradient norm
    float clip_value = 10.0f;       // Maximum absolute gradient value
    bool adaptive = false;          // Adaptive clipping based on gradient history
    
    json toJSON() const {
        return json{
            {"method", static_cast<int>(method)},
            {"max_norm", max_norm},
            {"clip_value", clip_value},
            {"adaptive", adaptive}
        };
    }
    
    static GradientClippingConfig fromJSON(const json& j) {
        GradientClippingConfig config;
        if (j.contains("method")) {
          config.method = static_cast<ClippingMethod>(j["method"].get<int>());
        }
        if (j.contains("max_norm")) {
          config.max_norm = j["max_norm"];
        }
        if (j.contains("clip_value")) {
          config.clip_value = j["clip_value"];
        }
        if (j.contains("adaptive")) {
          config.adaptive = j["adaptive"];
        }
        return config;
    }
};

/**
 * @brief Gradient accumulation configuration
 */
struct GradientAccumulationConfig {
    int accumulation_steps = 1;     // Number of steps to accumulate
    bool normalize = true;          // Normalize by accumulation steps
    
    json toJSON() const {
        return json{
            {"accumulation_steps", accumulation_steps},
            {"normalize", normalize}
        };
    }
    
    static GradientAccumulationConfig fromJSON(const json& j) {
        GradientAccumulationConfig config;
        if (j.contains("accumulation_steps")) {
          config.accumulation_steps = j["accumulation_steps"];
        }
        if (j.contains("normalize")) {
          config.normalize = j["normalize"];
        }
        return config;
    }
};

/**
 * @brief Gradient statistics
 */
struct GradientStats {
    virtual ~GradientStats() = default;
    float global_norm = 0.0f;       // L2 norm of all gradients
    float max_gradient = 0.0f;      // Maximum absolute gradient value
    float min_gradient = 0.0f;      // Minimum gradient value
    float mean_gradient = 0.0f;     // Mean gradient value
    int num_clipped = 0;            // Number of times clipping was applied
    int num_overflows = 0;          // Number of overflow/underflow detections
    
    json toJSON() const {
        return json{
            {"global_norm", global_norm},
            {"max_gradient", max_gradient},
            {"min_gradient", min_gradient},
            {"mean_gradient", mean_gradient},
            {"num_clipped", num_clipped},
            {"num_overflows", num_overflows}
        };
    }
};

/**
 * @brief Gradient utilities for training
 * 
 * Features:
 * - Gradient clipping (by norm, by value)
 * - Gradient accumulation
 * - Gradient statistics
 * - Overflow/underflow detection
 */
class GradientUtils {
public:
    /**
     * @brief Compute global L2 norm of gradients
     * @param gradients Vector of gradient tensors
     * @return Global gradient norm
     */
    static float compute_global_norm(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Clip gradients by global norm
     * @param gradients Vector of gradient tensors to clip
     * @param max_norm Maximum allowed norm
     * @return True if clipping was applied
     */
    static bool clip_by_norm(std::vector<Tensor*>& gradients, float max_norm);
    
    /**
     * @brief Clip gradients by value
     * @param gradients Vector of gradient tensors to clip
     * @param clip_value Maximum absolute value
     * @return True if clipping was applied
     */
    static bool clip_by_value(std::vector<Tensor*>& gradients, float clip_value);
    
    /**
     * @brief Apply gradient clipping based on configuration
     * @param gradients Vector of gradient tensors
     * @param config Clipping configuration
     * @return Gradient statistics
     */
    static GradientStats apply_clipping(
        std::vector<Tensor*>& gradients, 
        const GradientClippingConfig& config
    );
    
    /**
     * @brief Accumulate gradients
     * @param accumulated Accumulated gradients (in/out)
     * @param new_gradients New gradients to add
     */
    static void accumulate_gradients(
        std::vector<Tensor>& accumulated,
        const std::vector<Tensor*>& new_gradients
    );
    
    /**
     * @brief Normalize accumulated gradients
     * @param accumulated Accumulated gradients to normalize
     * @param num_steps Number of accumulation steps
     */
    static void normalize_gradients(
        std::vector<Tensor>& accumulated,
        int num_steps
    );
    
    /**
     * @brief Compute gradient statistics
     * @param gradients Vector of gradient tensors
     * @return Gradient statistics
     */
    static GradientStats compute_stats(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Check for NaN or Inf in gradients
     * @param gradients Vector of gradient tensors
     * @return True if overflow/underflow detected
     */
    static bool has_invalid_gradients(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Zero out all gradients
     * @param gradients Vector of gradient tensors
     */
    static void zero_gradients(std::vector<Tensor*>& gradients);
};

/**
 * @brief Gradient accumulator for multi-step training
 * 
 * Enables training with larger effective batch sizes by accumulating
 * gradients over multiple forward/backward passes before optimizer step.
 */
class GradientAccumulator {
public:
    explicit GradientAccumulator(const GradientAccumulationConfig& config);
    ~GradientAccumulator() = default;
    
    /**
     * @brief Add gradients to accumulator
     * @param gradients New gradients to add
     */
    void accumulate(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Check if ready for optimizer step
     * @return True if accumulated enough steps
     */
    bool should_step() const;
    
    /**
     * @brief Get accumulated gradients (after normalization)
     * @return Vector of accumulated gradients
     */
    std::vector<Tensor*> get_accumulated_gradients();
    
    /**
     * @brief Reset accumulator
     */
    void reset();
    
    /**
     * @brief Get current step count
     * @return Current accumulation step
     */
    int current_step() const { return current_step_; }
    
    /**
     * @brief Get configuration
     * @return Accumulation configuration
     */
    GradientAccumulationConfig config() const { return config_; }

private:
    GradientAccumulationConfig config_;
    std::vector<Tensor> accumulated_gradients_;
    int current_step_ = 0;
    bool initialized_ = false;
};

} // namespace lora
} // namespace llm
} // namespace themis

