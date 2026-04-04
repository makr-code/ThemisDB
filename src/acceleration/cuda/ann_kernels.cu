// CUDA Kernels for HNSW ANN (Approximate Nearest Neighbour) Search
// ThemisDB Hardware Acceleration — NVIDIA CUDA backend
//
// Provides CUDA device kernels for vector similarity search used during HNSW
// graph traversal, covering L2, Cosine, and Inner Product distance metrics
// plus a top-K selection pass.
//
// Kernel launcher functions conform to the ANNDistanceFn / ANNTopKFn typedefs
// declared in include/acceleration/kernel_invocation.h (INTERFACE_VERSION 100).
//
// opaque_stream must be a cudaStream_t cast to void*.  Pass nullptr to use the
// default (null) CUDA stream.
//
// This file is the CUDA-native equivalent of src/acceleration/hip/ann_kernels.hip.

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cub/cub.cuh>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
#include <thrust/tuple.h>
#include <thrust/copy.h>
#include <cmath>
#include <cstdint>
#include "acceleration/kernel_invocation.h"
#include "cosine_config.cuh"
#include "topk_shared.cuh"

namespace themis {
namespace acceleration {
namespace cuda {

// Sentinel value for "infinity" in top-K initialisation (exceeds any valid distance).
static constexpr float kMaxDistanceSentinel = 1e38f;

// =============================================================================
// Distance computation kernels
// =============================================================================

/**
 * Compute squared L2 (Euclidean) distance between query vectors and database
 * vectors.  Squared distance is stored (no sqrtf) for performance and
 * consistency with the rest of the CUDA backend.
 *
 * Thread layout: 2-D block covering (vectors, queries).
 *
 * @param queries      Query matrix     [numQueries × dim]
 * @param vectors      Database matrix  [numVectors × dim]
 * @param distances    Output           [numQueries × numVectors]
 * @param numQueries   Number of query vectors
 * @param numVectors   Number of database vectors
 * @param dim          Vector dimensionality
 */
__global__ void annComputeL2DistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__       distances,
    int numQueries,
    int numVectors,
    int dim
) {
    const int vIdx = blockIdx.x * blockDim.x + threadIdx.x;
    const int qIdx = blockIdx.y * blockDim.y + threadIdx.y;

    if (qIdx >= numQueries || vIdx >= numVectors) return;

    const float* query  = queries + qIdx * dim;
    const float* vector = vectors + vIdx * dim;

    float sum = 0.0f;

    #pragma unroll 4
    for (int i = 0; i < dim; ++i) {
        const float diff = query[i] - vector[i];
        sum += diff * diff;
    }

    distances[qIdx * numVectors + vIdx] = sum;
}

// Compile-time guard: require SM 7.0+; sm_6x emits a warning, <6.0 errors out.
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ < 700)
#  if (__CUDA_ARCH__ >= 600)
#    warning "Building ThemisDB ANN CUDA kernels for sm_60; expect reduced performance."
#  else
#    error "ThemisDB ANN CUDA kernels require sm_70 or newer."
#  endif
#endif

namespace {

constexpr size_t kSharedBytes = cuda_cosine::CosineSharedBytes<float>();
static_assert(kSharedBytes <= 64 * 1024, "CUDA ANN cosine kernel shared memory exceeds 64KB sm_70+ limit");

template <int TILE, int VECS_PER_BLOCK, int QUERIES_PER_BLOCK>
__global__ void annComputeCosineDistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__       distances,
    int numQueries,
    int numVectors,
    int dim
) {
    constexpr int kWarp = 32;
    static_assert(TILE == kWarp, "TILE must equal warp size");

    const int linearY = threadIdx.y;
    const int vLocal = linearY / QUERIES_PER_BLOCK;
    const int qLocal = linearY % QUERIES_PER_BLOCK;
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
        if (vLocal == 0 && threadIdx.x < TILE) {
            const int idx = base + threadIdx.x;
            queryTile[qLocal][threadIdx.x] = (idx < dim)
                ? queries[static_cast<size_t>(qIdx) * dim + idx]
                : 0.0f;
        }
        if (qLocal == 0 && threadIdx.x < TILE) {
            const int idx = base + threadIdx.x;
            vectorTile[vLocal][threadIdx.x] = (idx < dim)
                ? vectors[static_cast<size_t>(vIdx) * dim + idx]
                : 0.0f;
        }
        __syncthreads();

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
        distances[qIdx * numVectors + vIdx] = 1.0f - cosineSim;
    }
}

/**
 * Compute Inner Product distance = -dot(query, vector).
 * Negative dot product is stored so that smaller values correspond to higher
 * similarity (consistent with the frozen kernel invocation interface).
 *
 * @param queries      Query matrix     [numQueries × dim]
 * @param vectors      Database matrix  [numVectors × dim]
 * @param distances    Output           [numQueries × numVectors]
 */
