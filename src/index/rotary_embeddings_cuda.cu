// CUDA Implementation of Rotary Position Embeddings
// ThemisDB GPU Acceleration

#include "index/rotary_embeddings_gpu.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {

namespace {

template <typename T>
void freeCudaBuffer(T*& ptr) {
    if (ptr) {
        (void)cudaFree(ptr);
        ptr = nullptr;
    }
}

std::runtime_error makeCudaError(const char* operation, cudaError_t err) {
    return std::runtime_error(
        std::string(operation) + " failed: " + cudaGetErrorString(err));
}

} // namespace

// ============================================================================
// CUDA Kernel
// ============================================================================

/**
 * GPU kernel for parallel rotation of embeddings
 * Each thread handles one coordinate pair of one embedding
 * 
 * @param embeddings    Input embeddings (batch_size * hidden_dim)
 * @param positions     Position indices (batch_size)
 * @param theta_cache   Precomputed theta values (num_rotation_pairs)
 * @param output        Output rotated embeddings (batch_size * hidden_dim)
 * @param batch_size    Number of embeddings
 * @param hidden_dim    Embedding dimension
 * @param num_pairs     Number of rotation pairs
 */
__global__ void rotateKernel(
    const float* embeddings,
    const size_t* positions,
    const double* theta_cache,
    float* output,
    size_t batch_size,
    size_t hidden_dim,
    size_t num_pairs
) {
    // Global thread index
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Total work items: batch_size * num_pairs (one per coordinate pair)
    int total_pairs = batch_size * num_pairs;
    if (idx >= total_pairs) return;
    
    // Decode which embedding and which pair
    int batch_idx = idx / num_pairs;
    int pair_idx = idx % num_pairs;
    
    // Get position for this embedding
    size_t pos = positions[batch_idx];
    
    // Get theta and compute angle
    double theta = theta_cache[pair_idx];
    double angle = static_cast<double>(pos) * theta;
    
    // Compute cos and sin
    float cos_val = cosf(angle);
    float sin_val = sinf(angle);
    
    // Get indices for this coordinate pair
    int offset = batch_idx * hidden_dim + pair_idx * 2;
    
    // Check bounds
    if (offset + 1 >= batch_size * hidden_dim) return;
    
    // Load coordinate pair
    float x = embeddings[offset];
    float y = embeddings[offset + 1];
    
    // Apply 2D rotation
    // [x']   [cos(θ)  -sin(θ)] [x]
    // [y'] = [sin(θ)   cos(θ)] [y]
    output[offset]     = x * cos_val - y * sin_val;
    output[offset + 1] = x * sin_val + y * cos_val;
}

// ============================================================================
// GPU Resources Implementation
// ============================================================================

struct RotaryEmbeddingGPU::GPUResources {
    double* d_theta_cache = nullptr;   // Device memory for theta cache
    size_t theta_cache_size = 0;
    int device_id = 0;
    
    // Temporary device buffers for batch operations
    float* d_embeddings = nullptr;
    size_t* d_positions = nullptr;
    float* d_output = nullptr;
    size_t allocated_batch_size = 0;
    
    ~GPUResources() {
        if (d_theta_cache) cudaFree(d_theta_cache);
        if (d_embeddings) cudaFree(d_embeddings);
        if (d_positions) cudaFree(d_positions);
        if (d_output) cudaFree(d_output);
    }
};

// ============================================================================
// RotaryEmbeddingGPU Implementation
// ============================================================================

RotaryEmbeddingGPU::RotaryEmbeddingGPU(const RotationConfig& config, GPUBackend backend)
    : RotaryEmbedding(config)
    , backend_(backend)
    , gpu_available_(false)
    , gpu_resources_(std::make_unique<GPUResources>())
{
    // CUDA-specific implementation - only handle CUDA backend
    if (backend_ == GPUBackend::CUDA) {
        gpu_available_ = initializeGPU();
    }
    // Other backends (HIP, CPU) are not handled in this CUDA-specific file
}

RotaryEmbeddingGPU::~RotaryEmbeddingGPU() {
    cleanupGPU();
}

bool RotaryEmbeddingGPU::initializeGPU() {
    // Check if CUDA is available
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || device_count == 0) {
        return false;
    }
    
    // Select device 0
    err = cudaSetDevice(0);
    if (err != cudaSuccess) {
        return false;
    }
    
    gpu_resources_->device_id = 0;
    
    // Upload theta cache to GPU
    return uploadThetaCacheToGPU();
}

void RotaryEmbeddingGPU::cleanupGPU() {
    // Resources are cleaned up by GPUResources destructor
    gpu_available_ = false;
}

bool RotaryEmbeddingGPU::uploadThetaCacheToGPU() {
    const auto& theta_cache = getConfig().theta_cache;
    if (theta_cache.empty()) {
        return false;
    }
    
    size_t cache_size = theta_cache.size() * sizeof(double);
    
    // Allocate device memory
    cudaError_t err = cudaMalloc(&gpu_resources_->d_theta_cache, cache_size);
    if (err != cudaSuccess) {
        return false;
    }
    
    // Copy to device
    err = cudaMemcpy(
        gpu_resources_->d_theta_cache,
        theta_cache.data(),
        cache_size,
        cudaMemcpyHostToDevice
    );
    
    if (err != cudaSuccess) {
        cudaFree(gpu_resources_->d_theta_cache);
        gpu_resources_->d_theta_cache = nullptr;
        return false;
    }
    
    gpu_resources_->theta_cache_size = theta_cache.size();
    return true;
}

