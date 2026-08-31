/**
 * @file lora_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace training {

/**
 * @brief Weight entry for a single named LoRA adapter layer.
 *
 * Stores the low-rank matrices B and A along with the layer's
 * hyperparameters.  The effective weight delta is:
 *   ΔW = (B @ A) × scaling,   where scaling = alpha / rank
 *
 * Dimensions:
 *   B: (in_dim × rank)   – down-projection
 *   A: (rank × out_dim)  – up-projection
 */
struct LoRAWeightEntry {
    std::string        layer_name;          ///< Unique layer identifier
    size_t             in_dim    = 0;       ///< Input feature dimension
    size_t             out_dim   = 0;       ///< Output feature dimension
    size_t             rank      = 4;       ///< LoRA rank r
    float              alpha     = 8.0f;    ///< LoRA α  (scaling = alpha / rank)
    std::vector<float> B;                   ///< Down-projection (in_dim × rank)
    std::vector<float> A;                   ///< Up-projection   (rank × out_dim)
};

/**
 * @brief A batch of additive weight deltas for one or more layers.
 *
 * All three parallel vectors must have the same length.
 */
struct WeightUpdateBatch {
    std::vector<std::string>            layer_names; ///< Target layer identifiers
    std::vector<std::vector<float>>     delta_B;     ///< Additive delta for each B
    std::vector<std::vector<float>>     delta_A;     ///< Additive delta for each A
};

/**
 * @brief Result returned by weight-update operations.
 */
struct WeightUpdateResult {
    bool        success          = false;
    size_t      layers_updated   = 0;   ///< Number of layers actually modified
    size_t      layers_skipped   = 0;   ///< Unknown layer names that were skipped
    std::string error_message;
};

/**
 * @brief Training-module LoRA adapter for real weight manipulation.
 *
 * Manages a collection of named LoRA weight entries and provides:
 *  - Adding/removing adapter layers with configurable rank and alpha
 *  - Applying individual or batched additive weight updates (no simulation)
 *  - A real forward pass:  output = (input @ B @ A) × scaling
 *  - Exporting/importing weight snapshots for checkpoint integration
 *
 * Initialization convention (per the LoRA paper, §3):
 *  - B is Kaiming-uniform initialised (non-zero)
 *  - A is zero-initialised (so the initial adapter output is exactly zero)
 *
 * Thread-safety: NOT guaranteed.  Callers must provide external
 * synchronisation when sharing a single instance across threads.
 *
 * @note The public API is stable; internal implementation is Pimpl.
 */
class LoRAAdapter {
public:
    /**
     * @brief Construct a LoRA adapter.
     * @param default_rank  Default LoRA rank for addLayer() calls (> 0)
     * @param default_alpha Default LoRA alpha for addLayer() calls (> 0)
     */
    explicit LoRAAdapter(size_t default_rank = 4, float default_alpha = 8.0f);

    ~LoRAAdapter();

    LoRAAdapter(const LoRAAdapter&)            = delete;
    LoRAAdapter& operator=(const LoRAAdapter&) = delete;
    LoRAAdapter(LoRAAdapter&&) noexcept;
    LoRAAdapter& operator=(LoRAAdapter&&) noexcept;

    // -------------------------------------------------------------------------
    // Layer management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a new LoRA adapter layer.
     *
     * B is Kaiming-uniform initialised; A is zero-initialised so that the
     * initial adapter contribution to the base model is zero.
     *
     * @param layer_name Unique identifier (must not already exist)
     * @param in_dim     Input feature dimension (> 0)
     * @param out_dim    Output feature dimension (> 0)
     * @param rank       Override rank (0 = use adapter default)
     * @param alpha      Override alpha (0.0 = use adapter default)
     * @throws std::invalid_argument if any dimension or rank is zero, or if
     *         a layer with the same name already exists
     */
    void addLayer(const std::string& layer_name,
                  size_t in_dim,
                  size_t out_dim,
                  size_t rank  = 0,
                  float  alpha = 0.0f);

