/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_layer_graph.cpp                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     440                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • (initial)  2026-04-09  feat(graph): multi-layer graph implementation ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "graph/multi_layer_graph.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace themis {
namespace graph {

// =============================================================================
// Construction
// =============================================================================

MultiLayerGraph::MultiLayerGraph(MultiLayerGraphConfig config)
    : config_(std::move(config))
{}

// =============================================================================
// Topology mutation
// =============================================================================

bool MultiLayerGraph::addLayer(const GraphLayer& layer)
{
    if (layer_meta_.count(layer.name)) return false;
    layer_meta_[layer.name] = layer;
    adj_[layer.name]; // create empty adjacency map
    return true;
}

bool MultiLayerGraph::removeLayer(const std::string& name)
{
    if (!layer_meta_.count(name)) return false;
    layer_meta_.erase(name);
    adj_.erase(name);

    // Rebuild vertex_set_ from remaining layers
    vertex_set_.clear();
    for (const auto& [lname, adj_map] : adj_) {
        for (const auto& [from, nbrs] : adj_map) {
            vertex_set_[from] = 0;
            for (const auto& [to, _] : nbrs) vertex_set_[to] = 0;
        }
    }
    return true;
}

bool MultiLayerGraph::addEdge(
    const std::string& layer_name,
    const std::string& from,
    const std::string& to,
    double weight)
{
    auto layer_it = layer_meta_.find(layer_name);
    if (layer_it == layer_meta_.end()) return false;

    const GraphLayer& meta = layer_it->second;
    double w = (weight < 0.0) ? meta.default_weight : weight;

    auto& adj_map = adj_[layer_name];

    // Check for duplicate directed edge
    auto& from_nbrs = adj_map[from];
    bool already_exists = false;
    for (const auto& [nbr, _] : from_nbrs) {
        if (nbr == to) { already_exists = true; break; }
    }
    if (!already_exists) {
        from_nbrs.push_back({to, w});
        vertex_set_[from] = 0;
        vertex_set_[to]   = 0;
    }

    // For UNDIRECTED / BIDIRECTIONAL layers, also add reverse edge
    if (meta.edge_type == LayerEdgeType::UNDIRECTED ||
        meta.edge_type == LayerEdgeType::BIDIRECTIONAL)
    {
        auto& rev_nbrs = adj_map[to];
        bool rev_exists = false;
        for (const auto& [nbr, _] : rev_nbrs) {
            if (nbr == from) { rev_exists = true; break; }
        }
        if (!rev_exists) rev_nbrs.push_back({from, w});
    }

    return true;
}

// =============================================================================
// Build merged adjacency list
// =============================================================================

std::unordered_map<std::string, std::vector<std::pair<std::string, double>>>
MultiLayerGraph::buildMergedAdj(
    const std::vector<std::string>& layers,
    const std::vector<double>& layer_weights) const
{
    std::unordered_map<std::string, std::vector<std::pair<std::string, double>>> merged;

    for (size_t li = 0; li < layers.size(); ++li) {
        double lw = (li < layer_weights.size()) ? layer_weights[li] : 1.0;
        if (lw < 0.0) lw = 0.0;

        auto adj_it = adj_.find(layers[li]);
        if (adj_it == adj_.end()) continue;

        for (const auto& [from, nbrs] : adj_it->second) {
            auto& merged_nbrs = merged[from];
            for (const auto& [to, w] : nbrs) {
                merged_nbrs.push_back({to, w * lw});
            }
        }
    }
    return merged;
}

// =============================================================================
// Dijkstra helper
// =============================================================================

MultiLayerPath MultiLayerGraph::dijkstra(
    const std::unordered_map<std::string,
        std::vector<std::pair<std::string, double>>>& adj,
    const std::string& from,
    const std::string& to,
    int max_hops,
    const std::vector<std::string>& layers) const
{
    using Node = std::pair<double, std::string>; // (cost, id)
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
    std::unordered_map<std::string, double> dist;
    std::unordered_map<std::string, std::string> prev;
    std::unordered_map<std::string, int> hops;

    dist[from] = 0.0;
    hops[from] = 0;
    pq.push({0.0, from});

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();

        if (u == to) break;
        if (dist.count(u) && d > dist[u] + 1e-12) continue;

        auto it = adj.find(u);
        if (it == adj.end()) continue;

        int cur_hops = hops.count(u) ? hops[u] : 0;
        if (max_hops > 0 && cur_hops >= max_hops) continue;

        for (const auto& [v, w] : it->second) {
            double nd = d + w;
            auto dit = dist.find(v);
            if (dit == dist.end() || nd < dit->second - 1e-12) {
                dist[v]  = nd;
                hops[v]  = cur_hops + 1;
                prev[v]  = u;
                pq.push({nd, v});
            }
        }
    }

