/**
 * @file hiss_structural_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=4, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "tensor/hiss_structural_search.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace tensor {

// ============================================================================
// Static bridge slots — QuanticsFn
// ============================================================================

namespace {
    std::mutex           g_quantics_mtx;
    HissReshaper::QuanticsFn g_quantics_fn;
} // namespace

void HissReshaper::setQuanticsFn(HissReshaper::QuanticsFn fn) {
    std::lock_guard<std::mutex> lk(g_quantics_mtx);
    g_quantics_fn = std::move(fn);
}

void HissReshaper::clearQuanticsFn() {
    std::lock_guard<std::mutex> lk(g_quantics_mtx);
    g_quantics_fn = nullptr;
}

HissReshaper::QuanticsFn HissReshaper::getQuanticsFn() {
    std::lock_guard<std::mutex> lk(g_quantics_mtx);
    return g_quantics_fn;
}

namespace {

double coreEntropy(const storage::TTCore& core) {
    if (core.data.empty()) {
      return 0.0;
    }
    constexpr std::size_t kBins = 16;
    std::array<double, kBins> bins{};
    double total = 0.0;
    double maxv = 0.0;
    for (const auto v : core.data) {
        const auto av = std::abs(static_cast<double>(v));
        maxv = std::max(maxv, av);
    }
    if (maxv <= std::numeric_limits<double>::epsilon()) {
      return 0.0;
    }

    for (const auto v : core.data) {
        const auto av = std::abs(static_cast<double>(v));
        std::size_t b = static_cast<std::size_t>((av / maxv) * (kBins - 1));
        b = std::min(b, kBins - 1);
        bins[b] += 1.0;
        total += 1.0;
    }
    double h = 0.0;
    for (const auto c : bins) {
        if (c <= 0.0) {
          continue;
        }
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

static std::size_t denseElementCount(const std::vector<std::size_t>& shape) {
    if (shape.empty()) {
      return 0;
    }
    std::size_t product = 1;
    for (const auto dim : shape) {
        if (dim == 0) {
            throw std::invalid_argument("shape dimension must be > 0");
        }
        if (product > (std::numeric_limits<std::size_t>::max() / dim)) {
            throw std::overflow_error("shape product overflow");
        }
        product *= dim;
    }
    return product;
}

static storage::TTTrain buildExactBinaryTT(const std::vector<float>& dense,
                                           std::size_t               bit_count) {
    if (bit_count == 0) {
        throw std::invalid_argument("buildExactBinaryTT requires bit_count > 0");
    }
    if (bit_count >= std::numeric_limits<std::size_t>::digits) {
        throw std::overflow_error("buildExactBinaryTT bit_count too large: " +
                                  std::to_string(bit_count));
    }

    const std::size_t total = static_cast<std::size_t>(1ULL << bit_count);
    if (dense.size() != total) {
        throw std::invalid_argument("buildExactBinaryTT dense.size() (" +
                                    std::to_string(dense.size()) +
                                    ") must equal 2^bit_count (" +
                                    std::to_string(total) + ")");
    }

    storage::TTTrain train;
    train.mode_sizes.assign(bit_count, 2);
    train.cores.resize(bit_count);

    for (std::size_t k = 0; k + 1 < bit_count; ++k) {
        const std::size_t r_left = static_cast<std::size_t>(1ULL << k);
        const std::size_t r_right = static_cast<std::size_t>(1ULL << (k + 1));
        auto& core = train.cores[k];
        core.r_left = r_left;
        core.n = 2;
        core.r_right = r_right;
        core.data.assign(r_left * 2 * r_right, 0.0f);

        for (std::size_t prefix = 0; prefix < r_left; ++prefix) {
            const std::size_t next0 = (prefix << 1U);
            const std::size_t next1 = next0 | 1U;
            core.at(prefix, 0, next0) = 1.0f;
            core.at(prefix, 1, next1) = 1.0f;
        }
    }

    {
        const std::size_t r_left = static_cast<std::size_t>(1ULL << (bit_count - 1));
        auto& core = train.cores.back();
        core.r_left = r_left;
        core.n = 2;
        core.r_right = 1;
        core.data.assign(r_left * 2, 0.0f);
        for (std::size_t prefix = 0; prefix < r_left; ++prefix) {
            core.at(prefix, 0, 0) = dense[(prefix << 1U)];
            core.at(prefix, 1, 0) = dense[(prefix << 1U) | 1U];
        }
    }

    return train;
}

[[nodiscard]] std::size_t calculateBitDepth(std::size_t grid_size) {
    if (grid_size == 0) {
        throw std::invalid_argument("grid_size must be > 0, got: " + std::to_string(grid_size));
    }
    std::size_t depth = 0;
    std::size_t v = 1;
    while (v < grid_size) {
        if (depth >= std::numeric_limits<std::size_t>::digits - 1) {
            throw std::overflow_error("grid_size " + std::to_string(grid_size) +
                                      " is too large for bit-depth calculation (max depth: " +
                                      std::to_string(std::numeric_limits<std::size_t>::digits - 1) + ")");
        }
        v <<= 1U;
        ++depth;
    }
    return std::max<std::size_t>(depth, 1);
}

} // namespace

// ============================================================================
// QTTMappingDescriptor — reversible physical ↔ QTT index mapping
// ============================================================================

std::size_t QTTMappingDescriptor::physicalToQTT(std::size_t physical_idx) const {
    const auto ndims = grid_sizes.size();
    if (ndims == 0 || ndims != bit_depths.size() || ndims != padded_grid_sizes.size()) {
        throw std::invalid_argument(
            "QTTMappingDescriptor: grid_sizes, bit_depths, and padded_grid_sizes "
            "must all be non-empty and have the same length");
    }

    // Compute total physical element count and validate input.
    std::size_t total_physical = 1;
    for (const auto n : grid_sizes) {
      total_physical *= n;
    }
    if (physical_idx >= total_physical) {
        throw std::out_of_range(
            "physical_idx " + std::to_string(physical_idx) +
            " >= product(grid_sizes) " + std::to_string(total_physical));
    }

    // Convert flat physical index to C-contiguous multi-index in grid_sizes.
    std::vector<std::size_t> multi_idx(ndims);
    {
        auto remaining = physical_idx;
        std::size_t stride = total_physical;
        for (std::size_t d = 0; d < ndims; ++d) {
            stride /= grid_sizes[d];
            multi_idx[d] = remaining / stride;
            remaining -= multi_idx[d] * stride;
        }
    }

    // Total number of QTT bits B = sum(bit_depths).
    std::size_t B = 0;
    for (const auto b : bit_depths) {
      B += b;
    }

    // Encode each per-dimension index into bit_depths[d] bits (MSB first)
    // and accumulate into the QTT flat index.
    std::size_t qtt_idx = 0;
    std::size_t bit_pos = B;
    for (std::size_t d = 0; d < ndims; ++d) {
        const auto bd = bit_depths[d];
        for (std::size_t b = 0; b < bd; ++b) {
            --bit_pos;
            const auto bit = (multi_idx[d] >> (bd - 1u - b)) & 1ULL;
            qtt_idx |= bit << bit_pos;
        }
    }

    return qtt_idx;
}

std::optional<std::size_t> QTTMappingDescriptor::qttToPhysical(std::size_t qtt_idx) const {
    const auto ndims = grid_sizes.size();
    if (ndims == 0 || ndims != bit_depths.size() || ndims != padded_grid_sizes.size()) {
        throw std::invalid_argument(
            "QTTMappingDescriptor: grid_sizes, bit_depths, and padded_grid_sizes "
            "must all be non-empty and have the same length");
    }

    // Validate input against total padded element count.
    std::size_t total_padded = 1;
    for (const auto p : padded_grid_sizes) {
      total_padded *= p;
    }
    if (qtt_idx >= total_padded) {
        throw std::out_of_range(
            "qtt_idx " + std::to_string(qtt_idx) +
            " >= product(padded_grid_sizes) " + std::to_string(total_padded));
    }

    // Total number of QTT bits B = sum(bit_depths).
    std::size_t B = 0;
    for (const auto b : bit_depths) {
      B += b;
    }

    // Decode per-dimension indices from the packed QTT bit sequence (MSB first).
    std::vector<std::size_t> multi_idx(ndims);
    std::size_t bit_pos = B;
    for (std::size_t d = 0; d < ndims; ++d) {
        const auto bd = bit_depths[d];
        std::size_t idx_d = 0;
        for (std::size_t b = 0; b < bd; ++b) {
            --bit_pos;
            const auto bit = (qtt_idx >> bit_pos) & 1ULL;
            idx_d = (idx_d << 1u) | bit;
        }
        if (idx_d >= grid_sizes[d]) {
            return std::nullopt;
        }
        multi_idx[d] = idx_d;
    }

    // Convert C-contiguous multi-index in grid_sizes back to a flat physical index.
    std::size_t physical_idx = 0;
    std::size_t stride = 1;
    for (std::size_t d = ndims; d-- > 0;) {
        physical_idx += multi_idx[d] * stride;
        stride *= grid_sizes[d];
    }

    return physical_idx;
}

// ============================================================================
// TensorNetworkGraph
// ============================================================================

std::size_t TensorNetworkGraph::addNode(TensorGraphNode node) {
    nodes_.push_back(std::move(node));
    return nodes_.size() - 1;
}

bool TensorNetworkGraph::addEdge(TensorGraphEdge edge) {
    if (edge.from >= nodes_.size() || edge.to >= nodes_.size() || edge.from == edge.to) {
        return false;
    }
    const auto exists = std::any_of(edges_.begin(), edges_.end(),
                                    [&]([[maybe_unused]] const auto& e) { return e.from == edge.from && e.to == edge.to; });
    if (exists) {
      return false;
    }
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
        if (e.from == node_index) {
          out.push_back(e.to);
        }
        if (e.to == node_index) {
          out.push_back(e.from);
        }
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

TensorNetworkGraph
HissStructuralSearchEngine::search(const storage::TTTrain& train, const HissConfig& cfg) const {
    TensorNetworkGraph graph;
    if (train.cores.empty()) {
      return graph;
    }

    std::vector<double> entropy(train.cores.size(), 0.0);
    for (std::size_t i = 0; i < train.cores.size(); ++i) {
        const auto& c = train.cores[i];
        TensorGraphNode node;
        node.id = "core_" + std::to_string(i);
        node.mode_index = i;
        node.rank_left = c.r_left;
        node.rank_right = c.r_right;
        node.mode_size = c.n;
        node.entropy_score = coreEntropy(c);
        entropy[i] = node.entropy_score;
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

    // PERMANENT FALLBACK NOTE (Hiss TN-SS — global greedy-best-first sampling):
    // Purpose: Greedy-best-first global sub-network structure search via
    //          entropy-gated xorshift64 sampling.  This is the production CPU
    //          implementation for offline and memory-constrained deployments.
    //          It provides deterministic, reproducible candidate skip-edges using
    //          cfg.random_seed; entropy gating prunes low-information-content
    //          core pairs; diversity_budget controls result breadth.
    //          The full Hiss TN-SS global sampling + local refinement + diversity
    //          objective (Q2 2028) will add hierarchical local refinement on top
    //          of this greedy pass.  Until then this CPU greedy pass IS the
    //          production path for non-CUDA builds.
    // Activation: Always — runs on every call to HissStructuralSearch::search().
    // Notes: O(num_samples) candidate generation, O(k log k) sort, O(n) edge pack.
    //        For THEMIS_HAS_HISS_GLOBAL see include/tensor/hiss_structural_search.h.
    std::uint64_t rng = cfg.random_seed;
    std::vector<TensorGraphEdge> candidates;
    candidates.reserve(std::min<std::size_t>(cfg.num_samples, train.cores.size() * 2));

    const std::size_t max_depth = std::max<std::size_t>(cfg.max_reshape_depth, 1);
    for (std::size_t s = 0; s < cfg.num_samples; ++s) {
        const auto i = static_cast<std::size_t>(xorshift64(rng) % train.cores.size());
        const auto d = 2 + static_cast<std::size_t>(xorshift64(rng) % max_depth);
        const auto j = i + d;
        if (j >= train.cores.size()) {
          continue;
        }

        if (entropy[i] < cfg.entropy_threshold && entropy[j] < cfg.entropy_threshold) {
          continue;
        }

        const auto avg_entropy = 0.5 * (entropy[i] + entropy[j]);
        const auto span_bonus = 1.0 / static_cast<double>(1 + (j - i));

        TensorGraphEdge e;
        e.from = i;
        e.to = j;
        e.weight = 1.0 + avg_entropy + span_bonus;
        e.topology = "reshaped";
        candidates.push_back(std::move(e));
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.weight > b.weight; });

    constexpr std::size_t kMaxPackedIndex = 0xFFFFFFFFULL;
    std::unordered_map<std::uint64_t, TensorGraphEdge> best_by_edge;
    for (const auto& e : candidates) {
        // Packed edge key uses 32-bit lanes per endpoint.
        if (e.from > kMaxPackedIndex || e.to > kMaxPackedIndex) {
            throw std::invalid_argument("tensor graph index from=" + std::to_string(e.from) +
                                        " or to=" + std::to_string(e.to) +
                                        " exceeds packed edge-key limit of " +
                                        std::to_string(kMaxPackedIndex));
        }
        const auto key = (static_cast<std::uint64_t>(e.from) << 32U) | static_cast<std::uint64_t>(e.to);
        const auto it = best_by_edge.find(key);
        if (it == best_by_edge.end() || e.weight > it->second.weight) {
            best_by_edge[key] = e;
        }
    }

    std::vector<TensorGraphEdge> unique_candidates;
    unique_candidates.reserve(best_by_edge.size());
    for (const auto& kv : best_by_edge) {
      unique_candidates.push_back(kv.second);
    }
    std::sort(unique_candidates.begin(), unique_candidates.end(),
              [](const auto& a, const auto& b) { return a.weight > b.weight; });

    std::size_t added = 0;
    for (const auto& e : unique_candidates) {
        if (added >= cfg.diversity_budget) {
          break;
        }
        if (graph.addEdge(e)) {
          ++added;
        }
    }

    for (const auto& e : graph.edges()) {
        if (e.topology != "reshaped") {
          continue;
        }
        const auto avg_entropy = 0.5 * (entropy[e.from] + entropy[e.to]);
        if (avg_entropy >= (cfg.entropy_threshold * 1.5)) {
            const auto rerouted = graph.rerouteEdge(e.from, e.to, "clustered");
            if (!rerouted) {
                throw std::logic_error(
                    "failed to reroute edge from " + std::to_string(e.from) +
                    " to " + std::to_string(e.to) + " to clustered topology");
            }
        }
    }

    return graph;
}

QTTrain
HissReshaper::exposeQuantics(const storage::TTTrain& train, const std::vector<std::size_t>& grid_sizes) {
    if (train.cores.empty() || train.mode_sizes.empty()) {
        throw std::invalid_argument("train must contain at least one core and one mode size");
    }

    if (!grid_sizes.empty() && !train.mode_sizes.empty() && grid_sizes.size() != train.mode_sizes.size()) {
        throw std::invalid_argument("grid_sizes.size() (" + std::to_string(grid_sizes.size()) +
                                    ") must match train.mode_sizes.size() (" +
                                    std::to_string(train.mode_sizes.size()) + ")");
    }

    const auto resolved_grid_sizes = !grid_sizes.empty() ? grid_sizes : train.mode_sizes;
    const auto original_dense_elements = denseElementCount(train.mode_sizes);
    const auto reshaped_dense_elements = denseElementCount(resolved_grid_sizes);
    if (original_dense_elements != reshaped_dense_elements) {
        throw std::invalid_argument("grid_sizes product (" + std::to_string(reshaped_dense_elements) +
                                    ") must match train.mode_sizes product (" +
                                    std::to_string(original_dense_elements) + ")");
    }

    std::vector<std::size_t> bit_depths;
    std::vector<std::size_t> padded_grid_sizes;
    std::vector<std::size_t> quantics_mode_sizes;
    bit_depths.reserve(resolved_grid_sizes.size());
    padded_grid_sizes.reserve(resolved_grid_sizes.size());

    QuanticsFn quantics_fn;
    {
        std::lock_guard<std::mutex> lk(g_quantics_mtx);
        quantics_fn = g_quantics_fn;
    }
    if (quantics_fn) {
        return quantics_fn(train, grid_sizes);
    }

    // Pure-binary quantics layout:
    // - every physical dimension is padded to the next power-of-two extent
    // - the reshaped TT uses only size-2 quantics modes
    // - QTTrain metadata keeps both original and padded physical extents
    for (const auto grid_size : resolved_grid_sizes) {
        const auto bit_depth = calculateBitDepth(grid_size);
        bit_depths.push_back(bit_depth);
        const auto padded_grid_size = static_cast<std::size_t>(1ULL << bit_depth);
        padded_grid_sizes.push_back(padded_grid_size);
        quantics_mode_sizes.insert(quantics_mode_sizes.end(), bit_depth, std::size_t{2});
    }

    const auto dense_tensor = train.reconstruct();
    const auto padded_dense_elements = denseElementCount(padded_grid_sizes);
    std::vector<float> padded_dense_tensor(padded_dense_elements, 0.0f);

    // Place values according to reversible physical->QTT mapping so payload
    // and padding align with the non-linear quantics index space.
    QTTMappingDescriptor dense_to_qtt;
    dense_to_qtt.grid_sizes = resolved_grid_sizes;
    dense_to_qtt.padded_grid_sizes = padded_grid_sizes;
    dense_to_qtt.bit_depths = bit_depths;
    for (std::size_t physical_idx = 0; physical_idx < dense_tensor.size(); ++physical_idx) {
        const auto qtt_idx = dense_to_qtt.physicalToQTT(physical_idx);
        padded_dense_tensor[qtt_idx] = dense_tensor[physical_idx];
    }

    storage::TTTrain reshaped_train;
    // Avoid numerical drift for small quantics tensors in strict roundtrip
    // tests by building an exact binary TT directly.
    if (padded_dense_elements <= 2048) {
        reshaped_train = buildExactBinaryTT(padded_dense_tensor, quantics_mode_sizes.size());
    } else {
        storage::TensorTrainDecomposer decomposer;
        storage::TensorTrainConfig cfg;
        cfg.eps = 0.0;
        cfg.max_rank = 0;
        auto decomposed = decomposer.decompose(padded_dense_tensor, quantics_mode_sizes, cfg);
        reshaped_train = std::move(decomposed.first);
    }
    reshaped_train.original_norm = train.original_norm;

    QTTrain qt;
    qt.bit_depths = std::move(bit_depths);
    qt.grid_sizes = resolved_grid_sizes;
    qt.padded_grid_sizes = std::move(padded_grid_sizes);
    qt.quantics_mode_sizes = quantics_mode_sizes;
    qt.original_element_count = dense_tensor.size();
    qt.tt_train = std::move(reshaped_train);

    // Populate the reversible mapping descriptor so callers can convert
    // between flat physical indices and flat QTT indices without data loss.
    qt.mapping.grid_sizes        = qt.grid_sizes;
    qt.mapping.padded_grid_sizes = qt.padded_grid_sizes;
    qt.mapping.bit_depths        = qt.bit_depths;

    return qt;
}

void TemplateCatalog::registerTemplate(const std::string& domain_tag, TensorNetworkGraph graph) {
    std::lock_guard<std::mutex> lk(mutex_);
    templates_[domain_tag] = std::move(graph);
}

std::optional<TensorNetworkGraph> TemplateCatalog::lookup(const std::string& domain_tag) const {
    std::lock_guard<std::mutex> lk(mutex_);
    const auto it = templates_.find(domain_tag);
    if (it == templates_.end()) {
      return std::nullopt;
    }
    return it->second;
}

std::size_t TemplateCatalog::size() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return templates_.size();
}

} // namespace tensor
} // namespace themis
