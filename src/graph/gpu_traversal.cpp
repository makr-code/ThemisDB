/**
 * @file gpu_traversal.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// GPU-accelerated BFS/DFS implementation for ThemisDB graph module.
//
// When THEMIS_ENABLE_CUDA is defined this file is compiled with NVCC and the
// runBFS / runDFS helpers dispatch to CUDA kernels.  Without CUDA the functions
// fall back to an efficient CPU implementation that mirrors the GPU algorithm
// so that the same interface and result semantics are guaranteed regardless of
// the underlying hardware.

#include "graph/gpu_traversal.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <queue>
#include <unordered_set>
#include <vector>

#include "utils/error_registry.h"

#if defined(THEMIS_ENABLE_CUDA)
#include <cuda_runtime_api.h>
#endif

namespace themis {
namespace graph {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

GPUGraphTraversal::GPUGraphTraversal(GraphIndexManager &graph_manager) : graph_manager_(graph_manager) {}

// ---------------------------------------------------------------------------
// GPU availability probe
// ---------------------------------------------------------------------------

namespace {

bool probeGPUAvailability(int /*device*/) noexcept {
#if defined(THEMIS_ENABLE_CUDA)
    int count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess)
        return false = {};
    return count > 0;
#else
    return false;
#endif
}

void recordTraversalRoute(std::string_view operation, std::string_view route, std::string_view reason) {
    observability::MetricsCollector::getInstance().addCounter(
        "graph_acceleration_routes_total", 1,
        {{"operation", std::string(operation)},
         {"route", std::string(route)},
         {"reason", std::string(reason)}});
}

std::optional<std::string> validateTraversalConfig(const GPUGraphTraversal::Config &config) {
    if (config.gpu_device < 0) {
        return "GPUGraphTraversal requires a non-negative gpu_device index";
    }
    if (config.max_depth < 0) {
        return "GPUGraphTraversal requires a non-negative max_depth";
    }
    return std::nullopt;
}

} // anonymous namespace

#if defined(THEMIS_ENABLE_CUDA)
namespace cuda_impl {
bool runBFSCuda(const std::vector<uint32_t> &row_offsets, const std::vector<uint32_t> &column_indices,
                uint32_t start_id, const std::vector<uint8_t> &forbidden_mask,
                const themis::graph::GPUGraphTraversal::Config &config,
                themis::graph::GPUGraphTraversal::TraversalResult &result, std::vector<int> &out_distances);

bool runDFSCuda(const std::vector<uint32_t> &row_offsets, const std::vector<uint32_t> &column_indices,
                uint32_t start_id, const std::vector<uint8_t> &forbidden_mask,
                const themis::graph::GPUGraphTraversal::Config &config,
                themis::graph::GPUGraphTraversal::TraversalResult &result, std::vector<int> &out_order);
} // namespace cuda_impl
#endif

