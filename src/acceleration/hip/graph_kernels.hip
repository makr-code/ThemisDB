// HIP Kernels for Graph Algorithms — BFS and Bellman-Ford Shortest Path
// ThemisDB Hardware Acceleration — AMD ROCm/HIP backend
//
// Provides HIP device kernels for:
//   • Parallel Breadth-First Search (BFS) with level-by-level frontier expansion
//   • Bellman-Ford shortest-path relaxation (naturally parallel across edges)
//
// Adjacency format (both BFS and SP):
//   Flat row-major N×N matrix of uint32_t (adjacency[u*N + v] != 0 means edge u→v).
//   Weights matrix for SP: flat N×N float array (weights[u*N + v] = edge weight).
//
// HIP Graph Capture compatibility:
//   All launcher signatures are fully determined by the workload shape
//   (numVertices, numStarts/numPairs, maxDepth).  Device buffer pointers are
//   stable across replays; only the data in those buffers changes between
//   invocations.  All launchers accept an explicit hipStream_t so the caller
//   can capture them on a dedicated non-blocking stream.
//
// External-C launcher functions are the HIP equivalents of those declared
// in cuda_backend.cpp.
//
// This file is the HIP-native equivalent of src/acceleration/cuda/graph_kernels.cu.

#include <hip/hip_runtime.h>
#include <cstdint>
#include <climits>

namespace themis {
namespace acceleration {
namespace hip_graph {

static constexpr float kInfDistance = 1e38f;

// =============================================================================
// BFS kernels
// =============================================================================

/**
 * Initialise frontier, visited, and depths arrays for a batch of BFS queries.
 *
 * Thread layout: flat 1-D covering numStarts × numVertices cells.
 * Each thread (s, v) resets cell (s, v) and, when v == startVertices[s],
 * seeds the initial frontier.
 *
 * @param startVertices  [numStarts] — start vertex for each BFS query
 * @param frontier_a     [numStarts × numVertices] — initial (depth-0) frontier
 * @param frontier_b     [numStarts × numVertices] — scratch frontier (zeroed here)
 * @param visited        [numStarts × numVertices] — visited flags
 * @param depths         [numStarts × numVertices] — discovery depth per vertex
 * @param numVertices    Number of vertices in the graph
 * @param numStarts      Number of BFS start vertices (batch size)
 */
__global__ void graphBFSInitKernel(
    const uint32_t* __restrict__ startVertices,
    uint32_t* __restrict__ frontier_a,
    uint32_t* __restrict__ frontier_b,
    uint32_t* __restrict__ visited,
    uint32_t* __restrict__ depths,
    int numVertices,
    int numStarts
) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const int s   = tid / numVertices;
    const int v   = tid % numVertices;
    if (s >= numStarts || v >= numVertices) return;

    const int idx = s * numVertices + v;
    frontier_a[idx] = 0;
    frontier_b[idx] = 0;
    visited[idx]    = 0;
    depths[idx]     = UINT32_MAX;

    const uint32_t sv = startVertices[s];
    if (sv < (uint32_t)numVertices && (uint32_t)v == sv) {
        frontier_a[idx] = 1;
        visited[idx]    = 1;
        depths[idx]     = 0;
    }
}

/**
 * Expand BFS frontier by one level.
 *
 * Thread layout: flat 1-D covering numStarts × numVertices cells.
 * Each thread (s, v) checks whether vertex v is reachable from any u in
 * frontier_in for query s.  If yes, and v has not been visited, it is added
 * to frontier_out and its depth is recorded.
 *
 * @param adjacency    [numVertices × numVertices] — dense adjacency matrix
 * @param frontier_in  [numStarts × numVertices] — current BFS frontier
 * @param frontier_out [numStarts × numVertices] — next BFS frontier (output)
 * @param visited      [numStarts × numVertices] — visited flags (updated)
 * @param depths       [numStarts × numVertices] — discovery depths (updated)
 * @param numVertices  Number of vertices in the graph
 * @param numStarts    Number of BFS start vertices (batch size)
 * @param currentDepth Current BFS depth being written
 */
__global__ void graphBFSExpandKernel(
    const uint32_t* __restrict__ adjacency,
    const uint32_t* __restrict__ frontier_in,
    uint32_t* __restrict__ frontier_out,
    uint32_t* __restrict__ visited,
    uint32_t* __restrict__ depths,
    int numVertices,
    int numStarts,
    uint32_t currentDepth
) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const int s   = tid / numVertices;
    const int v   = tid % numVertices;
    if (s >= numStarts || v >= numVertices) return;

