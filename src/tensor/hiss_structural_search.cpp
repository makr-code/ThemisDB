/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/hiss_structural_search.cpp                  ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "tensor/hiss_structural_search.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace themis {
namespace tensor {

namespace {

double coreEntropy(const storage::TTCore& core) {
    if (core.data.empty()) return 0.0;
    constexpr std::size_t kBins = 16;
    std::array<double, kBins> bins{};
    double total = 0.0;
    double maxv = 0.0;
    for (const auto v : core.data) {
        const auto av = std::abs(static_cast<double>(v));
        maxv = std::max(maxv, av);
    }
    if (maxv <= std::numeric_limits<double>::epsilon()) return 0.0;

    for (const auto v : core.data) {
        const auto av = std::abs(static_cast<double>(v));
        std::size_t b = static_cast<std::size_t>((av / maxv) * (kBins - 1));
        b = std::min(b, kBins - 1);
        bins[b] += 1.0;
        total += 1.0;
    }
    double h = 0.0;
    for (const auto c : bins) {
        if (c <= 0.0) continue;
        const auto p = c / total;
        h -= p * std::log2(p);
    }
    return h / std::log2(static_cast<double>(kBins)); // normalized [0,1]
}

std::uint64_t xorshift64(std::uint64_t& x) {
    x ^= x << 13U;
    x ^= x >> 7U;
    x ^= x << 17U;
    return x;
}

} // namespace

std::size_t TensorNetworkGraph::addNode(TensorGraphNode node) {
    nodes_.push_back(std::move(node));
    return nodes_.size() - 1;
}

bool TensorNetworkGraph::addEdge(TensorGraphEdge edge) {
    if (edge.from >= nodes_.size() || edge.to >= nodes_.size() || edge.from == edge.to) {
        return false;
    }
    const auto exists = std::any_of(edges_.begin(), edges_.end(),
                                    [&](const auto& e) { return e.from == edge.from && e.to == edge.to; });
    if (exists) return false;
    edges_.push_back(std::move(edge));
    return true;
}

bool TensorNetworkGraph::rerouteEdge(std::size_t from, std::size_t to, const std::string& new_topology) {
    for (auto& e : edges_) {
        if (e.from == from && e.to == to) {
            e.topology = new_topology;
            return true;
        }
    }
    return false;
}

std::vector<std::size_t> TensorNetworkGraph::neighbors(std::size_t node_index) const {
    std::vector<std::size_t> out;
    for (const auto& e : edges_) {
        if (e.from == node_index) out.push_back(e.to);
        if (e.to == node_index) out.push_back(e.from);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

TensorNetworkGraph
HissStructuralSearchEngine::search(const storage::TTTrain& train, const HissConfig& cfg) const {
    TensorNetworkGraph graph;
    if (train.cores.empty()) return graph;

    for (std::size_t i = 0; i < train.cores.size(); ++i) {
        const auto& c = train.cores[i];
        TensorGraphNode node;
        node.id = "core_" + std::to_string(i);
        node.mode_index = i;
        node.rank_left = c.r_left;
        node.rank_right = c.r_right;
        node.mode_size = c.n;
        node.entropy_score = coreEntropy(c);
        graph.addNode(std::move(node));
    }

    for (std::size_t i = 0; i + 1 < train.cores.size(); ++i) {
        const auto avg_rank = static_cast<double>(train.cores[i].r_right + train.cores[i + 1].r_left) * 0.5;
        TensorGraphEdge edge;
        edge.from = i;
        edge.to = i + 1;
        edge.weight = std::max(1.0, avg_rank);
        edge.topology = "chain";
        graph.addEdge(std::move(edge));
    }

    // STUB/SIMULATION NOTE:
    // Purpose: Provide a deterministic approximation of global stochastic sub-network
    //          sampling so TensorNetworkGraph/TemplateCatalog integration can ship.
    // Activation: Always.
    // Production Delta: Samples deterministic candidate skip-edges using xorshift64
    //                   and entropy gating; no true global TN-SS search or local
    //                   hierarchical refinement loop yet.
    // Removal Plan: Q2 2028 — replace with full Hiss TN-SS (global sampling +
    //               local refinement + diversity objective).
    std::uint64_t rng = cfg.random_seed;
    std::size_t added = 0;
    for (std::size_t i = 0; i < train.cores.size() && added < cfg.diversity_budget; ++i) {
        const auto entropy_i = coreEntropy(train.cores[i]);
        if (entropy_i < cfg.entropy_threshold) continue;
        for (std::size_t d = 2; d <= cfg.max_reshape_depth + 1; ++d) {
            const auto j = i + d;
            if (j >= train.cores.size() || added >= cfg.diversity_budget) break;
            const auto coin = xorshift64(rng) & 0xFFU;
            if (coin > 96U) { // ~38% hit rate
                TensorGraphEdge e;
                e.from = i;
                e.to = j;
                e.weight = 1.0 + entropy_i;
                e.topology = "reshaped";
                if (graph.addEdge(std::move(e))) ++added;
            }
        }
    }

    (void)cfg.num_samples; // reserved for full TN-SS implementation
    return graph;
}

QTTrain
HissReshaper::exposeQuantics(const storage::TTTrain& train, const std::vector<std::size_t>& grid_sizes) {
    // STUB/SIMULATION NOTE:
    // Purpose: Keep QTT interface available for Phase-6 integration points.
    // Activation: Always.
    // Production Delta: Returns a pass-through QTTrain wrapper around TTTrain;
    //                   no binary index-factorization into true quantics cores.
    // Removal Plan: Q2 2028 — implement Quantics decomposition with per-dimension
    //               bit-depths and reversible QTTrain <-> TTTrain mapping.
    QTTrain qt;
    qt.bit_depths = grid_sizes;
    qt.tt_train = train;
    return qt;
}

void TemplateCatalog::registerTemplate(const std::string& domain_tag, TensorNetworkGraph graph) {
    std::lock_guard<std::mutex> lk(mutex_);
    templates_[domain_tag] = std::move(graph);
}

std::optional<TensorNetworkGraph> TemplateCatalog::lookup(const std::string& domain_tag) const {
    std::lock_guard<std::mutex> lk(mutex_);
    const auto it = templates_.find(domain_tag);
    if (it == templates_.end()) return std::nullopt;
    return it->second;
}

std::size_t TemplateCatalog::size() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return templates_.size();
}

} // namespace tensor
} // namespace themis
