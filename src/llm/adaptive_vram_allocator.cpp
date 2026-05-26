/*
 * ThemisDB | File: adaptive_vram_allocator.cpp | Version: 0.0.47 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 225
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=61 | delta=58 | status=divergent
 * External Severity (v3): C=13, H=21, M=27
 * PR: #4333 [LORA-123] Implement LoRA adapter hot-loading at inference time (2026-03-19T12:22:45Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
bool checked_mul(size_t a, size_t b, size_t& out) {
    if (a != 0 && b > std::numeric_limits<size_t>::max() / a) {
        return false;
    }
    out = a * b;
    return true;
}
} // namespace

// Private implementation
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
        config.max_seq_length == 0) {
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
    plan.kv_cache_dynamic = static_cast<size_t>(
        plan.kv_cache_static * config.kv_cache_growth_factor
    );
    
    // 5. Estimate activation memory
    plan.activations = estimateActivationMemory(model, config.batch_size, config.max_seq_length);
    
    // 6. Calculate overhead (5% for system, fragmentation, etc.)
    size_t subtotal = plan.model_weights + plan.kv_cache_static + 
                      plan.kv_cache_dynamic + plan.activations;
    plan.overhead = subtotal / 20;  // 5%
    
    // 7. Calculate total
    plan.total = subtotal + plan.overhead;
    
    // 8. Calculate expected fragmentation
    // PagedAttention reduces fragmentation to ~3-5%
    if (config.enable_prefix_caching) {
        plan.expected_fragmentation = 0.03f;  // 3%
    } else {
        plan.expected_fragmentation = 0.15f;  // 15%
    }
    
    // 9. Calculate max tokens that can be cached
    size_t available_for_kv = hw.available_vram_bytes > plan.model_weights + plan.activations + plan.overhead
        ? hw.available_vram_bytes - plan.model_weights - plan.activations - plan.overhead
        : 0;
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
        ss << "KV Cache: " << ((plan.kv_cache_static + plan.kv_cache_dynamic) / (1024.0 * 1024 * 1024)) << " GB, ";
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
    return 2 * model.num_layers * model.num_kv_heads * model.head_dim * model.precision_bytes;
}

size_t AdaptiveVRAMAllocator::calculateModelSize(size_t num_parameters, float precision_bytes) {
    return static_cast<size_t>(num_parameters * precision_bytes);
}

size_t AdaptiveVRAMAllocator::estimateActivationMemory(
    const ModelConfig& model,
    size_t batch_size,
    size_t seq_length
) {
    // Estimate based on typical transformer architecture
    // Activations scale with: batch_size × seq_length × hidden_dim × num_layers
    // Rough estimate: ~4-8 bytes per activation depending on precision
    
    size_t activation_elements = batch_size * seq_length * model.hidden_dim;
    size_t bytes_per_activation = model.precision_bytes * 2;  // Forward + backward
    
    // Only a subset of layers have activations stored at once
    // Typically ~20-30% of layers depending on checkpointing
    double checkpoint_ratio = 0.25;
    
    return static_cast<size_t>(
        activation_elements * bytes_per_activation * model.num_layers * checkpoint_ratio
    );
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

    // Build a local draft config with the resolved precision so that the
    // existing helpers (calculateModelSize, estimateActivationMemory) work
    // without modification.
    ModelConfig resolved_draft = draft_config;
    resolved_draft.precision_bytes =
        (draft_config.precision_bytes > 0) ? draft_config.precision_bytes : 1;
    // For calculateModelSize we pass the float precision directly.

    // --- Draft model weight footprint -----------------------------------------
    const size_t draft_weights =
        calculateModelSize(draft_config.num_parameters, draft_precision);

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
    plan.model_weights        += draft_weights;

    // Re-compute total and fits_in_vram to account for the draft model.
    plan.total += draft_weights;
    plan.fits_in_vram = (plan.total <= hw.available_vram_bytes);

    // Rebuild the recommendation string with dual-model context.
    std::stringstream ss;
    if (plan.fits_in_vram) {
        ss << "✓ Dual-model allocation fits in available VRAM. ";
        ss << "Target: " << (target_plan.model_weights / (1024.0 * 1024 * 1024)) << " GB, ";
        ss << "Draft (INT" << (draft_precision < 1.0f ? 4 : static_cast<int>(draft_precision * 8))
           << "): " << (draft_weights / (1024.0 * 1024 * 1024)) << " GB, ";
        ss << "KV Cache: "
           << ((plan.kv_cache_static + plan.kv_cache_dynamic) / (1024.0 * 1024 * 1024)) << " GB, ";
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
