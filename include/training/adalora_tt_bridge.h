/**
 * @file adalora_tt_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=11; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "training/ada_lora_adapter.h"
#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace training {

// ============================================================================
// AdaLoraTTLayerExport — one adapter layer as a TT-train
// ============================================================================

/**
 * @brief One AdaLoRA adapter layer exported to TT-format.
 *
 * Each layer (identified by @p layer_name) contributes a rank-2 TT-train
 * (two cores G₀ and G₁) computed from the layer's B/A weight matrices and
 * current importance scores (singular values approximated from norms).
 *
 * The @p active_rank field mirrors AdaLoRALayerStats::active_rank and
 * controls how many TT-core columns are non-zero.
 */
struct AdaLoraTTLayerExport {
    std::string       layer_name;
    storage::TTTrain  train;      ///< TT-train: G₀ (1×d×r) and G₁ (r×k×1)
    std::size_t       active_rank = 0;  ///< Effective rank (pruned entries zeroed)
    float             scaling     = 1.0f; ///< α/r scaling factor (preserved separately)

    /// Whether the TT-cores passed orthogonality validation (‖G₀^T·G₀ - I‖_F < ε_orth).
    bool              orthogonal_validated = false;
};

// ============================================================================
// AdaLoraTTExport — complete adapter export (all layers)
// ============================================================================

struct AdaLoraTTExport {
    std::string                          adapter_name = {};
    std::string                          tenant = {};
    std::vector<AdaLoraTTLayerExport>    layers;

    /// Total active TT-rank (sum across layers).
    std::size_t totalActiveRank() const noexcept {
        std::size_t s = 0;
        for (const auto& l : layers) {
          s += l.active_rank;
        }
        return s;
    }

    /// Total number of float32 parameters stored (TT-cores only, not base weights).
    std::size_t totalParameters() const noexcept;
};

// ============================================================================
// AdaLoraTTBridgeConfig
// ============================================================================

struct AdaLoraTTBridgeConfig {
    /// Maximum rank for which TT-storage is recommended (latency break-even).
    /// Adapters with active_rank > max_tt_rank are stored in native format.
    std::size_t max_tt_rank = 64;

    /// Orthogonality tolerance for P/Q validation: ‖G₀^T·G₀ - I‖_F < ε_orth
    double eps_orth = 1e-4;

    /// Sign normalisation: ensure G₀[0, first_nonzero_row, 0] > 0 for
    /// canonical fingerprinting (prevents sign-ambiguity in LSH buckets).
    bool normalise_sign = true;

    /// If true, run `TensorDeduplicationManager::insert()` on every stored adapter
    /// to enable cross-adapter deduplication automatically.
    bool auto_deduplicate = true;

    /// Similarity threshold for dedup (same semantics as TensorDeduplicationManager).
    double dedup_similarity_threshold = 0.999;

    /// If true, apply TT-rounding (ε-optimal global rank cut) before storing.
    /// This replaces AdaLoRA's greedy rank allocation with a globally optimal cut.
    bool apply_tt_rounding = false;

    /// TT-rounding epsilon (active when apply_tt_rounding=true).
    double tt_rounding_eps = 0.01;
};

// ============================================================================
// AdaLoraTTBridge
// ============================================================================

/**
 * @brief Converts AdaLoRA adapters to TT-format and manages their lifecycle
 *        in `TensorNetworkStorageEngine`.
 *
 * ## Core Conversion (§2 of ADALORA_TT_BRIDGE_RESEARCH.md)
 *
 * For each adapter layer with weight matrices B ∈ ℝ^{d×r} and A ∈ ℝ^{r×k}:
 *
 * ```
 * // Step 1: Approximate singular values from importance norms
 * λᵢ ≈ ‖B[:, i]‖₂ · ‖A[i, :]‖₂           (for i = 0, …, r-1)
 *
 * // Step 2: Normalise to form approximate left/right singular vectors
 * p_i = B[:, i] / ‖B[:, i]‖₂               (if ‖B[:,i]‖ > 0)
 * q_i = A[i, :] / ‖A[i, :]‖₂
 *
 * // Step 3: Build TT-cores
 * G₀[0, :, i] = p_i · √λᵢ · √scaling      (shape: 1 × d × r)
 * G₁[i, :, 0] = q_i · √λᵢ · √scaling      (shape: r × k × 1)
 *
 * // Step 4: Zero out pruned (inactive) rank components
 * if i >= active_rank: G₀[0,:,i] = G₁[i,:,0] = 0
 * ```
 *
 * ## Inverse Conversion (TT → AdaLoRA)
 *
 * ```
 * B[:, i] = G₀[0, :, i]   (absorbing √λ · √scaling back into B)
 * A[i, :] = G₁[i, :, 0]
 * scaling  = stored separately in AdaLoraTTLayerExport::scaling
 * ```
 *
 * ## Usage Example (post-training export)
 *
 * ```cpp
 * AdaLoraTTBridge bridge(engine, config);
 *
 * // Export a trained adapter to TT-format and store it
 * auto exp = bridge.exportToTT(adapter, "legal_tax_v3", "my_tenant");
 * bridge.store(exp);
 *
 * // Later: load it back for training continuation
 * auto loaded = bridge.loadAdapter("my_tenant", "legal_tax_v3");
 *
 * // For inference: use GgmlTensorBridge to mmap the TT-cores directly
 * // (see include/storage/ggml_tensor_bridge.h)
 * ```
 *
 * ## Usage Example (FLARE live adapter switch)
 *
 * ```cpp
 * // Called from the FLARE retrieval callback mid-generation:
 * auto similar = bridge.findSimilarAdapters(current_adapter, top_k=1);
 * if (!similar.empty()) {
 *     auto handle = ggml_bridge.mapAdapter(ctx, similar[0].name, "my_tenant");
 *     if (handle.valid())
 *         llama_lora_apply(llama_ctx, handle.ggmlTensor(), 1.0f);
 * }
 * // Total latency: ~5–15 ms (vs 300–2000 ms for model reload)
 * ```
 */
