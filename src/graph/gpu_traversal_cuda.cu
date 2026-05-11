#include "graph/gpu_traversal.h"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace themis {
namespace graph {
namespace cuda_impl {

namespace {

// ---------------------------------------------------------------------------
// Kernel block sizes
// ---------------------------------------------------------------------------
constexpr uint32_t kBfsThreadsPerBlock = 256;
constexpr uint32_t kDfsThreadsPerBlock = 256;

// ---------------------------------------------------------------------------
// BFS: parallel frontier expansion
// Each thread handles one vertex from the current frontier and atomically
// claims unvisited neighbours for the next frontier.
// ---------------------------------------------------------------------------
__global__ void bfsExpandKernel(
    const uint32_t* row_offsets,
    const uint32_t* column_indices,
    const uint32_t* frontier,
    uint32_t frontier_size,
    int current_depth,
    int* distances,
    const uint8_t* forbidden_mask,
    uint32_t* next_frontier,
    uint32_t* next_size) {
    const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= frontier_size) {
        return;
    }

    const uint32_t vertex = frontier[idx];
    const uint32_t begin = row_offsets[vertex];
    const uint32_t end = row_offsets[vertex + 1];

    for (uint32_t edge = begin; edge < end; ++edge) {
        const uint32_t neighbor = column_indices[edge];
        if (forbidden_mask[neighbor] != 0) {
            continue;
        }

        if (atomicCAS(&distances[neighbor], -1, current_depth + 1) == -1) {
            const uint32_t out = atomicAdd(next_size, 1u);
            next_frontier[out] = neighbor;
        }
    }
}

// ---------------------------------------------------------------------------
// DFS: parallel wave-expansion kernel
//
// Genuinely parallel DFS approximation via iterative wavefront expansion.
// Each wave contains all vertices "seen" at the current depth.  All vertices
// in a wave are processed concurrently by GPU threads:
//   1. Each thread atomically assigns a discovery order to its vertex.
//   2. Each thread then expands the vertex's neighbours: unvisited neighbours
//      are claimed with atomicExch(&visited[nb], 1u) and pushed into the
//      next-wave buffer.
//
// This gives O(diameter) kernel launches and true intra-wave parallelism
// without device-side dynamic memory allocation.
//
// Note on visited[]: uint32_t is used (not uint8_t) because CUDA's
// atomicExch only supports int/unsigned int/unsigned long long/float.
// ---------------------------------------------------------------------------
__global__ void dfsWaveExpandKernel(
    const uint32_t* row_offsets,
    const uint32_t* column_indices,
    uint32_t wave_size,
    const uint32_t* current_wave,
    int current_depth,
    int max_depth,
    const uint8_t* forbidden_mask,
    uint32_t* visited,
    int* discovery_order,
    uint32_t* next_wave,
    uint32_t* next_size,
    uint32_t* order_counter,
    uint32_t* nodes_counter,
    uint32_t* edges_counter,
    int max_results,
    int* truncated) {

    const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= wave_size) {
        return;
    }

    const uint32_t vertex = current_wave[idx];

    // Assign a globally-unique, monotonically-increasing discovery order.
    const uint32_t my_order = atomicAdd(order_counter, 1u);
    discovery_order[vertex] = static_cast<int>(my_order);
    atomicAdd(nodes_counter, 1u);

    // Signal truncation when the result cap is reached so future waves
    // skip further work.  Truncation is also checked on the host before
    // each kernel launch so this flag principally helps intra-wave threads.
    if (max_results > 0 && static_cast<int>(my_order + 1) >= max_results) {
        atomicExch(truncated, 1);
        return;
    }
    if (current_depth >= max_depth) {
        return;
    }

    // Expand outgoing edges and claim unvisited, allowed neighbours.
    const uint32_t begin = row_offsets[vertex];
    const uint32_t end = row_offsets[vertex + 1];
    for (uint32_t e = begin; e < end; ++e) {
        const uint32_t nb = column_indices[e];
        atomicAdd(edges_counter, 1u);
        if (forbidden_mask[nb] != 0) {
            continue;
        }
        // atomicExch returns the old value; 0u → we are first to claim nb.
        if (atomicExch(&visited[nb], 1u) == 0u) {
            const uint32_t out = atomicAdd(next_size, 1u);
            next_wave[out] = nb;
        }
    }
}

} // namespace

