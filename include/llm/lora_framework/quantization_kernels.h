/**
 * @file quantization_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief CUDA kernel launcher for NF4 quantization
 * 
 * Quantizes float32 values to 4-bit NormalFloat format using block-wise quantization.
 * Each block has its own scale and zero point for better accuracy.
 * 
 * @param input Input float array (device pointer)
 * @param output Output quantized data (device pointer, packed 2 values per byte)
 * @param scales Output scale factors per block (device pointer)
 * @param zeros Output zero points per block (device pointer)
 * @param num_elements Total number of elements to quantize
 * @param block_size Number of elements per quantization block
 * @param stream CUDA stream for async execution
 * @return cudaError_t CUDA error code
 */
cudaError_t launch_quantize_nf4_kernel(
    const float* input,
    uint8_t* output,
    float* scales,
    float* zeros,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for INT8 quantization
 * 
 * Quantizes float32 values to 8-bit integer format using symmetric quantization.
 * Each block has its own scale factor for better accuracy.
 * 
 * @param input Input float array (device pointer)
 * @param output Output quantized data (device pointer, 1 value per byte)
 * @param scales Output scale factors per block (device pointer)
 * @param num_elements Total number of elements to quantize
 * @param block_size Number of elements per quantization block
 * @param stream CUDA stream for async execution
 * @return cudaError_t CUDA error code
 */
cudaError_t launch_quantize_int8_kernel(
    const float* input,
    int8_t* output,
    float* scales,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for NF4 dequantization
 * 
 * Dequantizes 4-bit NormalFloat values back to float32 using block-wise parameters.
 * 
 * @param input Input quantized data (device pointer, packed 2 values per byte)
 * @param scales Scale factors per block (device pointer)
 * @param zeros Zero points per block (device pointer)
 * @param output Output float array (device pointer)
 * @param num_elements Total number of elements to dequantize
 * @param block_size Number of elements per quantization block
 * @param stream CUDA stream for async execution
 * @return cudaError_t CUDA error code
 */
cudaError_t launch_dequantize_nf4_kernel(
    const uint8_t* input,
    const float* scales,
    const float* zeros,
    float* output,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for INT8 dequantization
 * 
 * Dequantizes 8-bit integer values back to float32 using block-wise scales.
 * 
 * @param input Input quantized data (device pointer, 1 value per byte)
 * @param scales Scale factors per block (device pointer)
 * @param output Output float array (device pointer)
 * @param num_elements Total number of elements to dequantize
 * @param block_size Number of elements per quantization block
 * @param stream CUDA stream for async execution
 * @return cudaError_t CUDA error code
 */
cudaError_t launch_dequantize_int8_kernel(
    const int8_t* input,
    const float* scales,
    float* output,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream = nullptr
);

/**
 * @brief Fused CUDA kernel launcher for dequantize + matrix multiply
 * 
 * Performs on-the-fly dequantization during matrix multiplication to save memory bandwidth.
 * Computes: output = input @ dequantize(quantized_weights)
 * 
 * @param quantized_weights Quantized weight matrix (device pointer)
 * @param scales Scale factors per block (device pointer)
 * @param zeros Zero points per block (device pointer, nullable for INT8)
 * @param input Input matrix (device pointer)
 * @param output Output matrix (device pointer)
 * @param M Number of rows in input/output
 * @param K Number of columns in input, rows in weights
 * @param N Number of columns in weights/output
 * @param block_size Quantization block size
 * @param use_nf4 True for NF4, false for INT8
 * @param stream CUDA stream for async execution
 * @return cudaError_t CUDA error code
 */
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

/**
 * @brief Mixed precision matrix multiply with FP16 compute
 * 
 * Performs matrix multiplication using FP16 compute for faster performance on Volta+.
 * Automatically converts FP32 input/output.
 * 
 * @param A Input matrix A (device pointer, FP32)
 * @param B Input matrix B (device pointer, FP32)
 * @param C Output matrix C (device pointer, FP32)
 * @param M Number of rows in A and C
 * @param K Number of columns in A, rows in B
 * @param N Number of columns in B and C
 * @param alpha Scaling factor
 * @param stream CUDA stream for async execution
 * @return cudaError_t CUDA error code
 */
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

/**
 * @brief GPU memory manager for efficient allocation
 */
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
     * @brief Allocate GPU memory for quantized buffer
     * @param num_params Number of parameters to store
     * @param use_nf4 True for NF4 (4-bit), false for INT8 (8-bit)
     * @return Device pointer to allocated memory, or nullptr on allocation failure
     */
    void* allocateQuantizedBuffer(size_t num_params, bool use_nf4);
    
    /**
     * @brief Allocate pinned host memory for fast transfers
     * @param size Size in bytes
     * @return Host pointer to pinned memory, or nullptr on allocation failure
     */
    void* allocatePinnedHost(size_t size);
    
    /**
     * @brief Free GPU memory
     * @param ptr Device pointer to free
     */
    void freeDevice(void* ptr);
    
    /**
     * @brief Free pinned host memory
     * @param ptr Host pointer to free
     */
    void freePinned(void* ptr);
    
    /**
     * @brief Asynchronous host-to-device transfer
     * @param dst Device destination
     * @param src Host source
     * @param size Size in bytes
     * @param stream CUDA stream
     */
    cudaError_t transferToGPUAsync(
        void* dst,
        const void* src,
        size_t size,
        cudaStream_t stream
    );
    
    /**
     * @brief Asynchronous device-to-host transfer
     * @param dst Host destination
     * @param src Device source
     * @param size Size in bytes
     * @param stream CUDA stream
     */
    cudaError_t transferFromGPUAsync(
        void* dst,
        const void* src,
        size_t size,
        cudaStream_t stream
    );
    
    /**
     * @brief Get total allocated GPU memory in bytes
     */
    size_t getTotalAllocated() const { return total_allocated_; }
    
private:
    size_t total_allocated_ = 0;
};

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
