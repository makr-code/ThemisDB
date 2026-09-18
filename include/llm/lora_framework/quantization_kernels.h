/**
 * @file quantization_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_CUDA

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {

cudaError_t launch_quantize_nf4_kernel(
    const float* input,
    uint8_t* output,
    float* scales,
    float* zeros,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

cudaError_t launch_quantize_int8_kernel(
    const float* input,
    int8_t* output,
    float* scales,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

cudaError_t launch_dequantize_nf4_kernel(
    const uint8_t* input,
    const float* scales,
    const float* zeros,
    float* output,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

cudaError_t launch_dequantize_int8_kernel(
    const int8_t* input,
    const float* scales,
    float* output,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

cudaError_t launch_fused_dequant_matmul_kernel(
    const uint8_t* quantized_weights,
    const float* scales,
    const float* zeros,
    const float* input,
    float* output,
    size_t M,
    size_t K,
    size_t N,
    size_t block_size,
    bool use_nf4,
    cudaStream_t stream = nullptr
);

cudaError_t launch_fp16_matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha = 1.0f,
    cudaStream_t stream = nullptr
);

class GPUMemoryManager {
public:
    GPUMemoryManager();
    ~GPUMemoryManager();
    
    // Disable copy, allow move
    GPUMemoryManager(const GPUMemoryManager&) = delete;
    GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;
    GPUMemoryManager(GPUMemoryManager&&) noexcept;
    GPUMemoryManager& operator=(GPUMemoryManager&&) noexcept;
    
    /**
     * @brief Allocate Quantized Buffer.
     * @param[in] num_params Input parameter.
     * @param[in] use_nf4 Input parameter.
     * @return Pointer to the result.
     */
    void* allocateQuantizedBuffer(size_t num_params, bool use_nf4);
    
    /**
     * @brief Allocate Pinned Host.
     * @param[in] size Input parameter.
     * @return Pointer to the result.
     */
    void* allocatePinnedHost(size_t size);
    
    /**
     * @brief Free Device.
     * @param[in,out] ptr Input/output parameter.
     */
    void freeDevice(void* ptr);
    
    /**
     * @brief Free Pinned.
     * @param[in,out] ptr Input/output parameter.
     */
    void freePinned(void* ptr);
    
    /**
     * @brief Transfer To GPUAsync.
     * @param[in,out] dst Input/output parameter.
     * @param[in] src Input parameter.
     * @param[in] size Input parameter.
     * @param[in] stream Input parameter.
     * @return Return value.
     */
    cudaError_t transferToGPUAsync(
        void* dst,
        const void* src,
        size_t size,
        cudaStream_t stream
    );
    
    /**
     * @brief Transfer From GPUAsync.
     * @param[in,out] dst Input/output parameter.
     * @param[in] src Input parameter.
     * @param[in] size Input parameter.
     * @param[in] stream Input parameter.
     * @return Return value.
     */
    cudaError_t transferFromGPUAsync(
        void* dst,
        const void* src,
        size_t size,
        cudaStream_t stream
    );
    
    size_t getTotalAllocated() const { return total_allocated_; }
    
private:
    size_t total_allocated_ = 0;
};

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
