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
#    warning "Building ThemisDB CUDA vector kernels for sm_60; Tensor Core optimisations disabled and performance may degrade."
#  else
#    error "ThemisDB CUDA vector kernels require sm_70 or newer."
#  endif
#endif

namespace {

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
    static_assert(TILE == kWarp, "TILE must equal warp size for warp reduction");

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

__global__ void fillMonotonicIndices(int* indices, int numVectors, int numQueries) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const size_t total = static_cast<size_t>(numVectors) * static_cast<size_t>(numQueries);
    if (static_cast<size_t>(tid) >= total) return;
    indices[tid] = tid % numVectors;
}

template <typename IndexT>
__global__ void scatterTopKFromSorted(
    const float* __restrict__ sortedDistances,
    const IndexT* __restrict__ sortedIndices,
    int numVectors,
    int k,
    int topKStride,
    float* __restrict__ topkDistances,
    IndexT* __restrict__ topkIndices,
    int numQueries)
{
    const int q = blockIdx.x;
    const int i = threadIdx.x;
    if (q >= numQueries || i >= k) return;
    const size_t inOffset  = static_cast<size_t>(q) * numVectors + i;
    const size_t outOffset = static_cast<size_t>(q) * topKStride + i;
    topkDistances[outOffset] = sortedDistances[inOffset];
    topkIndices[outOffset]   = sortedIndices[inOffset];
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
    dim3 blockDim(16, 16);
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
    constexpr int kTile = 32;
    constexpr int kVecsPerBlock = 4;
    constexpr int kQueriesPerBlock = 2;
    dim3 blockDim(kTile, kVecsPerBlock * kQueriesPerBlock);
    dim3 gridDim(
        (numVectors + kVecsPerBlock - 1) / kVecsPerBlock,
        (numQueries + kQueriesPerBlock - 1) / kQueriesPerBlock
    );
    
    fusedCosineDistanceKernel<kTile, kVecsPerBlock, kQueriesPerBlock><<<gridDim, blockDim, 0, stream>>>(
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
    dim3 blockDim(16, 16);
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
    const int cappedK = k > numVectors ? numVectors : k;

    if (cappedK <= 1024) {
        const size_t total = static_cast<size_t>(numQueries) * static_cast<size_t>(numVectors);

        // Build index and offset buffers
        thrust::device_vector<int> indices(total);
        thrust::sequence(thrust::cuda::par.on(stream), indices.begin(), indices.end(), 0, 1);

        thrust::device_vector<int> offsets(static_cast<size_t>(numQueries) + 1);
        thrust::sequence(thrust::cuda::par.on(stream), offsets.begin(), offsets.end(), 0, numVectors);

        thrust::device_vector<float> sortedDistances(total);
        thrust::device_vector<int>   sortedIndices(total);

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
        scatterTopKFromSorted<<<numQueries, threads, 0, stream>>>(
            thrust::raw_pointer_cast(sortedDistances.data()),
            thrust::raw_pointer_cast(sortedIndices.data()),
            numVectors, cappedK, k,
            d_topkDistances,
            d_topkIndices,
            numQueries);
    } else {
        // Fallback: thrust::partial_sort for larger k
        thrust::device_vector<int> workingIdx(numVectors);
        thrust::device_vector<float> workingDist(numVectors);

        for (int q = 0; q < numQueries; ++q) {
            thrust::sequence(thrust::cuda::par.on(stream), workingIdx.begin(), workingIdx.end(), 0, 1);
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
                           d_topkDistances + static_cast<size_t>(q) * k);
            thrust::copy_n(thrust::cuda::par.on(stream), workingIdx.begin(), cappedK,
                           d_topkIndices + static_cast<size_t>(q) * k);
        }
    }
}

/**
 * Launch Inner Product distance computation kernel
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
    dim3 blockDim(16, 16);
    dim3 gridDim(
        (numVectors + blockDim.x - 1) / blockDim.x,
        (numQueries + blockDim.y - 1) / blockDim.y
    );

    computeInnerProductDistanceKernel<<<gridDim, blockDim, 0, stream>>>(
        d_queries, d_vectors, d_distances,
        numQueries, numVectors, dim
    );
}

} // extern "C"

} // namespace cuda
} // namespace acceleration
} // namespace themis
