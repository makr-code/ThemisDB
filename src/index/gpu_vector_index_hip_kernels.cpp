#ifdef THEMIS_ENABLE_HIP

#include <hip/hip_runtime.h>
#include <cmath>

namespace themis {
namespace index {

// =============================================================================
// HIP Kernels for Distance Computation
// HIP kernels are source-compatible with CUDA
// =============================================================================

/**
 * L2 Distance Kernel for HIP
 */
__global__ void hipL2DistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ results,
    int numQueries,
    int numVectors,
    int dimension)
{
    int queryIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vectorIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (queryIdx >= numQueries || vectorIdx >= numVectors) {
        return;
    }
    
    const float* query = queries + queryIdx * dimension;
    const float* vector = vectors + vectorIdx * dimension;
    
    float sum = 0.0f;
    
    #pragma unroll 4
    for (int i = 0; i < dimension; ++i) {
        float diff = query[i] - vector[i];
        sum += diff * diff;
    }
    
    results[queryIdx * numVectors + vectorIdx] = sum;
}

/**
 * Cosine Distance Kernel for HIP
 */
__global__ void hipCosineDistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ results,
    int numQueries,
    int numVectors,
    int dimension)
{
    int queryIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vectorIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (queryIdx >= numQueries || vectorIdx >= numVectors) {
        return;
    }
    
    const float* query = queries + queryIdx * dimension;
    const float* vector = vectors + vectorIdx * dimension;
    
    float dot = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    #pragma unroll 4
    for (int i = 0; i < dimension; ++i) {
        float a = query[i];
        float b = vector[i];
        dot += a * b;
        normA += a * a;
        normB += b * b;
    }
    
    float denominator = sqrtf(normA * normB);
    float similarity = (denominator > 1e-10f) ? (dot / denominator) : 0.0f;
    float distance = 1.0f - similarity;
    
    results[queryIdx * numVectors + vectorIdx] = distance;
}

/**
 * Inner Product Kernel for HIP
 */
__global__ void hipInnerProductKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ results,
    int numQueries,
    int numVectors,
    int dimension)
{
    int queryIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vectorIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (queryIdx >= numQueries || vectorIdx >= numVectors) {
        return;
    }
    
    const float* query = queries + queryIdx * dimension;
    const float* vector = vectors + vectorIdx * dimension;
    
    float dot = 0.0f;
    
    #pragma unroll 4
    for (int i = 0; i < dimension; ++i) {
        dot += query[i] * vector[i];
    }
    
    results[queryIdx * numVectors + vectorIdx] = fmaxf(0.0f, -dot);
}

// =============================================================================
// Optimized kernels for AMD RDNA architecture
// =============================================================================

/**
 * RDNA-optimized L2 Distance Kernel with Wave32 support
 * Uses LDS (Local Data Share) efficiently
 */
__global__ void hipRDNAL2DistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ results,
    int numQueries,
    int numVectors,
    int dimension)
{
    // Use Wave32 optimization for RDNA
    constexpr int WAVE_SIZE = 32;
    __shared__ float sharedQuery[WAVE_SIZE][256]; // Shared memory for query cache
    
    int queryIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vectorIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (queryIdx >= numQueries || vectorIdx >= numVectors) {
        return;
    }
    
    // Load query into shared memory
    int laneId = threadIdx.x % WAVE_SIZE;
    for (int i = laneId; i < dimension; i += WAVE_SIZE) {
        if (threadIdx.x < WAVE_SIZE) {
            sharedQuery[threadIdx.y][i] = queries[queryIdx * dimension + i];
        }
    }
    __syncthreads();
    
    const float* vector = vectors + vectorIdx * dimension;
    float sum = 0.0f;
    
    #pragma unroll 4
    for (int i = 0; i < dimension; ++i) {
        float diff = sharedQuery[threadIdx.y][i] - vector[i];
        sum += diff * diff;
    }
    
    results[queryIdx * numVectors + vectorIdx] = sum;
}

// =============================================================================
// Kernel Launch Wrappers
// =============================================================================

extern "C" void launchHIPL2DistanceKernel(
    const float* queries,
    const float* vectors,
    float* results,
    int numQueries,
    int numVectors,
    int dimension,
    hipStream_t stream)
{
    dim3 blockSize(16, 16);
    dim3 gridSize(
        (numVectors + blockSize.x - 1) / blockSize.x,
        (numQueries + blockSize.y - 1) / blockSize.y
    );
    
    // Use RDNA-optimized kernel for supported architectures
    // For now, use standard kernel
    hipLaunchKernelGGL(hipL2DistanceKernel, gridSize, blockSize, 0, stream,
                      queries, vectors, results, numQueries, numVectors, dimension);
}

extern "C" void launchHIPCosineDistanceKernel(
    const float* queries,
    const float* vectors,
    float* results,
    int numQueries,
    int numVectors,
    int dimension,
    hipStream_t stream)
{
    dim3 blockSize(16, 16);
    dim3 gridSize(
        (numVectors + blockSize.x - 1) / blockSize.x,
        (numQueries + blockSize.y - 1) / blockSize.y
    );
    
    hipLaunchKernelGGL(hipCosineDistanceKernel, gridSize, blockSize, 0, stream,
                      queries, vectors, results, numQueries, numVectors, dimension);
}

extern "C" void launchHIPInnerProductKernel(
    const float* queries,
    const float* vectors,
    float* results,
    int numQueries,
    int numVectors,
    int dimension,
    hipStream_t stream)
{
    dim3 blockSize(16, 16);
    dim3 gridSize(
        (numVectors + blockSize.x - 1) / blockSize.x,
        (numQueries + blockSize.y - 1) / blockSize.y
    );
    
    hipLaunchKernelGGL(hipInnerProductKernel, gridSize, blockSize, 0, stream,
                      queries, vectors, results, numQueries, numVectors, dimension);
}

} // namespace index
} // namespace themis

#endif // THEMIS_ENABLE_HIP
