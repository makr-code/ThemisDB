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

// Compile-time guard: require SM 7.0+ for performance-sensitive kernels.
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ < 700)
#  if (__CUDA_ARCH__ >= 600)
#    warning "Building ThemisDB ANN CUDA kernels for sm_60; expect reduced performance."
#  else
#    error "ThemisDB ANN CUDA kernels require sm_70 or newer."
#  endif
#endif

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

__global__ void fillSequential(uint32_t* indices, int numVectors, int numQueries) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const size_t total = static_cast<size_t>(numVectors) * static_cast<size_t>(numQueries);
    if (static_cast<size_t>(tid) >= total) return;
    indices[tid] = static_cast<uint32_t>(tid % numVectors);
}

template <typename IndexT>
__global__ void annScatterTopK(
    const float* __restrict__ sortedDistances,
    const IndexT* __restrict__ sortedIndices,
    int numVectors,
    int topK,
    int topKStride,
    float* __restrict__ topkDistances,
    IndexT* __restrict__ topkIndices,
    int numQueries)
{
    const int q = blockIdx.x;
    const int i = threadIdx.x;
    if (q >= numQueries || i >= topK) return;
    const size_t inOffset  = static_cast<size_t>(q) * numVectors + i;
    const size_t outOffset = static_cast<size_t>(q) * topKStride + i;
    topkDistances[outOffset] = sortedDistances[inOffset];
    topkIndices[outOffset]   = sortedIndices[inOffset];
}

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

    constexpr int kTile = 32;
    constexpr int kVecsPerBlock = 4;
    constexpr int kQueriesPerBlock = 2;
    const dim3 blockDim(kTile, kVecsPerBlock * kQueriesPerBlock);
    const dim3 gridDim(
        (static_cast<unsigned>(numVectors) + kVecsPerBlock - 1u) / kVecsPerBlock,
        (static_cast<unsigned>(numQueries) + kQueriesPerBlock - 1u) / kQueriesPerBlock
    );

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    annComputeCosineDistanceKernel<kTile, kVecsPerBlock, kQueriesPerBlock><<<gridDim, blockDim, 0, stream>>>(
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
    const int cappedK = topK > numVectors ? numVectors : topK;

    const cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    if (cappedK <= 1024) {
        const size_t total = static_cast<size_t>(numQueries) * static_cast<size_t>(numVectors);
        thrust::device_vector<uint32_t> indices(total);
        thrust::sequence(thrust::cuda::par.on(stream), indices.begin(), indices.end(), 0u, 1u);

        thrust::device_vector<int> offsets(static_cast<size_t>(numQueries) + 1);
        thrust::sequence(thrust::cuda::par.on(stream), offsets.begin(), offsets.end(), 0, numVectors);

        thrust::device_vector<float> sortedDistances(total);
        thrust::device_vector<uint32_t> sortedIndices(total);

        size_t tempBytes = 0;
        cub::DeviceSegmentedRadixSort::SortPairs(
            nullptr, tempBytes,
            d_distances, thrust::raw_pointer_cast(sortedDistances.data()),
            thrust::raw_pointer_cast(indices.data()), thrust::raw_pointer_cast(sortedIndices.data()),
            static_cast<int>(total), numQueries,
            thrust::raw_pointer_cast(offsets.data()),
            thrust::raw_pointer_cast(offsets.data()) + 1,
            stream);

        thrust::device_vector<uint8_t> tempBuffer(tempBytes);
        cub::DeviceSegmentedRadixSort::SortPairs(
            thrust::raw_pointer_cast(tempBuffer.data()), tempBytes,
            d_distances, thrust::raw_pointer_cast(sortedDistances.data()),
            thrust::raw_pointer_cast(indices.data()), thrust::raw_pointer_cast(sortedIndices.data()),
            static_cast<int>(total), numQueries,
            thrust::raw_pointer_cast(offsets.data()),
            thrust::raw_pointer_cast(offsets.data()) + 1,
            stream);

        const int threads = (cappedK < 256) ? cappedK : 256;
        annScatterTopK<<<numQueries, threads, 0, stream>>>(
            thrust::raw_pointer_cast(sortedDistances.data()),
            thrust::raw_pointer_cast(sortedIndices.data()),
            numVectors, cappedK, topK,
            d_topk_dists,
            d_topk_indices,
            numQueries);
    } else {
        thrust::device_vector<uint32_t> workingIdx(numVectors);
        thrust::device_vector<float> workingDist(numVectors);

        for (int q = 0; q < numQueries; ++q) {
            thrust::sequence(thrust::cuda::par.on(stream), workingIdx.begin(), workingIdx.end(), 0u, 1u);
            thrust::copy_n(thrust::cuda::par.on(stream),
                           d_distances + static_cast<size_t>(q) * numVectors,
                           numVectors,
                           workingDist.begin());

            auto zipBegin = thrust::make_zip_iterator(thrust::make_tuple(workingDist.begin(), workingIdx.begin()));
            auto zipEnd   = thrust::make_zip_iterator(thrust::make_tuple(workingDist.end(),   workingIdx.end()));
            thrust::partial_sort(thrust::cuda::par.on(stream), zipBegin, zipBegin + cappedK, zipEnd,
                [] __device__ (const auto& a, const auto& b) {
                    return thrust::get<0>(a) < thrust::get<0>(b);
                });

            thrust::copy_n(thrust::cuda::par.on(stream), workingDist.begin(), cappedK,
                           d_topk_dists + static_cast<size_t>(q) * topK);
            thrust::copy_n(thrust::cuda::par.on(stream), workingIdx.begin(), cappedK,
                           d_topk_indices + static_cast<size_t>(q) * topK);
        }
    }

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
