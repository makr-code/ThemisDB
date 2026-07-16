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

// Module-level 2-D block dimension for the L2 and Inner Product distance
// kernels.  The launchers use a (g_cuda_vec_block_dim × g_cuda_vec_block_dim)
// workgroup; the default 16×16 gives 256 total threads which is optimal for
// sm_86 (Ampere).  CUDAVectorBackend::initialize() overwrites this via
// tuneVecKernelBlockSize() to accommodate different GPU micro-architectures.
static int g_cuda_vec_block_dim = 16;

extern "C" {

/**
 * Update the 2-D block dimension used by L2 / InnerProduct kernel launchers.
 *
 * @param dim  Per-axis thread count; total threads = dim².
 *             Must be a power of 2 between 8 and 32.
 */
void setVecKernelBlockDim(int dim) {
    g_cuda_vec_block_dim = dim;
}

/**
 * Query the CUDA occupancy API for the L2 distance kernel and set the optimal
 * 2-D block dimension.  Total threads = g_cuda_vec_block_dim².
 *
 * @return  The per-axis block dimension stored in g_cuda_vec_block_dim.
 */
int tuneVecKernelBlockSize() {
    int minGridSize    = 0;
    int tunedBlockSize = 256;  // 1-D suggestion from the occupancy API
    cudaError_t err = cudaOccupancyMaxPotentialBlockSize(
        &minGridSize, &tunedBlockSize, computeL2DistanceKernel, 0, 0);
    if (err == cudaSuccess && tunedBlockSize > 0) {
        // Convert 1-D count to a square 2-D block size (integer square root
        // rounded to nearest power of 2, clamped to [8, 32]).
        int dim = 1;
        while (dim * dim * 4 <= tunedBlockSize) dim *= 2;
        // dim is now the largest power of 2 so that dim*dim*4 <= tunedBlockSize
        // Clamp: minimum 8, maximum 32 (32×32 = 1024, sm_70+ max threads).
        if (dim < 8)  dim = 8;
        if (dim > 32) dim = 32;
        g_cuda_vec_block_dim = dim;
    }
    return g_cuda_vec_block_dim;
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
    const int bd = g_cuda_vec_block_dim;
    const dim3 blockDim(bd, bd);
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
    const int bd = g_cuda_vec_block_dim;
    const dim3 blockDim(bd, bd);
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
    const int bd = g_cuda_vec_block_dim;
    const dim3 blockDim(bd, bd);
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
