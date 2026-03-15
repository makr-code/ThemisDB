#pragma once

#include <algorithm>
#include <cuda_runtime.h>
#include <cub/cub.cuh>
#include <thrust/device_ptr.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
#include <thrust/tuple.h>
#include <thrust/copy.h>

// Shared top-k selection helper used by both ANN and brute-force CUDA backends.
// Roadmap acceptance (v1.7.0) requires CUB DeviceSegmentedSort for k <= 1024;
// beyond that threshold thrust::partial_sort is preferred to keep workspace and
// launch overhead in check on sm_70+ parts.
namespace cuda_topk {

// Benchmark note: on RTX 3090/SM_86, segmented radix sort outperforms partial
// sort up to k=1024 with modest temp storage; beyond that partial_sort avoids
// large scratch buffers and excessive kernel launches (roadmap acceptance).
constexpr int kCUBTopKThreshold = 1024;

template <typename IndexT>
struct DistanceLess {
    __device__ bool operator()(const thrust::tuple<float, IndexT>& a,
                               const thrust::tuple<float, IndexT>& b) const {
        return thrust::get<0>(a) < thrust::get<0>(b);
    }
};

template <typename IndexT>
__global__ void scatterTopKKernel(
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
    const size_t sortedOffset  = static_cast<size_t>(q) * numVectors + i;
    const size_t outputOffset = static_cast<size_t>(q) * topKStride + i;
    topkDistances[outputOffset] = sortedDistances[sortedOffset];
    topkIndices[outputOffset]   = sortedIndices[sortedOffset];
}

template <typename IndexT>
inline void segmentedTopK(
    const float* d_distances,
    IndexT* d_topkIndices,
    float* d_topkDistances,
    int numQueries,
    int numVectors,
    int k,
    cudaStream_t stream)
{
    if (k <= 0 || numQueries <= 0 || numVectors <= 0) return;
    const int cappedK = std::min(k, numVectors);
    DistanceLess<IndexT> distanceLess{};

    if (cappedK <= kCUBTopKThreshold) {
        const size_t total = static_cast<size_t>(numQueries) * static_cast<size_t>(numVectors);

        thrust::device_vector<IndexT> indices(total);
        thrust::sequence(thrust::cuda::par.on(stream), indices.begin(), indices.end(), 0, 1);

        thrust::device_vector<int> offsets(static_cast<size_t>(numQueries) + 1);
        thrust::sequence(thrust::cuda::par.on(stream), offsets.begin(), offsets.end(), 0, numVectors);

        thrust::device_vector<float> sortedDistances(total);
        thrust::device_vector<IndexT> sortedIndices(total);

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
        scatterTopKKernel<<<numQueries, threads, 0, stream>>>(
            thrust::raw_pointer_cast(sortedDistances.data()),
            thrust::raw_pointer_cast(sortedIndices.data()),
            numVectors, cappedK, k,
            d_topkDistances,
            d_topkIndices,
            numQueries);
    } else {
        // Large-k path: used rarely; per-call allocations are acceptable for now.
        thrust::device_vector<IndexT> workingIdx(numVectors);
        thrust::device_vector<float> workingDist(numVectors);

        for (int q = 0; q < numQueries; ++q) {
            thrust::sequence(thrust::cuda::par.on(stream), workingIdx.begin(), workingIdx.end(), 0, 1);
            thrust::copy_n(thrust::cuda::par.on(stream),
                           d_distances + static_cast<size_t>(q) * numVectors,
                           numVectors,
                           workingDist.begin());

            auto zipBegin = thrust::make_zip_iterator(thrust::make_tuple(workingDist.begin(), workingIdx.begin()));
            auto zipEnd   = thrust::make_zip_iterator(thrust::make_tuple(workingDist.end(),   workingIdx.end()));
            // Compare on distance component (tuple element 0) to retain ascending order.
            thrust::partial_sort(thrust::cuda::par.on(stream), zipBegin, zipBegin + cappedK, zipEnd, distanceLess);

            thrust::copy_n(thrust::cuda::par.on(stream), workingDist.begin(), cappedK,
                           d_topkDistances + static_cast<size_t>(q) * k);
            thrust::copy_n(thrust::cuda::par.on(stream), workingIdx.begin(), cappedK,
                           d_topkIndices + static_cast<size_t>(q) * k);
        }
    }
}

} // namespace cuda_topk
