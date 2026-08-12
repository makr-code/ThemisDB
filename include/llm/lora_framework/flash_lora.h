/**
 * @file flash_lora.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include <cstddef>
#include <tuple>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief FlashLoRA: Memory-efficient LoRA computation inspired by FlashAttention
 * 
 * Key optimizations from FlashAttention papers:
 * 1. Tile input/output to fit in shared memory (SRAM)
 * 2. Compute LoRA path in single fused kernel
 * 3. Use register blocking for intermediate values
 * 4. Minimize HBM accesses (maximize SRAM usage)
 * 
 * Standard LoRA: X → [X @ B^T] → [intermediate @ A^T] → Output
 * Memory: Stores intermediate [batch, seq_len, rank] in HBM
 * 
 * FlashLoRA: X → [tiled computation in SRAM] → Output
 * Memory: No intermediate HBM storage, keeps tiles in SRAM (100x faster)
 * 
 * Expected performance gains:
 * - 2-4x speedup over standard implementation
 * - 50-70% memory reduction
 * - 4-8x longer sequence support (4K-16K tokens vs 512-2K)
 * 
 * KNOWN LIMITATIONS:
 * 
 * 1. **GPU Backend Support**:
 *    - Currently CUDA only (NVIDIA GPUs)
 *    - Requires compute capability >= 7.0 (Volta architecture or newer)
 *    - HIP (AMD), Vulkan, DirectX backends not yet implemented
 *    - Minimum 48KB shared memory per SM required
 * 
 * 2. **Rank Support**:
 *    - Forward pass: ranks 4, 8, 16, 32, 64
 *    - Backward pass: ranks 8, 16 only (limited template instantiations)
 *    - Other rank values require adding explicit template instantiations
 *    - Compile-time rank selection (no dynamic dispatch)
 * 
 * 3. **Gradient Computation Performance**:
 *    - Backward kernels use atomicAdd for gradient accumulation
 *    - Can cause performance bottlenecks under high thread parallelism
 *    - Production workloads should consider reduction trees or warp primitives
 *    - Future: CUB library integration for optimized reductions
 * 
 * 4. **Memory Optimization Trade-offs**:
 *    - Integration with GPULoRALayer still caches intermediate (cached_h_)
 *    - Partial memory overhead for backward pass compatibility
 *    - Forward pass is fully optimized (no intermediate storage)
 *    - Future: FlashLoRA::backward_cached() to eliminate this overhead
 * 
 * 5. **Numerical Precision**:
 *    - FP32 only (FP16/BF16 support planned)
 *    - Numerical accuracy: <1e-3 error vs standard implementation
 *    - Register accumulation may have different rounding behavior
 *    - Validated for typical LoRA training scenarios
 * 
 * 6. **Configuration Constraints**:
 *    - Tile sizes must divide sequence length evenly for optimal performance
 *    - Large ranks (>64) may exceed register limits
 *    - Shared memory size limits maximum tile dimensions
 *    - Auto-tuning provides good defaults but may not be optimal for all cases
 * 
 * 7. **Integration Limitations**:
 *    - No attention fusion yet (separate kernels for attention and LoRA)
 *    - Cannot be used with some LoRA variants (e.g., LoRA+, AdaLoRA)
 *    - Requires contiguous tensors in memory
 *    - No support for sparse or quantized LoRA matrices
 * 
 * References:
 * - Dao et al. (2022): "FlashAttention: Fast and Memory-Efficient Exact Attention"
 * - Dao (2023): "FlashAttention-2: Faster Attention with Better Parallelism"
 */
class FlashLoRA {
public:
    /**
     * @brief Configuration for FlashLoRA tiling and optimizations
     */
    struct Config {
        size_t tile_size_m = 128;      // Tile size for batch/sequence dimension
        size_t tile_size_k = 64;       // Tile size for hidden/rank dimension
        bool use_fp16 = false;         // Use FP16 for faster compute (if supported)
        bool fuse_with_attention = false;  // Fuse LoRA with attention (future)
        
        Config() = default;
        
        /**
         * @brief Auto-tune tile sizes for specific GPU architecture
         * @param device_name GPU device name (e.g., "NVIDIA A100")
         */
        void auto_tune_for_device(const std::string& device_name);
    };
    
