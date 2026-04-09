// CUDA Kernels for Vector Operations
// ThemisDB Hardware Acceleration

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cub/cub.cuh>
#include <thrust/device_vector.h>
#include <thrust/device_ptr.h>
#include <thrust/execution_policy.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
#include <thrust/tuple.h>
#include <thrust/copy.h>
#include <cmath>
#include <memory>

#include "cosine_config.cuh"
#include "topk_shared.cuh"

namespace themis {
namespace acceleration {
namespace cuda {

// ============================================================================
// Distance Computation Kernels
// ============================================================================

/**
 * Compute L2 (Euclidean) distance between query vectors and database vectors
 * 
 * @param queries      Query vectors (numQueries x dim)
 * @param vectors      Database vectors (numVectors x dim)
 * @param distances    Output distances (numQueries x numVectors)
 * @param numQueries   Number of query vectors
 * @param numVectors   Number of database vectors
 * @param dim          Vector dimension
 */
__global__ void computeL2DistanceKernel(
    const float* queries,
    const float* vectors,
    float* distances,
    int numQueries,
    int numVectors,
    int dim
) {
    int qIdx = blockIdx.y * blockDim.y + threadIdx.y;  // Query index
    int vIdx = blockIdx.x * blockDim.x + threadIdx.x;  // Vector index
    
    if (qIdx >= numQueries || vIdx >= numVectors) return;
    
    const float* query = queries + qIdx * dim;
    const float* vector = vectors + vIdx * dim;
    
    float sum = 0.0f;
    
    // Compute squared L2 distance
    #pragma unroll 4
    for (int i = 0; i < dim; i++) {
        float diff = query[i] - vector[i];
        sum += diff * diff;
    }
    
    // Store squared L2 distance (no sqrt for consistency and performance)
    distances[qIdx * numVectors + vIdx] = sum;
}

// Compile-time guard: require SM 7.0+ for vector kernels (Tensor Core availability).
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ < 700)
#  if (__CUDA_ARCH__ >= 600)
#    warning "Building ThemisDB CUDA vector kernels for sm_6x; Tensor Core optimizations disabled and performance may degrade."
#  else
#    error "ThemisDB CUDA vector kernels require sm_70 or newer."
#  endif
#endif

namespace {

constexpr size_t kSharedBytes = cuda_cosine::CosineSharedBytes<float>();
static_assert(kSharedBytes <= 64 * 1024, "CUDA cosine kernel shared memory exceeds 64KB sm_70+ limit");

template <int TILE, int VECS_PER_BLOCK, int QUERIES_PER_BLOCK>
__global__ void fusedCosineDistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ distances,
    int numQueries,
    int numVectors,
    int dim)
{
    constexpr int kWarp = 32;
    static_assert(TILE == kWarp, "TILE must equal 32 (warp size) for CUB warp reduction");

    const int linearY = threadIdx.y;
    const int vLocal = linearY / QUERIES_PER_BLOCK;   // which vector in this block
    const int qLocal = linearY % QUERIES_PER_BLOCK;   // which query lane
    const int vIdx   = blockIdx.x * VECS_PER_BLOCK + vLocal;
    const int qIdx   = blockIdx.y * QUERIES_PER_BLOCK + qLocal;

    if (qIdx >= numQueries || vIdx >= numVectors) return;

    __shared__ float queryTile[QUERIES_PER_BLOCK][TILE];
    __shared__ float vectorTile[VECS_PER_BLOCK][TILE];
    __shared__ typename cub::WarpReduce<float>::TempStorage warpReduceBuf[VECS_PER_BLOCK][QUERIES_PER_BLOCK][3];

    float partialDot   = 0.0f;
    float partialNormQ = 0.0f;
    float partialNormV = 0.0f;

    for (int base = 0; base < dim; base += TILE) {
        // Load one query tile per query lane
        if (vLocal == 0 && threadIdx.x < TILE) {
            const int idx = base + threadIdx.x;
            queryTile[qLocal][threadIdx.x] = (idx < dim)
                ? queries[static_cast<size_t>(qIdx) * dim + idx]
                : 0.0f;
        }
        // Load one vector tile per vector lane
        if (qLocal == 0 && threadIdx.x < TILE) {
            const int idx = base + threadIdx.x;
            vectorTile[vLocal][threadIdx.x] = (idx < dim)
                ? vectors[static_cast<size_t>(vIdx) * dim + idx]
                : 0.0f;
        }
        __syncthreads();

        // Each warp processes one vector lane
        const int idx = base + threadIdx.x;
        if (idx < dim) {
            const float q = queryTile[qLocal][threadIdx.x];
            const float v = vectorTile[vLocal][threadIdx.x];
            partialDot   += q * v;
            partialNormQ += q * q;
            partialNormV += v * v;
        }
        __syncwarp();
    }

    const float dot   = cub::WarpReduce<float>(warpReduceBuf[vLocal][qLocal][0]).Sum(partialDot);
    const float normQ = cub::WarpReduce<float>(warpReduceBuf[vLocal][qLocal][1]).Sum(partialNormQ);
    const float normV = cub::WarpReduce<float>(warpReduceBuf[vLocal][qLocal][2]).Sum(partialNormV);

    if (threadIdx.x == 0) {
        const float denomQ = fmaxf(normQ, 1e-10f);
        const float denomV = fmaxf(normV, 1e-10f);
        const float cosineSim = dot * rsqrtf(denomQ) * rsqrtf(denomV);
        distances[static_cast<size_t>(qIdx) * numVectors + vIdx] = 1.0f - cosineSim;
    }
}

} // anonymous namespace

/**
 * Compute Inner Product distance between query vectors and database vectors
 * Distance = -dot(query, vector) (negative so smaller = more similar)
 *
 * @param queries      Query vectors (numQueries x dim)
 * @param vectors      Database vectors (numVectors x dim)
 * @param distances    Output distances (numQueries x numVectors)
 * @param numQueries   Number of query vectors
 * @param numVectors   Number of database vectors
 * @param dim          Vector dimension
 */
__global__ void computeInnerProductKernel(
    const float* queries,
    const float* vectors,
    float* distances,
    int numQueries,
    int numVectors,
    int dim
) {
    int qIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vIdx = blockIdx.x * blockDim.x + threadIdx.x;

    if (qIdx >= numQueries || vIdx >= numVectors) return;

    const float* query  = queries + qIdx * dim;
    const float* vector = vectors + vIdx * dim;

    float dot = 0.0f;

    #pragma unroll 4
    for (int i = 0; i < dim; i++) {
        dot += query[i] * vector[i];
    }

    // Store negative dot product so smaller values correspond to more similar vectors
    distances[qIdx * numVectors + vIdx] = -dot;
}

// ============================================================================
// Top-K Selection Kernels (for KNN)
// ============================================================================

/**
 * Bitonic sort for finding top-k elements
 * Used for small k values
 */
__device__ void bitonicSortStep(
    int* indices,
    float* values,
    int k,
    int j,
    int dir
) {
    int i = threadIdx.x;
    if (i >= k) return;
    
    int ixj = i ^ j;
    if (ixj > i) {
        if ((i & dir) == 0) {
            // Ascending
            if (values[i] > values[ixj]) {
                // Swap
                float tempVal = values[i];
                values[i] = values[ixj];
                values[ixj] = tempVal;
                
                int tempIdx = indices[i];
                indices[i] = indices[ixj];
                indices[ixj] = tempIdx;
            }
        } else {
            // Descending
            if (values[i] < values[ixj]) {
                float tempVal = values[i];
                values[i] = values[ixj];
                values[ixj] = tempVal;
                
                int tempIdx = indices[i];
                indices[i] = indices[ixj];
                indices[ixj] = tempIdx;
            }
        }
    }
}

// ============================================================================
// Kernel Launchers (C++ interface)
// ============================================================================

extern "C" {

// ============================================================================
// Occupancy-tuned block dimension helper.
//
// Queries cudaOccupancyMaxPotentialBlockSize to determine the block dimensions
// that maximise SM occupancy for a 2-D (queries × vectors) launch.  The
// result is clamped to ensure total threads per block ≤ 1024 (hardware limit)
// and each dimension is rounded down to the nearest power of 2.
//
// Falls back to dim3(16, 16) = 256 threads when the runtime query fails
// (e.g., inside a non-CUDA test environment).
// ============================================================================

template <typename KernelFn>
static dim3 occupancyBlockDim2D(KernelFn kernel)
{
    int minGridSize = 0, blockSizeFlat = 0;
    const cudaError_t err = cudaOccupancyMaxPotentialBlockSize(
        &minGridSize, &blockSizeFlat, kernel, 0, 0);

    if (err != cudaSuccess || blockSizeFlat <= 0) {
        return dim3(16, 16);  // safe default
    }

    // Split the flat block size into a square 2-D block.
    // Pick the largest power-of-2 tile such that tile² ≤ blockSizeFlat.
    unsigned int tile = 1u;
    while ((tile * 2u) * (tile * 2u) <= static_cast<unsigned>(blockSizeFlat))
        tile *= 2u;
    if (tile > 32u) tile = 32u;  // 32 × 32 = 1024 — hardware maximum
    return dim3(tile, tile);
}

// ============================================================================
// AMD GCN / RDNA wavefront-aware block size helper.
//
// On AMD hardware the warp equivalent (wavefront) is 64 threads rather than
// 32.  Using a block size that is not a multiple of 64 results in half-occupancy
// waste.  At runtime, hipGetDeviceProperties() exposes warpSize; here we use
// the CUDA-side equivalent cudaDeviceGetAttribute() to detect the warp size and
// default to 64-thread blocks when appropriate.
// ============================================================================

static int deviceWarpSize()
{
    int warpSize = 32;  // default for NVIDIA
    int deviceId = 0;
    if (cudaGetDevice(&deviceId) == cudaSuccess) {
        cudaDeviceGetAttribute(&warpSize, cudaDevAttrWarpSize, deviceId);
    }
    return warpSize;
}

/**
 * Launch L2 distance computation kernel
 */
void launchL2DistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float* d_distances,
    int numQueries,
    int numVectors,
    int dim,
    cudaStream_t stream
) {
    // Use occupancy-tuned block dimensions; fall back to (16, 16) on failure.
    // For AMD GCN targets where warpSize=64, prefer 64-thread blocks to avoid
    // half-occupancy (64-thread wavefront, 32-thread block = 50% utilisation).
    dim3 blockDim = occupancyBlockDim2D(computeL2DistanceKernel);
    if (deviceWarpSize() == 64 && blockDim.x * blockDim.y < 64u) {
        blockDim = dim3(8, 8);  // 64 threads, fits AMD GCN wavefront
    }
    dim3 gridDim(
        (numVectors + blockDim.x - 1) / blockDim.x,
        (numQueries + blockDim.y - 1) / blockDim.y
    );
    
    computeL2DistanceKernel<<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances,
        numQueries, numVectors, dim
    );
}

