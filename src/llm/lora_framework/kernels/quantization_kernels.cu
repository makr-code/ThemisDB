#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/quantization_kernels.h"
#include "security/vram_secure_clear.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cuda_fp16.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <limits>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {

/**
 * @brief Atomic max for floats using compare-and-swap
 */
__device__ inline void atomicMaxFloat(float* addr, float value) {
    int* addr_as_int = (int*)addr;
    int old = *addr_as_int;
    int assumed;
    
    do {
        assumed = old;
        float old_float = __int_as_float(assumed);
        old = atomicCAS(addr_as_int, assumed,
            __float_as_int(fmaxf(old_float, value)));
    } while (assumed != old);
}

/**
 * @brief Atomic min for floats using compare-and-swap
 */
__device__ inline void atomicMinFloat(float* addr, float value) {
    int* addr_as_int = (int*)addr;
    int old = *addr_as_int;
    int assumed;
    
    do {
        assumed = old;
        float old_float = __int_as_float(assumed);
        old = atomicCAS(addr_as_int, assumed,
            __float_as_int(fminf(old_float, value)));
    } while (assumed != old);
}

// ============================================================================
// NF4 Constants (Device-side)
// ============================================================================

__device__ __constant__ float NF4_QUANT_VALUES[16] = {
    -1.0f, -0.6962f, -0.5251f, -0.3949f,
    -0.2844f, -0.1848f, -0.0911f, 0.0f,
    0.0796f, 0.1609f, 0.2461f, 0.3379f,
    0.4407f, 0.5626f, 0.7230f, 1.0f
};

/**
 * @brief Find nearest NF4 bin for a normalized value (device function)
 */
__device__ inline uint8_t find_nf4_bin_device(float value) {
    // Clamp value to [-1, 1]
    value = fmaxf(-1.0f, fminf(1.0f, value));
    
    // Binary search for nearest bin
    uint8_t best_bin = 0;
    float min_dist = fabsf(value - NF4_QUANT_VALUES[0]);
    
    #pragma unroll
    for (uint8_t i = 1; i < 16; i++) {
        float dist = fabsf(value - NF4_QUANT_VALUES[i]);
        if (dist < min_dist) {
            min_dist = dist;
            best_bin = i;
        }
    }
    
    return best_bin;
}

// ============================================================================
// NF4 Quantization Kernel
// ============================================================================

__global__ void quantize_nf4_kernel(
    const float* input,
    uint8_t* output,
    float* scales,
    float* zeros,
    size_t num_elements,
    size_t block_size
) {
    // Each block processes one quantization block
    size_t block_id = blockIdx.x;
    size_t tid = threadIdx.x;
    size_t block_start = block_id * block_size;
    size_t block_end = min(block_start + block_size, num_elements);
    size_t actual_block_size = block_end - block_start;
    
    if (block_start >= num_elements) return;
    
    // Shared memory for parallel reduction
    __shared__ float shared_max;
    __shared__ float shared_min;
    __shared__ float shared_scale;
    __shared__ float shared_zero;
    
    // Initialize shared memory on first thread
    if (tid == 0) {
        shared_max = -FLT_MAX;
        shared_min = FLT_MAX;
    }
    __syncthreads();
    
    // Phase 1: Find min/max in block (parallel reduction)
    float local_max = -FLT_MAX;
    float local_min = FLT_MAX;
    
    for (size_t i = block_start + tid; i < block_end; i += blockDim.x) {
        float val = input[i];
        local_max = fmaxf(local_max, val);
        local_min = fminf(local_min, val);
    }
    
    // Reduce to shared memory using proper float atomics (compare-and-swap)
    atomicMaxFloat(&shared_max, local_max);
    atomicMinFloat(&shared_min, local_min);
    __syncthreads();
    
    // Phase 2: Compute scale and zero point
    if (tid == 0) {
        float block_max = shared_max;
        float block_min = shared_min;
        
        // Avoid division by zero
        if (fabsf(block_max - block_min) < 1e-8f) {
            shared_scale = 1.0f;
            shared_zero = 0.0f;
        } else {
            // Scale to [-1, 1] range for NF4
            shared_scale = (block_max - block_min) / 2.0f;
            shared_zero = (block_max + block_min) / 2.0f;
        }
        
        // Write to global memory
        scales[block_id] = shared_scale;
        zeros[block_id] = shared_zero;
    }
    __syncthreads();
    
    // Phase 3: Quantize values and pack into bytes
    float scale = shared_scale;
    float zero = shared_zero;
    
    for (size_t i = block_start + tid; i < block_end; i += blockDim.x) {
        // Normalize to [-1, 1]
        float normalized = (input[i] - zero) / scale;
        
        // Find NF4 bin
        uint8_t bin = find_nf4_bin_device(normalized);
        
        // Pack 2 values per byte
        size_t local_idx = i - block_start;
        size_t byte_idx = block_start / 2 + local_idx / 2;
        
        if (local_idx % 2 == 0) {
            // Lower 4 bits
            atomicAnd(&output[byte_idx], 0xF0);  // Clear lower bits
            atomicOr(&output[byte_idx], bin);     // Set lower bits
        } else {
            // Upper 4 bits
            atomicAnd(&output[byte_idx], 0x0F);  // Clear upper bits
            atomicOr(&output[byte_idx], bin << 4); // Set upper bits
        }
    }
}

