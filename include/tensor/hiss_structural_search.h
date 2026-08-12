/**
 * @file hiss_structural_search.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

/** @brief Tensor network graph component. */
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

/** @brief Hiss structural search engine component. */
class HissStructuralSearchEngine {
public:
    /**
     * @brief Builds an optimized TensorNetworkGraph from a TT train.
     */
    [[nodiscard]] TensorNetworkGraph
    search(const storage::TTTrain& train, const HissConfig& cfg) const;
};

/**
 * @brief Reversible index mapping descriptor for a pure-binary QTT layout.
 *
 * After `HissReshaper::exposeQuantics()` pads each physical dimension to the
 * next power-of-two extent and decomposes the zero-padded tensor into an
 * all-2 quantics-mode train, this descriptor provides lossless, O(D·B)
 * functions to convert between:
 *  - a **flat physical index** into the original (unpadded) tensor, and
 *  - a **flat QTT index** into the all-2 quantics-mode tensor.
 *
 * ### Bit-ordering convention (MSB-first per dimension)
 * For dimension @p d with `bit_depths[d]` = B_d bits, the most-significant
 * bit of the per-dimension index is stored in the first QTT mode for that
 * dimension and the least-significant bit in the last mode. Modes from
 * successive dimensions are concatenated in C-contiguous order.
 *
 * ### Padding region
 * QTT indices whose per-dimension index meets or exceeds `grid_sizes[d]`
 * correspond to the zero-padded region and have no valid physical
 * counterpart. `qttToPhysical()` returns `std::nullopt` for those indices.
 *
 * ### Preconditions
 * - `grid_sizes`, `padded_grid_sizes`, and `bit_depths` must all have the
 *   same non-zero length (number of physical dimensions).
 * - `padded_grid_sizes[d] == (1 << bit_depths[d])` for every dimension @p d.
 * - `padded_grid_sizes[d] >= grid_sizes[d]` for every dimension @p d.
 */
struct QTTMappingDescriptor {
    std::vector<std::size_t> grid_sizes;         ///< Original (unpadded) physical extents.
    std::vector<std::size_t> padded_grid_sizes;  ///< Power-of-2 padded extents (2^bit_depths[d]).
    std::vector<std::size_t> bit_depths;         ///< Number of binary QTT modes per dimension.

    /**
     * @brief Convert a flat physical index to a flat QTT index.
     *
        * Interprets @p physical_idx as a C-contiguous (row-major) flat index into
        * a tensor of shape `grid_sizes`, extracts the per-dimension indices, and
        * encodes each dimension's index into `bit_depths[d]` binary QTT modes
        * (MSB first), yielding a flat C-contiguous index into the all-2
        * quantics-mode tensor.
     *
     * @param physical_idx  Flat index into the unpadded tensor.
     *                      Must satisfy `physical_idx < product(grid_sizes)`.
     * @return Flat QTT index into the quantics-mode tensor.
     * @throws std::out_of_range if `physical_idx >= product(grid_sizes)`.
     * @throws std::invalid_argument if the descriptor is empty or inconsistent.
     */
    [[nodiscard]] std::size_t physicalToQTT(std::size_t physical_idx) const;

    /**
     * @brief Convert a flat QTT index to a flat physical index.
     *
        * Interprets @p qtt_idx as a C-contiguous (row-major) flat index into the
        * all-2 quantics-mode tensor, reconstructs the per-dimension indices from
        * the packed bit sequence (MSB first per dimension), and maps them back to
        * a flat physical index in the original unpadded tensor.
     *
     * @param qtt_idx  Flat index into the quantics-mode tensor.
     *                 Must satisfy `qtt_idx < product(padded_grid_sizes)`.
     * @return The corresponding flat physical index in the unpadded tensor,
     *         or `std::nullopt` when @p qtt_idx addresses a zero-padded element
     *         (any reconstructed per-dimension index ≥ `grid_sizes[d]`).
     * @throws std::out_of_range if `qtt_idx >= product(padded_grid_sizes)`.
     * @throws std::invalid_argument if the descriptor is empty or inconsistent.
     */
    [[nodiscard]] std::optional<std::size_t> qttToPhysical(std::size_t qtt_idx) const;
};