/**
 * Launch Cosine distance computation kernel
 */
void launchCosineDistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float* d_distances,
    int numQueries,
    int numVectors,
    int dim,
    cudaStream_t stream
) {
    dim3 blockDim(cuda_cosine::kTileSize, cuda_cosine::kVecsPerBlock * cuda_cosine::kQueriesPerBlock);
    dim3 gridDim(
        (numVectors + cuda_cosine::kVecsPerBlock - 1) / cuda_cosine::kVecsPerBlock,
        (numQueries + cuda_cosine::kQueriesPerBlock - 1) / cuda_cosine::kQueriesPerBlock
    );
    
    fusedCosineDistanceKernel<cuda_cosine::kTileSize, cuda_cosine::kVecsPerBlock, cuda_cosine::kQueriesPerBlock><<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances,
        numQueries, numVectors, dim
    );
}

/**
 * Launch Inner Product distance computation kernel
 */
void launchInnerProductKernel(
    const float* d_queries,
    const float* d_vectors,
    float* d_distances,
    int numQueries,
    int numVectors,
    int dim,
    cudaStream_t stream
) {
    dim3 blockDim = occupancyBlockDim2D(computeInnerProductKernel);
    if (deviceWarpSize() == 64 && blockDim.x * blockDim.y < 64u) {
        blockDim = dim3(8, 8);
    }
    dim3 gridDim(
        (numVectors + blockDim.x - 1) / blockDim.x,
        (numQueries + blockDim.y - 1) / blockDim.y
    );

    computeInnerProductKernel<<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances,
        numQueries, numVectors, dim
    );
}

