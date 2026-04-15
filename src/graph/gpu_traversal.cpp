/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_traversal.cpp                                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:16:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     863                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • dac91fef60  2026-04-04  Add local production readiness checklist and OpenAPI comp... ║
    • 39ac8c3efe  2026-03-20  Split default-arg constructors into overloads ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// GPU-accelerated BFS/DFS implementation for ThemisDB graph module.
//
// When THEMIS_ENABLE_CUDA is defined this file is compiled with NVCC and the
// runBFS / runDFS helpers dispatch to CUDA kernels.  Without CUDA the functions
// fall back to an efficient CPU implementation that mirrors the GPU algorithm
// so that the same interface and result semantics are guaranteed regardless of
// the underlying hardware.

#include "graph/gpu_traversal.h"
#include "utils/error_registry.h"
#include <algorithm>
#include <chrono>
#include <queue>
#include <unordered_set>
#include <vector>

#if defined(THEMIS_ENABLE_CUDA)
#include <cuda_runtime_api.h>
#endif

namespace themis {
namespace graph {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

GPUGraphTraversal::GPUGraphTraversal(GraphIndexManager& graph_manager)
    : graph_manager_(graph_manager) {}

// ---------------------------------------------------------------------------
// GPU availability probe
// ---------------------------------------------------------------------------

namespace {

bool probeGPUAvailability(int /*device*/) noexcept {
#if defined(THEMIS_ENABLE_CUDA)
    int count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess) return false;
    return count > 0;
#else
    return false;
#endif
}

} // anonymous namespace

#if defined(THEMIS_ENABLE_CUDA) && defined(__CUDACC__)
namespace {

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
    if (idx >= frontier_size) return;

    const uint32_t v = frontier[idx];
    const uint32_t begin = row_offsets[v];
    const uint32_t end = row_offsets[v + 1];

