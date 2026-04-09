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
#include <assert.h>  // for host-side assert() used in launchHnswSearchKernel

// Maximum ef value supported by the kernel (candidate list size cap).
// Queries with ef > kMaxEf are clamped to kMaxEf.
static constexpr uint32_t kMaxEf = 512u;

// Maximum k supported by a single-pass kernel launch.
// Result buffers (res_dist, res_id) are allocated in dynamically-sized shared
// memory so their size is determined at runtime rather than compile time.
// For k > kMaxK the launcher does NOT clamp silently; instead it sets the
// caller-supplied overflow flag and leaves outputs untouched.  The caller is
// expected to fall back to a multi-pass strategy (see launchHnswSearchKernel).
static constexpr uint32_t kMaxK = 1024u;

// Available shared memory per block (conservative default; 48 KB is the
// minimum guaranteed by SM 2.0+ and covers RTX-class GPUs up to sm_90).
static constexpr uint32_t kSharedMemBytes = 49152u;

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
// dynamically allocated shared memory for the per-thread result set so that the
// result buffer size matches the requested k at runtime rather than being fixed
// at the compile-time kMaxK constant.  Visited tracking uses a 1-bit-per-node
// bitset stored in the caller-provided per-query device buffer.
//
// Dynamic shared memory layout (per block, passed as third <<<>>> argument):
//   [blockDim.x * k * sizeof(float)]   — res_dist slices, one per thread
//   [blockDim.x * k * sizeof(int32_t)] — res_id   slices, one per thread
//
// Parameters:
//   d_vectors     — flat vector matrix [num_nodes × dim], device memory
//   dim           — vector dimensionality
//   d_offsets     — CSR row offsets [num_nodes + 1], device memory
//   d_neighbours  — CSR column indices (neighbour IDs), device memory
//   num_nodes     — number of nodes in the bottom layer
//   d_queries     — query matrix [num_queries × dim], device memory
//   num_queries   — number of queries
//   k             — number of results per query (≤ kMaxK)
//   ef            — search-time candidate list size
//   metric        — 0=L2, 1=Cosine, 2=Dot
//   entry_node    — starting node for the graph traversal (0 = default;
//                   set to a non-zero node for multi-pass searches)
//   d_result_ids  — output IDs   [num_queries × k], device memory
//   d_result_scores — output scores [num_queries × k], device memory
//   d_visited     — per-query 1-bit-per-node visited bitset
//                   [num_queries × ceil(num_nodes/8)] bytes, device memory
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
    uint32_t       entry_node,
    int64_t*       __restrict__ d_result_ids,
    float*         __restrict__ d_result_scores,
    uint8_t*       __restrict__ d_visited
) {
    // ── Dynamic shared memory: per-thread result buffers ─────────────────────
    // Layout: [ blockDim.x * k floats ] [ blockDim.x * k int32_t ]
    extern __shared__ float s_res_dist_base[];
    int32_t* s_res_id_base = (int32_t*)(s_res_dist_base + blockDim.x * k);

    float*   res_dist = s_res_dist_base + threadIdx.x * k;
    int32_t* res_id   = s_res_id_base   + threadIdx.x * k;

    const uint32_t qi = blockIdx.x * blockDim.x + threadIdx.x;
    if (qi >= num_queries) return;

    // Bytes required for the per-query 1-bit-per-node bitset
    const uint32_t visited_bytes = (num_nodes + 7u) / 8u;

    const float*  query    = d_queries + qi * dim;
    uint8_t*      visited  = d_visited + (size_t)qi * visited_bytes;
    int64_t*      out_ids  = d_result_ids   + (size_t)qi * k;
    float*        out_dists = d_result_scores + (size_t)qi * k;

    // ── Arrays for candidate list (thread-local, register/L1 cache) ──────────
    // Cap at kMaxEf to keep register pressure bounded.
    float   cand_dist[kMaxEf];
    int32_t cand_id  [kMaxEf];
    int     cand_size = 0;
    int     res_size  = 0;

    const uint32_t eff_ef = (ef < kMaxEf) ? ef : kMaxEf;
    const uint32_t eff_k  = (k  < kMaxK)  ? k  : kMaxK;

    // ── Clamp entry_node to valid range ──────────────────────────────────────
    const uint32_t safe_entry = (entry_node < num_nodes) ? entry_node : 0u;

    // ── Initialise visited bitset (zero all bytes) ────────────────────────────
    for (uint32_t i = 0; i < visited_bytes; ++i) visited[i] = 0u;

    // ── Entry point ──────────────────────────────────────────────────────────
    if (num_nodes == 0) {
        for (uint32_t i = 0; i < eff_k; ++i) {
            out_ids[i]   = -1;
            out_dists[i] = FLT_MAX;
        }
        return;
    }

    int32_t entry = static_cast<int32_t>(safe_entry);
    float entry_d = deviceDist(query, d_vectors + (size_t)entry * dim, dim, metric);
    // Mark entry node visited (bitset set)
    visited[static_cast<uint32_t>(entry) >> 3u] |= (1u << (static_cast<uint32_t>(entry) & 7u));

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
            // Check visited bit
            uint32_t nb_u = static_cast<uint32_t>(nb);
            if (visited[nb_u >> 3u] & (1u << (nb_u & 7u))) continue;
            // Mark visited
            visited[nb_u >> 3u] |= (1u << (nb_u & 7u));

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
    // res_dist/res_id are in dynamic shared memory (per-thread slice).
    // We need the k smallest; res is a max-heap of up to eff_ef entries.
    // Use insertion sort (small sizes, eff_ef ≤ kMaxEf = 512).
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
// Helper: compute threads-per-block for a given k such that the dynamic
// shared memory required for the result buffers fits within kSharedMemBytes.
// =============================================================================
static uint32_t computeThreadsPerBlock(uint32_t k) {
    const uint32_t smem_per_thread = k * static_cast<uint32_t>(sizeof(float) + sizeof(int32_t));
    if (smem_per_thread == 0) return 128u;
    uint32_t threads = kSharedMemBytes / smem_per_thread;
    if (threads == 0) return 1u;
    threads = threads < 128u ? threads : 128u;
    // Round down to nearest power of 2 using __builtin_clz (GCC/Clang/nvcc)
    return 1u << (31u - static_cast<uint32_t>(__builtin_clz(threads)));
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
    uint32_t       entry_node,
    int64_t*       d_result_ids,
    float*         d_result_scores,
    cudaStream_t   stream,
    bool*          h_overflow,
    uint8_t*       d_visited_pool,   // optional pre-allocated pool (may be nullptr)
    size_t         visited_pool_bytes // capacity of d_visited_pool in bytes
)
{
    if (num_queries == 0 || num_nodes == 0 || k == 0) return;

    if (ef > kMaxEf) ef = kMaxEf;

    // ── k > kMaxK: overflow — do NOT silently truncate ────────────────────────
    // Signal the caller via h_overflow so it can choose a multi-pass strategy.
    // In debug builds assert immediately to catch misuse early.
    if (k > kMaxK) {
        assert(k <= kMaxK &&
               "launchHnswSearchKernel: k > kMaxK — caller must use multi-pass");
        if (h_overflow) *h_overflow = true;
        return;
    }

    if (h_overflow) *h_overflow = false;

    // ── Compute block size so result-buffer shared memory fits in SM limits ───
    // Required per block: kThreadsPerBlock * k * (sizeof(float) + sizeof(int32_t))
    const uint32_t kThreadsPerBlock = computeThreadsPerBlock(k);
    const size_t   smem_bytes       = static_cast<size_t>(kThreadsPerBlock) *
                                      static_cast<size_t>(k) *
                                      (sizeof(float) + sizeof(int32_t));

    // ── Visited bitset allocation ─────────────────────────────────────────────
    // Prefer a pre-allocated pool (zero allocation overhead) over per-invocation
    // cudaMalloc/cudaFree (which stalls the stream and adds ~2-5 µs overhead).
    // Memory: num_queries × ceil(num_nodes / 8) bytes  (1-bit-per-node bitset)
    //   e.g. 512 queries × 10M nodes → 512 × 1.25 MB ≈ 640 MB
    const size_t visited_bytes_per_query = ((size_t)num_nodes + 7u) / 8u;
    const size_t visited_bytes = (size_t)num_queries * visited_bytes_per_query;

    uint8_t* d_visited = nullptr;
    bool     pool_owned = false;   // true when we allocated d_visited ourselves

    if (d_visited_pool != nullptr && visited_pool_bytes >= visited_bytes) {
        // Use the caller-supplied persistent pool — no allocation required.
        d_visited = d_visited_pool;
    } else {
        // Fall back to per-invocation allocation (pool absent or too small).
        cudaError_t merr = cudaMalloc(&d_visited, visited_bytes);
        if (merr != cudaSuccess || d_visited == nullptr) {
            // Cannot proceed without visited storage; signal degraded state via
            // h_overflow so the caller can surface BackendHealthStatus::makeDegraded().
            if (h_overflow) *h_overflow = true;
            return;
        }
        pool_owned = true;
    }

    // Zero-initialise visited bitset (mandatory for correctness)
    cudaMemsetAsync(d_visited, 0, visited_bytes, stream);

    const uint32_t numBlocks = (num_queries + kThreadsPerBlock - 1u) / kThreadsPerBlock;

    hnswSearchKernel<<<numBlocks, kThreadsPerBlock, smem_bytes, stream>>>(
        d_vectors, dim,
        d_offsets, d_neighbours, num_nodes,
        d_queries, num_queries,
        k, ef, metric,
        entry_node,
        d_result_ids, d_result_scores,
        d_visited);

    if (pool_owned) {
        // Synchronise before freeing the workspace (pool-based callers own sync)
        cudaStreamSynchronize(stream);
        cudaFree(d_visited);
    }
}

} // namespace cuda
} // namespace themis
