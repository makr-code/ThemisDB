// HIP HNSW Graph Traversal Kernel
// ThemisDB Hardware Acceleration — AMD ROCm/HIP backend
//
// Implements the GPU-side HNSW (Hierarchical Navigable Small World) approximate
// nearest-neighbour search kernel.  One HIP thread processes one query.  The
// thread traverses the pre-uploaded CSR bottom-layer graph using an ef-limited
// greedy best-first strategy, mirroring the CPU path in
// src/index/cuda_hnsw_graph_traversal.cpp.
//
// Entry point (C++ linkage, namespace themis::hip):
//   themis::hip::launchHipHnswSearchKernel(...)
//
// This translation unit is compiled only when THEMIS_ENABLE_HIP=ON.
//
// This file is the HIP-native equivalent of
// src/acceleration/cuda/cuda_hnsw_kernels.cu.

#include <hip/hip_runtime.h>
#include <float.h>
#include <stdint.h>
#include <assert.h>  // for host-side assert() used in launchHipHnswSearchKernel

// Maximum ef value supported by the kernel (candidate list size cap).
// Queries with ef > kMaxEf are clamped to kMaxEf.
static constexpr uint32_t kMaxEf = 512u;

// Maximum k supported by a single-pass kernel launch.
// For k > kMaxK the launcher sets the caller-supplied overflow flag and leaves
// outputs untouched.  The caller is expected to fall back to a multi-pass
// strategy (see launchHipHnswSearchKernel).
static constexpr uint32_t kMaxK = 1024u;

// Available LDS (Local Data Store) per block — conservative 48 KB default.
// Matches the minimum guaranteed by GCN/RDNA compute units and covers all
// common RDNA2/RDNA3 configurations.
static constexpr uint32_t kSharedMemBytes = 49152u;

