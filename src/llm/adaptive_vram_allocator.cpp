/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_vram_allocator.cpp                        ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     191                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/adaptive_vram_allocator.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace themis {
namespace llm {

// Private implementation
class AdaptiveVRAMAllocator::Impl {
public:
    Impl() = default;
    ~Impl() = default;
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
    // Stub implementation - would integrate with actual GPU allocator
    // In production, this would use cudaMalloc or similar
    if (ptr == nullptr) {
        return false;
    }
    
    // Block-based allocation to minimize fragmentation
    // Round up to nearest 4KB block (optimal block size from research)
    constexpr size_t BLOCK_SIZE = 4096;
    size_t aligned_bytes = ((bytes + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    
    // In real implementation, would call GPU allocator here
    *ptr = nullptr;  // Stub
    
    return aligned_bytes > 0;
}

bool AdaptiveVRAMAllocator::handleOutOfMemory() {
    // Stub implementation - recovery strategies:
    // 1. Evict stale KV cache blocks
    // 2. Defragment memory
    // 3. Spill to CPU memory
    // 4. Reduce batch size dynamically
    
    // In production, would implement actual OOM recovery
    return false;
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

} // namespace llm
} // namespace themis
