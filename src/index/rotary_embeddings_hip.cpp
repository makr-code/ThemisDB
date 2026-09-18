/**
 * @file rotary_embeddings_hip.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// HIP Implementation of Rotary Position Embeddings (AMD ROCm)
// ThemisDB GPU Acceleration

#include "index/rotary_embeddings_gpu.h"
#include <hip/hip_runtime.h>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {

namespace {

template <typename T>
/**
 * @brief Free Hip Buffer.
 * @param[in,out] ptr Input/output parameter.
 * @details Calls: hipFree().
 */
void freeHipBuffer(T*& ptr) {
    if (ptr) {
        (void)hipFree(ptr);
        ptr = nullptr;
    }
}

/**
 * @brief Make Hip Error.
 * @param[in] operation Input parameter.
 * @param[in] err Input parameter.
 * @return Return value.
 * @details Calls: std::runtime_error(), std::string(), hipGetErrorString().
 */
std::runtime_error makeHipError(const char* operation, hipError_t err) {
    return std::runtime_error(
        std::string(operation) + " failed: " + hipGetErrorString(err));
}

} // namespace

// ============================================================================
// HIP Kernel
// ============================================================================

/**
 * @brief Rotate Kernel HIP.
 * @param[in] embeddings Input parameter.
 * @param[in] positions Input parameter.
 * @param[in] theta_cache Input parameter.
 * @param[in,out] output Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] hidden_dim Input parameter.
 * @param[in] num_pairs Input parameter.
 * @return Return value.
 * @details Calls: cosf(), sinf().
 */
__global__ void rotateKernelHIP(
    const float* embeddings,
    const size_t* positions,
    const double* theta_cache,
    float* output,
    size_t batch_size,
    size_t hidden_dim,
    size_t num_pairs
) {
    // Global thread index
    int idx = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
    
    // Total work items: batch_size * num_pairs (one per coordinate pair)
    int total_pairs = batch_size * num_pairs;
    if (idx >= total_pairs) {
      return;
    }
    
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
    if (offset + 1 >= batch_size * hidden_dim) {
      return;
    }
    
    // Load coordinate pair
    float x = embeddings[offset];
    float y = embeddings[offset + 1];
    
    // Apply 2D rotation
    output[offset]     = x * cos_val - y * sin_val;
    output[offset + 1] = x * sin_val + y * cos_val;
}

// ============================================================================
// HIP GPU Resources
// ============================================================================

struct RotaryEmbeddingGPU::GPUResources {
    double* d_theta_cache = nullptr;
    size_t theta_cache_size = 0;
    int device_id = 0;
    
    float* d_embeddings = nullptr;
    size_t* d_positions = nullptr;
    float* d_output = nullptr;
    size_t allocated_batch_size = 0;
    
    ~GPUResources() {
        if (d_theta_cache) {
          hipFree(d_theta_cache);
        }
        if (d_embeddings) {
          hipFree(d_embeddings);
        }
        if (d_positions) {
          hipFree(d_positions);
        }
        if (d_output) {
          hipFree(d_output);
        }
    }
};

// ============================================================================
// HIP Implementation
// ============================================================================

RotaryEmbeddingGPU::RotaryEmbeddingGPU(const RotationConfig& config, GPUBackend backend)
    : RotaryEmbedding(config)
    , backend_(backend)
    , gpu_available_(false)
    , gpu_resources_(std::make_unique<GPUResources>())
{
    if (backend_ == GPUBackend::HIP) {
        gpu_available_ = initializeGPU();
    } else {
        gpu_available_ = false;
    }
}

RotaryEmbeddingGPU::~RotaryEmbeddingGPU() {
    cleanupGPU();
}

/**
 * @brief Initialize GPU.
 * @return True when the operation succeeds.
 * @details Calls: hipGetDeviceCount(), hipSetDevice(), uploadThetaCacheToGPU().
 */
bool RotaryEmbeddingGPU::initializeGPU() {
    int device_count = 0;
    hipError_t err = hipGetDeviceCount(&device_count);
    if (err != hipSuccess || device_count == 0) {
        return false;
    }
    
    err = hipSetDevice(0);
    if (err != hipSuccess) {
        return false;
    }
    
    gpu_resources_->device_id = 0;
    return uploadThetaCacheToGPU();
}

/**
 * @brief Cleanup GPU.
 * @details Implements cleanupGPU without additional internal calls.
 */
void RotaryEmbeddingGPU::cleanupGPU() {
    gpu_available_ = false;
}

/**
 * @brief Upload Theta Cache To GPU.
 * @return True when the operation succeeds.
 * @details Calls: lk(), getConfig(), empty(), size(), hipMalloc(), hipMemcpy(), data(), hipFree().
 */
