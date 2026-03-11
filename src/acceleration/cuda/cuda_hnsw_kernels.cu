// CUDA HNSW Graph Traversal Kernel
// ThemisDB Hardware Acceleration — NVIDIA CUDA backend
//
// Implements the GPU-side HNSW (Hierarchical Navigable Small World) approximate
// nearest-neighbour search kernel.  One CUDA thread processes one query.  The
// thread traverses the pre-uploaded CSR bottom-layer graph using an ef-limited
// greedy best-first strategy, mirroring the CPU path in
// src/index/cuda_hnsw_graph_traversal.cpp.
//
// Entry point:
//   themis::cuda::launchHnswSearchKernel(...)
//
// This translation unit is compiled only when THEMIS_ENABLE_CUDA=ON.

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <float.h>
#include <stdint.h>

// Maximum ef value supported by the kernel (candidate list size cap).
// Queries with ef > kMaxEf are clamped to kMaxEf.
static constexpr uint32_t kMaxEf = 512u;

// Maximum k supported by the kernel.
static constexpr uint32_t kMaxK = 256u;

namespace themis {
namespace cuda {

// =============================================================================
// Distance helpers (device functions)
// =============================================================================

__device__ __forceinline__ float deviceL2Sq(
    const float* __restrict__ a,
    const float* __restrict__ b,
    uint32_t dim)
{
    float acc = 0.0f;
    for (uint32_t i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        acc += d * d;
    }
    return acc;
}

__device__ __forceinline__ float deviceCosine(
    const float* __restrict__ a,
    const float* __restrict__ b,
    uint32_t dim)
{
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (uint32_t i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    float denom = sqrtf(na) * sqrtf(nb);
    return (denom > 1e-10f) ? 1.0f - dot / denom : 1.0f;
}

__device__ __forceinline__ float deviceDot(
    const float* __restrict__ a,
    const float* __restrict__ b,
    uint32_t dim)
{
    float dot = 0.0f;
    for (uint32_t i = 0; i < dim; ++i) dot += a[i] * b[i];
    return -dot;  // Negate: smaller value → more similar
}

__device__ __forceinline__ float deviceDist(
    const float* __restrict__ a,
    const float* __restrict__ b,
    uint32_t dim,
    uint8_t metric)
{
    switch (metric) {
        case 1: return deviceCosine(a, b, dim);
        case 2: return deviceDot(a, b, dim);
        default: return deviceL2Sq(a, b, dim);
    }
}

// =============================================================================
// Small fixed-capacity max-heap helpers (device-side, register-based)
//
// These operate on caller-provided arrays of length cap.  Entries are
// (distance, id) pairs; the heap invariant is that heap[0] has the maximum
// distance (so we can cheaply evict the worst candidate).
// =============================================================================

__device__ __forceinline__ void heapSiftDown(
    float* __restrict__    dist_arr,
    int32_t* __restrict__  id_arr,
    int top, int size)
{
    while (true) {
        int largest = top;
        int left  = 2 * top + 1;
        int right = 2 * top + 2;
        if (left  < size && dist_arr[left]  > dist_arr[largest]) largest = left;
        if (right < size && dist_arr[right] > dist_arr[largest]) largest = right;
        if (largest == top) break;
        float  td = dist_arr[top]; dist_arr[top]  = dist_arr[largest]; dist_arr[largest]  = td;
        int32_t ti = id_arr[top];  id_arr[top]   = id_arr[largest];   id_arr[largest]   = ti;
        top = largest;
    }
}

// Insert (d, id) into a max-heap of capacity cap, evicting the maximum when full.
// Returns true if the entry was inserted.
__device__ __forceinline__ bool heapPushCapped(
    float* __restrict__    dist_arr,
    int32_t* __restrict__  id_arr,
    int* size_ptr,
    int cap,
    float d,
    int32_t id)
{
    if (*size_ptr < cap) {
        int pos = *size_ptr;
        dist_arr[pos] = d;
        id_arr[pos]   = id;
        ++(*size_ptr);
        // Sift up to maintain max-heap
        while (pos > 0) {
            int parent = (pos - 1) / 2;
            if (dist_arr[parent] < dist_arr[pos]) {
                float  td = dist_arr[parent]; dist_arr[parent] = dist_arr[pos]; dist_arr[pos] = td;
                int32_t ti = id_arr[parent];  id_arr[parent]  = id_arr[pos];  id_arr[pos]  = ti;
                pos = parent;
            } else break;
        }
        return true;
    } else if (d < dist_arr[0]) {
        // Replace the current maximum
        dist_arr[0] = d;
        id_arr[0]   = id;
        heapSiftDown(dist_arr, id_arr, 0, cap);
        return true;
    }
    return false;
}

// =============================================================================
// HNSW search kernel — one thread per query
//
// Performs an ef-limited greedy best-first search on the bottom-layer CSR graph.
// Uses thread-local arrays (in registers / L1 cache) for the candidate list and
// the result set.  Visited tracking uses the caller-provided per-query bitset.
//
// Parameters:
//   d_vectors     — flat vector matrix [num_nodes × dim], device memory
//   dim           — vector dimensionality
//   d_offsets     — CSR row offsets [num_nodes + 1], device memory
//   d_neighbours  — CSR column indices (neighbour IDs), device memory
//   num_nodes     — number of nodes in the bottom layer
//   d_queries     — query matrix [num_queries × dim], device memory
//   num_queries   — number of queries
//   k             — number of results per query
//   ef            — search-time candidate list size
//   metric        — 0=L2, 1=Cosine, 2=Dot
//   d_result_ids  — output IDs   [num_queries × k], device memory
//   d_result_scores — output scores [num_queries × k], device memory
//   d_visited     — per-query visited bitset [num_queries × num_nodes], device
// =============================================================================

__global__ void hnswSearchKernel(
    const float*   __restrict__ d_vectors,
    uint32_t       dim,
    const int32_t* __restrict__ d_offsets,
    const int32_t* __restrict__ d_neighbours,
    uint32_t       num_nodes,
    const float*   __restrict__ d_queries,
    uint32_t       num_queries,
    uint32_t       k,
    uint32_t       ef,
    uint8_t        metric,
    int64_t*       __restrict__ d_result_ids,
    float*         __restrict__ d_result_scores,
    uint8_t*       __restrict__ d_visited
) {
    const uint32_t qi = blockIdx.x * blockDim.x + threadIdx.x;
    if (qi >= num_queries) return;

    const float*  query    = d_queries + qi * dim;
    uint8_t*      visited  = d_visited + (size_t)qi * num_nodes;
    int64_t*      out_ids  = d_result_ids   + (size_t)qi * k;
    float*        out_dists = d_result_scores + (size_t)qi * k;

    // ── Arrays for candidate list and result set (stack/register allocation) ──
    // Cap at kMaxEf to keep register pressure bounded.
    float   cand_dist[kMaxEf];
    int32_t cand_id  [kMaxEf];
    float   res_dist [kMaxK];
    int32_t res_id   [kMaxK];
    int     cand_size = 0;
    int     res_size  = 0;

    const uint32_t eff_ef = (ef < kMaxEf) ? ef : kMaxEf;
    const uint32_t eff_k  = (k  < kMaxK)  ? k  : kMaxK;

    // ── Initialise visited array ──────────────────────────────────────────────
    for (uint32_t i = 0; i < num_nodes; ++i) visited[i] = 0;

    // ── Entry point: node 0 ───────────────────────────────────────────────────
    if (num_nodes == 0) {
        for (uint32_t i = 0; i < eff_k; ++i) {
            out_ids[i]   = -1;
            out_dists[i] = FLT_MAX;
        }
        return;
    }

    int32_t entry = 0;
    float entry_d = deviceDist(query, d_vectors + (size_t)entry * dim, dim, metric);
    visited[static_cast<uint32_t>(entry)] = 1;

    // Insert into both candidate and result heaps
    heapPushCapped(cand_dist, cand_id, &cand_size, static_cast<int>(eff_ef), entry_d, entry);
    heapPushCapped(res_dist,  res_id,  &res_size,  static_cast<int>(eff_ef), entry_d, entry);

    // ── ef-limited greedy best-first search ──────────────────────────────────
    // candidates is a max-heap on distance (we always pick the minimum);
    // implement "pop minimum" via linear scan (simple for small ef).
    while (cand_size > 0) {
        // Find the minimum-distance candidate
        int   min_pos  = 0;
        float min_dist = cand_dist[0];
        for (int i = 1; i < cand_size; ++i) {
            if (cand_dist[i] < min_dist) {
                min_dist = cand_dist[i];
                min_pos  = i;
            }
        }

        // If the worst result so far is still better than this candidate, stop
        if (res_size >= static_cast<int>(eff_ef) && min_dist > res_dist[0]) break;

        int32_t cn = cand_id[min_pos];
        // Remove from candidates (swap with last)
        cand_dist[min_pos] = cand_dist[cand_size - 1];
        cand_id  [min_pos] = cand_id  [cand_size - 1];
        --cand_size;

        // Explore neighbours of cn
        int32_t start = d_offsets[static_cast<uint32_t>(cn)];
        int32_t end   = d_offsets[static_cast<uint32_t>(cn) + 1];
        for (int32_t ni = start; ni < end; ++ni) {
            int32_t nb = d_neighbours[ni];
            if (nb < 0 || static_cast<uint32_t>(nb) >= num_nodes) continue;
            if (visited[static_cast<uint32_t>(nb)]) continue;
            visited[static_cast<uint32_t>(nb)] = 1;

            float nd = deviceDist(query, d_vectors + (size_t)nb * dim, dim, metric);

            // Add to results if better than current worst, or results not full
            if (res_size < static_cast<int>(eff_ef) || nd < res_dist[0]) {
                heapPushCapped(res_dist, res_id, &res_size, static_cast<int>(eff_ef), nd, nb);
                // Also add to candidates if candidates not too large
                if (cand_size < static_cast<int>(eff_ef)) {
                    heapPushCapped(cand_dist, cand_id, &cand_size, static_cast<int>(eff_ef), nd, nb);
                }
            }
        }
    }

    // ── Extract top-k from result set (sort ascending) ────────────────────────
    // res_dist/res_id are a max-heap; extract in sorted order by repeated pop.
    // We need the k smallest, but res is a max-heap of size eff_ef.
    // Simplest: selection sort on the result array (small sizes, eff_ef ≤ 512).
    const uint32_t out_count = (static_cast<uint32_t>(res_size) < eff_k)
                               ? static_cast<uint32_t>(res_size) : eff_k;

    // Sort the result array ascending by distance (insertion sort)
    for (int i = 1; i < res_size; ++i) {
        float   kd = res_dist[i];
        int32_t ki = res_id[i];
        int j = i - 1;
        while (j >= 0 && res_dist[j] > kd) {
            res_dist[j + 1] = res_dist[j];
            res_id[j + 1]   = res_id[j];
            --j;
        }
        res_dist[j + 1] = kd;
        res_id[j + 1]   = ki;
    }

    // Write top-k to output
    for (uint32_t i = 0; i < eff_k; ++i) {
        if (i < out_count) {
            out_ids[i]   = static_cast<int64_t>(res_id[i]);
            out_dists[i] = res_dist[i];
        } else {
            out_ids[i]   = -1;
            out_dists[i] = FLT_MAX;
        }
    }
}

// =============================================================================
// Public launcher (C++ linkage, called from cuda_hnsw_graph_traversal.cpp)
// =============================================================================

void launchHnswSearchKernel(
    const float*   d_vectors,
    uint32_t       dim,
    const int32_t* d_offsets,
    const int32_t* d_neighbours,
    uint32_t       num_nodes,
    const float*   d_queries,
    uint32_t       num_queries,
    uint32_t       k,
    uint32_t       ef,
    uint8_t        metric,
    int64_t*       d_result_ids,
    float*         d_result_scores,
    cudaStream_t   stream)
{
    if (num_queries == 0 || num_nodes == 0 || k == 0) return;

    // Clamp to kernel limits
    if (ef > kMaxEf) ef = kMaxEf;
    if (k  > kMaxK)  k  = kMaxK;

    // Per-query visited bitset — one byte per node
    const size_t visited_bytes = (size_t)num_queries * num_nodes * sizeof(uint8_t);
    uint8_t* d_visited = nullptr;
    cudaError_t merr = cudaMalloc(&d_visited, visited_bytes);
    if (merr != cudaSuccess || d_visited == nullptr) {
        // Cannot proceed without visited storage — leave output zeroed
        return;
    }
    // Zero-initialise visited array (mandatory for correctness)
    cudaMemsetAsync(d_visited, 0, visited_bytes, stream);

    constexpr uint32_t kThreadsPerBlock = 128u;
    const uint32_t numBlocks = (num_queries + kThreadsPerBlock - 1u) / kThreadsPerBlock;

    hnswSearchKernel<<<numBlocks, kThreadsPerBlock, 0, stream>>>(
        d_vectors, dim,
        d_offsets, d_neighbours, num_nodes,
        d_queries, num_queries,
        k, ef, metric,
        d_result_ids, d_result_scores,
        d_visited);

    // Synchronise before freeing the workspace
    cudaStreamSynchronize(stream);
    cudaFree(d_visited);
}

} // namespace cuda
} // namespace themis
