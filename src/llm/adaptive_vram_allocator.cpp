/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_vram_allocator.cpp                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     240                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fe135d5215  2026-04-13  feat(llm): Speculative Decoding for Latency Reduction — v... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/adaptive_vram_allocator.h"
#include "llm/active_vram_allocator.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace themis {
namespace llm {

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
    AllocationPlan plan;
    
    // 1. Calculate model weights size
    plan.model_weights = static_cast<size_t>(model.num_parameters) * model.precision_bytes;
    
    // 2. Calculate KV cache size per token
    // Formula: 2 × num_layers × num_kv_heads × head_dim × precision_bytes
    plan.kv_size_per_token = 2 * model.num_layers * model.num_kv_heads * 
                             model.head_dim * model.precision_bytes;
    
    // 3. Calculate static KV cache allocation
    // Allocate for batch_size × max_seq_length
    size_t total_tokens = config.batch_size * config.max_seq_length;
    plan.kv_cache_static = plan.kv_size_per_token * total_tokens;
    
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
    return impl_->active_allocator_.allocateWithFragmentation(bytes, ptr);
}

bool AdaptiveVRAMAllocator::handleOutOfMemory() {
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
        ss << "Draft (INT" << (draft_precision < 1.0f ? 4 : static_cast&lt;int&gt;(draft_precision * 8))
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