bool RotaryEmbeddingGPU::uploadThetaCacheToGPU() {
    std::lock_guard<std::mutex> lk(gpu_mutex_);
    const auto& theta_cache = getConfig().theta_cache;
    if (theta_cache.empty()) {
        return false;
    }
    
    size_t cache_size = theta_cache.size() * sizeof(double);
    
    hipError_t err = hipMalloc(&gpu_resources_->d_theta_cache, cache_size);
    if (err != hipSuccess) {
        return false;
    }
    
    err = hipMemcpy(
        gpu_resources_->d_theta_cache,
        theta_cache.data(),
        cache_size,
        hipMemcpyHostToDevice
    );
    
    if (err != hipSuccess) {
        hipFree(gpu_resources_->d_theta_cache);
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
    if (gpu_available_ && embeddings.size() >= gpu_batch_threshold_) {
        return rotateBatchGPU(embeddings, positions);
    }
    return RotaryEmbedding::rotateBatch(embeddings, positions);
}

std::vector<std::vector<float>> RotaryEmbeddingGPU::rotateBatchGPU(
    const std::vector<std::vector<float>>& embeddings,
    const std::vector<size_t>& positions
) const {
    /**
     * @brief Lk.
     * @param[in] gpu_mutex_ Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief Flat embeddings.
     * @param[in,out] hidden_dim Input/output parameter.
     * @return Return value.
     */
    std::vector<float> flat_embeddings(batch_size * hidden_dim);
    for (size_t i = 0; i < batch_size; ++i) {
        if (embeddings[i].size() != hidden_dim) {
            throw std::invalid_argument("Embedding dimension mismatch");
        }
        std::copy(embeddings[i].begin(), embeddings[i].end(), 
                  flat_embeddings.begin() + i * hidden_dim);
    }
    
    size_t required_size = batch_size * hidden_dim * sizeof(float);
    if (gpu_resources_->allocated_batch_size < batch_size) {
        freeHipBuffer(gpu_resources_->d_embeddings);
        freeHipBuffer(gpu_resources_->d_positions);
        freeHipBuffer(gpu_resources_->d_output);

        hipError_t err = hipMalloc(&gpu_resources_->d_embeddings, required_size);
        if (err != hipSuccess) {
            gpu_resources_->allocated_batch_size = 0;
            throw makeHipError("hipMalloc(d_embeddings)", err);
        }

        err = hipMalloc(&gpu_resources_->d_positions, batch_size * sizeof(size_t));
        if (err != hipSuccess) {
            freeHipBuffer(gpu_resources_->d_embeddings);
            gpu_resources_->allocated_batch_size = 0;
            throw makeHipError("hipMalloc(d_positions)", err);
        }

        err = hipMalloc(&gpu_resources_->d_output, required_size);
        if (err != hipSuccess) {
            freeHipBuffer(gpu_resources_->d_embeddings);
            freeHipBuffer(gpu_resources_->d_positions);
            gpu_resources_->allocated_batch_size = 0;
            throw makeHipError("hipMalloc(d_output)", err);
        }

        gpu_resources_->allocated_batch_size = batch_size;
    }
    
    hipError_t err = hipMemcpy(gpu_resources_->d_embeddings, flat_embeddings.data(),
                               required_size, hipMemcpyHostToDevice);
    if (err != hipSuccess) {
        throw makeHipError("hipMemcpy(d_embeddings H2D)", err);
    }
    err = hipMemcpy(gpu_resources_->d_positions, positions.data(),
                    batch_size * sizeof(size_t), hipMemcpyHostToDevice);
    if (err != hipSuccess) {
        throw makeHipError("hipMemcpy(d_positions H2D)", err);
    }
    
    int total_work = batch_size * num_pairs;
    int threads_per_block = 256;
    int num_blocks = (total_work + threads_per_block - 1) / threads_per_block;
    
    hipLaunchKernelGGL(rotateKernelHIP, 
                       dim3(num_blocks), dim3(threads_per_block), 0, 0,
                       gpu_resources_->d_embeddings,
                       gpu_resources_->d_positions,
                       gpu_resources_->d_theta_cache,
                       gpu_resources_->d_output,
                       batch_size,
                       hidden_dim,
                       num_pairs);
    
    err = hipGetLastError();
    if (err != hipSuccess) {
        throw makeHipError("HIP kernel launch", err);
    }
    
    err = hipDeviceSynchronize();
    if (err != hipSuccess) {
        throw makeHipError("hipDeviceSynchronize", err);
    }
    
    /**
     * @brief Flat output.
     * @param[in,out] hidden_dim Input/output parameter.
     * @return Return value.
     */
    std::vector<float> flat_output(batch_size * hidden_dim);
    err = hipMemcpy(flat_output.data(), gpu_resources_->d_output,
                    required_size, hipMemcpyDeviceToHost);
    if (err != hipSuccess) {
        throw makeHipError("hipMemcpy(d_output D2H)", err);
    }
    
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
    /**
     * @brief Lk.
     * @param[in] gpu_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(gpu_mutex_);
    if (!gpu_available_) {
        throw std::runtime_error("GPU not available");
    }
    
    size_t hidden_dim = getConfig().hidden_dim;
    size_t num_pairs = getConfig().num_rotation_pairs;
    
    int total_work = batch_size * num_pairs;
    int threads_per_block = 256;
    int num_blocks = (total_work + threads_per_block - 1) / threads_per_block;
    
    hipStream_t hip_stream = static_cast<hipStream_t>(stream);
    
    hipLaunchKernelGGL(rotateKernelHIP, 
                       dim3(num_blocks), dim3(threads_per_block), 0, hip_stream,
                       d_embeddings,
                       d_positions,
                       gpu_resources_->d_theta_cache,
                       d_output,
                       batch_size,
                       hidden_dim,
                       num_pairs);

    const hipError_t err = hipGetLastError();
    if (err != hipSuccess) {
        throw makeHipError("HIP stream kernel launch", err);
    }
}

} // namespace themis
