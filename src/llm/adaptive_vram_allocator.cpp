/**
 * @file adaptive_vram_allocator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=0, L=0
 * @note Status: Production Ready - RAII and exception safety enhancements complete
 * @note This block is auto-generated and will be overwritten.
 * @note Improvements: All helper functions marked noexcept, RAII pattern enforced,
 *       arithmetic overflow protection with checked operations.
 */

#include "llm/adaptive_vram_allocator.h"
#include "llm/active_vram_allocator.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace themis {
namespace llm {

namespace {
/**
 * @brief Safe multiplication with overflow checking
 * @return true if result fits in size_t, false if overflow would occur
 */
bool checked_mul(size_t a, size_t b, size_t& out) noexcept {
    if (a != 0 && b > std::numeric_limits<size_t>::max() / a) {
        return false;
    }
    out = a * b;
    return true;
}

/**
 * @brief Safe addition with overflow checking
 * @return true if result fits in size_t, false if overflow would occur
 */
bool checked_add(size_t a, size_t b, size_t& out) noexcept {
    if (b > std::numeric_limits<size_t>::max() - a) {
        return false;
    }
    out = a + b;
    return true;
}

/**
 * @brief Safe scaling operation with bounds checking
 * @return true if scaled value fits in size_t, false otherwise
 */
bool checked_scale(size_t value, double factor, size_t& out) noexcept {
    if (!std::isfinite(factor) || factor < 0.0) {
        return false;
    }
    const long double scaled = static_cast<long double>(value) * static_cast<long double>(factor);
    if (scaled > static_cast<long double>(std::numeric_limits<size_t>::max())) {
        return false;
    }
    out = static_cast<size_t>(scaled);
    return true;
}
} // namespace

// Private implementation
/** @brief Private implementation. */
class AdaptiveVRAMAllocator::Impl {
public:
    Impl() : active_allocator_(ActiveVRAMAllocator::Config{}) {}
    ~Impl() = default;

