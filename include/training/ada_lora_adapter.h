/**
 * @file ada_lora_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace training {

/**
 * @brief Per-layer rank allocation and importance score snapshot.
 */
struct AdaLoRALayerStats {
    std::string layer_name;     ///< Layer identifier
    size_t      max_rank   = 0; ///< Maximum allowed rank for this layer
    size_t      active_rank = 0;///< Currently active (unpruned) rank
    float       importance = 0.0f; ///< Estimated importance score (higher = more important)
};

/**
 * @brief Result of a rank-reallocation pass.
 */
struct ReallocResult {
    bool success = true;           ///< Legacy compatibility flag
    size_t total_active_rank = 0;   ///< Sum of active ranks after reallocation
    size_t layers_pruned     = 0;   ///< Layers whose rank was reduced
    size_t layers_expanded   = 0;   ///< Layers whose rank was increased
};

/**
 * @brief AdaLoRA adapter with importance-based rank pruning.
 *
 * Usage:
 * @code
 * AdaLoRAAdapter ada(4, 8.0f, 128);  // default_rank=4, scale=8.0, total_rank_budget=128
 * ada.addLayer("q_proj", 768, 768, 8);
 * ada.addLayer("v_proj", 768, 768, 8);
 * ada.addLayer("k_proj", 768, 768, 4);
 *
 * // After a training step, update importance and reallocate:
 * ada.updateImportance("q_proj");
 * ada.updateImportance("v_proj");
 * ada.updateImportance("k_proj");
 * ada.reallocateRanks(20);  // total budget of 20
 *
 * // Forward pass using active rank only:
 * auto out = ada.forward("q_proj", input, batch_size);
 * @endcode
 */
class AdaLoRAAdapter {
public:
    /**
     * @brief Construct AdaLoRA adapter.
     * @param default_rank   Default maximum rank per layer (> 0)
     * @param default_alpha  Default LoRA alpha (> 0)
     * @param rank_budget    Global rank budget used by reallocateRanks() (> 0)
     */
    explicit AdaLoRAAdapter(size_t default_rank   = 4,
                            float  default_alpha  = 8.0f,
                            size_t rank_budget    = 64);

    ~AdaLoRAAdapter();

    AdaLoRAAdapter(const AdaLoRAAdapter&)            = delete;
    AdaLoRAAdapter& operator=(const AdaLoRAAdapter&) = delete;
    AdaLoRAAdapter(AdaLoRAAdapter&&) noexcept;
    AdaLoRAAdapter& operator=(AdaLoRAAdapter&&) noexcept;

    // -------------------------------------------------------------------------
    // Layer management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a new adapter layer.
     *
     * B is Kaiming-uniform initialised; A is zero-initialised.  The effective
     * rank starts at @p max_rank (or the adapter default if 0).
     *
     * @param layer_name  Unique identifier (must not already exist)
     * @param in_dim      Input feature dimension (> 0)
     * @param out_dim     Output feature dimension (> 0)
     * @param max_rank    Maximum rank for this layer (0 = use adapter default)
     * @param alpha       LoRA alpha override (0.0 = use adapter default)
     * @throws std::invalid_argument on bad dimensions or duplicate name
     */
    void addLayer(const std::string& layer_name,
                  size_t in_dim,
                  size_t out_dim,
                  size_t max_rank = 0,
                  float  alpha    = 0.0f);

    /**
     * @brief Remove a registered layer.
     * @return true if the layer existed and was removed; false otherwise.
     */
    bool removeLayer(const std::string& layer_name);

    /** @brief Whether a layer with the given name is registered. */
    bool hasLayer(const std::string& layer_name) const;

    /** @brief Names of all registered layers (order unspecified). */
    std::vector<std::string> layerNames() const;

    /** @brief Number of registered layers. */
    size_t layerCount() const;

    // -------------------------------------------------------------------------
    // Importance scoring and rank reallocation
    // -------------------------------------------------------------------------

    /**
     * @brief Recompute the importance score for a layer.
     *
     * Importance is estimated as the mean squared Frobenius norm of the rank-
     * component contributions: sum_i( ||B[:,i]||^2 * ||A[i,:]||^2 ) / active_rank.
     * This approximates the singular-value importance without a full SVD.
     *
     * @param layer_name  Target layer
     * @throws std::out_of_range if the layer does not exist
     */
    void updateImportance(const std::string& layer_name);

    /**
     * @brief Update importance for all registered layers.
     */
    void updateAllImportances();

    /**
     * @brief Reallocate rank budget across layers based on current importance scores.
     *
     * Distributes @p total_budget rank slots proportionally to each layer's
     * normalised importance score, subject to [1, max_rank] per-layer bounds.
     * Layers with zero importance receive rank 1 (minimum).
     *
     * @param total_budget  Global rank budget to distribute (> 0)
     * @return ReallocResult describing the changes made
     * @throws std::invalid_argument if total_budget is zero
     */
    ReallocResult reallocateRanks(size_t total_budget);

    /**
     * @brief Reallocate using the adapter's configured rank_budget.
     */
    ReallocResult reallocateRanks();