// ============================================================================
// INT8 Quantization Kernel
// ============================================================================

__global__ void quantize_int8_kernel(
    const float* input,
    int8_t* output,
    float* scales,
    size_t num_elements,
    size_t block_size
) {
    size_t block_id = blockIdx.x;
    size_t tid = threadIdx.x;
    size_t block_start = block_id * block_size;
    size_t block_end = min(block_start + block_size, num_elements);
    
    if (block_start >= num_elements) return;
    
    __shared__ float shared_absmax;
    __shared__ float shared_scale;
    
    if (tid == 0) {
        shared_absmax = 0.0f;
    }
    __syncthreads();
    
    // Phase 1: Find absolute maximum
    float local_absmax = 0.0f;
    for (size_t i = block_start + tid; i < block_end; i += blockDim.x) {
        local_absmax = fmaxf(local_absmax, fabsf(input[i]));
    }
    
    // Use proper atomic for positive float using compare-and-swap
    atomicMaxFloat(&shared_absmax, local_absmax);
    __syncthreads();
    
    // Phase 2: Compute scale
    if (tid == 0) {
        float absmax = shared_absmax;
        if (absmax < 1e-8f) {
            shared_scale = 1.0f;
        } else {
            // Symmetric quantization to [-127, 127]
            shared_scale = absmax / 127.0f;
        }
        scales[block_id] = shared_scale;
    }
    __syncthreads();
    
    // Phase 3: Quantize values
    float scale = shared_scale;
    for (size_t i = block_start + tid; i < block_end; i += blockDim.x) {
        float val = input[i];
        int8_t quantized = (int8_t)roundf(val / scale);
        // Clamp to [-128, 127] for full int8 range
        quantized = max((int8_t)-128, min((int8_t)127, quantized));
        output[i] = quantized;
    }
}

// ============================================================================
// NF4 Dequantization Kernel
// ============================================================================

__global__ void dequantize_nf4_kernel(
    const uint8_t* input,
    const float* scales,
    const float* zeros,
    float* output,
    size_t num_elements,
    size_t block_size
) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= num_elements) return;
    
    // Determine block
    size_t block_id = idx / block_size;
    float scale = scales[block_id];
    float zero = zeros[block_id];
    
    // Unpack quantized value (2 values per byte)
    size_t byte_idx = idx / 2;
    uint8_t packed = input[byte_idx];
    uint8_t bin = (idx % 2 == 0) ? (packed & 0x0F) : ((packed >> 4) & 0x0F);
    
    // Dequantize
    float normalized = NF4_QUANT_VALUES[bin];
    output[idx] = normalized * scale + zero;
}

// ============================================================================
// INT8 Dequantization Kernel
// ============================================================================

__global__ void dequantize_int8_kernel(
    const int8_t* input,
    const float* scales,
    float* output,
    size_t num_elements,
    size_t block_size
) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= num_elements) return;
    
    size_t block_id = idx / block_size;
    float scale = scales[block_id];
    
    int8_t quantized = input[idx];
    output[idx] = (float)quantized * scale;
}

// ============================================================================
// Fused Dequantize + MatMul Kernel
// ============================================================================

__global__ void fused_dequant_matmul_nf4_kernel(
    const uint8_t* quantized_weights,
    const float* scales,
    const float* zeros,
    const float* input,
    float* output,
    size_t M,
    size_t K,
    size_t N,
    size_t block_size
) {
    size_t row = blockIdx.y * blockDim.y + threadIdx.y;
    size_t col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row >= M || col >= N) return;
    
    float sum = 0.0f;
    
    // Matrix multiply with on-the-fly dequantization
    for (size_t k = 0; k < K; k++) {
        // Dequantize weight on-the-fly
        size_t weight_idx = k * N + col;
        size_t block_id = weight_idx / block_size;
        
        float scale = scales[block_id];
        float zero = zeros[block_id];
        
        size_t byte_idx = weight_idx / 2;
        uint8_t packed = quantized_weights[byte_idx];
        uint8_t bin = (weight_idx % 2 == 0) ? (packed & 0x0F) : ((packed >> 4) & 0x0F);
        
        float weight = NF4_QUANT_VALUES[bin] * scale + zero;
        
        // Accumulate
        sum += input[row * K + k] * weight;
    }
    
    output[row * N + col] = sum;
}