__global__ void annComputeInnerProductKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__       distances,
    int numQueries,
    int numVectors,
    int dim
) {
    const int vIdx = blockIdx.x * blockDim.x + threadIdx.x;
    const int qIdx = blockIdx.y * blockDim.y + threadIdx.y;

    if (qIdx >= numQueries || vIdx >= numVectors) return;

    const float* query  = queries + qIdx * dim;
    const float* vector = vectors + vIdx * dim;

    float dot = 0.0f;

    #pragma unroll 4
    for (int i = 0; i < dim; ++i) {
        dot += query[i] * vector[i];
    }

    // Negative dot so smaller value → more similar.
    distances[qIdx * numVectors + vIdx] = -dot;
}

// =============================================================================
// Top-K selection kernel
// =============================================================================

} // anonymous namespace

// =============================================================================
// Kernel launchers — conform to ANNDistanceFn / ANNTopKFn in kernel_invocation.h
// =============================================================================

extern "C" {

/**
 * Launch the squared L2 distance kernel.
 * Matches the ANNDistanceFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero cudaError_t on failure.
 */
int cuda_launchL2DistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float*       d_distances,
    int          numQueries,
    int          numVectors,
    int          dim,
    void*        opaque_stream
) {
    if (numQueries <= 0 || numVectors <= 0 || dim <= 0) return 0;

    const dim3 blockDim(16, 16);
    const dim3 gridDim(
        (static_cast<unsigned>(numVectors) + blockDim.x - 1u) / blockDim.x,
        (static_cast<unsigned>(numQueries) + blockDim.y - 1u) / blockDim.y
    );

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    annComputeL2DistanceKernel<<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances, numQueries, numVectors, dim);

    return static_cast<int>(cudaGetLastError());
}

/**
 * Launch the Cosine distance kernel.
 * Matches the ANNDistanceFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero cudaError_t on failure.
 */
int cuda_launchCosineDistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float*       d_distances,
    int          numQueries,
    int          numVectors,
    int          dim,
    void*        opaque_stream
) {
    if (numQueries <= 0 || numVectors <= 0 || dim <= 0) return 0;

    const dim3 blockDim(cuda_cosine::kTileSize, cuda_cosine::kVecsPerBlock * cuda_cosine::kQueriesPerBlock);
    const dim3 gridDim(
        (static_cast<unsigned>(numVectors) + cuda_cosine::kVecsPerBlock - 1u) / cuda_cosine::kVecsPerBlock,
        (static_cast<unsigned>(numQueries) + cuda_cosine::kQueriesPerBlock - 1u) / cuda_cosine::kQueriesPerBlock
    );

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    annComputeCosineDistanceKernel<cuda_cosine::kTileSize, cuda_cosine::kVecsPerBlock, cuda_cosine::kQueriesPerBlock><<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances, numQueries, numVectors, dim);

    return static_cast<int>(cudaGetLastError());
}

/**
 * Launch the Inner Product distance kernel.
 * Matches the ANNDistanceFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero cudaError_t on failure.
 */
int cuda_launchInnerProductKernel(
    const float* d_queries,
    const float* d_vectors,
    float*       d_distances,
    int          numQueries,
    int          numVectors,
    int          dim,
    void*        opaque_stream
) {
    if (numQueries <= 0 || numVectors <= 0 || dim <= 0) return 0;

    const dim3 blockDim(16, 16);
    const dim3 gridDim(
        (static_cast<unsigned>(numVectors) + blockDim.x - 1u) / blockDim.x,
        (static_cast<unsigned>(numQueries) + blockDim.y - 1u) / blockDim.y
    );

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    annComputeInnerProductKernel<<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances, numQueries, numVectors, dim);

    return static_cast<int>(cudaGetLastError());
}

/**
 * Launch the top-K extraction kernel.
 * Matches the ANNTopKFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero cudaError_t on failure.
 */
int cuda_launchTopKKernel(
    const float* d_distances,
    uint32_t*    d_topk_indices,
    float*       d_topk_dists,
    int          numQueries,
    int          numVectors,
    int          topK,
    void*        opaque_stream
) {
    if (numQueries <= 0 || numVectors <= 0 || topK <= 0) return 0;
    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    cuda_topk::segmentedTopK(
        d_distances,
        d_topk_indices,
        d_topk_dists,
        numQueries,
        numVectors,
        topK,
        stream);

    return static_cast<int>(cudaGetLastError());
}

} // extern "C"

/**
 * Populate an ANNKernelDispatch table with the CUDA kernel launchers defined
 * in this translation unit.
 *
 * Call this function during CUDA backend initialisation to wire the dispatch
 * table used by the BackendRegistry.
 */
void populateCUDAANNDispatch(ANNKernelDispatch& dispatch) {
    dispatch.launchL2Distance   = &cuda_launchL2DistanceKernel;
    dispatch.launchCosine       = &cuda_launchCosineDistanceKernel;
    dispatch.launchInnerProduct = &cuda_launchInnerProductKernel;
    dispatch.launchTopK         = &cuda_launchTopKKernel;
}

} // namespace cuda
} // namespace acceleration
} // namespace themis