    // -------------------------------------------------------------------------
    // Rank and importance access
    // -------------------------------------------------------------------------

    /**
     * @brief Current active rank for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    size_t getActiveRank(const std::string& layer_name) const;

    /**
     * @brief Maximum rank for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    size_t getMaxRank(const std::string& layer_name) const;

    /**
     * @brief Current importance score for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    float getImportance(const std::string& layer_name) const;

    /**
     * @brief Snapshot of all layer statistics.
     * @return Vector of AdaLoRALayerStats (one per registered layer)
     */
    std::vector<AdaLoRALayerStats> getLayerStats() const;

    /**
     * @brief Total active parameter count (sum over layers of 2 * active_rank *
     *        (in_dim + out_dim) / 2).
     */
    size_t totalActiveParameterCount() const;

    // -------------------------------------------------------------------------
    // Weight access
    // -------------------------------------------------------------------------

    /**
     * @brief Overwrite B and A matrices for a layer.
     *
     * @param layer_name  Target layer
     * @param B           Flat row-major B data (in_dim × max_rank floats)
     * @param A           Flat row-major A data (max_rank × out_dim floats)
     * @throws std::out_of_range    if the layer does not exist
     * @throws std::invalid_argument if the vector sizes do not match
     */
    void setWeights(const std::string&        layer_name,
                    const std::vector<float>& B,
                    const std::vector<float>& A);

    /**
     * @brief Read B and A weight matrices for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    std::pair<std::vector<float>, std::vector<float>>
    getWeights(const std::string& layer_name) const;

    // -------------------------------------------------------------------------
    // Forward pass
    // -------------------------------------------------------------------------

    /**
     * @brief Compute the LoRA contribution using only the active rank components.
     *
     * Performs (using only the first active_rank columns of B / rows of A):
     *   hidden = input @ B[:, :active_rank]     (batch_size × active_rank)
     *   output = hidden @ A[:active_rank, :]    (batch_size × out_dim)
     *   return  output × scaling               where scaling = alpha / max_rank
     *
     * @param layer_name  Layer whose weights are applied
     * @param input       Flat row-major input (batch_size × in_dim)
     * @param batch_size  Number of rows in @p input
     * @return Flat row-major output (batch_size × out_dim)
     * @throws std::out_of_range    if the layer does not exist
     * @throws std::invalid_argument if input size != batch_size × in_dim
     */
    std::vector<float> forward(const std::string&        layer_name,
                               const std::vector<float>& input,
                               size_t                    batch_size) const;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** @brief Global rank budget used by reallocateRanks(). */
    size_t rankBudget() const;

    /** @brief Update the global rank budget. */
    void setRankBudget(size_t budget);

    // -------------------------------------------------------------------------
    // Persistence — adapter checkpoint save / load
    // -------------------------------------------------------------------------

    /**
     * @brief Persist the complete adapter state to a binary checkpoint file.
     *
     * The file format is self-describing:
     *   - 8-byte magic ("ADALORA\0")
     *   - 4-byte uint32 version (currently 1)
     *   - 64-byte model fingerprint (SHA-256 hex, NUL-padded)
     *   - 4-byte uint32 layer count
     *   - For each layer (in insertion order):
     *       - 4-byte uint32 name length
     *       - name bytes (UTF-8, no NUL)
     *       - 8-byte uint64 in_dim, out_dim, max_rank, active_rank
     *       - 4-byte float alpha, importance
     *       - B matrix: (in_dim × max_rank) float32 values
     *       - A matrix: (max_rank × out_dim) float32 values
     *
     * @param path             Destination file path (parent directory must exist).
     * @param model_fingerprint Optional SHA-256 fingerprint of the base model
     *                          (64 hex chars); stored in the header for cache
     *                          invalidation.  Empty string stores all-zero bytes.
     * @throws std::runtime_error on I/O failure.
     */
    void saveToFile(const std::string& path,
                    const std::string& model_fingerprint = "") const;

    /**
     * @brief Restore adapter state from a binary checkpoint file.
     *
     * Replaces all current layers with the layers stored in the file.
     * The adapter's rank_budget is updated to match the stored budget
     * (sum of stored max_ranks).
     *
     * @param path  Source file path.
     * @return The model fingerprint string stored in the checkpoint header
     *         (64 hex chars, may be all zeros if none was stored).
     * @throws std::runtime_error on I/O failure or format mismatch.
     */
    std::string loadFromFile(const std::string& path);

    /**
     * @brief Check whether a saved checkpoint is still valid for the given model.
     *
     * Reads only the header of @p checkpoint_path and compares the stored
     * fingerprint against @p current_fingerprint.
     *
     * @param checkpoint_path     Path to a checkpoint written by saveToFile().
     * @param current_fingerprint SHA-256 fingerprint of the current base model.
     * @return true  — checkpoint exists and fingerprint matches (no rebuild needed).
     * @return false — checkpoint absent, unreadable, or fingerprint differs
     *                 (rebuild required).
     */
    static bool isCacheValid(const std::string& checkpoint_path,
                             const std::string& current_fingerprint);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
