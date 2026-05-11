#include "graph/gpu_traversal.h"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace themis {
namespace graph {
namespace cuda_impl {

namespace {
constexpr uint32_t kBfsThreadsPerBlock = 256;

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

__global__ void dfsSingleSourceKernel(
    const uint32_t* row_offsets,
    const uint32_t* column_indices,
    uint32_t vertex_count,
    uint32_t start_id,
    int max_depth,
    int max_results,
    const uint8_t* forbidden_mask,
    int* discovery_order,
    uint32_t* nodes_explored,
    uint32_t* edges_traversed,
    uint32_t* written_count,
    int* truncated) {
    if (vertex_count == 0) {
        *written_count = 0;
        return;
    }

    auto* stack_vertices =
        reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * vertex_count));
    auto* stack_depth = reinterpret_cast<int*>(malloc(sizeof(int) * vertex_count));
    if (!stack_vertices || !stack_depth) {
        if (stack_vertices) {
            free(stack_vertices);
        }
        if (stack_depth) {
            free(stack_depth);
        }
        *written_count = 0;
        return;
    }

    uint32_t sp = 0;
    stack_vertices[sp] = start_id;
    stack_depth[sp] = 0;
    ++sp;

    int order = 0;
    uint32_t local_nodes = 0;
    uint32_t local_edges = 0;
    int local_truncated = 0;

    while (sp > 0) {
        --sp;
        const uint32_t current = stack_vertices[sp];
        const int depth = stack_depth[sp];

        if (forbidden_mask[current] != 0 || discovery_order[current] != -1) {
            continue;
        }

        discovery_order[current] = order++;
        ++local_nodes;

        if (max_results > 0 && order >= max_results) {
            local_truncated = 1;
            break;
        }
        if (depth >= max_depth) {
            continue;
        }

        const uint32_t begin = row_offsets[current];
        const uint32_t end = row_offsets[current + 1];
        for (uint32_t edge = begin; edge < end; ++edge) {
            const uint32_t neighbor = column_indices[edge];
            ++local_edges;
            if (forbidden_mask[neighbor] != 0 || discovery_order[neighbor] != -1) {
                continue;
            }
            if (sp < vertex_count) {
                stack_vertices[sp] = neighbor;
                stack_depth[sp] = depth + 1;
                ++sp;
            }
        }
    }

    *nodes_explored = local_nodes;
    *edges_traversed = local_edges;
    *written_count = static_cast<uint32_t>(order);
    *truncated = local_truncated;

    free(stack_vertices);
    free(stack_depth);
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

    uint32_t* d_row_offsets = nullptr;
    uint32_t* d_col_indices = nullptr;
    uint8_t* d_forbidden = nullptr;
    int* d_order = nullptr;
    uint32_t* d_nodes = nullptr;
    uint32_t* d_edges = nullptr;
    uint32_t* d_written = nullptr;
    int* d_truncated = nullptr;

    std::vector<int> host_order(vertex_count, -1);

    auto cleanup = [&]() {
        if (d_row_offsets) cudaFree(d_row_offsets);
        if (d_col_indices) cudaFree(d_col_indices);
        if (d_forbidden) cudaFree(d_forbidden);
        if (d_order) cudaFree(d_order);
        if (d_nodes) cudaFree(d_nodes);
        if (d_edges) cudaFree(d_edges);
        if (d_written) cudaFree(d_written);
        if (d_truncated) cudaFree(d_truncated);
    };

    if (cudaMalloc(&d_row_offsets, sizeof(uint32_t) * row_offsets.size()) != cudaSuccess ||
        cudaMalloc(&d_col_indices, sizeof(uint32_t) * column_indices.size()) != cudaSuccess ||
        cudaMalloc(&d_forbidden, sizeof(uint8_t) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_order, sizeof(int) * vertex_count) != cudaSuccess ||
        cudaMalloc(&d_nodes, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_edges, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_written, sizeof(uint32_t)) != cudaSuccess ||
        cudaMalloc(&d_truncated, sizeof(int)) != cudaSuccess) {
        cleanup();
        return false;
    }

    uint32_t zero_u32 = 0;
    int zero_i32 = 0;
    if (cudaMemcpy(d_row_offsets, row_offsets.data(),
                   sizeof(uint32_t) * row_offsets.size(),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_col_indices, column_indices.data(),
                   sizeof(uint32_t) * column_indices.size(),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_forbidden, forbidden_mask.data(),
                   sizeof(uint8_t) * vertex_count,
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_order, host_order.data(), sizeof(int) * vertex_count,
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_nodes, &zero_u32, sizeof(uint32_t),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_edges, &zero_u32, sizeof(uint32_t),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_written, &zero_u32, sizeof(uint32_t),
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_truncated, &zero_i32, sizeof(int),
                   cudaMemcpyHostToDevice) != cudaSuccess) {
        cleanup();
        return false;
    }

    const int max_results =
        config.max_results > 0
            ? static_cast<int>(std::min<size_t>(config.max_results, vertex_count))
            : 0;

    dfsSingleSourceKernel<<<1, 1>>>(
        d_row_offsets, d_col_indices, vertex_count, start_id, config.max_depth,
        max_results, d_forbidden, d_order, d_nodes, d_edges, d_written,
        d_truncated);

    if (cudaDeviceSynchronize() != cudaSuccess) {
        cleanup();
        return false;
    }

    uint32_t nodes = 0;
    uint32_t edges = 0;
    int truncated = 0;
    if (cudaMemcpy(host_order.data(), d_order, sizeof(int) * vertex_count,
                   cudaMemcpyDeviceToHost) != cudaSuccess ||
        cudaMemcpy(&nodes, d_nodes, sizeof(uint32_t), cudaMemcpyDeviceToHost) !=
            cudaSuccess ||
        cudaMemcpy(&edges, d_edges, sizeof(uint32_t), cudaMemcpyDeviceToHost) !=
            cudaSuccess ||
        cudaMemcpy(&truncated, d_truncated, sizeof(int), cudaMemcpyDeviceToHost) !=
            cudaSuccess) {
        cleanup();
        return false;
    }

    cleanup();
    result.nodes_explored = nodes;
    result.edges_traversed = edges;
    result.truncated = (truncated != 0);
    out_order = std::move(host_order);
    return true;
}

} // namespace cuda_impl
} // namespace graph
} // namespace themis
