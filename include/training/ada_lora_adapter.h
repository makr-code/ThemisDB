/**
 * @file ada_lora_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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
    std::string layer_name;         ///< Layer identifier
    size_t      max_rank    = 0;    ///< Maximum allowed rank for this layer
    size_t      active_rank = 0;    ///< Currently active (unpruned) rank
    float       importance  = 0.0f; ///< Mean triplet score over all max_rank triplets
    float       alpha       = 0.0f; ///< LoRA alpha (scaling = alpha / max_rank)
};

/**
 * @brief Result of a rank-reallocation pass.
 *
 * Invalid budgets are reported via exceptions, so a returned result always
 * has @c success == true.
 */
struct ReallocResult {
    bool success = true;            ///< Always true for a returned result
    size_t total_active_rank = 0;   ///< Sum of active ranks after reallocation (== budget)
    size_t layers_pruned     = 0;   ///< Layers whose rank was reduced
    size_t layers_expanded   = 0;   ///< Layers whose rank was increased
};

/**
 * @brief AdaLoRA adapter with importance-based rank allocation
 *        (Zhang et al., ICLR 2023, arXiv:2303.10512).
 *
 * Each layer stores ΔW = B · A with B (in_dim × max_rank) and A
 * (max_rank × out_dim).  Rank component i (column B[:,i], row A[i,:]) is one
 * AdaLoRA triplet with P = B and Q = A.  The class does not hold a separate
 * diagonal Λ (it is folded into the factors), therefore the triplet score is
 *   S_i = mean_j s(B[j,i]) + mean_j s(A[i,j]),
 * where s(w) = Ī(w) · Ū(w) is the smoothed sensitivity times its uncertainty,
 * maintained by updateImportanceFromGradients().
 *
 * reallocateRanks() ranks all triplets globally, keeps exactly @c budget of
 * them (at least one per layer), and reorders the rank components within
 * each layer so that the active set is always the prefix [0, active_rank).
 * The full-rank product B · A is invariant under this reordering.
 *
 * Usage:
 * @code
 * AdaLoRAAdapter ada(4, 8.0f, 128);
 * ada.addLayer("q_proj", 768, 768, 8);
 * ada.addLayer("v_proj", 768, 768, 8);
 * ada.setBudgetSchedule(16, 8, 100, 200);   // b(0)=16, b(T)=8, t_i=100, t_f=200
 *
 * // Training loop, after backward() and before the optimiser step:
 * ada.updateImportanceFromGradients("q_proj", grad_B_q, grad_A_q);
 * ada.updateImportanceFromGradients("v_proj", grad_B_v, grad_A_v);
 * ada.reallocateRanks(ada.budgetAt(step, total_steps));
 * double reg = ada.orthogonalityPenalty("q_proj");   // R(P,Q)
 * @endcode
 *
 * Not thread-safe: an instance must not be shared between threads without
 * external synchronisation.
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
     * @brief Weight-magnitude fallback score (no gradients available).
     *
     * Sets triplet score S_i = ||B[:,i]||² · ||A[i,:]||² for every one of the
     * max_rank triplets.  This is a magnitude heuristic, NOT the AdaLoRA
     * sensitivity score; use updateImportanceFromGradients() during training.
     * Any previously accumulated sensitivity EMA state of the layer is kept
     * but no longer used for ranking until the next gradient update.
     *
     * @param layer_name  Target layer
     * @throws std::out_of_range if the layer does not exist
     */
    void updateImportance(const std::string& layer_name);

    /**
     * @brief Apply the magnitude fallback updateImportance() to all layers.
     */
    void updateAllImportances();

    /**
     * @brief AdaLoRA sensitivity-based importance update for one layer.
     *
     * For every element w of B and A (using the currently stored weights and
     * the gradients of the loss w.r.t. them at the same step):
     *   I(t) = |w · ∂L/∂w|
     *   Ī(t) = β1 · Ī(t-1) + (1-β1) · I(t)
     *   Ū(t) = β2 · Ū(t-1) + (1-β2) · |I(t) - Ī(t)|
     *   s    = Ī(t) · Ū(t)
     * and the triplet score becomes S_i = mean_j s(B[j,i]) + mean_j s(A[i,j]).
     * Ī and Ū start at zero.  Call before the optimiser updates the weights.
     *
     * @param layer_name  Target layer
     * @param grad_B      ∂L/∂B, same layout as B (in_dim × max_rank, row-major)
     * @param grad_A      ∂L/∂A, same layout as A (max_rank × out_dim, row-major)
     * @throws std::out_of_range     if the layer does not exist
     * @throws std::invalid_argument on size mismatch or non-finite gradients
     */
    void updateImportanceFromGradients(const std::string&        layer_name,
                                       const std::vector<float>& grad_B,
                                       const std::vector<float>& grad_A);

    /**
     * @brief Configure the EMA coefficients for sensitivity smoothing.
     * @param beta1  Sensitivity EMA coefficient β1, in [0, 1) (default 0.85)
     * @param beta2  Uncertainty EMA coefficient β2, in [0, 1) (default 0.85)
     * @throws std::invalid_argument if a coefficient is outside [0, 1)
     */
    void setSensitivityBetas(float beta1, float beta2);

    /** @brief Current (β1, β2) EMA coefficients. */
    std::pair<float, float> sensitivityBetas() const;

    /**
     * @brief Current per-triplet scores S_i of a layer (max_rank entries).
     *
     * Entry i corresponds to rank component i in the current physical order
     * (entries [0, active_rank) are the active triplets).
     * @throws std::out_of_range if the layer does not exist
     */
    std::vector<float> getTripletScores(const std::string& layer_name) const;

    /**
     * @brief Reallocate exactly @p total_budget ranks by global triplet ranking.
     *
     * Every layer keeps its best triplet (minimum rank 1); the remaining
     * total_budget - layerCount() slots go to the highest-scoring remaining
     * triplets across all layers, bounded by each layer's max_rank.  Ties are
     * broken deterministically by (position within the layer ascending, layer
     * insertion order ascending), which yields a round-robin distribution
     * when all scores are equal.  Rank components are reordered within each
     * layer (weights and score state together) so that the active triplets
     * occupy indices [0, active_rank).
     *
     * @param total_budget  Total number of active ranks after the call
     * @return ReallocResult with total_active_rank == total_budget
     * @throws std::invalid_argument if total_budget < layerCount(),
     *         total_budget > sum of max ranks, or total_budget == 0
     */
    ReallocResult reallocateRanks(size_t total_budget);

    /**
     * @brief Reallocate using the adapter's configured rank_budget.
     * @throws std::invalid_argument under the same conditions as
     *         reallocateRanks(size_t).
     */
    ReallocResult reallocateRanks();

    // -------------------------------------------------------------------------
    // Budget schedule and orthogonality regulariser
    // -------------------------------------------------------------------------

    /**
     * @brief Configure the cubic AdaLoRA budget schedule.
     *
     * @param initial_budget  b(0) (>= target_budget)
     * @param target_budget   b(T) (> 0)
     * @param warmup_steps    t_i: steps that keep b(0)
     * @param final_steps     t_f: final steps that keep b(T)
     * @throws std::invalid_argument if target_budget == 0 or
     *         initial_budget < target_budget
     */
    void setBudgetSchedule(size_t initial_budget,
                           size_t target_budget,
                           size_t warmup_steps,
                           size_t final_steps);

    /**
     * @brief Budget b(t) of the configured cubic schedule.
     *
     *   b(t) = b(0)                                                 t < t_i
     *   b(t) = floor(b(T) + (b(0)-b(T)) · (1 - (t-t_i)/(T-t_i-t_f))³) t_i <= t < T-t_f
     *   b(t) = b(T)                                                 otherwise
     *
     * @param step         Current step t
     * @param total_steps  Total number of training steps T (> t_i + t_f)
     * @throws std::logic_error      if no schedule was configured
     * @throws std::invalid_argument if total_steps <= t_i + t_f
     */
    size_t budgetAt(size_t step, size_t total_steps) const;

    /**
     * @brief Orthogonality regulariser R(P,Q) = ||PᵀP − I||²_F + ||QQᵀ − I||²_F
     *        for a layer, with P = B and Q = A over all max_rank components.
     * @throws std::out_of_range if the layer does not exist
     */
    double orthogonalityPenalty(const std::string& layer_name) const;

    /**
     * @brief Gradients of R(P,Q) w.r.t. B and A.
     *
     * ∂R/∂B = 4 · B (BᵀB − I),  ∂R/∂A = 4 · (AAᵀ − I) A, returned in the
     * same layouts as getWeights().
     * @throws std::out_of_range if the layer does not exist
     */
    std::pair<std::vector<float>, std::vector<float>>
    orthogonalityPenaltyGradients(const std::string& layer_name) const;

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
     * @brief Mean triplet score of a layer over all max_rank triplets.
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
     * The file format (host byte order) is:
     *   - 8-byte magic ("ADALORA\0")
     *   - 4-byte uint32 version (currently 2; other versions are rejected)
     *   - 64-byte model fingerprint (SHA-256 hex, NUL-padded)
     *   - uint64 default_rank, float default_alpha, uint64 rank_budget
     *   - float β1, float β2
     *   - uint8 schedule_configured, uint64 b(0), b(T), t_i, t_f
     *   - 4-byte uint32 layer count
     *   - For each layer (in insertion order):
     *       - uint32 name length, name bytes (UTF-8, no NUL)
     *       - uint64 in_dim, out_dim, max_rank, active_rank
     *       - float alpha, float importance
     *       - uint8 score_source, uint64 sensitivity_steps
     *       - B (in_dim × max_rank) and A (max_rank × out_dim) float32 values
     *       - triplet scores (max_rank float32 values)
     *       - uint8 has_sensitivity_state; if 1: Ī(B), Ū(B), Ī(A), Ū(A)
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
     * Replaces the complete adapter state (layers, weights, alpha, active
     * ranks, triplet scores, sensitivity EMA state, β1/β2, rank budget and
     * budget schedule).  On any error the current state is left unchanged.
     *
     * @param path  Source file path.
     * @return The model fingerprint string stored in the checkpoint header
     *         (empty if none was stored).
     * @throws std::runtime_error on I/O failure, bad magic, unsupported
     *         format version or inconsistent content.
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