/**
 * Launch top-k extraction kernel
 */
void launchTopKKernel(
    const float* d_distances,
    int* d_topkIndices,
    float* d_topkDistances,
    int numQueries,
    int numVectors,
    int k,
    cudaStream_t stream
) {
    if (k <= 0 || numQueries <= 0 || numVectors <= 0) return;
    cuda_topk::segmentedTopK(
        d_distances,
        d_topkIndices,
        d_topkDistances,
        numQueries,
        numVectors,
        k,
        stream);
}

/**
 * Launch Inner Product distance computation kernel (alias).
 * Note: Returns negative dot product (smaller = more similar), same implementation
 * as launchInnerProductKernel; kept for interface compatibility.
 */
void launchInnerProductDistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float* d_distances,
    int numQueries,
    int numVectors,
    int dim,
    cudaStream_t stream
) {
    dim3 blockDim = occupancyBlockDim2D(computeInnerProductKernel);
    if (deviceWarpSize() == 64 && blockDim.x * blockDim.y < 64u) {
        blockDim = dim3(8, 8);
    }
    dim3 gridDim(
        (numVectors + blockDim.x - 1) / blockDim.x,
        (numQueries + blockDim.y - 1) / blockDim.y
    );

    computeInnerProductKernel<<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances,
        numQueries, numVectors, dim
    );
}

} // extern "C"

} // namespace cuda
} // namespace acceleration
} // namespace themis