class AdaLoraTTBridge {
public:
    explicit AdaLoraTTBridge(
        std::shared_ptr<storage::TensorNetworkStorageEngine> engine,
        AdaLoraTTBridgeConfig                                cfg = {});

    ~AdaLoraTTBridge();

    // Non-copyable
    AdaLoraTTBridge(const AdaLoraTTBridge&)            = delete;
    AdaLoraTTBridge& operator=(const AdaLoraTTBridge&) = delete;

    // -----------------------------------------------------------------------
    // Export: AdaLoRA → TT
    // -----------------------------------------------------------------------

    /**
     * @brief Export all layers of an AdaLoRAAdapter to TT-format.
     *
     * For each registered layer, converts B/A matrices to TT-cores G₀, G₁.
     * Applies QR sign-normalisation and optional TT-rounding per the config.
     *
     * @param adapter      Source AdaLoRAAdapter (must have at least one layer).
     * @param adapter_name Logical name for storage (used as field key).
     * @param tenant       ThemisDB tenant ID.
     * @return Complete AdaLoraTTExport with one entry per layer.
     *
     * @throws std::invalid_argument if any layer has active_rank == 0 or
     *         if active_rank > config.max_tt_rank (use native storage instead).
     */
    AdaLoraTTExport exportToTT(const AdaLoRAAdapter& adapter,
                                const std::string&    adapter_name,
                                const std::string&    tenant = "") const;

    /**
     * @brief Export a single named layer.
     *
     * @param adapter     Source AdaLoRAAdapter.
     * @param layer_name  Name of the layer to export.
     * @return TT-export for that layer.
     */
    AdaLoraTTLayerExport exportLayer(const AdaLoRAAdapter& adapter,
                                      const std::string&    layer_name) const;

    // -----------------------------------------------------------------------
    // Import: TT → AdaLoRA
    // -----------------------------------------------------------------------

    /**
     * @brief Reconstruct an AdaLoRAAdapter from TT-format.
     *
     * Inverse of exportToTT().  Validates orthogonality of reconstructed P/Q
     * matrices (‖P^T·P - I‖_F < eps_orth) and logs a warning if violated.
     *
     * @param exp       TT-export to import from.
     * @return Reconstructed AdaLoRAAdapter with weights set to G₀/G₁ columns.
     */
    AdaLoRAAdapter importFromTT(const AdaLoraTTExport& exp) const;

    // -----------------------------------------------------------------------
    // Storage
    // -----------------------------------------------------------------------

    /**
     * @brief Store an AdaLoraTTExport in TensorNetworkStorageEngine.
     *
     * Each layer is stored under key:
     *   `__lora_adapters__:<tenant>:<adapter_name>:<layer_name>:G<0|1>`
     *
     * If config.auto_deduplicate is true, inserts into TensorFingerprintGraph.
     *
     * @return true on success.
     */
    bool store(const AdaLoraTTExport& exp);

    /**
     * @brief Backward-compatible alias for store().
     *
     * Forwards to store(exp) to preserve existing call sites and tests that
     * still use the historical method name.
     */
    bool storeAdapter(const AdaLoraTTExport& exp) { return store(exp); }

    /**
     * @brief Load an adapter from storage and reconstruct as AdaLoRAAdapter.
     *
     * @param tenant       ThemisDB tenant.
     * @param adapter_name Adapter logical name.
     * @return Reconstructed AdaLoRAAdapter, or nullopt if not found.
     */
    std::optional<AdaLoRAAdapter> loadAdapter(const std::string& tenant,
                                               const std::string& adapter_name) const;

    // -----------------------------------------------------------------------
    // Unified rank control (TT-Rounding as drop-in for AdaLoRA pruning)
    // -----------------------------------------------------------------------