namespace themis {
namespace hip {

// =============================================================================
// Distance helpers (device functions)
// =============================================================================

/**
 * Compute squared L2 distance between two vectors of length @p dim.
 *
 * @param a    First vector (device pointer)
 * @param b    Second vector (device pointer)
 * @param dim  Vector dimensionality
 * @return     Squared L2 distance (no sqrtf; consistent with HIP ANN backend)
 */
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

/**
 * Compute cosine distance (1 − cosine_similarity) between two vectors.
 *
 * @param a    First vector (device pointer)
 * @param b    Second vector (device pointer)
 * @param dim  Vector dimensionality
 * @return     Cosine distance in [0, 2]; returns 1.0 when either vector is zero.
 */
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

/**
 * Compute inner-product distance = −dot(a, b).
 * Negated so that smaller values correspond to more similar vectors.
 *
 * @param a    First vector (device pointer)
 * @param b    Second vector (device pointer)
 * @param dim  Vector dimensionality
 * @return     Negative inner product
 */
__device__ __forceinline__ float deviceDot(
    const float* __restrict__ a,
    const float* __restrict__ b,
    uint32_t dim)
{
    float dot = 0.0f;
    for (uint32_t i = 0; i < dim; ++i) dot += a[i] * b[i];
    return -dot;  // Negate: smaller value → more similar
}

/**
 * Dispatch distance computation based on the @p metric selector.
 *
 * @param a      First vector
 * @param b      Second vector
 * @param dim    Vector dimensionality
 * @param metric 0=L2 (squared), 1=Cosine, 2=Dot (negated)
 * @return       Distance value
 */
__device__ __forceinline__ float deviceDist(
    const float* __restrict__ a,
    const float* __restrict__ b,
    uint32_t dim,
    uint8_t metric)
{
    switch (metric) {
        case 1:  return deviceCosine(a, b, dim);
        case 2:  return deviceDot(a, b, dim);
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

/**
 * Sift down the element at position @p top in a max-heap of @p size elements.
 *
 * @param dist_arr  Distance values array
 * @param id_arr    ID values array
 * @param top       Index of the element to sift down
 * @param size      Current heap size
 */
__device__ __forceinline__ void heapSiftDown(
    float* __restrict__   dist_arr,
    int32_t* __restrict__ id_arr,
    int top,
    int size)
{
    while (true) {
        int largest = top;
        int left    = 2 * top + 1;
        int right   = 2 * top + 2;
        if (left  < size && dist_arr[left]  > dist_arr[largest]) largest = left;
        if (right < size && dist_arr[right] > dist_arr[largest]) largest = right;
        if (largest == top) break;
        float  td = dist_arr[top];  dist_arr[top]  = dist_arr[largest];  dist_arr[largest]  = td;
        int32_t ti = id_arr[top];  id_arr[top]   = id_arr[largest];    id_arr[largest]   = ti;
        top = largest;
    }
}

/**
 * Insert (@p d, @p id) into a max-heap of capacity @p cap, evicting the
 * current maximum when the heap is full.
 *
 * @param dist_arr  Distance values array (heap storage)
 * @param id_arr    ID values array (heap storage)
 * @param size_ptr  Pointer to current heap size (updated in-place)
 * @param cap       Maximum heap capacity
 * @param d         Distance of the new candidate
 * @param id        ID of the new candidate
 * @return          true if the entry was inserted
 */
__device__ __forceinline__ bool heapPushCapped(
    float* __restrict__   dist_arr,
    int32_t* __restrict__ id_arr,
    int*                  size_ptr,
    int                   cap,
    float                 d,
    int32_t               id)
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
// dynamically allocated LDS (shared memory) for the per-thread result set so
// that the result buffer size matches the requested k at runtime.
// Visited tracking uses a 1-bit-per-node bitset in the caller-provided device
// buffer (per-query slice zeroed by the thread itself before traversal).
//
// Dynamic shared memory layout (per block, passed as third <<< >>> argument):
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
//   metric        — 0=L2 (squared), 1=Cosine, 2=Dot (negated)
//   entry_node    — starting node for traversal (0 = default)
//   d_result_ids  — output IDs    [num_queries × k], device memory
//   d_result_scores — output scores [num_queries × k], device memory
//   d_visited     — per-query 1-bit-per-node visited bitset
//                   [num_queries × ceil(num_nodes/8)] bytes, device memory;
//                   caller allocates once, reused across launches.
// =============================================================================

/**
 * @brief HNSW greedy best-first search kernel (one thread per query).
 *
 * Performs ef-limited search on the bottom-layer CSR graph.  Writes top-k
 * results sorted ascending by distance to device output buffers.
 *
 * @param d_vectors       Database vector matrix [num_nodes × dim]
 * @param dim             Vector dimensionality
 * @param d_offsets       CSR row offsets [num_nodes + 1]
 * @param d_neighbours    CSR column indices
 * @param num_nodes       Number of graph nodes
 * @param d_queries       Query matrix [num_queries × dim]
 * @param num_queries     Number of queries
 * @param k               Results per query (≤ kMaxK)
 * @param ef              Candidate list size for search (≤ kMaxEf)
 * @param metric          Distance metric selector (0=L2, 1=Cosine, 2=Dot)
 * @param entry_node      Graph entry point for traversal
 * @param d_result_ids    Output IDs [num_queries × k]
 * @param d_result_scores Output scores [num_queries × k]
 * @param d_visited       Per-query visited bitset (caller-allocated, reused)
 */
__global__ void hipHnswSearchKernel(
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
    // ── Dynamic LDS: per-thread result buffers ────────────────────────────────
    // Layout: [ blockDim.x * k floats ] [ blockDim.x * k int32_t ]
    extern __shared__ float s_res_dist_base[];
    int32_t* s_res_id_base = (int32_t*)(s_res_dist_base + blockDim.x * k);

    float*   res_dist = s_res_dist_base + threadIdx.x * k;
    int32_t* res_id   = s_res_id_base   + threadIdx.x * k;

    const uint32_t qi = blockIdx.x * blockDim.x + threadIdx.x;
    if (qi >= num_queries) return;

    // Bytes required for the per-query 1-bit-per-node bitset
    const uint32_t visited_bytes = (num_nodes + 7u) / 8u;

    const float*  query     = d_queries + qi * dim;
    uint8_t*      visited   = d_visited + (size_t)qi * visited_bytes;
    int64_t*      out_ids   = d_result_ids    + (size_t)qi * k;
    float*        out_dists = d_result_scores + (size_t)qi * k;

    // ── Thread-local candidate list (registers / L1 cache) ───────────────────
    float   cand_dist[kMaxEf];
    int32_t cand_id  [kMaxEf];
    int     cand_size = 0;
    int     res_size  = 0;

    const uint32_t eff_ef = (ef < kMaxEf) ? ef : kMaxEf;
    const uint32_t eff_k  = (k  < kMaxK)  ? k  : kMaxK;

    // Clamp entry_node to valid range
    const uint32_t safe_entry = (entry_node < num_nodes) ? entry_node : 0u;

    // Initialise visited bitset (zero all bytes for this query's slice)
    for (uint32_t i = 0; i < visited_bytes; ++i) visited[i] = 0u;

    // Handle empty graph
    if (num_nodes == 0) {
        for (uint32_t i = 0; i < eff_k; ++i) {
            out_ids[i]   = -1;
            out_dists[i] = FLT_MAX;
        }
        return;
    }

    // ── Seed with entry point ─────────────────────────────────────────────────
    int32_t entry   = static_cast<int32_t>(safe_entry);
    float   entry_d = deviceDist(query, d_vectors + (size_t)entry * dim, dim, metric);

    visited[static_cast<uint32_t>(entry) >> 3u] |= (1u << (static_cast<uint32_t>(entry) & 7u));

    heapPushCapped(cand_dist, cand_id, &cand_size, static_cast<int>(eff_ef), entry_d, entry);
    heapPushCapped(res_dist,  res_id,  &res_size,  static_cast<int>(eff_ef), entry_d, entry);

    // ── ef-limited greedy best-first search ──────────────────────────────────
    while (cand_size > 0) {
        // Find the minimum-distance candidate (linear scan; cand_size ≤ kMaxEf)
        int   min_pos  = 0;
        float min_dist = cand_dist[0];
        for (int i = 1; i < cand_size; ++i) {
            if (cand_dist[i] < min_dist) {
                min_dist = cand_dist[i];
                min_pos  = i;
            }
        }

        // Early exit: all remaining candidates are worse than the worst result
        if (res_size >= static_cast<int>(eff_ef) && min_dist > res_dist[0]) break;

        int32_t cn = cand_id[min_pos];
        // Remove from candidates (swap with last element)
        cand_dist[min_pos] = cand_dist[cand_size - 1];
        cand_id  [min_pos] = cand_id  [cand_size - 1];
        --cand_size;

        // Explore neighbours of cn via the CSR adjacency list
        int32_t start = d_offsets[static_cast<uint32_t>(cn)];
        int32_t end   = d_offsets[static_cast<uint32_t>(cn) + 1];
        for (int32_t ni = start; ni < end; ++ni) {
            int32_t nb = d_neighbours[ni];
            if (nb < 0 || static_cast<uint32_t>(nb) >= num_nodes) continue;

            uint32_t nb_u = static_cast<uint32_t>(nb);
            if (visited[nb_u >> 3u] & (1u << (nb_u & 7u))) continue;
            visited[nb_u >> 3u] |= (1u << (nb_u & 7u));

            float nd = deviceDist(query, d_vectors + (size_t)nb * dim, dim, metric);

            if (res_size < static_cast<int>(eff_ef) || nd < res_dist[0]) {
                heapPushCapped(res_dist, res_id, &res_size,  static_cast<int>(eff_ef), nd, nb);
                if (cand_size < static_cast<int>(eff_ef)) {
                    heapPushCapped(cand_dist, cand_id, &cand_size, static_cast<int>(eff_ef), nd, nb);
                }
            }
        }
    }

    // ── Sort result set ascending and write top-k to output ──────────────────
    const uint32_t out_count = (static_cast<uint32_t>(res_size) < eff_k)
                               ? static_cast<uint32_t>(res_size) : eff_k;

    // Insertion sort (res_size ≤ kMaxEf = 512; acceptable for this range)
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
// Helper: compute threads-per-block for a given k so that the dynamic
// LDS required for the result buffers fits within kSharedMemBytes.
// =============================================================================

/// @brief Compute threads-per-block such that dynamic LDS fits in kSharedMemBytes.
/// @param k Number of results per query.
/// @return  Power-of-two thread count ≤ 128 and ≥ 1.
static uint32_t computeThreadsPerBlock(uint32_t k) {
    const uint32_t smem_per_thread = k * static_cast<uint32_t>(sizeof(float) + sizeof(int32_t));
    if (smem_per_thread == 0) return 128u;
    uint32_t threads = kSharedMemBytes / smem_per_thread;
    if (threads == 0) return 1u;
    threads = threads < 128u ? threads : 128u;
    // Round down to nearest power of 2 using __builtin_clz (Clang/ROCm)
    return 1u << (31u - static_cast<uint32_t>(__builtin_clz(threads)));
}

// =============================================================================
// Public launcher (C++ linkage, called from HipHnswTraversalEngine)
//
// d_visited — caller-provided device buffer sized at least
//   num_queries × ceil(num_nodes / 8) bytes.
//   The caller is responsible for allocating this pool once during buildIndex()
//   and reusing it across launches.  Each kernel thread zeroes its own
//   per-query slice before traversal, so the pool does NOT need to be zeroed
//   between launches by the caller.
//
//   Passing nullptr is not supported; the launcher sets h_overflow = true
//   and returns without launching when d_visited is null.
// =============================================================================

/**
 * @brief Launch the HIP HNSW search kernel (C++ linkage).
 *
 * @param d_vectors       Database vector matrix [num_nodes × dim], device memory
 * @param dim             Vector dimensionality
 * @param d_offsets       CSR row offsets [num_nodes + 1], device memory
 * @param d_neighbours    CSR column indices, device memory
 * @param num_nodes       Number of graph nodes
 * @param d_queries       Query matrix [num_queries × dim], device memory
 * @param num_queries     Number of queries
 * @param k               Results per query; must be ≤ kMaxK
 * @param ef              Candidate list size; clamped to kMaxEf
 * @param metric          Distance metric (0=L2, 1=Cosine, 2=Dot)
 * @param entry_node      Graph entry point
 * @param d_result_ids    Output IDs [num_queries × k], device memory
 * @param d_result_scores Output scores [num_queries × k], device memory
 * @param stream          HIP stream for asynchronous execution
 * @param h_overflow      Set to true when k > kMaxK or d_visited is null;
 *                        must not be nullptr
 * @param d_visited       Per-query visited bitset, caller-allocated device memory
 */
void launchHipHnswSearchKernel(
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
    hipStream_t    stream,
    bool*          h_overflow,
    uint8_t*       d_visited)
{
    if (num_queries == 0 || num_nodes == 0 || k == 0) return;

    if (ef > kMaxEf) ef = kMaxEf;

    // k > kMaxK: overflow — do NOT silently truncate; signal the caller.
    if (k > kMaxK) {
        assert(k <= kMaxK &&
               "launchHipHnswSearchKernel: k > kMaxK — caller must use multi-pass");
        if (h_overflow) *h_overflow = true;
        return;
    }

    // Visited pool must be supplied by the caller (persistent across launches).
    if (d_visited == nullptr) {
        if (h_overflow) *h_overflow = true;
        return;
    }

    if (h_overflow) *h_overflow = false;

    const uint32_t kThreadsPerBlock = computeThreadsPerBlock(k);
    const size_t   smem_bytes = static_cast<size_t>(kThreadsPerBlock) *
                                static_cast<size_t>(k) *
                                (sizeof(float) + sizeof(int32_t));

    const uint32_t numBlocks = (num_queries + kThreadsPerBlock - 1u) / kThreadsPerBlock;

    hipLaunchKernelGGL(hipHnswSearchKernel,
        dim3(numBlocks), dim3(kThreadsPerBlock), smem_bytes, stream,
        d_vectors, dim,
        d_offsets, d_neighbours, num_nodes,
        d_queries, num_queries,
        k, ef, metric,
        entry_node,
        d_result_ids, d_result_scores,
        d_visited);
    // Note: the caller is responsible for hipStreamSynchronize after this launch.
}

} // namespace hip
} // namespace themis