namespace {

bool runBFSCudaIfAvailable(const std::vector<uint32_t> &row_offsets,
                           const std::vector<uint32_t> &column_indices,
                           uint32_t start_id,
                           const std::vector<uint8_t> &forbidden_mask,
                           const themis::graph::GPUGraphTraversal::Config &config,
                           themis::graph::GPUGraphTraversal::TraversalResult &result,
                           std::vector<int> &out_distances) {
#if defined(THEMIS_ENABLE_CUDA)
    return cuda_impl::runBFSCuda(row_offsets, column_indices, start_id, forbidden_mask, config, result, out_distances);
#else
    return false;
#endif
}

bool runDFSCudaIfAvailable(const std::vector<uint32_t> &row_offsets,
                           const std::vector<uint32_t> &column_indices,
                           uint32_t start_id,
                           const std::vector<uint8_t> &forbidden_mask,
                           const themis::graph::GPUGraphTraversal::Config &config,
                           themis::graph::GPUGraphTraversal::TraversalResult &result,
                           std::vector<int> &out_order) {
#if defined(THEMIS_ENABLE_CUDA)
    return cuda_impl::runDFSCuda(row_offsets, column_indices, start_id, forbidden_mask, config, result, out_order);
#else
    return false;
#endif
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// load()
// ---------------------------------------------------------------------------

Result<bool> GPUGraphTraversal::load(const std::vector<std::string> &vertex_ids) {
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
            return Err<bool>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                             "GPUGraphTraversal::load: failed to enumerate vertices: " + status.message);
        }
        all_vertices = std::move(verts);
    }

    // Build vertex ↔ integer mapping.
    vertex_to_id_.clear();
    id_to_vertex_.clear();
    vertex_to_id_.reserve(all_vertices.size());
    id_to_vertex_.reserve(all_vertices.size());

    for (const auto &v : all_vertices) {
        if (vertex_to_id_.count(v)) {
            continue;
        }
        const uint32_t id = static_cast<uint32_t>(id_to_vertex_.size());
        vertex_to_id_[v]  = id;
        id_to_vertex_.push_back(v);
    }

    vertex_count_ = id_to_vertex_.size();

    // Build adjacency list first, then convert to CSR.
    std::vector<std::vector<uint32_t>> adj(vertex_count_);
    edge_count_ = 0;

    for (uint32_t i = 0; i < vertex_count_; ++i) {
        const std::string &src = id_to_vertex_[i];
        auto [st, neighbors]   = graph_manager_.outNeighbors(src);
        if (!st.ok) {
            continue;
        }
        for (const auto &nb : neighbors) {
            auto it = vertex_to_id_.find(nb);
            if (it == vertex_to_id_.end()) {
                // Neighbour not in our vertex set – insert it.
                const uint32_t new_id = static_cast<uint32_t>(id_to_vertex_.size());
                vertex_to_id_[nb]     = new_id;
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
        if (static_cast<int>(adj.size()) > i) {
            for (uint32_t nb : adj[i]) {
                column_indices_.push_back(nb);
            }
        }
    }
    row_offsets_.push_back(static_cast<uint32_t>(column_indices_.size()));

    gpu_available_ = probeGPUAvailability(0);
    if (gpu_available_) {
        gpu_device_ = 0;
    }

    return Ok(true);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::optional<uint32_t> GPUGraphTraversal::findVertexId(const std::string &v) const {
    auto it = vertex_to_id_.find(v);
    if (it == vertex_to_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

GPUGraphTraversal::Stats GPUGraphTraversal::getStats() const noexcept {
    Stats s;
    s.vertex_count    = vertex_count_;
    s.edge_count      = edge_count_;
    s.gpu_available   = gpu_available_;
    s.gpu_device_used = gpu_device_;
    return s;
}

// ---------------------------------------------------------------------------
// Internal BFS (level-synchronous / GPU-style)
// ---------------------------------------------------------------------------

GPUGraphTraversal::TraversalResult GPUGraphTraversal::runBFS(uint32_t start_id, const Config &config) {
    auto wall_start = std::chrono::steady_clock::now();

    // Build forbidden-vertex set using integer IDs for O(1) lookup.
    std::unordered_set<uint32_t> forbidden_ids = {};

    for (const auto &fv : config.forbidden_vertices) {
        auto opt = findVertexId(fv);
        if (opt) {
            forbidden_ids.insert(*opt);
        }
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

    const bool can_use_gpu = gpu_available_ && vertex_count_ >= config.min_vertices_for_gpu;
    const std::string cpu_route_reason =
        can_use_gpu ? "gpu_kernel_fallback"
                    : (gpu_available_ ? "graph_below_threshold" : "gpu_unavailable");

    if (can_use_gpu) {
        std::vector<int> gpu_dist = {};

        if (runBFSCudaIfAvailable(row_offsets_, column_indices_, start_id, forbidden_mask, config, result, gpu_dist)) {
            result.used_cpu_fallback = false;
            recordTraversalRoute("bfs", "gpu", "hardware_accelerated");

            std::vector<std::pair<int, uint32_t>> ordered;
            ordered.reserve(gpu_dist.size());
            for (uint32_t i = 0; i < gpu_dist.size(); ++i) {
                if (gpu_dist[i] >= 0) {
                    ordered.emplace_back(gpu_dist[i], i);
                }
            }

            std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
                if (a.first != b.first) {
                    return a.first < b.first;
                }
                return a.second < b.second;
            });

            for (const auto &entry : ordered) {
                const uint32_t vid = entry.second;
                result.visited_vertices.push_back(id_to_vertex_[vid]);
                result.distances[id_to_vertex_[vid]] = entry.first;
                ++result.nodes_explored;

                if (config.max_results > 0 && static_cast<int>(result.visited_vertices.size()) >= config.max_results) {
                    result.truncated = true;
                    break;
                }
            }

            result.edges_traversed   = edge_count_;
            auto wall_end            = std::chrono::steady_clock::now();
            result.execution_time_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();
            return result;
        }
    }

    recordTraversalRoute("bfs", "cpu", cpu_route_reason);

    std::vector<uint32_t> current_frontier;
    current_frontier.push_back(start_id);

    for (int depth = 0; depth <= config.max_depth; ++depth) {
        if (current_frontier.empty()) {
            break;
        }

        // Record visited vertices for this level.
        for (uint32_t v : current_frontier) {
            result.visited_vertices.push_back(id_to_vertex_[v]);
            result.distances[id_to_vertex_[v]] = dist[v];
            ++result.nodes_explored;

            if (config.max_results > 0 && static_cast<int>(result.visited_vertices.size()) >= config.max_results) {
                result.truncated = true;
                break;
            }
        }
        if (result.truncated) {
            break;
        }
        if (depth == config.max_depth) {
            break;
        }

        // Expand frontier (GPU-style: all vertices in parallel, here sequential)
        std::vector<uint32_t> next_frontier = {};

        for (uint32_t v : current_frontier) {
            const uint32_t begin = row_offsets_[v];
            const uint32_t end   = row_offsets_[v + 1];
            for (uint32_t e = begin; e < end; ++e) {
                const uint32_t nb = column_indices_[e];
                ++result.edges_traversed;
                if (dist[nb] != -1) {
                    continue;
                }
                if (forbidden_ids.count(nb)) {
                    continue;
                }
                dist[nb] = depth + 1;
                next_frontier.push_back(nb);
            }
        }
        current_frontier = std::move(next_frontier);
    }

    auto wall_end            = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    return result;
}

// ---------------------------------------------------------------------------
// Internal DFS (iterative with depth tracking)
// ---------------------------------------------------------------------------

GPUGraphTraversal::TraversalResult GPUGraphTraversal::runDFS(uint32_t start_id, const Config &config) {
    auto wall_start = std::chrono::steady_clock::now();

    std::unordered_set<uint32_t> forbidden_ids = {};

    for (const auto &fv : config.forbidden_vertices) {
        auto opt = findVertexId(fv);
        if (opt) {
            forbidden_ids.insert(*opt);
        }
    }

    const uint32_t n = static_cast<uint32_t>(vertex_count_);
    std::vector<int> disc_order(n, -1);

    std::vector<uint8_t> forbidden_mask(n, 0);
    for (uint32_t id : forbidden_ids) {
        forbidden_mask[id] = 1;
    }

    const bool can_use_gpu = gpu_available_ && vertex_count_ >= config.min_vertices_for_gpu;
    const std::string cpu_route_reason =
        can_use_gpu ? "gpu_kernel_fallback"
                    : (gpu_available_ ? "graph_below_threshold" : "gpu_unavailable");

    TraversalResult result;
    result.used_cpu_fallback = true;

    if (can_use_gpu) {
        std::vector<int> gpu_order = {};

        if (runDFSCudaIfAvailable(row_offsets_, column_indices_, start_id, forbidden_mask, config, result, gpu_order)) {
            result.used_cpu_fallback = false;
            recordTraversalRoute("dfs", "gpu", "hardware_accelerated");

            std::vector<std::pair<int, uint32_t>> ordered;
            ordered.reserve(gpu_order.size());
            for (uint32_t i = 0; i < gpu_order.size(); ++i) {
                if (gpu_order[i] >= 0) {
                    ordered.emplace_back(gpu_order[i], i);
                }
            }

            std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

            for (const auto &entry : ordered) {
                const uint32_t vid = entry.second;
                result.visited_vertices.push_back(id_to_vertex_[vid]);
                result.distances[id_to_vertex_[vid]] = entry.first;
            }

            auto wall_end            = std::chrono::steady_clock::now();
            result.execution_time_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();
            return result;
        }
    }

    recordTraversalRoute("dfs", "cpu", cpu_route_reason);

    // Stack holds (vertex_id, depth)
    std::vector<std::pair<uint32_t, int>> stack;
    stack.emplace_back(start_id, 0);

    int discovery_counter = 0;

    while (!stack.empty()) {
        auto [cur, depth] = stack.back();
        stack.pop_back();

        if (disc_order[cur] != -1) {
            continue; // already visited
        }
        if (forbidden_ids.count(cur)) {
            continue;
        }

        disc_order[cur] = discovery_counter++;
        result.visited_vertices.push_back(id_to_vertex_[cur]);
        result.distances[id_to_vertex_[cur]] = disc_order[cur];
        ++result.nodes_explored;

        if (config.max_results > 0 && static_cast<int>(result.visited_vertices.size()) >= config.max_results) {
            result.truncated = true;
            break;
        }

        if (depth >= config.max_depth) {
            continue;
        }

        const uint32_t begin = row_offsets_[cur];
        const uint32_t end   = row_offsets_[cur + 1];
        for (uint32_t e = begin; e < end; ++e) {
            const uint32_t nb = column_indices_[e];
            ++result.edges_traversed;
            if (disc_order[nb] != -1) {
                continue;
            }
            stack.emplace_back(nb, depth + 1);
        }
    }

    auto wall_end            = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    return result;
}

// ---------------------------------------------------------------------------
// Public BFS
// ---------------------------------------------------------------------------

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::bfs(const std::string &start_vertex) {
    return bfs(start_vertex, Config{});
}

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::bfs(const std::string &start_vertex,
                                                                  const Config &config) {
    if (const auto config_error = validateTraversalConfig(config); config_error.has_value()) {
        recordTraversalRoute("bfs", "rejected", "invalid_config");
        return Err<TraversalResult>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, *config_error);
    }
    if (vertex_count_ == 0) {
        recordTraversalRoute("bfs", "rejected", "graph_not_loaded");
        return Err<TraversalResult>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                                    "GPUGraphTraversal::bfs: graph not loaded — call load() first");
    }

    auto opt_id = findVertexId(start_vertex);
    if (!opt_id) {
        recordTraversalRoute("bfs", "rejected", "unknown_start_vertex");
        return Err<TraversalResult>(errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
                                    "BFS start vertex not found: " + start_vertex);
    }

    return Ok(runBFS(*opt_id, config));
}

// ---------------------------------------------------------------------------
// Public DFS
// ---------------------------------------------------------------------------

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::dfs(const std::string &start_vertex) {
    return dfs(start_vertex, Config{});
}

Result<GPUGraphTraversal::TraversalResult> GPUGraphTraversal::dfs(const std::string &start_vertex,
                                                                  const Config &config) {
    if (const auto config_error = validateTraversalConfig(config); config_error.has_value()) {
        recordTraversalRoute("dfs", "rejected", "invalid_config");
        return Err<TraversalResult>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT, *config_error);
    }
    if (vertex_count_ == 0) {
        recordTraversalRoute("dfs", "rejected", "graph_not_loaded");
        return Err<TraversalResult>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                                    "GPUGraphTraversal::dfs: graph not loaded — call load() first");
    }

    auto opt_id = findVertexId(start_vertex);
    if (!opt_id) {
        recordTraversalRoute("dfs", "rejected", "unknown_start_vertex");
        return Err<TraversalResult>(errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
                                    "DFS start vertex not found: " + start_vertex);
    }

    return Ok(runDFS(*opt_id, config));
}

} // namespace graph
} // namespace themis