std::vector<std::vector<float>> RotaryEmbeddingGPU::rotateBatch(
    const std::vector<std::vector<float>>& embeddings,
    const std::vector<size_t>& positions
) const {
    // Automatic fallback logic:
    // Use GPU if available and batch size is above threshold
    if (gpu_available_ && embeddings.size() >= gpu_batch_threshold_) {
        return rotateBatchGPU(embeddings, positions);
    }
    
    // Fall back to CPU implementation
    return RotaryEmbedding::rotateBatch(embeddings, positions);
}

std::vector<std::vector<float>> RotaryEmbeddingGPU::rotateBatchGPU(
    const std::vector<std::vector<float>>& embeddings,
    const std::vector<size_t>& positions
) const {
    std::lock_guard<std::mutex> lk(gpu_mutex_);
    if (!gpu_available_) {
        throw std::runtime_error("GPU not available for batch rotation");
    }
    
    if (embeddings.size() != positions.size()) {
        throw std::invalid_argument("Batch size mismatch");
    }
    
    size_t batch_size = embeddings.size();
    size_t hidden_dim = getConfig().hidden_dim;
    size_t num_pairs = getConfig().num_rotation_pairs;
    
    // Flatten embeddings to contiguous memory
    std::vector<float> flat_embeddings(batch_size * hidden_dim);
    for (size_t i = 0; i < batch_size; ++i) {
        if (embeddings[i].size() != hidden_dim) {
            throw std::invalid_argument("Embedding dimension mismatch");
        }
        std::copy(embeddings[i].begin(), embeddings[i].end(), 
                  flat_embeddings.begin() + i * hidden_dim);
    }
    
    // Allocate or reuse device memory
    size_t required_size = batch_size * hidden_dim * sizeof(float);
    if (gpu_resources_->allocated_batch_size < batch_size) {
        // Need to reallocate
        freeCudaBuffer(gpu_resources_->d_embeddings);
        freeCudaBuffer(gpu_resources_->d_positions);
        freeCudaBuffer(gpu_resources_->d_output);

        cudaError_t err = cudaMalloc(&gpu_resources_->d_embeddings, required_size);
        if (err != cudaSuccess) {
            gpu_resources_->allocated_batch_size = 0;
            throw makeCudaError("cudaMalloc(d_embeddings)", err);
        }

        err = cudaMalloc(&gpu_resources_->d_positions, batch_size * sizeof(size_t));
        if (err != cudaSuccess) {
            freeCudaBuffer(gpu_resources_->d_embeddings);
            gpu_resources_->allocated_batch_size = 0;
            throw makeCudaError("cudaMalloc(d_positions)", err);
        }

        err = cudaMalloc(&gpu_resources_->d_output, required_size);
        if (err != cudaSuccess) {
            freeCudaBuffer(gpu_resources_->d_embeddings);
            freeCudaBuffer(gpu_resources_->d_positions);
            gpu_resources_->allocated_batch_size = 0;
            throw makeCudaError("cudaMalloc(d_output)", err);
        }

        gpu_resources_->allocated_batch_size = batch_size;
    }
    
    // Copy data to device
    cudaError_t err = cudaMemcpy(gpu_resources_->d_embeddings, flat_embeddings.data(),
                                 required_size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        throw makeCudaError("cudaMemcpy(d_embeddings H2D)", err);
    }
    err = cudaMemcpy(gpu_resources_->d_positions, positions.data(),
                     batch_size * sizeof(size_t), cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        throw makeCudaError("cudaMemcpy(d_positions H2D)", err);
    }
    
    // Launch kernel
    int total_work = batch_size * num_pairs;
    int threads_per_block = 256;
    int num_blocks = (total_work + threads_per_block - 1) / threads_per_block;
    
    rotateKernel<<<num_blocks, threads_per_block>>>(
        gpu_resources_->d_embeddings,
        gpu_resources_->d_positions,
        gpu_resources_->d_theta_cache,
        gpu_resources_->d_output,
        batch_size,
        hidden_dim,
        num_pairs
    );
    
    // Check for kernel errors
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        throw makeCudaError("CUDA kernel launch", err);
    }
    
    // Wait for kernel to complete
    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        throw makeCudaError("cudaDeviceSynchronize", err);
    }
    
    // Copy results back
    std::vector<float> flat_output(batch_size * hidden_dim);
    err = cudaMemcpy(flat_output.data(), gpu_resources_->d_output,
                     required_size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        throw makeCudaError("cudaMemcpy(d_output D2H)", err);
    }
    
    // Unflatten results
    std::vector<std::vector<float>> result(batch_size, std::vector<float>(hidden_dim));
    for (size_t i = 0; i < batch_size; ++i) {
        std::copy(flat_output.begin() + i * hidden_dim,
                  flat_output.begin() + (i + 1) * hidden_dim,
                  result[i].begin());
    }
    
    return result;
}

void RotaryEmbeddingGPU::rotateBatchStreamGPU(
    const float* d_embeddings,
    const size_t* d_positions,
    float* d_output,
    size_t batch_size,
    void* stream
) const {
    std::lock_guard<std::mutex> lk(gpu_mutex_);
    if (!gpu_available_) {
        throw std::runtime_error("GPU not available");
    }
    
    size_t hidden_dim = getConfig().hidden_dim;
    size_t num_pairs = getConfig().num_rotation_pairs;
    
    // Launch kernel on provided stream
    int total_work = batch_size * num_pairs;
    int threads_per_block = 256;
    int num_blocks = (total_work + threads_per_block - 1) / threads_per_block;
    
    cudaStream_t cuda_stream = static_cast<cudaStream_t>(stream);
    
    rotateKernel<<<num_blocks, threads_per_block, 0, cuda_stream>>>(
        d_embeddings,
        d_positions,
        gpu_resources_->d_theta_cache,
        d_output,
        batch_size,
        hidden_dim,
        num_pairs
    );

    const cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        throw makeCudaError("CUDA stream kernel launch", err);
    }
}

} // namespace themis