    MultiLayerPath path;
    if (!dist.count(to)) return path; // not reachable

    // Reconstruct path
    std::vector<std::string> vertices;
    for (std::string cur = to; !cur.empty(); ) {
        vertices.push_back(cur);
        auto pit = prev.find(cur);
        if (pit == prev.end()) break;
        cur = pit->second;
    }
    std::reverse(vertices.begin(), vertices.end());

    path.vertices    = vertices;
    path.total_cost  = dist[to];
    path.hop_count   = static_cast<int>(vertices.size()) - 1;

    // Build edge identifiers and layer info
    if (layers.empty()) return path;
    const std::string& layer_tag = layers[0];
    path.edges.reserve(static_cast<size_t>(path.hop_count));
    path.layers_traversed.reserve(static_cast<size_t>(path.hop_count));
    for (size_t i = 0; i + 1 < path.vertices.size(); ++i) {
        path.edges.push_back(path.vertices[i] + "-" + layer_tag + "-" + path.vertices[i+1]);
        path.layers_traversed.push_back(layer_tag);
    }

    return path;
}

// =============================================================================
// shortestPath()
// =============================================================================

MultiLayerPath MultiLayerGraph::shortestPath(
    const std::string& from,
    const std::string& to,
    const std::vector<std::string>& layers,
    const std::vector<double>& layer_weights,
    int max_hops) const
{
    if (from.empty() || to.empty() || layers.empty()) return {};

    int effective_hops = max_hops > 0
        ? max_hops
        : (config_.max_path_hops > 0 ? config_.max_path_hops : 0);

    auto merged = buildMergedAdj(layers, layer_weights);
    return dijkstra(merged, from, to, effective_hops, layers);
}

// =============================================================================
// reachableFrom()
// =============================================================================

std::unordered_map<std::string, int> MultiLayerGraph::reachableFrom(
    const std::string& source,
    const std::vector<std::string>& layers,
    int max_hops) const
{
    std::unordered_map<std::string, int> visited;
    if (source.empty() || layers.empty()) return visited;

    auto merged = buildMergedAdj(layers, {});

    std::queue<std::pair<std::string, int>> bfs;
    bfs.push({source, 0});
    visited[source] = 0;

    while (!bfs.empty()) {
        auto [u, d] = bfs.front(); bfs.pop();
        if (max_hops > 0 && d >= max_hops) continue;

        auto it = merged.find(u);
        if (it == merged.end()) continue;

        for (const auto& [v, _] : it->second) {
            if (!visited.count(v)) {
                visited[v] = d + 1;
                bfs.push({v, d + 1});
            }
        }
    }
    visited.erase(source); // exclude source itself
    return visited;
}

// =============================================================================
// isReachable()
// =============================================================================

ReachabilityResult MultiLayerGraph::isReachable(
    const std::string& from,
    const std::string& to,
    const std::vector<std::string>& layers) const
{
    ReachabilityResult res;
    if (from.empty() || to.empty() || layers.empty()) return res;

    auto merged = buildMergedAdj(layers, {});

    std::queue<std::pair<std::string, int>> bfs;
    std::unordered_map<std::string, int> visited;
    bfs.push({from, 0});
    visited[from] = 0;

    while (!bfs.empty()) {
        auto [u, d] = bfs.front(); bfs.pop();

        if (u == to) {
            res.reachable = true;
            res.min_hops  = d;
            // Identify which layer the edge was found on
            for (const auto& layer_name : layers) {
                auto adj_it = adj_.find(layer_name);
                if (adj_it == adj_.end()) continue;
                // just pick first matching layer as annotation
                res.layer = layer_name;
                break;
            }
            return res;
        }

        auto it = merged.find(u);
        if (it == merged.end()) continue;
        for (const auto& [v, _] : it->second) {
            if (!visited.count(v)) {
                visited[v] = d + 1;
                bfs.push({v, d + 1});
            }
        }
    }
    return res;
}