bool runBFSCuda(
    const std::vector<uint32_t>& row_offsets,
    const std::vector<uint32_t>& column_indices,
    uint32_t start_id,
    const std::vector<uint8_t>& forbidden_mask,
    const GPUGraphTraversal::Config& config,
    GPUGraphTraversal::TraversalResult& result,
    std::vector<int>& out_distances) {
    const auto vertex_count = static_cast<uint32_t>(row_offsets.size() - 1);
    if (vertex_count == 0) {
        return false;
    }

    uint32_t* d_row_offsets = nullptr;
    uint32_t* d_col_indices = nullptr;
    int* d_distances = nullptr;
    uint8_t* d_forbidden = nullptr;
    uint32_t* d_frontier = nullptr;
    uint32_t* d_next_frontier = nullptr;
    uint32_t* d_next_size = nullptr;

    std::vector<int> host_dist(vertex_count, -1);
    host_dist[start_id] = 0;

    auto cleanup = [&]() {
        if (d_row_offsets) cudaFree(d_row_offsets);
        if (d_col_indices) cudaFree(d_col_indices);
        if (d_distances) cudaFree(d_distances);
        if (d_forbidden) cudaFree(d_forbidden);
        if (d_frontier) cudaFree(d_frontier);
        if (d_next_frontier) cudaFree(d_next_frontier);
        if (d_next_size) cudaFree(d_next_size);
    };

    if (cudaMalloc(&d_row_offsets, sizeof(uint32_t) * row_offsets.size()) != cudaSuccess) {
        return false;
    }
    if (cudaMalloc(&d_col_indices, sizeof(uint32_t) * column_indices.size()) != cudaSuccess ||
        cudaMalloc(&d_distances, sizeof(int) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_forbidden, sizeof(uint8_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_frontier, sizeof(uint32_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_next_frontier, sizeof(uint32_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_next_size, sizeof(uint32_t)) != cudaSuccess) {
        cleanup();
        return false;
    }

    if (cudaMemcpy(d_row_offsets, row_offsets.data(),
                   sizeof(uint32_t) * row_offsets.size(),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_col_indices, column_indices.data(),
                   sizeof(uint32_t) * column_indices.size(),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_distances, host_dist.data(), sizeof(int) * vertex_count,
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_forbidden, forbidden_mask.data(),
                   sizeof(uint8_t) * vertex_count,
                   cudaMemcpyHostToDevice) != cudaSuccess) {
        cleanup();
        return false;
    }

    std::vector<uint32_t> frontier{start_id};
    int current_depth = 0;

    while (!frontier.empty() && current_depth < config.max_depth) {
        if (config.max_results > 0 &&
            result.visited_vertices.size() >= config.max_results) {
            result.truncated = true;
            break;
        }

        const auto frontier_size = static_cast<uint32_t>(frontier.size());
        if (cudaMemcpy(d_frontier, frontier.data(),
                       sizeof(uint32_t) * frontier_size,
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            cleanup();
            return false;
        }

        uint32_t zero = 0;
        if (cudaMemcpy(d_next_size, &zero, sizeof(uint32_t),
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            cleanup();
            return false;
        }

        const uint32_t blocks =
            (frontier_size + kBfsThreadsPerBlock - 1) / kBfsThreadsPerBlock;
        bfsExpandKernel<<<blocks, kBfsThreadsPerBlock>>>(
            d_row_offsets, d_col_indices, d_frontier, frontier_size, current_depth,
            d_distances, d_forbidden, d_next_frontier, d_next_size);

        if (cudaDeviceSynchronize() != cudaSuccess) {
            cleanup();
            return false;
        }

        uint32_t next_size = 0;
        if (cudaMemcpy(&next_size, d_next_size, sizeof(uint32_t),
                       cudaMemcpyDeviceToHost) != cudaSuccess) {
            cleanup();
            return false;
        }

        frontier.assign(next_size, 0);
        if (next_size > 0 &&
            cudaMemcpy(frontier.data(), d_next_frontier,
                       sizeof(uint32_t) * next_size,
                       cudaMemcpyDeviceToHost) != cudaSuccess) {
            cleanup();
            return false;
        }

        ++current_depth;
    }

    if (cudaMemcpy(host_dist.data(), d_distances, sizeof(int) * vertex_count,
                   cudaMemcpyDeviceToHost) != cudaSuccess) {
        cleanup();
        return false;
    }

    cleanup();
    out_distances = std::move(host_dist);
    return true;
}

bool runDFSCuda(
    const std::vector<uint32_t>& row_offsets,
    const std::vector<uint32_t>& column_indices,
    uint32_t start_id,
    const std::vector<uint8_t>& forbidden_mask,
    const GPUGraphTraversal::Config& config,
    GPUGraphTraversal::TraversalResult& result,
    std::vector<int>& out_order) {
    const auto vertex_count = static_cast<uint32_t>(row_offsets.size() - 1);
    if (vertex_count == 0) {
        return false;
    }

    // -----------------------------------------------------------------------
    // Device allocations
    // -----------------------------------------------------------------------
    uint32_t* d_row_offsets = nullptr;
    uint32_t* d_col_indices = nullptr;
    uint8_t* d_forbidden = nullptr;
    uint32_t* d_visited = nullptr;   // uint32_t: atomicExch supports int/unsigned int/
                                     // unsigned long long/float, but NOT uint8_t
    int* d_order = nullptr;
    uint32_t* d_wave_a = nullptr;  // ping-pong wave buffers
    uint32_t* d_wave_b = nullptr;
    uint32_t* d_next_size = nullptr;
    uint32_t* d_order_counter = nullptr;
    uint32_t* d_nodes_counter = nullptr;
    uint32_t* d_edges_counter = nullptr;
    int* d_truncated = nullptr;

    std::vector<int> host_order(vertex_count, -1);

    auto cleanup = [&]() {
        if (d_row_offsets) cudaFree(d_row_offsets);
        if (d_col_indices) cudaFree(d_col_indices);
        if (d_forbidden) cudaFree(d_forbidden);
        if (d_visited) cudaFree(d_visited);
        if (d_order) cudaFree(d_order);
        if (d_wave_a) cudaFree(d_wave_a);
        if (d_wave_b) cudaFree(d_wave_b);
        if (d_next_size) cudaFree(d_next_size);
        if (d_order_counter) cudaFree(d_order_counter);
        if (d_nodes_counter) cudaFree(d_nodes_counter);
        if (d_edges_counter) cudaFree(d_edges_counter);
        if (d_truncated) cudaFree(d_truncated);
    };

    if (cudaMalloc(&d_row_offsets, sizeof(uint32_t) * row_offsets.size()) != cudaSuccess ||
        cudaMalloc(&d_col_indices, sizeof(uint32_t) * column_indices.size()) != cudaSuccess ||
        cudaMalloc(&d_forbidden, sizeof(uint8_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_visited, sizeof(uint32_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_order, sizeof(int) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_wave_a, sizeof(uint32_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_wave_b, sizeof(uint32_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_next_size, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_order_counter, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_nodes_counter, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_edges_counter, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_truncated, sizeof(int)) != cudaSuccess) {
        cleanup();
        return false;
    }

    // -----------------------------------------------------------------------
    // Initialise device memory
    // -----------------------------------------------------------------------
    if (cudaMemcpy(d_row_offsets, row_offsets.data(),
                   sizeof(uint32_t) * row_offsets.size(),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_col_indices, column_indices.data(),
                   sizeof(uint32_t) * column_indices.size(),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_forbidden, forbidden_mask.data(),
                   sizeof(uint8_t) * vertex_count,
                   cudaMemcpyHostToDevice) != cudaSuccess) {
        cleanup();
        return false;
    }

    // Zero the visited array (uint32_t, one word per vertex).
    if (cudaMemset(d_visited, 0, sizeof(uint32_t) * vertex_count) != cudaSuccess) {
        cleanup();
        return false;
    }
    // cudaMemset fills byte-by-byte with the low byte of its value argument.
    // 0xFF → each 4-byte int becomes 0xFFFFFFFF = -1 (two's complement),
    // which is the sentinel for "unvisited" in discovery_order.
    if (cudaMemset(d_order, 0xFF, sizeof(int) * vertex_count) != cudaSuccess) {
        cleanup();
        return false;
    }
    uint32_t zero_u32 = 0;
    int zero_i32 = 0;
    if (cudaMemcpy(d_order_counter, &zero_u32, sizeof(uint32_t), cudaMemcpyHostToDevice) !=
            cudaSuccess ||
        cudaMemcpy(d_nodes_counter, &zero_u32, sizeof(uint32_t), cudaMemcpyHostToDevice) !=
            cudaSuccess ||
        cudaMemcpy(d_edges_counter, &zero_u32, sizeof(uint32_t), cudaMemcpyHostToDevice) !=
            cudaSuccess ||
        cudaMemcpy(d_truncated, &zero_i32, sizeof(int), cudaMemcpyHostToDevice) !=
            cudaSuccess) {
        cleanup();
        return false;
    }

    // Seed the first wave with the start vertex and mark it visited.
    if (cudaMemcpy(d_wave_a, &start_id, sizeof(uint32_t), cudaMemcpyHostToDevice) !=
            cudaSuccess) {
        cleanup();
        return false;
    }
    // Mark the start vertex as visited before launching the first wave so
    // that when its neighbours are expanded they cannot re-enqueue it.
    const uint32_t one_u32 = 1u;
    if (cudaMemcpy(d_visited + start_id, &one_u32, sizeof(uint32_t),
                   cudaMemcpyHostToDevice) != cudaSuccess) {
        cleanup();
        return false;
    }

    const int max_results =
        config.max_results > 0
            ? static_cast<int>(std::min<size_t>(config.max_results, vertex_count))
            : 0;

    // -----------------------------------------------------------------------
    // Wave-by-wave parallel DFS approximation
    // Each wave processes all vertices at the current depth in parallel.
    // -----------------------------------------------------------------------
    uint32_t* d_current_wave = d_wave_a;
    uint32_t* d_next_wave = d_wave_b;
    uint32_t current_wave_size = 1u;

    for (int depth = 0; depth <= config.max_depth && current_wave_size > 0; ++depth) {
        // Reset next-wave size.
        if (cudaMemcpy(d_next_size, &zero_u32, sizeof(uint32_t),
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            cleanup();
            return false;
        }

        const uint32_t blocks =
            (current_wave_size + kDfsThreadsPerBlock - 1) / kDfsThreadsPerBlock;
        dfsWaveExpandKernel<<<blocks, kDfsThreadsPerBlock>>>(
            d_row_offsets, d_col_indices,
            current_wave_size, d_current_wave,
            depth, config.max_depth,
            d_forbidden, d_visited, d_order,
            d_next_wave, d_next_size,
            d_order_counter, d_nodes_counter, d_edges_counter,
            max_results, d_truncated);

        if (cudaDeviceSynchronize() != cudaSuccess) {
            cleanup();
            return false;
        }

        // Check truncation flag before issuing another wave.
        int truncated_flag = 0;
        if (cudaMemcpy(&truncated_flag, d_truncated, sizeof(int),
                       cudaMemcpyDeviceToHost) != cudaSuccess) {
            cleanup();
            return false;
        }
        if (truncated_flag) {
            result.truncated = true;
            break;
        }

        // Read next wave size and swap buffers for the next iteration.
        if (cudaMemcpy(&current_wave_size, d_next_size, sizeof(uint32_t),
                       cudaMemcpyDeviceToHost) != cudaSuccess) {
            cleanup();
            return false;
        }
        std::swap(d_current_wave, d_next_wave);
    }

    // -----------------------------------------------------------------------
    // Copy results back to host
    // -----------------------------------------------------------------------
    uint32_t nodes = 0;
    uint32_t edges = 0;
    if (cudaMemcpy(host_order.data(), d_order, sizeof(int) * vertex_count,
                   cudaMemcpyDeviceToHost) != cudaSuccess ||
        cudaMemcpy(&nodes, d_nodes_counter, sizeof(uint32_t),
                   cudaMemcpyDeviceToHost) != cudaSuccess ||
        cudaMemcpy(&edges, d_edges_counter, sizeof(uint32_t),
                   cudaMemcpyDeviceToHost) != cudaSuccess) {
        cleanup();
        return false;
    }

    cleanup();
    result.nodes_explored = nodes;
    result.edges_traversed = edges;
    out_order = std::move(host_order);
    return true;
}

} // namespace cuda_impl
} // namespace graph
} // namespace themis
