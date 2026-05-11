/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/hiss_structural_search.h                    ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 6 (Q2-Q3 2028)                      ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/hiss_structural_search.h
 * @brief Tensor-network graph primitives for Hiss/TNSR Phase-6.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace tensor {

struct TensorGraphNode {
    std::string id;
    std::size_t mode_index = 0;
    std::size_t rank_left  = 0;
    std::size_t rank_right = 0;
    std::size_t mode_size  = 0;
    double      entropy_score = 0.0;
};

struct TensorGraphEdge {
    std::size_t from = 0;
    std::size_t to   = 0;
    double      weight = 0.0;
    std::string topology = "tree";
};

class TensorNetworkGraph {
public:
    std::size_t addNode(TensorGraphNode node);
    bool addEdge(TensorGraphEdge edge);

    [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }
    [[nodiscard]] std::size_t edgeCount() const noexcept { return edges_.size(); }
    [[nodiscard]] const std::vector<TensorGraphNode>& nodes() const noexcept { return nodes_; }
    [[nodiscard]] const std::vector<TensorGraphEdge>& edges() const noexcept { return edges_; }

    [[nodiscard]] bool rerouteEdge(std::size_t from, std::size_t to, const std::string& new_topology);
    [[nodiscard]] std::vector<std::size_t> neighbors(std::size_t node_index) const;

private:
    std::vector<TensorGraphNode> nodes_;
    std::vector<TensorGraphEdge> edges_;
};

struct HissConfig {
    std::size_t num_samples       = 64;
    double      entropy_threshold = 0.35;
    std::size_t max_reshape_depth = 2;
    std::size_t diversity_budget  = 8;
    std::uint64_t random_seed     = 0x54484D4953444201ULL; // "THMISDB\1"
};

class HissStructuralSearchEngine {
public:
    /**
     * @brief Builds an optimized TensorNetworkGraph from a TT train.
     */
    [[nodiscard]] TensorNetworkGraph
    search(const storage::TTTrain& train, const HissConfig& cfg) const;
};

struct QTTrain {
    std::vector<std::size_t> bit_depths;
    std::vector<std::size_t> grid_sizes;
    std::vector<std::size_t> quantics_mode_sizes;
    storage::TTTrain         tt_train;

    [[nodiscard]] storage::TTTrain toTTTrain() const { return tt_train; }
};

class HissReshaper {
public:
    // -------------------------------------------------------------------------
    // STUB #254 bridge — pure-binary QTT quantics encoder
    // -------------------------------------------------------------------------

    /**
     * @brief Callable type for quantics reshape encoding.
     *
     * @param train      Input TT train.
     * @param grid_sizes Physical dimension sizes (may be empty → use mode_sizes).
     * @return QTTrain in the quantics layout.
     *
     * When installed, replaces the residual-factor fallback entirely.
     * The callable MUST satisfy the same size/product contracts as
     * `exposeQuantics()` or it will produce undefined results.
     */
    using QuanticsFn = std::function<QTTrain(const storage::TTTrain&        train,
                                             const std::vector<std::size_t>& grid_sizes)>;

    /// Install a pure-binary QTT encode backend.  Thread-safe.
    static void setQuanticsFn(QuanticsFn fn);

    /// Remove the quantics backend (fallback: residual-factor factorisation).
    static void clearQuanticsFn();

    /// Return the currently installed QuanticsFn (empty if none).
    static QuanticsFn getQuanticsFn();

    /**
     * @brief Reinterprets a TT train in a quantics-friendly reshaped mode layout.
     *
     * The reshaper reconstructs the dense tensor, expands each provided
     * physical dimension into a sequence of quantics factors, and decomposes
     * the tensor again in the reshaped layout.
     *
     * Factorisation strategy (STUB #254 fallback):
     * - powers of two become repeated `2` modes
     * - non-power-of-two dimensions are decomposed into repeated `2` modes
     *   plus a final residual factor (e.g. `12 -> {2, 2, 3}`)
     *
     * This exposes latent low-rank structure to later Hiss/QTT phases while
     * preserving the exact dense element count.
     *
     * When a `QuanticsFn` bridge is installed via `setQuanticsFn()`, the
     * bridge is called instead of the residual-factor path.
     */
    [[nodiscard]] static QTTrain
    exposeQuantics(const storage::TTTrain& train, const std::vector<std::size_t>& grid_sizes);
};

class TemplateCatalog {
public:
    void registerTemplate(const std::string& domain_tag, TensorNetworkGraph graph);
    [[nodiscard]] std::optional<TensorNetworkGraph> lookup(const std::string& domain_tag) const;
    [[nodiscard]] std::size_t size() const;

private:
    mutable std::mutex                                        mutex_;
    std::unordered_map<std::string, TensorNetworkGraph>       templates_;
};

} // namespace tensor
} // namespace themis