    const int cell = s * numVertices + v;
    if (visited[cell]) return;  // already discovered

    // Check if any frontier vertex u has an edge to v
    for (int u = 0; u < numVertices; ++u) {
        if (frontier_in[s * numVertices + u] &&
            adjacency[u * numVertices + v] != 0) {
            // First thread to win the atomicCAS sets visited and records depth
            if (atomicCAS(&visited[cell], 0u, 1u) == 0u) {
                frontier_out[cell] = 1;
                depths[cell]       = currentDepth;
            }
            return;
        }
    }
}

/**
 * Gather BFS results: for each query collect all visited vertices (in ascending
 * vertex-index order) into a flat output array.
 *
 * Thread layout: one thread per query (numStarts threads).
 *
 * @param visited          [numStarts × numVertices] — visited flags
 * @param numVertices      Number of vertices in the graph
 * @param numStarts        Number of BFS start vertices (batch size)
 * @param result_vertices  [numStarts × numVertices] — output vertex indices
 * @param result_sizes     [numStarts] — output: count of visited vertices per query
 */
__global__ void graphBFSGatherKernel(
    const uint32_t* __restrict__ visited,
    int numVertices,
    int numStarts,
    uint32_t* __restrict__ result_vertices,
    int*      __restrict__ result_sizes
) {
    const int s = blockIdx.x * blockDim.x + threadIdx.x;
    if (s >= numStarts) return;

    int cnt = 0;
    for (int v = 0; v < numVertices; ++v) {
        if (visited[s * numVertices + v]) {
            result_vertices[s * numVertices + cnt] = static_cast<uint32_t>(v);
            ++cnt;
        }
    }
    result_sizes[s] = cnt;
}

// =============================================================================
// Bellman-Ford shortest-path kernels
// =============================================================================

/**
 * Initialise distance and predecessor arrays for a batch of shortest-path
 * queries.
 *
 * Thread layout: flat 1-D covering numPairs × numVertices cells.
 *
 * @param startVertices  [numPairs] — source vertex for each SP query
 * @param distances      [numPairs × numVertices] — output distances (INF except source)
 * @param predecessors   [numPairs × numVertices] — output predecessors (-1 initially)
 * @param numVertices    Number of vertices in the graph
 * @param numPairs       Number of shortest-path query pairs (batch size)
 */
__global__ void graphBFInitDistancesKernel(
    const uint32_t* __restrict__ startVertices,
    float* __restrict__ distances,
    int*   __restrict__ predecessors,
    int numVertices,
    int numPairs
) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const int p   = tid / numVertices;
    const int v   = tid % numVertices;
    if (p >= numPairs || v >= numVertices) return;

    const int idx = p * numVertices + v;
    distances[idx]    = (startVertices[p] == (uint32_t)v) ? 0.0f : kInfDistance;
    predecessors[idx] = -1;
}

/**
 * Bellman-Ford relaxation — one pass over all edges.
 *
 * Thread layout: flat 1-D covering numPairs × numVertices threads.
 * Thread (p, u) relaxes all outgoing edges from vertex u for SP query p.
 *
 * Atomic float CAS loop ensures race-free minimum updates across threads
 * that converge on the same destination vertex v.
 *
 * @param adjacency    [numVertices × numVertices] — dense adjacency matrix
 * @param weights      [numVertices × numVertices] — edge weights
 * @param distances    [numPairs × numVertices] — distances (updated in-place)
 * @param predecessors [numPairs × numVertices] — predecessors (updated in-place)
 * @param numVertices  Number of vertices in the graph
 * @param numPairs     Number of shortest-path query pairs (batch size)
 */