    for (uint32_t e = begin; e < end; ++e) {
        const uint32_t nb = column_indices[e];
        if (forbidden_mask[nb] != 0) {
            continue;
        }

        if (atomicCAS(&distances[nb], -1, current_depth + 1) == -1) {
            const uint32_t out = atomicAdd(next_size, 1u);
            next_frontier[out] = nb;
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

    if (blockIdx.x != 0 || threadIdx.x != 0) {
        return;
    }

    if (vertex_count == 0) {
        *written_count = 0;
        return;
    }

    uint32_t* stack_vertices = reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * vertex_count));
    int* stack_depth = reinterpret_cast<int*>(malloc(sizeof(int) * vertex_count));
    if (!stack_vertices || !stack_depth) {
        if (stack_vertices) free(stack_vertices);
        if (stack_depth) free(stack_depth);
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
        const uint32_t cur = stack_vertices[sp];
        const int depth = stack_depth[sp];

        if (forbidden_mask[cur] != 0) {
            continue;
        }
        if (discovery_order[cur] != -1) {
            continue;
        }

        discovery_order[cur] = order++;
        ++local_nodes;

        if (max_results > 0 && order >= max_results) {
            local_truncated = 1;
            break;
        }
        if (depth >= max_depth) {
            continue;
        }

        const uint32_t begin = row_offsets[cur];
        const uint32_t end = row_offsets[cur + 1];

        for (uint32_t e = begin; e < end; ++e) {
            const uint32_t nb = column_indices[e];
            ++local_edges;
            if (forbidden_mask[nb] != 0) {
                continue;
            }
            if (discovery_order[nb] != -1) {
                continue;
            }
            if (sp < vertex_count) {
                stack_vertices[sp] = nb;
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

} // anonymous namespace
#endif

namespace {

bool runBFSCudaIfAvailable(
    const std::vector<uint32_t>& row_offsets,
    const std::vector<uint32_t>& column_indices,
    uint32_t start_id,
    const std::vector<uint8_t>& forbidden_mask,
    const themis::graph::GPUGraphTraversal::Config& config,
    themis::graph::GPUGraphTraversal::TraversalResult& result,
    std::vector<int>& out_distances) {

#if defined(THEMIS_ENABLE_CUDA) && defined(__CUDACC__)
    const uint32_t vertex_count = static_cast<uint32_t>(row_offsets.size() - 1);
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
    if (cudaMalloc(&d_col_indices, sizeof(uint32_t) * column_indices.size()) != cudaSuccess) {
        cleanup();
        return false;
    }
    if (cudaMalloc(&d_distances, sizeof(int) * vertex_count) != cudaSuccess) {
        cleanup();
        return false;
    }
    if (cudaMalloc(&d_forbidden, sizeof(uint8_t) * vertex_count) != cudaSuccess) {
        cleanup();
        return false;
    }
    if (cudaMalloc(&d_frontier, sizeof(uint32_t) * vertex_count) != cudaSuccess) {
        cleanup();
        return false;
    }
    if (cudaMalloc(&d_next_frontier, sizeof(uint32_t) * vertex_count) != cudaSuccess) {
        cleanup();
        return false;
    }
    if (cudaMalloc(&d_next_size, sizeof(uint32_t)) != cudaSuccess) {
        cleanup();
        return false;
    }

    if (cudaMemcpy(d_row_offsets, row_offsets.data(), sizeof(uint32_t) * row_offsets.size(), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_col_indices, column_indices.data(), sizeof(uint32_t) * column_indices.size(), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_distances, host_dist.data(), sizeof(int) * vertex_count, cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_forbidden, forbidden_mask.data(), sizeof(uint8_t) * vertex_count, cudaMemcpyHostToDevice) != cudaSuccess) {
        cleanup();
        return false;
    }

    std::vector<uint32_t> frontier;
    frontier.push_back(start_id);
    int current_depth = 0;

    while (!frontier.empty() && current_depth < config.max_depth) {
        if (config.max_results > 0 && result.visited_vertices.size() >= config.max_results) {
            result.truncated = true;
            break;
        }

        const uint32_t frontier_size = static_cast<uint32_t>(frontier.size());
        if (cudaMemcpy(d_frontier, frontier.data(), sizeof(uint32_t) * frontier_size, cudaMemcpyHostToDevice) != cudaSuccess) {
            cleanup();
            return false;
        }

        uint32_t zero = 0;
        if (cudaMemcpy(d_next_size, &zero, sizeof(uint32_t), cudaMemcpyHostToDevice) != cudaSuccess) {
            cleanup();
            return false;
        }

        const uint32_t threads = 256;
        const uint32_t blocks = (frontier_size + threads - 1) / threads;
        bfsExpandKernel<<<blocks, threads>>>(
            d_row_offsets,
            d_col_indices,
            d_frontier,
            frontier_size,
            current_depth,
            d_distances,
            d_forbidden,
            d_next_frontier,
            d_next_size);

        if (cudaDeviceSynchronize() != cudaSuccess) {
            cleanup();
            return false;
        }

        uint32_t next_size = 0;
        if (cudaMemcpy(&next_size, d_next_size, sizeof(uint32_t), cudaMemcpyDeviceToHost) != cudaSuccess) {
            cleanup();
            return false;
        }

        frontier.assign(next_size, 0);
        if (next_size > 0 && cudaMemcpy(frontier.data(), d_next_frontier, sizeof(uint32_t) * next_size, cudaMemcpyDeviceToHost) != cudaSuccess) {
            cleanup();
            return false;
        }

        ++current_depth;
    }

    if (cudaMemcpy(host_dist.data(), d_distances, sizeof(int) * vertex_count, cudaMemcpyDeviceToHost) != cudaSuccess) {
        cleanup();
        return false;
    }

    cleanup();
    out_distances = std::move(host_dist);
    return true;
#else
    return false;
#endif
}

bool runDFSCudaIfAvailable(
    const std::vector<uint32_t>& row_offsets,
    const std::vector<uint32_t>& column_indices,
    uint32_t start_id,
    const std::vector<uint8_t>& forbidden_mask,
    const themis::graph::GPUGraphTraversal::Config& config,
    themis::graph::GPUGraphTraversal::TraversalResult& result,
    std::vector<int>& out_order) {

#if defined(THEMIS_ENABLE_CUDA) && defined(__CUDACC__)
    const uint32_t vertex_count = static_cast<uint32_t>(row_offsets.size() - 1);
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
    if (cudaMemcpy(d_row_offsets, row_offsets.data(), sizeof(uint32_t) * row_offsets.size(), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_col_indices, column_indices.data(), sizeof(uint32_t) * column_indices.size(), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_forbidden, forbidden_mask.data(), sizeof(uint8_t) * vertex_count, cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_order, host_order.data(), sizeof(int) * vertex_count, cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_nodes, &zero_u32, sizeof(uint32_t), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_edges, &zero_u32, sizeof(uint32_t), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_written, &zero_u32, sizeof(uint32_t), cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(d_truncated, &zero_i32, sizeof(int), cudaMemcpyHostToDevice) != cudaSuccess) {
        cleanup();
        return false;
    }

    const int max_results = config.max_results > 0
        ? static_cast<int>(std::min<size_t>(config.max_results, vertex_count))
        : 0;

    dfsSingleSourceKernel<<<1, 1>>>(
        d_row_offsets,
        d_col_indices,
        vertex_count,
        start_id,
        config.max_depth,
        max_results,
        d_forbidden,
        d_order,
        d_nodes,
        d_edges,
        d_written,
        d_truncated);

    if (cudaDeviceSynchronize() != cudaSuccess) {
        cleanup();
        return false;
    }

    uint32_t nodes = 0;
    uint32_t edges = 0;
    int truncated = 0;
    if (cudaMemcpy(host_order.data(), d_order, sizeof(int) * vertex_count, cudaMemcpyDeviceToHost) != cudaSuccess ||
        cudaMemcpy(&nodes, d_nodes, sizeof(uint32_t), cudaMemcpyDeviceToHost) != cudaSuccess ||
        cudaMemcpy(&edges, d_edges, sizeof(uint32_t), cudaMemcpyDeviceToHost) != cudaSuccess ||
        cudaMemcpy(&truncated, d_truncated, sizeof(int), cudaMemcpyDeviceToHost) != cudaSuccess) {
        cleanup();
        return false;
    }

    cleanup();
    result.nodes_explored = nodes;
    result.edges_traversed = edges;
    result.truncated = (truncated != 0);
    out_order = std::move(host_order);
    return true;
#else
    return false;
#endif
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// load()
// ---------------------------------------------------------------------------

Result<bool> GPUGraphTraversal::load(const std::vector<std::string>& vertex_ids) {
    // Collect all vertex IDs that appear in the graph.
    std::vector<std::string> all_vertices;

    if (!vertex_ids.empty()) {
        all_vertices = vertex_ids;
    } else {
        // Enumerate vertices from edge data: for every vertex that has
        // outgoing edges, collect both the vertex itself and all its
        // out-neighbours.
        // We obtain vertices by calling outNeighbors on each previously
        // discovered vertex starting from a seed walk.  Because we do not
        // have a dedicated "list all vertices" API we traverse the stored
        // edges by querying the adjacency of all vertices discovered so far,
        // initialised with the source side of every edge in the graph.
        //
        // GraphIndexManager exposes allVertices() – use it when available.
        auto [status, verts] = graph_manager_.allVertices();
        if (!status.ok) {
            return Err<bool>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                "GPUGraphTraversal::load: failed to enumerate vertices: " +
                    status.message);
        }
        all_vertices = std::move(verts);
    }

    // Build vertex ↔ integer mapping.
    vertex_to_id_.clear();
    id_to_vertex_.clear();
    vertex_to_id_.reserve(all_vertices.size());
    id_to_vertex_.reserve(all_vertices.size());

    for (const auto& v : all_vertices) {
        if (vertex_to_id_.count(v)) continue;
        const uint32_t id = static_cast<uint32_t>(id_to_vertex_.size());
        vertex_to_id_[v] = id;
        id_to_vertex_.push_back(v);
    }

    vertex_count_ = id_to_vertex_.size();

    // Build adjacency list first, then convert to CSR.
    std::vector<std::vector<uint32_t>> adj(vertex_count_);
    edge_count_ = 0;

    for (uint32_t i = 0; i < vertex_count_; ++i) {
        const std::string& src = id_to_vertex_[i];
        auto [st, neighbors] = graph_manager_.outNeighbors(src);
        if (!st.ok) continue;
        for (const auto& nb : neighbors) {
            auto it = vertex_to_id_.find(nb);
            if (it == vertex_to_id_.end()) {
                // Neighbour not in our vertex set – insert it.
                const uint32_t new_id =
                    static_cast<uint32_t>(id_to_vertex_.size());
                vertex_to_id_[nb] = new_id;
                id_to_vertex_.push_back(nb);
                // Extend adj list to accommodate new vertex.
                adj.emplace_back();
                adj[i].push_back(new_id);
            } else {
                adj[i].push_back(it->second);
            }
            ++edge_count_;
        }
    }

    // Update vertex_count_ in case new vertices were added above.
    vertex_count_ = id_to_vertex_.size();

    // Convert adjacency list to CSR.
    row_offsets_.clear();
    column_indices_.clear();
    row_offsets_.reserve(vertex_count_ + 1);
    column_indices_.reserve(edge_count_);

    for (uint32_t i = 0; i < vertex_count_; ++i) {
        row_offsets_.push_back(static_cast<uint32_t>(column_indices_.size()));
        // Ensure adj has a row for i (may be absent if vertex was added later)
        if (i < adj.size()) {
            for (uint32_t nb : adj[i]) {
                column_indices_.push_back(nb);
            }
        }
    }
    row_offsets_.push_back(static_cast<uint32_t>(column_indices_.size()));

    gpu_available_ = probeGPUAvailability(0);
    if (gpu_available_) gpu_device_ = 0;

    return Ok(true);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::optional<uint32_t> GPUGraphTraversal::findVertexId(
    const std::string& v) const {
    auto it = vertex_to_id_.find(v);
    if (it == vertex_to_id_.end()) return std::nullopt;
    return it->second;
}

GPUGraphTraversal::Stats GPUGraphTraversal::getStats() const noexcept {
    Stats s;
    s.vertex_count   = vertex_count_;
    s.edge_count     = edge_count_;
    s.gpu_available  = gpu_available_;
    s.gpu_device_used = gpu_device_;
    return s;
}

// ---------------------------------------------------------------------------
// Internal BFS (level-synchronous / GPU-style)
// ---------------------------------------------------------------------------

GPUGraphTraversal::TraversalResult GPUGraphTraversal::runBFS(
    uint32_t start_id,
    const Config& config) {

    auto wall_start = std::chrono::steady_clock::now();

    // Build forbidden-vertex set using integer IDs for O(1) lookup.
    std::unordered_set<uint32_t> forbidden_ids;
    for (const auto& fv : config.forbidden_vertices) {
        auto opt = findVertexId(fv);
        if (opt) forbidden_ids.insert(*opt);
    }

    const uint32_t n = static_cast<uint32_t>(vertex_count_);
    std::vector<int> dist(n, -1);
    dist[start_id] = 0;

    TraversalResult result;
    result.used_cpu_fallback = true;

    std::vector<uint8_t> forbidden_mask(n, 0);
    for (uint32_t id : forbidden_ids) {
        forbidden_mask[id] = 1;
    }

    const bool can_use_gpu =
        gpu_available_ &&
        vertex_count_ >= config.min_vertices_for_gpu;

    if (can_use_gpu) {
        std::vector<int> gpu_dist;
        if (runBFSCudaIfAvailable(row_offsets_, column_indices_, start_id, forbidden_mask, config, result, gpu_dist)) {
            result.used_cpu_fallback = false;

            std::vector<std::pair<int, uint32_t>> ordered;
            ordered.reserve(gpu_dist.size());
            for (uint32_t i = 0; i < gpu_dist.size(); ++i) {
                if (gpu_dist[i] >= 0) {
                    ordered.emplace_back(gpu_dist[i], i);
                }
            }

            std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
                if (a.first != b.first) {
                    return a.first < b.first;
                }
                return a.second < b.second;
            });

            for (const auto& entry : ordered) {
                const uint32_t vid = entry.second;
                result.visited_vertices.push_back(id_to_vertex_[vid]);
                result.distances[id_to_vertex_[vid]] = entry.first;
                ++result.nodes_explored;

                if (config.max_results > 0 && result.visited_vertices.size() >= config.max_results) {
                    result.truncated = true;
                    break;
                }
            }

            result.edges_traversed = edge_count_;
            auto wall_end = std::chrono::steady_clock::now();
            result.execution_time_ms =
                std::chrono::duration<double, std::milli>(wall_end - wall_start).count();
            return result;
        }
    }

    std::vector<uint32_t> current_frontier;
    current_frontier.push_back(start_id);

    for (int depth = 0; depth <= config.max_depth; ++depth) {
        if (current_frontier.empty()) break;

        // Record visited vertices for this level.
        for (uint32_t v : current_frontier) {
            result.visited_vertices.push_back(id_to_vertex_[v]);
            result.distances[id_to_vertex_[v]] = dist[v];
            ++result.nodes_explored;

            if (config.max_results > 0 &&
                result.visited_vertices.size() >= config.max_results) {
                result.truncated = true;
                break;
            }
        }
        if (result.truncated) break;
        if (depth == config.max_depth) break;

        // Expand frontier (GPU-style: all vertices in parallel, here sequential)
        std::vector<uint32_t> next_frontier;
        for (uint32_t v : current_frontier) {
            const uint32_t begin = row_offsets_[v];
            const uint32_t end   = row_offsets_[v + 1];
            for (uint32_t e = begin; e < end; ++e) {
                const uint32_t nb = column_indices_[e];
                ++result.edges_traversed;
                if (dist[nb] != -1) continue;
                if (forbidden_ids.count(nb)) continue;
                dist[nb] = depth + 1;
                next_frontier.push_back(nb);
            }
        }
        current_frontier = std::move(next_frontier);
    }

    auto wall_end = std::chrono::steady_clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    return result;
}

// ---------------------------------------------------------------------------
// Internal DFS (iterative with depth tracking)
// ---------------------------------------------------------------------------

GPUGraphTraversal::TraversalResult GPUGraphTraversal::runDFS(
    uint32_t start_id,
    const Config& config) {

    auto wall_start = std::chrono::steady_clock::now();

    std::unordered_set<uint32_t> forbidden_ids;
    for (const auto& fv : config.forbidden_vertices) {
        auto opt = findVertexId(fv);
        if (opt) forbidden_ids.insert(*opt);
    }

    const uint32_t n = static_cast<uint32_t>(vertex_count_);
    std::vector<int> disc_order(n, -1);

    std::vector<uint8_t> forbidden_mask(n, 0);
    for (uint32_t id : forbidden_ids) {
        forbidden_mask[id] = 1;
    }

    const bool can_use_gpu =
        gpu_available_ &&
        vertex_count_ >= config.min_vertices_for_gpu;

    TraversalResult result;
    result.used_cpu_fallback = true;

    if (can_use_gpu) {
        std::vector<int> gpu_order;
        if (runDFSCudaIfAvailable(row_offsets_, column_indices_, start_id, forbidden_mask, config, result, gpu_order)) {
            result.used_cpu_fallback = false;

            std::vector<std::pair<int, uint32_t>> ordered;
            ordered.reserve(gpu_order.size());
            for (uint32_t i = 0; i < gpu_order.size(); ++i) {
                if (gpu_order[i] >= 0) {
                    ordered.emplace_back(gpu_order[i], i);
                }
            }

            std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

            for (const auto& entry : ordered) {
                const uint32_t vid = entry.second;
                result.visited_vertices.push_back(id_to_vertex_[vid]);
                result.distances[id_to_vertex_[vid]] = entry.first;
            }

            auto wall_end = std::chrono::steady_clock::now();
            result.execution_time_ms =
                std::chrono::duration<double, std::milli>(wall_end - wall_start).count();
            return result;
        }
    }

    // Stack holds (vertex_id, depth)
    std::vector<std::pair<uint32_t, int>> stack;
    stack.emplace_back(start_id, 0);

    int discovery_counter = 0;

    while (!stack.empty()) {
        auto [cur, depth] = stack.back();
        stack.pop_back();

        if (disc_order[cur] != -1) continue; // already visited
        if (forbidden_ids.count(cur)) continue;

        disc_order[cur] = discovery_counter++;
        result.visited_vertices.push_back(id_to_vertex_[cur]);
        result.distances[id_to_vertex_[cur]] = disc_order[cur];
        ++result.nodes_explored;

        if (config.max_results > 0 &&
            result.visited_vertices.size() >= config.max_results) {
            result.truncated = true;
            break;
        }

        if (depth >= config.max_depth) continue;

        const uint32_t begin = row_offsets_[cur];
        const uint32_t end   = row_offsets_[cur + 1];
        for (uint32_t e = begin; e < end; ++e) {
            const uint32_t nb = column_indices_[e];
            ++result.edges_traversed;
            if (disc_order[nb] != -1) continue;
            stack.emplace_back(nb, depth + 1);
        }
    }

    auto wall_end = std::chrono::steady_clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    return result;
}

// ---------------------------------------------------------------------------
// Public BFS
// ---------------------------------------------------------------------------

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::bfs(
    const std::string& start_vertex) {

    return bfs(start_vertex, Config{});
}

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::bfs(
    const std::string& start_vertex,
    const Config& config) {

    if (vertex_count_ == 0) {
        return Err<TraversalResult>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "GPUGraphTraversal::bfs: graph not loaded — call load() first"
        );
    }

    auto opt_id = findVertexId(start_vertex);
    if (!opt_id) {
        return Err<TraversalResult>(
            errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
            "BFS start vertex not found: " + start_vertex
        );
    }

    return Ok(runBFS(*opt_id, config));
}

// ---------------------------------------------------------------------------
// Public DFS
// ---------------------------------------------------------------------------

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::dfs(
    const std::string& start_vertex) {

    return dfs(start_vertex, Config{});
}

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::dfs(
    const std::string& start_vertex,
    const Config& config) {

    if (vertex_count_ == 0) {
        return Err<TraversalResult>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "GPUGraphTraversal::dfs: graph not loaded — call load() first"
        );
    }

    auto opt_id = findVertexId(start_vertex);
    if (!opt_id) {
        return Err<TraversalResult>(
            errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
            "DFS start vertex not found: " + start_vertex
        );
    }

    return Ok(runDFS(*opt_id, config));
}

} // namespace graph
} // namespace themis