// =============================================================================
// pageRank()
// =============================================================================

std::map<std::string, double> MultiLayerGraph::pageRank(
    const std::vector<std::string>& layers,
    const std::vector<double>& layer_weights,
    LayerAggregation aggregation,
    double damping,
    int max_iter) const
{
    if (layers.empty()) return {};

    // Compute PageRank per layer then aggregate
    std::map<std::string, std::vector<double>> per_layer_scores;
    size_t valid_layers = 0;

    for (size_t li = 0; li < layers.size(); ++li) {
        double lw = (li < layer_weights.size()) ? layer_weights[li] : 1.0;
        if (lw <= 0.0) continue;

        auto adj_it = adj_.find(layers[li]);
        if (adj_it == adj_.end()) continue;
        const auto& adj_map = adj_it->second;

        // Collect all vertices in this layer
        std::unordered_map<std::string, double> rank;
        std::unordered_map<std::string, size_t> out_degree;

        for (const auto& [from, nbrs] : adj_map) {
            out_degree[from] = nbrs.size();
            for (const auto& [to, _] : nbrs)
                rank.emplace(to, 0.0);
            rank.emplace(from, 0.0);
        }

        size_t N = rank.size();
        if (N == 0) continue;

        double init = 1.0 / static_cast<double>(N);
        for (auto& [v, r] : rank) r = init;

        for (int iter = 0; iter < max_iter; ++iter) {
            std::unordered_map<std::string, double> new_rank;
            for (auto& [v, _] : rank) new_rank[v] = (1.0 - damping) / N;

            for (const auto& [from, nbrs] : adj_map) {
                size_t deg = out_degree.count(from) ? out_degree[from] : 0;
                if (deg == 0) continue;
                double contrib = damping * rank[from] / static_cast<double>(deg);
                for (const auto& [to, w] : nbrs)
                    new_rank[to] += contrib * (w * lw);
            }

            double delta = 0.0;
            for (const auto& [v, r] : new_rank)
                delta += std::abs(r - rank[v]);
            rank = std::move(new_rank);
            if (delta < 1e-8) break;
        }

        for (const auto& [v, r] : rank)
            per_layer_scores[v].push_back(r);
        valid_layers++;
    }

    std::map<std::string, double> result;
    for (const auto& [v, scores] : per_layer_scores) {
        if (scores.empty()) { result[v] = 0.0; continue; }
        double agg = 0.0;
        switch (aggregation) {
            case LayerAggregation::SUM:
                for (double s : scores) agg += s;
                break;
            case LayerAggregation::AVG:
                for (double s : scores) agg += s;
                agg /= static_cast<double>(scores.size());
                break;
            case LayerAggregation::MAX:
                agg = *std::max_element(scores.begin(), scores.end());
                break;
            case LayerAggregation::MIN:
                agg = *std::min_element(scores.begin(), scores.end());
                break;
            case LayerAggregation::COUNT:
                for (double s : scores) if (s > 0.0) agg += 1.0;
                break;
        }
        result[v] = agg;
    }
    return result;
}

// =============================================================================
// Introspection
// =============================================================================

std::vector<GraphLayer> MultiLayerGraph::layers() const
{
    std::vector<GraphLayer> result;
    result.reserve(layer_meta_.size());
    for (const auto& [name, meta] : layer_meta_) result.push_back(meta);
    std::sort(result.begin(), result.end(),
              [](const GraphLayer& a, const GraphLayer& b) {
                  return a.name < b.name;
              });
    return result;
}

size_t MultiLayerGraph::vertexCount() const
{
    return vertex_set_.size();
}

size_t MultiLayerGraph::edgeCount(const std::string& layer_name) const
{
    auto adj_it = adj_.find(layer_name);
    if (adj_it == adj_.end()) return 0;
    size_t count = 0;
    for (const auto& [_, nbrs] : adj_it->second) count += nbrs.size();

    // For undirected layers each physical edge was stored twice
    const auto& meta = layer_meta_.at(layer_name);
    if (meta.edge_type == LayerEdgeType::UNDIRECTED) count /= 2;

    return count;
}

bool MultiLayerGraph::hasLayer(const std::string& layer_name) const
{
    return layer_meta_.count(layer_name) > 0;
}

} // namespace graph
} // namespace themis