__global__ void fused_dequant_matmul_int8_kernel(
    const int8_t* quantized_weights,
    const float* scales,
    const float* input,
    float* output,
    size_t M,
    size_t K,
    size_t N,
    size_t block_size
) {
    size_t row = blockIdx.y * blockDim.y + threadIdx.y;
    size_t col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row >= M || col >= N) return;
    
    float sum = 0.0f;
    
    for (size_t k = 0; k < K; k++) {
        size_t weight_idx = k * N + col;
        size_t block_id = weight_idx / block_size;
        
        float scale = scales[block_id];
        float weight = (float)quantized_weights[weight_idx] * scale;
        
        sum += input[row * K + k] * weight;
    }
    
    output[row * N + col] = sum;
}

// ============================================================================
// FP16 Matrix Multiply Kernel
// ============================================================================

__global__ void fp16_matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha
) {
    size_t row = blockIdx.y * blockDim.y + threadIdx.y;
    size_t col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row >= M || col >= N) return;
    
    // Use FP16 accumulation
    half sum = __float2half(0.0f);
    
    for (size_t k = 0; k < K; k++) {
        half a = __float2half(A[row * K + k]);
        half b = __float2half(B[k * N + col]);
        sum = __hadd(sum, __hmul(a, b));
    }
    
    C[row * N + col] = __half2float(sum) * alpha;
}

// ============================================================================
// Kernel Launchers
// ============================================================================