    /**
     * @brief Apply TT-rounding (globally optimal rank cut) to an exported adapter.
     *
     * TT-rounding minimises ‖ΔW - ΔW_approx‖_F for a given ε budget, making
     * it globally optimal vs. AdaLoRA's greedy singular-value pruning.
     *
     * After rounding, the new `active_rank` per layer is set to the number of
     * kept singular values and can be fed back to `AdaLoRAAdapter::reallocateRanks()`.
     *
     * @param exp    TT-export to round (modified in-place).
     * @param eps    Target Frobenius relative error (e.g. 0.01 = 1%).
     * @return Total active rank after rounding.
     *
     * STUB/SIMULATION NOTE:
     * Purpose: Calls TensorTrainDecomposer::round() per layer.
     * Activation: Used only post-training (not during backprop).
     * Production Delta: Full integration with AdaLoRA training loop requires
     *   differentiable TT-layer (ggml-autograd or libtorch custom op).
     * Removal Plan: Not removed; training-loop integration added in Phase 4.
     */
    std::size_t roundAndReallocate(AdaLoraTTExport& exp, double eps) const;

    // -----------------------------------------------------------------------
    // Cross-adapter similarity (Adapter Soup / TIES-Merging)
    // -----------------------------------------------------------------------

    struct SimilarAdapter {
        std::string  adapter_name;
        std::string  layer_name;
        double       similarity;   ///< TT-cosine similarity (0–1)
    };

    /**
     * @brief Find adapters in storage similar to a query adapter.
     *
     * Uses `TensorFingerprintGraph::findSimilar()` on the stored TT-cores.
     * Primary use-case: FLARE live adapter selection (≤ 15 ms per call).
     *
     * @param query_exp   TT-export to compare against stored adapters.
     * @param top_k       Maximum number of results.
     * @param tenant      Tenant scope (empty = all tenants).
     */
    std::vector<SimilarAdapter> findSimilarAdapters(const AdaLoraTTExport& query_exp,
                                                      std::size_t            top_k = 5,
                                                      const std::string&     tenant = "") const;

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    struct BridgeStats {
        std::size_t exports_total        = 0;
        std::size_t imports_total        = 0;
        std::size_t stores_total         = 0;
        std::size_t dedup_hits           = 0;    ///< Layers deduplicated
        std::size_t orth_violations      = 0;    ///< Orthogonality check failures
        double      avg_export_us        = 0.0;
        double      avg_compression_ratio = 0.0; ///< TT vs. flat B+A matrices
    };

    BridgeStats stats() const noexcept;

    const AdaLoraTTBridgeConfig& config() const noexcept;

    // -----------------------------------------------------------------------
    // Phase 3 bridge — GgmlTensorBridge::mapAdapter() injection (STUB #271)
    // -----------------------------------------------------------------------

    /**
     * @brief Injectable backend for zero-copy adapter mapping (Phase 3).
     *
     * Signature: `bool fn(const AdaLoraTTExport& exp)`.
     *
     * When set, `mapAdapter(exp)` delegates to this function instead of
     * returning `false` (the Phase 3 stub).  Typically wired to
     * `GgmlTensorBridge::mapAdapter()` during production bootstrap.
     *
     * Thread-safe (instance-level; stored in Impl).
     */
    using MapAdapterFn = std::function<bool(const AdaLoraTTExport&)>;

    /**
     * @brief Set the Phase 3 GgmlTensorBridge::mapAdapter() bridge.
     * @param fn  Function called by `mapAdapter()` to inject TT-cores into llama.cpp.
     */
    void setMapAdapterFn(MapAdapterFn fn);
    void clearMapAdapterFn();

    /**
     * @brief Map a stored adapter into the GGML context (Phase 3 bridge).
     *
     * Delegates to the injected `MapAdapterFn` when set.  Returns `false`
     * when no fn is injected (Phase 3 not yet wired — STUB #271).
     *
     * @param exp  AdaLoraTTExport to inject.
     * @return true on success; false when the bridge is not yet wired.
     */
    bool mapAdapter(const AdaLoraTTExport& exp) const;

    // -----------------------------------------------------------------------
    // Phase 4 bridge — training-loop integration for roundAndReallocate (STUB #271)
    // -----------------------------------------------------------------------

    /**
     * @brief Injectable training-step backend (Phase 4).
     *
     * Signature: `std::size_t fn(AdaLoraTTExport& exp, double eps)`.
     *
     * When set, `roundAndReallocate()` delegates to this function instead of
     * using the standalone `TensorTrainDecomposer::round()` path.  The fn
     * receives the full export and epsilon budget; it is responsible for
     * interfacing with the live training loop (e.g. ggml-autograd or libtorch
     * custom op) and returning the total active rank after the step.
     *
     * Thread-safe (process-wide static; same pattern as FourierTransformFn).
     */
    using TrainingStepFn = std::function<std::size_t(AdaLoraTTExport&, double)>;

    /** @brief Inject the Phase 4 training-loop backend. Thread-safe. */
    static void setTrainingStepFn(TrainingStepFn fn);
    /** @brief Clear the Phase 4 training-loop backend. */
    static void clearTrainingStepFn();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
