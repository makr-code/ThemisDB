/**
 * @file mixed_precision.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=3, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/mixed_precision.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <cstring>
#include <cstdint>
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
        for (size_t i = 0; i <static_cast<int>(output.size()); ++i) {
            output[i] = fp16_to_fp32(fp32_to_fp16(output[i]));
        }
    } else if (config_.mode == PrecisionMode::BF16) {
        // BF16 has same exponent range as FP32, just reduced mantissa
        for (size_t i = 0; i <static_cast<int>(output.size()); ++i) {
            // Truncate mantissa bits (simplified)
            float val = output[i];
            uint32_t bits = {};
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

float MixedPrecisionTrainer::scale_loss([[maybe_unused]] float loss) {
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
        if (!grad_ptr) {
          continue;
        }
        
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

void MixedPrecisionTrainer::update_loss_scale([[maybe_unused]] bool had_overflow) {
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

// File-local helper: decode a raw FP16 bit-pattern to float32.
// Defined before fp32_to_fp16 so it can be called from there.
static float fp16_to_fp32_bits([[maybe_unused]] uint16_t f16) {
    const uint32_t sign   = static_cast<uint32_t>((f16 >> 15) & 0x1u);
    const uint32_t exp16  = static_cast<uint32_t>((f16 >> 10) & 0x1Fu);
    const uint32_t mant16 = static_cast<uint32_t>( f16        & 0x3FFu);

    uint32_t exp32 = {};
    uint32_t mant32 = 0;

    if (exp16 == 0x1Fu) {
        exp32  = 0xFFu;
        mant32 = mant16 ? (mant16 << 13) : 0u;
    } else if (exp16 == 0u) {
        if (mant16 == 0u) {
            exp32  = 0u;
            mant32 = 0u;
        } else {
            // Subnormal FP16 → normalise into FP32
            int e = -1;
            uint32_t m = mant16 << 1;
            while (!(m & 0x400u)) { m <<= 1; ++e; }
            exp32  = static_cast<uint32_t>(127 - 15 - e);
            mant32 = (m & 0x3FFu) << 13;
        }
    } else {
        exp32  = static_cast<uint32_t>(exp16 + 127 - 15);
        mant32 = mant16 << 13;
    }

    uint32_t f32 = (sign << 31) | (exp32 << 23) | mant32;
    float out = 0;
    std::memcpy(&out, &f32, sizeof(out));
    return out;
}

// CPU-based IEEE 754 FP32↔FP16 conversion using bit manipulation.
// FP32: 1 sign + 8 exponent + 23 mantissa bits (bias 127)
// FP16: 1 sign + 5 exponent + 10 mantissa bits (bias 15)
float MixedPrecisionTrainer::fp32_to_fp16([[maybe_unused]] float value) {
    // Bit-cast float to uint32 without UB
    uint32_t f32 = {};
    std::memcpy(&f32, &value, sizeof(f32));

    const uint32_t sign     = (f32 >> 31) & 0x1u;
    const uint32_t exp32    = (f32 >> 23) & 0xFFu;
    const uint32_t mant32   =  f32        & 0x7FFFFFu;

    uint32_t exp16 = {};
    uint32_t mant16 = 0;

    if (exp32 == 0xFFu) {
        // Inf or NaN
        exp16  = 0x1Fu;
        mant16 = (mant32 != 0) ? 0x200u : 0u; // preserve NaN vs Inf
    } else if (exp32 == 0u) {
        // Subnormal FP32 → FP16 zero (too small for FP16 subnormals)
        exp16  = 0u;
        mant16 = 0u;
    } else {
        int32_t exp_shifted = static_cast<int32_t>(exp32) - 127 + 15;
        if (exp_shifted >= 31) {
            // Overflow → FP16 infinity
            exp16  = 0x1Fu;
            mant16 = 0u;
        } else if (exp_shifted <= 0) {
            // Underflow → FP16 subnormal or zero
            exp16  = 0u;
            // Shift mantissa (implicit leading 1 included)
            uint32_t mant_with_implicit = (mant32 | 0x800000u) >> (1 - exp_shifted);
            mant16 = mant_with_implicit >> 13; // truncate to 10 bits
        } else {
            exp16  = static_cast<uint32_t>(exp_shifted);
            // Round-to-nearest: look at bit 12 of the FP32 mantissa
            uint32_t round_bit = (mant32 >> 12) & 0x1u;
            mant16 = (mant32 >> 13) + round_bit;
            if (mant16 > 0x3FFu) {
                // Mantissa overflow → increment exponent
                mant16 = 0u;
                ++exp16;
                if (exp16 >= 0x1Fu) { exp16 = 0x1Fu; mant16 = 0u; } // clamp to Inf
            }
        }
    }

    // Pack into 16-bit pattern, then reconstruct a float for storage.
    // We store the FP16 bit-pattern in the lower 16 bits of a float32.
    uint16_t f16 = static_cast<uint16_t>((sign << 15) | (exp16 << 10) | mant16);
    // Decode the FP16 back to float so the caller works with ordinary floats.
    return fp16_to_fp32_bits(f16);
}

float MixedPrecisionTrainer::fp16_to_fp32([[maybe_unused]] float value) {
    // value stores the FP16 bit-pattern that fp32_to_fp16() encoded.
    // Re-interpret the lower 16 bits as a raw FP16 word.
    uint32_t f32_bits = {};
    std::memcpy(&f32_bits, &value, sizeof(f32_bits));
    uint16_t f16 = static_cast<uint16_t>(f32_bits & 0xFFFFu);
    return fp16_to_fp32_bits(f16);
}

} // namespace lora
} // namespace llm
} // namespace themis