    /**
     * @brief Remove a layer entry.
     * @return true if the layer existed and was removed; false otherwise
     */
    bool removeLayer(const std::string& layer_name);

    /** @brief Whether a layer with the given name is registered. */
    bool hasLayer(const std::string& layer_name) const;

    /** @brief Names of all registered layers (order unspecified). */
    std::vector<std::string> layerNames() const;

    /** @brief Number of registered layers. */
    size_t layerCount() const;

    /** @brief Total trainable parameter count across all layers. */
    size_t totalParameterCount() const;

    // -------------------------------------------------------------------------
    // Weight access and mutation
    // -------------------------------------------------------------------------

    /**
     * @brief Read-only snapshot of the weight entry for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    const LoRAWeightEntry& getWeights(const std::string& layer_name) const;

    /**
     * @brief Overwrite the B and A matrices of an existing layer.
     *
     * @param layer_name Target layer
     * @param B          New flat row-major B data (must be in_dim × rank floats)
     * @param A          New flat row-major A data (must be rank × out_dim floats)
     * @throws std::out_of_range    if the layer does not exist
     * @throws std::invalid_argument if the vector sizes do not match
     */
    void setWeights(const std::string&        layer_name,
                    const std::vector<float>& B,
                    const std::vector<float>& A);

    /**
     * @brief Apply an additive delta to a single layer's B and A matrices.
     *
     *   B_new = B + delta_B
     *   A_new = A + delta_A
     *
     * @param layer_name Target layer
     * @param delta_B    Additive delta for B (same size as B)
     * @param delta_A    Additive delta for A (same size as A)
     * @return WeightUpdateResult (layers_updated == 1 on success)
     * @throws std::out_of_range    if the layer does not exist
     * @throws std::invalid_argument if delta sizes are mismatched
     */
    WeightUpdateResult applyUpdate(const std::string&        layer_name,
                                   const std::vector<float>& delta_B,
                                   const std::vector<float>& delta_A);

    /**
     * @brief Apply a batch of additive weight deltas atomically.
     *
     * Each (layer_names[i], delta_B[i], delta_A[i]) triple is applied in
     * sequence.  Unknown layer names are silently skipped and counted in
     * WeightUpdateResult::layers_skipped.
     *
     * @param batch Batch of deltas; all three parallel vectors must have the
     *              same length
     * @return WeightUpdateResult summarising how many layers were updated
     * @throws std::invalid_argument if the batch vectors have mismatched sizes
     */
    WeightUpdateResult applyBatchUpdate(const WeightUpdateBatch& batch);

    // -------------------------------------------------------------------------
    // Forward pass
    // -------------------------------------------------------------------------

    /**
     * @brief Compute the LoRA contribution for a given layer and input batch.
     *
     * Performs the following (no simulation):
     *   hidden = input @ B          (batch_size × rank)
     *   output = hidden @ A         (batch_size × out_dim)
     *   return  output × scaling    where scaling = alpha / rank
     *
     * @param layer_name Layer whose weights are applied
     * @param input      Flat row-major input (batch_size × in_dim)
     * @param batch_size Number of rows in @p input
     * @return Flat row-major output (batch_size × out_dim)
     * @throws std::out_of_range    if the layer does not exist
     * @throws std::invalid_argument if input size != batch_size × in_dim
     */
    std::vector<float> forward(const std::string&        layer_name,
                               const std::vector<float>& input,
                               size_t                    batch_size) const;

    // -------------------------------------------------------------------------
    // Serialisation
    // -------------------------------------------------------------------------

    /**
     * @brief Export copies of all weight entries.
     * @return Vector of LoRAWeightEntry (one per registered layer)
     */
    std::vector<LoRAWeightEntry> exportWeights() const;

    /**
     * @brief Import a set of weight entries, overwriting matching names.
     *
     * Entries whose layer_name does not yet exist are added automatically;
     * entries whose layer_name already exists replace the current weights.
     *
     * @param entries Vector of weight entries to import
     * @throws std::invalid_argument if any entry has inconsistent sizes
     */
    void importWeights(const std::vector<LoRAWeightEntry>& entries);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