__global__ void graphBFRelaxKernel(
    const uint32_t* __restrict__ adjacency,
    const float*    __restrict__ weights,
    float* __restrict__ distances,
    int*   __restrict__ predecessors,
    int numVertices,
    int numPairs
) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const int p   = tid / numVertices;
    const int u   = tid % numVertices;
    if (p >= numPairs || u >= numVertices) return;

    const float dist_u = distances[p * numVertices + u];
    if (dist_u >= kInfDistance) return;  // u not yet reachable

    for (int v = 0; v < numVertices; ++v) {
        if (adjacency[u * numVertices + v] == 0) continue;

        const float w     = weights[u * numVertices + v];
        const float new_d = dist_u + w;

        // Atomic float min via CAS loop using int reinterpretation
        int* addr = reinterpret_cast<int*>(&distances[p * numVertices + v]);
        int  old_bits, new_bits;
        do {
            old_bits = *addr;
            float old_dist = __int_as_float(old_bits);
            if (old_dist <= new_d) break;
            new_bits = __float_as_int(new_d);
        } while (atomicCAS(addr, old_bits, new_bits) != old_bits);

        // Update predecessor if this thread lowered the distance
        if (__int_as_float(*addr) == new_d) {
            predecessors[p * numVertices + v] = u;
        }
    }
}

} // namespace hip_graph
} // namespace acceleration
} // namespace themis

// =============================================================================
// External-C launchers — called from hip_backend.cpp or HipGraphBackend
// =============================================================================

using namespace themis::acceleration::hip_graph;

static int kBFSBlockDim = 256;

/**
 * Update the block size used by BFS/Bellman-Ford kernel launchers.
 *
 * Called from HipGraphBackend::initialize() with the value returned by
 * hipOccupancyMaxPotentialBlockSize().  Falls back to 256 when not called.
 *
 * @param blockDim  Number of threads per block (must be a multiple of 32 or 64
 *                  depending on the wavefront size, and ≤ the device's
 *                  maxThreadsPerBlock).
 */
extern "C" void hipSetGraphBFSBlockDim(int blockDim) {
    kBFSBlockDim = blockDim;
}

/**
 * Query the HIP occupancy API for the BFS init kernel and update kBFSBlockDim
 * with the device-optimal block size.
 *
 * Called by HipGraphBackend::initialize() so all BFS/SP launchers use the
 * tuned block size from the first dispatch.
 *
 * @return  The occupancy-tuned block size (also stored in kBFSBlockDim).
 */
extern "C" int hipTuneGraphBFSBlockDim() {
    int minGridSize   = 0;
    int tunedBlockDim = 256;
    hipError_t err = hipOccupancyMaxPotentialBlockSize(
        &minGridSize, &tunedBlockDim, graphBFSInitKernel, 0, 0);
    if (err == hipSuccess && tunedBlockDim > 0) {
        // Round down to a wavefront multiple (64 for GCN/RDNA) and clamp.
        const int warpSize = 64;  // conservative: matches both wave64 and wave32
        tunedBlockDim = (tunedBlockDim / warpSize) * warpSize;
        if (tunedBlockDim < warpSize) tunedBlockDim = warpSize;
        kBFSBlockDim = tunedBlockDim;
    }
    return kBFSBlockDim;
}

/**
 * Launch the BFS initialisation kernel on the given HIP stream.
 *
 * @param d_startVertices  [numStarts] device pointer to BFS source vertices
 * @param d_frontier_a     [numStarts × numVertices] initial frontier buffer
 * @param d_frontier_b     [numStarts × numVertices] scratch frontier buffer
 * @param d_visited        [numStarts × numVertices] visited flags
 * @param d_depths         [numStarts × numVertices] discovery depths
 * @param numVertices      Number of graph vertices
 * @param numStarts        Number of BFS queries in the batch
 * @param stream           HIP stream for asynchronous execution
 */
extern "C" void hipLaunchGraphBFSInitKernel(
    const uint32_t* d_startVertices,
    uint32_t*       d_frontier_a,
    uint32_t*       d_frontier_b,
    uint32_t*       d_visited,
    uint32_t*       d_depths,
    int             numVertices,
    int             numStarts,
    hipStream_t     stream
) {
    const int total  = numStarts * numVertices;
    const int blocks = (total + kBFSBlockDim - 1) / kBFSBlockDim;
    hipLaunchKernelGGL(graphBFSInitKernel,
        dim3(blocks), dim3(kBFSBlockDim), 0, stream,
        d_startVertices, d_frontier_a, d_frontier_b,
        d_visited, d_depths, numVertices, numStarts);
}

/**
 * Launch the BFS frontier-expansion kernel on the given HIP stream.
 *
 * @param d_adjacency      [numVertices × numVertices] dense adjacency matrix
 * @param d_frontier_in    [numStarts × numVertices] current BFS frontier
 * @param d_frontier_out   [numStarts × numVertices] next BFS frontier (output)
 * @param d_visited        [numStarts × numVertices] visited flags (updated)
 * @param d_depths         [numStarts × numVertices] discovery depths (updated)
 * @param numVertices      Number of graph vertices
 * @param numStarts        Number of BFS queries in the batch
 * @param currentDepth     BFS level currently being expanded
 * @param stream           HIP stream for asynchronous execution
 */
