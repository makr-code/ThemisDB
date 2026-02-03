#ifdef THEMIS_ENABLE_CUDA

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cmath>

namespace themis {
namespace index {

// =============================================================================
// CUDA Kernels for Distance Computation
// =============================================================================

/**
 * L2 Distance Kernel
 * Computes Euclidean distance: ||a - b||²
 * 
 * Memory coalescing optimized: threads read consecutive elements
 */
__global__ void l2DistanceKernel(
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
    
    // Vectorized reduction with loop unrolling
    #pragma unroll 4
    for (int i = 0; i < dimension; ++i) {
        float diff = query[i] - vector[i];
        sum += diff * diff;
    }
    
    results[queryIdx * numVectors + vectorIdx] = sum;
}

/**
 * Cosine Distance Kernel
 * Computes: 1 - (a·b)/(||a|| ||b||)
 */
__global__ void cosineDistanceKernel(
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
 * Inner Product Kernel
 * Computes: max(0, -a·b)
 */
__global__ void innerProductKernel(
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
// Mixed Precision Kernels (FP16)
// =============================================================================

/**
 * L2 Distance Kernel with FP16 Tensor Core acceleration
 * Uses __half2 for efficient computation on Tensor Cores
 */
__global__ void l2DistanceKernelFP16(
    const __half* __restrict__ queries,
    const __half* __restrict__ vectors,
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
    
    const __half* query = queries + queryIdx * dimension;
    const __half* vector = vectors + vectorIdx * dimension;
    
    float sum = 0.0f;
    
    // Process pairs for __half2 operations
    for (int i = 0; i < dimension - 1; i += 2) {
        __half2 q = *reinterpret_cast<const __half2*>(&query[i]);
        __half2 v = *reinterpret_cast<const __half2*>(&vector[i]);
        __half2 diff = __hsub2(q, v);
        float2 diff_f = __half22float2(diff);
        sum += diff_f.x * diff_f.x + diff_f.y * diff_f.y;
    }
    
    // Handle odd dimension
    if (dimension % 2 == 1) {
        float diff = __half2float(query[dimension - 1]) - __half2float(vector[dimension - 1]);
        sum += diff * diff;
    }
    
    results[queryIdx * numVectors + vectorIdx] = sum;
}

// =============================================================================
// Top-K Selection Kernel (using bitonic sort)
// =============================================================================

/**
 * Bitonic sort step for top-k selection
 */
__device__ void bitonicSortStep(float* distances, uint32_t* indices, int n, int k) {
    // Simple bubble sort for small k (will be replaced with proper bitonic sort)
    for (int i = 0; i < k; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (distances[j] < distances[i]) {
                // Swap distances
                float tempDist = distances[i];
                distances[i] = distances[j];
                distances[j] = tempDist;
                
                // Swap indices
                uint32_t tempIdx = indices[i];
                indices[i] = indices[j];
                indices[j] = tempIdx;
            }
        }
    }
}

/**
 * Extract top-k nearest neighbors for each query
 */
__global__ void topKKernel(
    const float* __restrict__ distances,
    const uint32_t* __restrict__ indices,
    float* __restrict__ topKDistances,
    uint32_t* __restrict__ topKIndices,
    int numQueries,
    int numVectors,
    int k)
{
    int queryIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (queryIdx >= numQueries) {
        return;
    }
    
    // Copy distances and indices to shared memory for sorting
    extern __shared__ float sharedMem[];
    float* localDistances = sharedMem;
    uint32_t* localIndices = (uint32_t*)(localDistances + numVectors);
    
    // Initialize
    for (int i = 0; i < numVectors; ++i) {
        localDistances[i] = distances[queryIdx * numVectors + i];
        localIndices[i] = i;
    }
    
    __syncthreads();
    
    // Partial sort to find top-k
    bitonicSortStep(localDistances, localIndices, numVectors, k);
    
    // Write results
    for (int i = 0; i < k; ++i) {
        topKDistances[queryIdx * k + i] = localDistances[i];
        topKIndices[queryIdx * k + i] = localIndices[i];
    }
}

// =============================================================================
// Flash Attention-style Optimization
// =============================================================================

/**
 * Tiled distance computation with shared memory optimization
 * Inspired by Flash Attention (Dao et al., 2022)
 */
__global__ void tiledL2DistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ results,
    int numQueries,
    int numVectors,
    int dimension)
{
    constexpr int TILE_SIZE = 32;
    __shared__ float queryTile[TILE_SIZE][TILE_SIZE];
    __shared__ float vectorTile[TILE_SIZE][TILE_SIZE];
    
    int queryIdx = blockIdx.y * TILE_SIZE + threadIdx.y;
    int vectorIdx = blockIdx.x * TILE_SIZE + threadIdx.x;
    
    float sum = 0.0f;
    
    // Tile across dimension
    for (int t = 0; t < (dimension + TILE_SIZE - 1) / TILE_SIZE; ++t) {
        int dimIdx = t * TILE_SIZE + threadIdx.x;
        
        // Load query tile
        if (queryIdx < numQueries && dimIdx < dimension) {
            queryTile[threadIdx.y][threadIdx.x] = queries[queryIdx * dimension + dimIdx];
        } else {
            queryTile[threadIdx.y][threadIdx.x] = 0.0f;
        }
        
        // Load vector tile
        dimIdx = t * TILE_SIZE + threadIdx.y;
        if (vectorIdx < numVectors && dimIdx < dimension) {
            vectorTile[threadIdx.y][threadIdx.x] = vectors[vectorIdx * dimension + dimIdx];
        } else {
            vectorTile[threadIdx.y][threadIdx.x] = 0.0f;
        }
        
        __syncthreads();
        
        // Compute partial sum
        #pragma unroll
        for (int k = 0; k < TILE_SIZE; ++k) {
            float diff = queryTile[threadIdx.y][k] - vectorTile[k][threadIdx.x];
            sum += diff * diff;
        }
        
        __syncthreads();
    }
    
    if (queryIdx < numQueries && vectorIdx < numVectors) {
        results[queryIdx * numVectors + vectorIdx] = sum;
    }
}

// =============================================================================
// Kernel Launch Wrappers
// =============================================================================

extern "C" void launchL2DistanceKernel(
    const float* queries,
    const float* vectors,
    float* results,
    int numQueries,
    int numVectors,
    int dimension,
    cudaStream_t stream)
{
    dim3 blockSize(16, 16);
    dim3 gridSize(
        (numVectors + blockSize.x - 1) / blockSize.x,
        (numQueries + blockSize.y - 1) / blockSize.y
    );
    
    // Use tiled kernel for large dimensions
    if (dimension >= 64) {
        tiledL2DistanceKernel<<<gridSize, blockSize, 0, stream>>>(
            queries, vectors, results, numQueries, numVectors, dimension);
    } else {
        l2DistanceKernel<<<gridSize, blockSize, 0, stream>>>(
            queries, vectors, results, numQueries, numVectors, dimension);
    }
}

extern "C" void launchCosineDistanceKernel(
    const float* queries,
    const float* vectors,
    float* results,
    int numQueries,
    int numVectors,
    int dimension,
    cudaStream_t stream)
{
    dim3 blockSize(16, 16);
    dim3 gridSize(
        (numVectors + blockSize.x - 1) / blockSize.x,
        (numQueries + blockSize.y - 1) / blockSize.y
    );
    
    cosineDistanceKernel<<<gridSize, blockSize, 0, stream>>>(
        queries, vectors, results, numQueries, numVectors, dimension);
}

extern "C" void launchInnerProductKernel(
    const float* queries,
    const float* vectors,
    float* results,
    int numQueries,
    int numVectors,
    int dimension,
    cudaStream_t stream)
{
    dim3 blockSize(16, 16);
    dim3 gridSize(
        (numVectors + blockSize.x - 1) / blockSize.x,
        (numQueries + blockSize.y - 1) / blockSize.y
    );
    
    innerProductKernel<<<gridSize, blockSize, 0, stream>>>(
        queries, vectors, results, numQueries, numVectors, dimension);
}

extern "C" void launchTopKKernel(
    const float* distances,
    const uint32_t* indices,
    float* topKDistances,
    uint32_t* topKIndices,
    int numQueries,
    int numVectors,
    int k,
    cudaStream_t stream)
{
    int blockSize = 256;
    int gridSize = (numQueries + blockSize - 1) / blockSize;
    size_t sharedMemSize = numVectors * (sizeof(float) + sizeof(uint32_t));
    
    topKKernel<<<gridSize, blockSize, sharedMemSize, stream>>>(
        distances, indices, topKDistances, topKIndices,
        numQueries, numVectors, k);
}

} // namespace index
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