    ActiveVRAMAllocator active_allocator_;
};

AdaptiveVRAMAllocator::AdaptiveVRAMAllocator() 
    : impl_(std::make_unique<Impl>()) {}

AdaptiveVRAMAllocator::~AdaptiveVRAMAllocator() = default;

AdaptiveVRAMAllocator::AllocationPlan AdaptiveVRAMAllocator::calculateOptimalAllocation(
    const ModelConfig& model,
    const HardwareInfo& hw,
    const InferenceConfig& config
) {
    AllocationPlan plan{};

    if (model.precision_bytes <= 0 || model.num_parameters == 0 || model.num_layers == 0 ||
        model.num_kv_heads == 0 || model.head_dim == 0 || config.batch_size == 0 ||
        config.max_seq_length == 0 || !std::isfinite(config.kv_cache_growth_factor) ||
        config.kv_cache_growth_factor < 0.0f) {
        plan.fits_in_vram = false;
        plan.recommendation = "Invalid allocation parameters.";
        return plan;
    }
    
    // 1. Calculate model weights size
    if (!checked_mul(model.num_parameters, static_cast<size_t>(model.precision_bytes), plan.model_weights)) {
        plan.fits_in_vram = false;
        plan.recommendation = "Model weight size overflow.";
        return plan;
    }
    
    // 2. Calculate KV cache size per token
    // Formula: 2 × num_layers × num_kv_heads × head_dim × precision_bytes
    size_t kv_size = 0;
    if (!checked_mul(2, model.num_layers, kv_size) ||
        !checked_mul(kv_size, model.num_kv_heads, kv_size) ||
        !checked_mul(kv_size, model.head_dim, kv_size) ||
        !checked_mul(kv_size, static_cast<size_t>(model.precision_bytes), kv_size)) {
        plan.fits_in_vram = false;
        plan.recommendation = "KV cache size overflow.";
        return plan;
    }
    plan.kv_size_per_token = kv_size;
    
    // 3. Calculate static KV cache allocation
    // Allocate for batch_size × max_seq_length
    size_t total_tokens = 0;
    if (!checked_mul(config.batch_size, config.max_seq_length, total_tokens) ||
        !checked_mul(plan.kv_size_per_token, total_tokens, plan.kv_cache_static)) {
        plan.fits_in_vram = false;
        plan.recommendation = "Static KV allocation overflow.";
        return plan;
    }
    
    // 4. Calculate dynamic KV cache (for growth)
    if (!checked_scale(plan.kv_cache_static, config.kv_cache_growth_factor, plan.kv_cache_dynamic)) {
        plan.fits_in_vram = false;
        plan.recommendation = "Dynamic KV allocation overflow.";
        return plan;
    }
    
    // 5. Estimate activation memory
    plan.activations = estimateActivationMemory(model, config.batch_size, config.max_seq_length);
    
    // 6. Calculate overhead (5% for system, fragmentation, etc.)
    size_t subtotal = 0;
    if (!checked_add(plan.model_weights, plan.kv_cache_static, subtotal) ||
        !checked_add(subtotal, plan.kv_cache_dynamic, subtotal) ||
        !checked_add(subtotal, plan.activations, subtotal)) {
        plan.fits_in_vram = false;
        plan.recommendation = "Allocation subtotal overflow.";
        return plan;
    }
    plan.overhead = subtotal / 20;  // 5%
    
    // 7. Calculate total
    if (!checked_add(subtotal, plan.overhead, plan.total)) {
        plan.fits_in_vram = false;
        plan.recommendation = "Allocation total overflow.";
        return plan;
    }
    
    // 8. Calculate expected fragmentation
    // PagedAttention reduces fragmentation to ~3-5%
    if (config.enable_prefix_caching) {
        plan.expected_fragmentation = 0.03f;  // 3%
    } else {
        plan.expected_fragmentation = 0.15f;  // 15%
    }
    
    // 9. Calculate max tokens that can be cached
    size_t required_before_kv = 0;
    size_t available_for_kv = 0;
    if (checked_add(plan.model_weights, plan.activations, required_before_kv) &&
        checked_add(required_before_kv, plan.overhead, required_before_kv) &&
        hw.available_vram_bytes > required_before_kv) {
        available_for_kv = hw.available_vram_bytes - required_before_kv;
    }
    plan.max_tokens_cached = plan.kv_size_per_token > 0 
        ? available_for_kv / plan.kv_size_per_token 
        : 0;
    
    // 10. Check if plan fits in VRAM
    plan.fits_in_vram = plan.total <= hw.available_vram_bytes;
    
    // 11. Generate recommendation
    std::stringstream ss;
    if (plan.fits_in_vram) {
        ss << "✓ Allocation fits in available VRAM. ";
        ss << "Model: " << (plan.model_weights / (1024.0 * 1024 * 1024)) << " GB, ";
        size_t kv_cache_total = 0;
        if (!checked_add(plan.kv_cache_static, plan.kv_cache_dynamic, kv_cache_total)) {
            kv_cache_total = std::numeric_limits<size_t>::max();
        }
        ss << "KV Cache: " << (kv_cache_total / (1024.0 * 1024 * 1024)) << " GB, ";
        ss << "Total: " << (plan.total / (1024.0 * 1024 * 1024)) << " GB";
    } else {
        ss << "✗ Allocation exceeds available VRAM. ";
        ss << "Need: " << (plan.total / (1024.0 * 1024 * 1024)) << " GB, ";
        ss << "Available: " << (hw.available_vram_bytes / (1024.0 * 1024 * 1024)) << " GB. ";
        
        // Suggest alternatives
        if (model.precision_bytes >= 2) {
            ss << "Consider: (1) Use INT8 quantization to reduce model size by 50-75%, ";
            ss << "(2) Reduce batch size or sequence length, ";
            ss << "(3) Use multiple GPUs with tensor parallelism.";
        } else {
            ss << "Consider: (1) Reduce batch size or sequence length, ";
            ss << "(2) Use multiple GPUs with tensor parallelism.";
        }
    }
    
    plan.recommendation = ss.str();
    
    return plan;
}

bool AdaptiveVRAMAllocator::allocateWithFragmentation(size_t bytes, void** ptr) {
    if (!impl_ || ptr == nullptr || bytes == 0) {
        return false;
    }
    return impl_->active_allocator_.allocateWithFragmentation(bytes, ptr);
}

bool AdaptiveVRAMAllocator::handleOutOfMemory() {
    if (!impl_) {
        return false;
    }
    return impl_->active_allocator_.handleOutOfMemory();
}

size_t AdaptiveVRAMAllocator::calculateKVCacheSizePerToken(const ModelConfig& model) {
    // Formula: 2 × num_layers × num_kv_heads × head_dim × precision_bytes
    // The "2" accounts for both Key and Value caches
    if (model.precision_bytes <= 0 || model.num_layers == 0 || model.num_kv_heads == 0 ||
        model.head_dim == 0) {
        return 0;
    }
    size_t kv_size = 0;
    if (!checked_mul(2, model.num_layers, kv_size) ||
        !checked_mul(kv_size, model.num_kv_heads, kv_size) ||
        !checked_mul(kv_size, model.head_dim, kv_size) ||
        !checked_mul(kv_size, static_cast<size_t>(model.precision_bytes), kv_size)) {
        return 0;
    }
    return kv_size;
}

size_t AdaptiveVRAMAllocator::calculateModelSize(size_t num_parameters, float precision_bytes) {
    if (!std::isfinite(precision_bytes) || precision_bytes <= 0.0f) {
        return 0;
    }
    const long double size =
        static_cast<long double>(num_parameters) * static_cast<long double>(precision_bytes);
    if (size > static_cast<long double>(std::numeric_limits<size_t>::max())) {
        return 0;
    }
    return static_cast<size_t>(size);
}

size_t AdaptiveVRAMAllocator::estimateActivationMemory(
    const ModelConfig& model,
    size_t batch_size,
    size_t seq_length
) {
    // Estimate based on typical transformer architecture
    // Activations scale with: batch_size × seq_length × hidden_dim × num_layers
    // Rough estimate: ~4-8 bytes per activation depending on precision
    
    if (model.precision_bytes <= 0 || batch_size == 0 || seq_length == 0 ||
        model.hidden_dim == 0 || model.num_layers == 0) {
        return 0;
    }

    size_t activation_elements = 0;
    if (!checked_mul(batch_size, seq_length, activation_elements) ||
        !checked_mul(activation_elements, model.hidden_dim, activation_elements)) {
        return 0;
    }
    size_t bytes_per_activation = 0;  // Forward + backward
    if (!checked_mul(static_cast<size_t>(model.precision_bytes), 2, bytes_per_activation)) {
        return 0;
    }
    
    // Only a subset of layers have activations stored at once
    // Typically ~20-30% of layers depending on checkpointing
    double checkpoint_ratio = 0.25;
    
    size_t total_activation_bytes = 0;
    if (!checked_mul(activation_elements, bytes_per_activation, total_activation_bytes) ||
        !checked_mul(total_activation_bytes, model.num_layers, total_activation_bytes)) {
        return 0;
    }
    size_t estimated = 0;
    if (!checked_scale(total_activation_bytes, checkpoint_ratio, estimated)) {
        return 0;
    }
    return estimated;
}

AdaptiveVRAMAllocator::DualModelAllocationPlan
AdaptiveVRAMAllocator::calculateDualModelAllocation(
    const ModelConfig&    target_config,
    const ModelConfig&    draft_config,
    const HardwareInfo&   hw,
    const InferenceConfig& config
) {
    // INT4 precision: 0.5 bytes per parameter.
    // A precision_bytes value of 0 in the draft config signals "use INT4 default".
    constexpr float kInt4Bytes = 0.5f;
    const float draft_precision =
        (draft_config.precision_bytes > 0)
            ? static_cast<float>(draft_config.precision_bytes)
            : kInt4Bytes;

    // --- Draft model weight footprint -----------------------------------------
    const size_t draft_weights =
        calculateModelSize(draft_config.num_parameters, draft_precision);
    if (draft_config.num_parameters > 0 && draft_weights == 0) {
        DualModelAllocationPlan invalid_plan{};
        invalid_plan.fits_in_vram = false;
        invalid_plan.recommendation = "Invalid draft model allocation parameters.";
        return invalid_plan;
    }

    // --- Base allocation plan for the target model ----------------------------
    AllocationPlan target_plan =
        calculateOptimalAllocation(target_config, hw, config);

    // --- Build the combined dual-model plan -----------------------------------
    DualModelAllocationPlan plan;

    // Copy target-model fields first.
    static_cast<AllocationPlan&>(plan) = target_plan;

    // Add draft model weight footprint on top of the target model weights.
    plan.draft_model_weights  = draft_weights;
    plan.draft_precision_bytes =
        (draft_config.precision_bytes > 0) ? draft_config.precision_bytes : 0;
    if (!checked_add(plan.model_weights, draft_weights, plan.model_weights) ||
        !checked_add(plan.total, draft_weights, plan.total)) {
        plan.fits_in_vram = false;
        plan.recommendation = "Dual-model allocation overflow.";
        return plan;
    }

    // Re-compute total and fits_in_vram to account for the draft model.
    plan.fits_in_vram = (plan.total <= hw.available_vram_bytes);

    // Rebuild the recommendation string with dual-model context.
    std::stringstream ss;
    if (plan.fits_in_vram) {
        ss << "✓ Dual-model allocation fits in available VRAM. ";
        ss << "Target: " << (target_plan.model_weights / (1024.0 * 1024 * 1024)) << " GB, ";
        ss << "Draft (INT" << (draft_precision < 1.0f ? 4 : static_cast<int>(draft_precision * 8))
           << "): " << (draft_weights / (1024.0 * 1024 * 1024)) << " GB, ";
        size_t kv_cache_total = 0;
        if (!checked_add(plan.kv_cache_static, plan.kv_cache_dynamic, kv_cache_total)) {
            kv_cache_total = std::numeric_limits<size_t>::max();
        }
        ss << "KV Cache: "
           << (kv_cache_total / (1024.0 * 1024 * 1024)) << " GB, ";
        ss << "Total: " << (plan.total / (1024.0 * 1024 * 1024)) << " GB";
    } else {
        ss << "✗ Dual-model allocation exceeds available VRAM. ";
        ss << "Need: " << (plan.total / (1024.0 * 1024 * 1024)) << " GB, ";
        ss << "Available: " << (hw.available_vram_bytes / (1024.0 * 1024 * 1024)) << " GB. ";
        ss << "Consider: (1) Use a smaller/more-quantized draft model, ";
        ss << "(2) Disable speculative decoding, ";
        ss << "(3) Reduce batch size or sequence length.";
    }
    plan.recommendation = ss.str();

    return plan;
}

} // namespace llm
} // namespace themis