cudaError_t launch_quantize_nf4_kernel(
    const float* input,
    uint8_t* output,
    float* scales,
    float* zeros,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream
) {
    size_t num_blocks = (num_elements + block_size - 1) / block_size;
    int threads_per_block = min(static_cast<int>(block_size), 256);
    
    // Initialize output to zero
    if (stream) {
        cudaError_t memset_err = cudaMemsetAsync(output, 0, (num_elements + 1) / 2, stream);
        if (memset_err != cudaSuccess) {
            return memset_err;
        }
    } else {
        cudaError_t memset_err = cudaMemset(output, 0, (num_elements + 1) / 2);
        if (memset_err != cudaSuccess) {
            return memset_err;
        }
    }
    
    if (stream) {
        quantize_nf4_kernel<<<num_blocks, threads_per_block, 0, stream>>>(
            input, output, scales, zeros, num_elements, block_size);
    } else {
        quantize_nf4_kernel<<<num_blocks, threads_per_block>>>(
            input, output, scales, zeros, num_elements, block_size);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_quantize_int8_kernel(
    const float* input,
    int8_t* output,
    float* scales,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream
) {
    size_t num_blocks = (num_elements + block_size - 1) / block_size;
    int threads_per_block = min(static_cast<int>(block_size), 256);
    
    if (stream) {
        quantize_int8_kernel<<<num_blocks, threads_per_block, 0, stream>>>(
            input, output, scales, num_elements, block_size);
    } else {
        quantize_int8_kernel<<<num_blocks, threads_per_block>>>(
            input, output, scales, num_elements, block_size);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_dequantize_nf4_kernel(
    const uint8_t* input,
    const float* scales,
    const float* zeros,
    float* output,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream
) {
    int threads_per_block = 256;
    int num_blocks = (num_elements + threads_per_block - 1) / threads_per_block;
    
    if (stream) {
        dequantize_nf4_kernel<<<num_blocks, threads_per_block, 0, stream>>>(
            input, scales, zeros, output, num_elements, block_size);
    } else {
        dequantize_nf4_kernel<<<num_blocks, threads_per_block>>>(
            input, scales, zeros, output, num_elements, block_size);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_dequantize_int8_kernel(
    const int8_t* input,
    const float* scales,
    float* output,
    size_t num_elements,
    size_t block_size,
    cudaStream_t stream
) {
    int threads_per_block = 256;
    int num_blocks = (num_elements + threads_per_block - 1) / threads_per_block;
    
    if (stream) {
        dequantize_int8_kernel<<<num_blocks, threads_per_block, 0, stream>>>(
            input, scales, output, num_elements, block_size);
    } else {
        dequantize_int8_kernel<<<num_blocks, threads_per_block>>>(
            input, scales, output, num_elements, block_size);
    }
    
    return cudaGetLastError();
}

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
    cudaStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((N + 15) / 16, (M + 15) / 16);
    
    if (use_nf4) {
        if (stream) {
            fused_dequant_matmul_nf4_kernel<<<gridDim, blockDim, 0, stream>>>(
                quantized_weights, scales, zeros, input, output, M, K, N, block_size);
        } else {
            fused_dequant_matmul_nf4_kernel<<<gridDim, blockDim>>>(
                quantized_weights, scales, zeros, input, output, M, K, N, block_size);
        }
    } else {
        if (stream) {
            fused_dequant_matmul_int8_kernel<<<gridDim, blockDim, 0, stream>>>(
                (const int8_t*)quantized_weights, scales, input, output, M, K, N, block_size);
        } else {
            fused_dequant_matmul_int8_kernel<<<gridDim, blockDim>>>(
                (const int8_t*)quantized_weights, scales, input, output, M, K, N, block_size);
        }
    }
    
    return cudaGetLastError();
}

cudaError_t launch_fp16_matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    cudaStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((N + 15) / 16, (M + 15) / 16);
    
    if (stream) {
        fp16_matmul_kernel<<<gridDim, blockDim, 0, stream>>>(A, B, C, M, K, N, alpha);
    } else {
        fp16_matmul_kernel<<<gridDim, blockDim>>>(A, B, C, M, K, N, alpha);
    }
    
    return cudaGetLastError();
}

// ============================================================================
// GPU Memory Manager Implementation
// ============================================================================

GPUMemoryManager::GPUMemoryManager() : total_allocated_(0) {}

GPUMemoryManager::~GPUMemoryManager() {
    // Note: In production, track allocations and free them here
}

GPUMemoryManager::GPUMemoryManager(GPUMemoryManager&& other) noexcept
    : total_allocated_(other.total_allocated_) {
    other.total_allocated_ = 0;
}

GPUMemoryManager& GPUMemoryManager::operator=(GPUMemoryManager&& other) noexcept {
    if (this != &other) {
        total_allocated_ = other.total_allocated_;
        other.total_allocated_ = 0;
    }
    return *this;
}

void* GPUMemoryManager::allocateQuantizedBuffer(size_t num_params, bool use_nf4) {
    size_t size_bytes = use_nf4 ? (num_params + 1) / 2 : num_params;
    void* ptr = nullptr;
    cudaError_t err = cudaMalloc(&ptr, size_bytes);
    if (err != cudaSuccess) {
        spdlog::error("GPUMemoryManager::allocateQuantizedBuffer: cudaMalloc({}) failed: {}",
                      size_bytes, cudaGetErrorString(err));
        return nullptr;
    }
    total_allocated_ += size_bytes;
    return ptr;
}

void* GPUMemoryManager::allocatePinnedHost(size_t size) {
    void* ptr = nullptr;
    cudaError_t err = cudaMallocHost(&ptr, size);
    if (err != cudaSuccess) {
        spdlog::error("GPUMemoryManager::allocatePinnedHost: cudaMallocHost({}) failed: {}",
                      size, cudaGetErrorString(err));
        return nullptr;
    }
    return ptr;
}

void GPUMemoryManager::freeDevice(void* ptr) {
    if (ptr) {
        cudaError_t err = cudaFree(ptr);
        if (err != cudaSuccess) {
            spdlog::error("GPUMemoryManager::freeDevice: cudaFree failed: {}",
                          cudaGetErrorString(err));
        }
    }
}

void GPUMemoryManager::freePinned(void* ptr) {
    if (ptr) {
        cudaError_t err = cudaFreeHost(ptr);
        if (err != cudaSuccess) {
            spdlog::error("GPUMemoryManager::freePinned: cudaFreeHost failed: {}",
                          cudaGetErrorString(err));
        }
    }
}

cudaError_t GPUMemoryManager::transferToGPUAsync(
    void* dst,
    const void* src,
    size_t size,
    cudaStream_t stream
) {
    return cudaMemcpyAsync(dst, src, size, cudaMemcpyHostToDevice, stream);
}

cudaError_t GPUMemoryManager::transferFromGPUAsync(
    void* dst,
    const void* src,
    size_t size,
    cudaStream_t stream
) {
    return cudaMemcpyAsync(dst, src, size, cudaMemcpyDeviceToHost, stream);
}

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