    /**
     * @brief FlashLoRA forward pass
     * 
     * Computes: output = (input @ B^T @ A^T) * scaling
     * 
     * Memory optimization:
     * - Standard: O(batch * seq_len * rank) intermediate storage in HBM
     * - FlashLoRA: O(tile_size_m * rank) shared memory only
     * 
     * Speed optimization:
     * - Fused kernel reduces kernel launch overhead
     * - Shared memory access 100x faster than HBM
     * - Register blocking minimizes shared memory accesses
     * 
     * @param input Input tensor [batch, seq_len, in_dim] or [batch, in_dim]
     * @param B LoRA down-projection [rank, in_dim] (transposed internally)
     * @param A LoRA up-projection [out_dim, rank] (transposed internally)
     * @param scaling Scaling factor (typically lora_alpha / rank)
     * @return output tensor [batch, seq_len, out_dim] or [batch, out_dim]
     * 
     * Numerical accuracy: <1e-4 max error vs standard implementation
     */
    static GPUTensor forward(
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling
    );
    /**
     * @brief FlashLoRA forward pass with explicit tiling configuration.
     * 
     * @param input Input tensor [batch, seq_len, in_dim] or [batch, in_dim]
     * @param B LoRA down-projection [rank, in_dim] (transposed internally)
     * @param A LoRA up-projection [out_dim, rank] (transposed internally)
     * @param scaling Scaling factor (typically lora_alpha / rank)
     * @param config Tiling configuration (auto-tuned by default)
     * @return output tensor [batch, seq_len, out_dim] or [batch, out_dim]
     */
    static GPUTensor forward(
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling,
        const Config& config
    );
    
    /**
     * @brief FlashLoRA backward pass
     * 
     * Computes gradients for input, B, and A using tiled computation
     * 
     * Memory optimization same as forward pass:
     * - No intermediate gradients stored in HBM
     * - All computation in shared memory tiles
     * 
     * @param grad_output Gradient from next layer [batch, seq_len, out_dim]
     * @param input Cached input from forward pass [batch, seq_len, in_dim]
     * @param B LoRA down-projection [rank, in_dim]
     * @param A LoRA up-projection [out_dim, rank]
     * @param scaling Scaling factor
     * @return tuple of (grad_input, grad_B, grad_A)
     */
    static std::tuple<GPUTensor, GPUTensor, GPUTensor> backward(
        const GPUTensor& grad_output,
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling
    );
    
    /**
     * @brief FlashLoRA backward pass with custom configuration
     * 
     * Computes gradients for input, B, and A using custom tiling
     * 
     * Memory optimization same as forward pass:
     * - No intermediate gradients stored in HBM
     * - All computation in shared memory tiles
     * 
     * @param grad_output Gradient from next layer [batch, seq_len, out_dim]
     * @param input Cached input from forward pass [batch, seq_len, in_dim]
     * @param B LoRA down-projection [rank, in_dim]
     * @param A LoRA up-projection [out_dim, rank]
     * @param scaling Scaling factor
     * @param config Tiling configuration
     * @return tuple of (grad_input, grad_B, grad_A)
     */
    static std::tuple<GPUTensor, GPUTensor, GPUTensor> backward(
        const GPUTensor& grad_output,
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling,
        const Config& config
    );
    
    /**
     * @brief Check if FlashLoRA is available on current device
     * 
     * Requirements:
     * - CUDA device with compute capability >= 7.0 (Volta+)
     * - Sufficient shared memory (48KB+ per SM)
     * 
     * @param device Target device
     * @return true if FlashLoRA is supported
     */
    static bool is_available(const Device& device);
    
    /**
     * @brief Get recommended configuration for device
     * 
     * Auto-tunes tile sizes based on:
     * - GPU architecture (Volta, Turing, Ampere, Ada, Hopper)
     * - Shared memory size
     * - Register file size
     * - Sequence length
     * 
     * @param device Target device
     * @param rank LoRA rank
     * @param seq_len Typical sequence length
     * @return Optimized configuration
     */
    static Config get_recommended_config(
        const Device& device,
        size_t rank,
        size_t seq_len
    );
    
private:
    // Internal helper for shape validation
    static void validate_shapes(
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A
    );
};

} // namespace lora
} // namespace llm
} // namespace themis