extern "C" void hipLaunchGraphBFSExpandKernel(
    const uint32_t* d_adjacency,
    const uint32_t* d_frontier_in,
    uint32_t*       d_frontier_out,
    uint32_t*       d_visited,
    uint32_t*       d_depths,
    int             numVertices,
    int             numStarts,
    uint32_t        currentDepth,
    hipStream_t     stream
) {
    const int total  = numStarts * numVertices;
    const int blocks = (total + kBFSBlockDim - 1) / kBFSBlockDim;
    hipLaunchKernelGGL(graphBFSExpandKernel,
        dim3(blocks), dim3(kBFSBlockDim), 0, stream,
        d_adjacency, d_frontier_in, d_frontier_out,
        d_visited, d_depths, numVertices, numStarts, currentDepth);
}

/**
 * Launch the BFS result-gather kernel on the given HIP stream.
 *
 * @param d_visited          [numStarts × numVertices] visited flags
 * @param numVertices        Number of graph vertices
 * @param numStarts          Number of BFS queries in the batch
 * @param d_result_vertices  [numStarts × numVertices] output vertex indices
 * @param d_result_sizes     [numStarts] output: count of visited vertices per query
 * @param stream             HIP stream for asynchronous execution
 */
extern "C" void hipLaunchGraphBFSGatherKernel(
    const uint32_t* d_visited,
    int             numVertices,
    int             numStarts,
    uint32_t*       d_result_vertices,
    int*            d_result_sizes,
    hipStream_t     stream
) {
    const int blocks = (numStarts + kBFSBlockDim - 1) / kBFSBlockDim;
    hipLaunchKernelGGL(graphBFSGatherKernel,
        dim3(blocks), dim3(kBFSBlockDim), 0, stream,
        d_visited, numVertices, numStarts, d_result_vertices, d_result_sizes);
}

/**
 * Launch the Bellman-Ford distance-initialisation kernel on the given HIP stream.
 *
 * @param d_startVertices  [numPairs] source vertex for each SP query
 * @param d_distances      [numPairs × numVertices] distances (INF except source)
 * @param d_predecessors   [numPairs × numVertices] predecessors (-1 initially)
 * @param numVertices      Number of graph vertices
 * @param numPairs         Number of shortest-path queries in the batch
 * @param stream           HIP stream for asynchronous execution
 */
extern "C" void hipLaunchGraphBFInitDistancesKernel(
    const uint32_t* d_startVertices,
    float*          d_distances,
    int*            d_predecessors,
    int             numVertices,
    int             numPairs,
    hipStream_t     stream
) {
    const int total  = numPairs * numVertices;
    const int blocks = (total + kBFSBlockDim - 1) / kBFSBlockDim;
    hipLaunchKernelGGL(graphBFInitDistancesKernel,
        dim3(blocks), dim3(kBFSBlockDim), 0, stream,
        d_startVertices, d_distances, d_predecessors, numVertices, numPairs);
}

/**
 * Launch the Bellman-Ford edge-relaxation kernel on the given HIP stream.
 *
 * @param d_adjacency    [numVertices × numVertices] dense adjacency matrix
 * @param d_weights      [numVertices × numVertices] edge weights
 * @param d_distances    [numPairs × numVertices] distances (updated in-place)
 * @param d_predecessors [numPairs × numVertices] predecessors (updated in-place)
 * @param numVertices    Number of graph vertices
 * @param numPairs       Number of shortest-path queries in the batch
 * @param stream         HIP stream for asynchronous execution
 */
extern "C" void hipLaunchGraphBFRelaxKernel(
    const uint32_t* d_adjacency,
    const float*    d_weights,
    float*          d_distances,
    int*            d_predecessors,
    int             numVertices,
    int             numPairs,
    hipStream_t     stream
) {
    const int total  = numPairs * numVertices;
    const int blocks = (total + kBFSBlockDim - 1) / kBFSBlockDim;
    hipLaunchKernelGGL(graphBFRelaxKernel,
        dim3(blocks), dim3(kBFSBlockDim), 0, stream,
        d_adjacency, d_weights, d_distances, d_predecessors,
        numVertices, numPairs);
}