/**
 * @brief Quantics Tensor Train representation with reversible index metadata.
 *
 * Produced by `HissReshaper::exposeQuantics()`.  Contains the pure-binary
 * QTT decomposition of the (zero-padded) input tensor plus all metadata
 * required to round-trip back to the original dense tensor without data loss.
 *
 * The `mapping` field provides a `QTTMappingDescriptor` that maps flat
 * physical indices (into the unpadded tensor) to flat QTT indices and vice
 * versa; use it to distinguish valid payload elements from zero-padding when
 * reconstructing the original dense tensor from `toTTTrain()`.
 */
struct QTTrain {
    std::vector<std::size_t> bit_depths;         ///< Binary quantics depth per physical dimension.
    std::vector<std::size_t> grid_sizes;         ///< Original (unpadded) physical extents.
    std::vector<std::size_t> padded_grid_sizes;  ///< Power-of-2 padded extents.
    std::vector<std::size_t> quantics_mode_sizes;///< All-2 mode sizes for the quantics TT.
    std::size_t original_element_count = 0;      ///< Number of elements in the unpadded tensor.
    storage::TTTrain         tt_train;           ///< Underlying TT train in quantics layout.
    QTTMappingDescriptor     mapping;            ///< Reversible physical ↔ QTT index map.

    [[nodiscard]] storage::TTTrain toTTTrain() const { return tt_train; }
};

/** @brief Hiss reshaper. */
class HissReshaper {
public:
    // -------------------------------------------------------------------------
    // Optional external quantics encoder bridge (STUB #254)
    // -------------------------------------------------------------------------

    /**
     * @brief Callable type for a custom quantics reshape encoding backend.
     *
     * When installed via `setQuanticsFn()`, the callable is invoked instead
     * of the built-in pure-binary padded-QTT path.  The callable MUST:
     *  - satisfy the same size/product contracts as `exposeQuantics()`, and
     *  - populate `QTTrain::mapping` if the caller needs reversible index
     *    mapping (the built-in path always populates it).
     *
     * @param train      Input TT train.
     * @param grid_sizes Physical dimension sizes (may be empty → use mode_sizes).
     * @return QTTrain in the quantics layout.
     */
    using QuanticsFn = std::function<QTTrain(const storage::TTTrain&        train,
                                             const std::vector<std::size_t>& grid_sizes)>;

    /**
     * @brief Install a custom quantics encode backend.
     *
     * Thread-safe.  Replaces any previously installed backend.
     * Pass `nullptr` (or call `clearQuanticsFn()`) to restore the built-in
     * pure-binary padded-QTT path.
     */
    static void setQuanticsFn(QuanticsFn fn);

    /// Remove the custom quantics backend; the built-in path is used again.  Thread-safe.
    static void clearQuanticsFn();

    /// Return the currently installed QuanticsFn (empty if none).
    static QuanticsFn getQuanticsFn();

    /**
     * @brief Reinterprets a TT train in a pure-binary quantics-friendly layout.
     *
     * Reconstructs the dense tensor, pads each physical dimension to the next
     * power-of-two extent with trailing zeros (in flat C-contiguous element
     * order), and decomposes the padded tensor into an all-2 quantics-mode
     * train.  The returned `QTTrain` carries:
     *
     *  - `grid_sizes` / `padded_grid_sizes` — original and padded extents,
     *  - `bit_depths` — binary depth per dimension (`log₂(padded_grid_sizes[d])`),
     *  - `original_element_count` — number of valid (non-padding) elements,
     *  - `mapping` — `QTTMappingDescriptor` for lossless physical↔QTT index
     *    conversion; use it to distinguish valid payload from zero-padding
     *    when round-tripping back to the original dense tensor.
     *
     * Dimensions that are already a power of two are not padded further
     * (`grid_sizes[d] == padded_grid_sizes[d]`).
     *
     * When a `QuanticsFn` backend is installed via `setQuanticsFn()`, the
     * backend is called instead of the built-in path; in that case `mapping`
     * is the responsibility of the custom backend.
     *
     * @param train       Input TT train.  Must be non-empty.
     * @param grid_sizes  Physical dimension sizes for the reshaped view.
     *                    If empty, `train.mode_sizes` is used verbatim.
     *                    Must have the same length as `train.mode_sizes` and
     *                    the same element product when non-empty.
     * @return `QTTrain` in the pure-binary quantics layout.
     * @throws std::invalid_argument if `train` is empty, `grid_sizes` length
     *         mismatches `train.mode_sizes`, or element products differ.
     */
    [[nodiscard]] static QTTrain
    exposeQuantics(const storage::TTTrain& train, const std::vector<std::size_t>& grid_sizes);
};

/** @brief Template catalog. */
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
