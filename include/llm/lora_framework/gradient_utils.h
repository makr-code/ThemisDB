/**
 * @file gradient_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class ClippingMethod {
    NONE,           // No clipping
    BY_NORM,        // Clip by global norm
    BY_VALUE,       // Clip by value
    BY_GLOBAL_NORM  // Clip by global norm (all gradients)
};

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
    
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     * @details Calls: contains().
     */
    static GradientClippingConfig fromJSON(const json& j) {
        GradientClippingConfig config = {};
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

struct GradientAccumulationConfig {
    int accumulation_steps = 1;     // Number of steps to accumulate
    bool normalize = true;          // Normalize by accumulation steps
    
    json toJSON() const {
        return json{
            {"accumulation_steps", accumulation_steps},
            {"normalize", normalize}
        };
    }
    
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     * @details Calls: contains().
     */
    static GradientAccumulationConfig fromJSON(const json& j) {
        GradientAccumulationConfig config = {};
        if (j.contains("accumulation_steps")) {
          config.accumulation_steps = j["accumulation_steps"];
        }
        if (j.contains("normalize")) {
          config.normalize = j["normalize"];
        }
        return config;
    }
};

struct GradientStats {
    /**
     * @brief Gradient Stats.
     * @return Return value.
     */
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

class GradientUtils {
public:
    /**
     * @brief Compute global norm.
     * @param[in] gradients Input parameter.
     * @return Return value.
     */
    static float compute_global_norm(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Clip by norm.
     * @param[in,out] gradients Input/output parameter.
     * @param[in] max_norm Input parameter.
     * @return True when the operation succeeds.
     */
    static bool clip_by_norm(std::vector<Tensor*>& gradients, float max_norm);
    
    /**
     * @brief Clip by value.
     * @param[in,out] gradients Input/output parameter.
     * @param[in] clip_value Input parameter.
     * @return True when the operation succeeds.
     */
    static bool clip_by_value(std::vector<Tensor*>& gradients, float clip_value);
    
    /**
     * @brief Apply clipping.
     * @param[in,out] gradients Input/output parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static GradientStats apply_clipping(
        std::vector<Tensor*>& gradients, 
        const GradientClippingConfig& config
    );
    
    /**
     * @brief Accumulate gradients.
     * @param[in,out] accumulated Input/output parameter.
     * @param[in] new_gradients Input parameter.
     */
    static void accumulate_gradients(
        std::vector<Tensor>& accumulated,
        const std::vector<Tensor*>& new_gradients
    );
    
    /**
     * @brief Normalize gradients.
     * @param[in,out] accumulated Input/output parameter.
     * @param[in] num_steps Input parameter.
     */
    static void normalize_gradients(
        std::vector<Tensor>& accumulated,
        int num_steps
    );
    
    /**
     * @brief Compute stats.
     * @param[in] gradients Input parameter.
     * @return Return value.
     */
    static GradientStats compute_stats(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Has invalid gradients.
     * @param[in] gradients Input parameter.
     * @return True when the operation succeeds.
     */
    static bool has_invalid_gradients(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Zero gradients.
     * @param[in,out] gradients Input/output parameter.
     */
    static void zero_gradients(std::vector<Tensor*>& gradients);
};

class GradientAccumulator {
public:
    /**
     * @brief Gradient Accumulator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GradientAccumulator(const GradientAccumulationConfig& config);
    ~GradientAccumulator() = default;
    
    /**
     * @brief Accumulate.
     * @param[in] gradients Input parameter.
     */
    void accumulate(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Should step.
     * @return True when the operation succeeds.
     */
    bool should_step() const;
    
    /**
     * @brief Get accumulated gradients.
     * @return Return value.
     */
    std::vector<Tensor*> get_accumulated_gradients();
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
    int current_step() const { return current_step_; }
    
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

