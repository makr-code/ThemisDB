// HIP Kernels for ANN (Approximate Nearest Neighbour) Vector Operations
// ThemisDB Hardware Acceleration — AMD ROCm/HIP backend
//
// Provides HIP device kernels for vector similarity search with L2, Cosine,
// and Inner Product distance metrics, plus top-K selection.
//
// Kernel launcher functions conform to the ANNDistanceFn / ANNTopKFn typedefs
// declared in include/acceleration/kernel_invocation.h (INTERFACE_VERSION 100).
//
// opaque_stream must be a hipStream_t cast to void*.  Pass nullptr to use the
// default (null) HIP stream.
//
// This file is the HIP-native equivalent of src/acceleration/cuda/vector_kernels.cu.

#include <hip/hip_runtime.h>
#include <cmath>
#include <cstdint>
#include "acceleration/kernel_invocation.h"

namespace themis {
namespace acceleration {
namespace hip {

// Sentinel value for "infinity" in top-K initialisation (exceeds any valid distance).
static constexpr float kMaxDistanceSentinel = 1e38f;

// =============================================================================
// Distance computation kernels
// =============================================================================

/**
 * Compute squared L2 (Euclidean) distance between query vectors and database
 * vectors.  Squared distance is stored (no sqrtf) for performance and
 * consistency with the CUDA backend.
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
__global__ void computeL2DistanceKernel(
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

/**
 * Compute Cosine distance = 1 - cosine_similarity between query vectors and
 * database vectors.
 *
 * @param queries      Query matrix     [numQueries × dim]
 * @param vectors      Database matrix  [numVectors × dim]
 * @param distances    Output           [numQueries × numVectors]
 */
__global__ void computeCosineDistanceKernel(
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

    float dot   = 0.0f;
    float normQ = 0.0f;
    float normV = 0.0f;

    #pragma unroll 4
    for (int i = 0; i < dim; ++i) {
        const float q = query[i];
        const float v = vector[i];
        dot   += q * v;
        normQ += q * q;
        normV += v * v;
    }

    normQ = sqrtf(normQ);
    normV = sqrtf(normV);

    const float cosineSim = (normQ > 1e-10f && normV > 1e-10f)
        ? dot / (normQ * normV)
        : 0.0f;

    distances[qIdx * numVectors + vIdx] = 1.0f - cosineSim;
}

/**
 * Compute Inner Product distance = -dot(query, vector).
 * Negative dot product is stored so that smaller values correspond to higher
 * similarity (consistent with the CUDA backend and kernel_invocation.h).
 *
 * @param queries      Query matrix     [numQueries × dim]
 * @param vectors      Database matrix  [numVectors × dim]
 * @param distances    Output           [numQueries × numVectors]
 */
__global__ void computeInnerProductKernel(
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

/**
 * Extract the top-K nearest neighbours (smallest distances) for each query
 * from a precomputed full distance matrix.
 *
 * Each thread block handles one query.  The selected top-K slots are
 * maintained in shared memory using a simple insertion approach; results are
 * sorted ascending before being written out.
 *
 * Note: suited for small-to-medium K (≤ 256).  For larger K a heap-based or
 * radix-select variant should be preferred.
 *
 * @param distances      Input distance matrix   [numQueries × numVectors]
 * @param topkIndices    Output indices           [numQueries × topK]
 * @param topkDistances  Output distances         [numQueries × topK]
 * @param numQueries     Number of queries
 * @param numVectors     Number of database vectors
 * @param topK           K nearest neighbours to retrieve
 */
__global__ void extractTopKKernel(
    const float* __restrict__ distances,
    uint32_t* __restrict__    topkIndices,
    float* __restrict__       topkDistances,
    int numQueries,
    int numVectors,
    int topK
) {
    extern __shared__ char sharedMem[];
    float*    sharedDist = reinterpret_cast<float*>(sharedMem);
    uint32_t* sharedIdx  = reinterpret_cast<uint32_t*>(sharedMem + topK * sizeof(float));

    const int qIdx = blockIdx.x;
    if (qIdx >= numQueries) return;

    const float* queryDist = distances + qIdx * numVectors;

    // Initialise shared slots with the first topK distances.
    if (threadIdx.x < topK) {
        const bool inRange = (threadIdx.x < numVectors);
        sharedDist[threadIdx.x] = inRange ? queryDist[threadIdx.x] : kMaxDistanceSentinel;
        sharedIdx[threadIdx.x]  = static_cast<uint32_t>(threadIdx.x);
    }
    __syncthreads();

    // Process remaining distances — single-threaded update for correctness.
    for (int i = topK + threadIdx.x; i < numVectors; i += blockDim.x) {
        const float d = queryDist[i];

        if (threadIdx.x == 0) {
            // Find the current maximum in top-K.
            int   maxPos  = 0;
            float maxDist = sharedDist[0];
            for (int j = 1; j < topK; ++j) {
                if (sharedDist[j] > maxDist) {
                    maxDist = sharedDist[j];
                    maxPos  = j;
                }
            }
            if (d < maxDist) {
                sharedDist[maxPos] = d;
                sharedIdx[maxPos]  = static_cast<uint32_t>(i);
            }
        }
        __syncthreads();
    }

    // Sort the top-K ascending (insertion sort — fast for small K).
    if (threadIdx.x == 0) {
        for (int i = 1; i < topK; ++i) {
            float    kd  = sharedDist[i];
            uint32_t ki  = sharedIdx[i];
            int      j   = i - 1;
            while (j >= 0 && sharedDist[j] > kd) {
                sharedDist[j + 1] = sharedDist[j];
                sharedIdx[j + 1]  = sharedIdx[j];
                --j;
            }
            sharedDist[j + 1] = kd;
            sharedIdx[j + 1]  = ki;
        }
    }
    __syncthreads();

    // Write results.
    if (threadIdx.x < topK) {
        topkDistances[qIdx * topK + threadIdx.x] = sharedDist[threadIdx.x];
        topkIndices[qIdx * topK + threadIdx.x]   = sharedIdx[threadIdx.x];
    }
}

// =============================================================================
// Kernel launchers — conform to ANNDistanceFn / ANNTopKFn in kernel_invocation.h
// =============================================================================

// Module-level block size for the top-K extraction kernel.
// Tuned at initialize() time via hipOccupancyMaxPotentialBlockSize(); defaults
// to 256 which is optimal for RDNA2 (wave32) and GCN (wave64) devices.
// HIPVectorBackend::initialize() sets this to the device-optimal value.
static int g_hip_topk_block_size = 256;

extern "C" {

/**
 * Update the block size used by the top-K kernel launcher.
 *
 * Called from HIPVectorBackend::initialize() with the value returned by
 * hipOccupancyMaxPotentialBlockSize().  AMD GCN devices (warpSize == 64)
 * use 64 as their default to avoid half-occupancy.
 *
 * @param blockSize  Number of threads per block.
 */
void hipSetTopKBlockSize(int blockSize) {
    g_hip_topk_block_size = blockSize;
}

/**
 * Launch the squared L2 distance kernel.
 * Matches the ANNDistanceFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero hipError_t on failure.
 */
int hip_launchL2DistanceKernel(
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

    const hipStream_t stream = static_cast<hipStream_t>(opaque_stream);

    hipLaunchKernelGGL(computeL2DistanceKernel,
        gridDim, blockDim, 0, stream,
        d_queries, d_vectors, d_distances, numQueries, numVectors, dim);

    return static_cast<int>(hipGetLastError());
}

/**
 * Launch the Cosine distance kernel.
 * Matches the ANNDistanceFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero hipError_t on failure.
 */
int hip_launchCosineDistanceKernel(
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

    const hipStream_t stream = static_cast<hipStream_t>(opaque_stream);

    hipLaunchKernelGGL(computeCosineDistanceKernel,
        gridDim, blockDim, 0, stream,
        d_queries, d_vectors, d_distances, numQueries, numVectors, dim);

    return static_cast<int>(hipGetLastError());
}

/**
 * Launch the Inner Product distance kernel.
 * Matches the ANNDistanceFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero hipError_t on failure.
 */
int hip_launchInnerProductKernel(
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

    const hipStream_t stream = static_cast<hipStream_t>(opaque_stream);

    hipLaunchKernelGGL(computeInnerProductKernel,
        gridDim, blockDim, 0, stream,
        d_queries, d_vectors, d_distances, numQueries, numVectors, dim);

    return static_cast<int>(hipGetLastError());
}

/**
 * Launch the top-K extraction kernel.
 * Matches the ANNTopKFn typedef in kernel_invocation.h.
 *
 * @return 0 on success, non-zero hipError_t on failure.
 */
int hip_launchTopKKernel(
    const float* d_distances,
    uint32_t*    d_topk_indices,
    float*       d_topk_dists,
    int          numQueries,
    int          numVectors,
    int          topK,
    void*        opaque_stream
) {
    if (numQueries <= 0 || numVectors <= 0 || topK <= 0) return 0;

    const int kThreadsPerBlock = g_hip_topk_block_size;
    const int sharedMemBytes = topK * (static_cast<int>(sizeof(float)) +
                                       static_cast<int>(sizeof(uint32_t)));

    const hipStream_t stream = static_cast<hipStream_t>(opaque_stream);

    hipLaunchKernelGGL(extractTopKKernel,
        dim3(numQueries), dim3(kThreadsPerBlock), sharedMemBytes, stream,
        d_distances, d_topk_indices, d_topk_dists,
        numQueries, numVectors, topK);

    return static_cast<int>(hipGetLastError());
}

} // extern "C"

/**
 * Populate an ANNKernelDispatch table with the HIP kernel launchers defined
 * in this translation unit.
 *
 * Call this function during HIP backend initialisation to wire the dispatch
 * table used by the BackendRegistry.
 */
void populateHIPANNDispatch(ANNKernelDispatch& dispatch) {
    dispatch.launchL2Distance   = &hip_launchL2DistanceKernel;
    dispatch.launchCosine       = &hip_launchCosineDistanceKernel;
    dispatch.launchInnerProduct = &hip_launchInnerProductKernel;
    dispatch.launchTopK         = &hip_launchTopKKernel;
}

} // namespace hip
} // namespace acceleration
} // namespace themis
