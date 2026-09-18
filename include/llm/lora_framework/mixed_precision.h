/**
 * @file mixed_precision.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

enum class PrecisionMode {
    FP32,   // Full precision (32-bit float)
    FP16,   // Half precision (16-bit float)
    BF16,   // Brain Float 16
    AMP     // Automatic Mixed Precision
};

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

class MixedPrecisionTrainer {
public:
    explicit MixedPrecisionTrainer(const MixedPrecisionConfig& config = MixedPrecisionConfig{});
    ~MixedPrecisionTrainer() = default;
    
    /**
     * @brief To lower precision.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    Tensor to_lower_precision(const Tensor& input) const;
    
    /**
     * @brief To fp32.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    Tensor to_fp32(const Tensor& input) const;
    
    /**
     * @brief Scale loss.
     * @param[in] loss Input parameter.
     * @return Return value.
     */
    float scale_loss(float loss);
    
    /**
     * @brief Unscale gradients.
     * @param[in,out] gradients Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool unscale_gradients(std::vector<Tensor*>& gradients);
    
    /**
     * @brief Has overflow.
     * @param[in] gradients Input parameter.
     * @return True when the operation succeeds.
     */
    static bool has_overflow(const std::vector<Tensor*>& gradients);
    
    /**
     * @brief Update loss scale.
     * @param[in] had_overflow Input parameter.
     */
    void update_loss_scale(bool had_overflow);
    
    float get_loss_scale() const { return current_loss_scale_; }
    
    PrecisionMode get_precision_mode() const { return config_.mode; }
    
    bool is_enabled() const { return config_.mode != PrecisionMode::FP32; }
    
    /**
     * @brief Get stats.
     * @return Return value.
     */
    json get_stats() const;
    
    /**
     * @brief Reset stats.
     */
    void reset_stats();

private:
    MixedPrecisionConfig config_;
    float current_loss_scale_ = 0.0f;
    int steps_since_overflow_ = 0;
    int total_overflows_ = 0;
    int total_steps_ = 0;
    
    /**
     * @brief Helper for FP16/BF16 conversion (simplified for CPU)
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static float fp32_to_fp16(float value);
    /**
     * @brief Fp16 to fp32.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static float fp16_to_fp32(float value);
};

class MixedPrecisionScope {
public:
    /**
     * @brief Mixed Precision Scope.
     * @param[in,out] trainer Input/output parameter.
     * @return Return value.
     */
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

