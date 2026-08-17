/**
 * @file tensor_ingestion_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "ingestion/inference_backend.h"
#include "storage/tensor_train_decomposer.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// TensorIngestionBridge
// ============================================================================

/**
 * @brief Production implementation of `ingestion::ITensorDecompositionBackend`.
 *
 * Wraps `storage::TensorTrainDecomposer` and performs:
 *  1. **κ-gate** (`shouldDecompose`) — cheap pilot decomposition to check
 *     whether the data is worth compressing.
 *  2. **Mode-shape inference** — an embedding of length `d` is reshaped into a
 *     2D tensor (√d × √d when d is a perfect square, otherwise into a pair of
 *     factors maximally balanced in size).  This gives order-2 TT-cores which
 *     are mathematically equivalent to the AdaLoRA SVD format (see
 *     `ADALORA_TT_BRIDGE_RESEARCH.md §2.3`).
 *  3. **Provenance** — the resulting `TensorCoreRecord` metadata includes
 *     `epsilon`, `max_rank`, and `mode_shape` to allow faithful reconstruction
 *     and auditing by downstream consumers (FITKO, eJustice).
 */
class TensorIngestionBridge : public ingestion::ITensorDecompositionBackend {
public:
    /**
     * @brief Construct with optional global configuration overrides.
     *
     * Configuration can be overridden per-call via the `decompose()` arguments.
     * These settings serve as defaults when the ingestion step config does not
     * specify values.
     *
     * @param default_epsilon   Default ε for TT-SVD (default: 0.01).
     * @param default_max_rank  Default bond-dimension cap (0 = no cap).
     * @param default_min_kappa Default κ-gate threshold (default: 1.3).
     */
    explicit TensorIngestionBridge(double      default_epsilon   = 0.01,
                                   std::size_t default_max_rank  = 0,
                                   double      default_min_kappa = 1.3);

    ~TensorIngestionBridge() override = default;

    // Non-copyable (stateful counters, shared decomposer)
    TensorIngestionBridge(const TensorIngestionBridge&)            = delete;
    TensorIngestionBridge& operator=(const TensorIngestionBridge&) = delete;
    TensorIngestionBridge(TensorIngestionBridge&&)            noexcept noexcept = default;
    TensorIngestionBridge& operator=(TensorIngestionBridge&&) noexcept noexcept = default;

    // ── Configuration setters (call before multi-threaded use) ────────────

    /// Override the default reconstruction error tolerance ε.
    void setEpsilon(double eps)        noexcept { default_epsilon_   = eps; }
    /// Override the default bond-dimension cap (0 = no cap).
    void setMaxRank(std::size_t rank)  noexcept { default_max_rank_  = rank; }
    /// Override the default κ-gate threshold.
    void setMinKappa(double kappa)     noexcept { default_min_kappa_ = kappa; }

    // ── ITensorDecompositionBackend ───────────────────────────────────────

    /**
     * @brief Decompose a dense embedding into TT-format.
     *
     * Infers mode-shapes automatically (balanced 2D factorisation) and
     * delegates to `TensorTrainDecomposer::decompose()`.
     *
     * @param embedding     Dense float32 vector produced by `chunk_embed`.
     * @param chunk_id      Identifier linking result to the chunk.
     * @param source_file_id SHA-256 of the source file.
     * @param epsilon       Relative error ε (overrides `default_epsilon_`
     *                      when > 0).
     * @param max_rank      Bond cap (overrides `default_max_rank_` when > 0).
     * @return Populated `TensorCoreRecord`.  `serialized_train` is empty on
     *         error (caller should log the warning in `ctx.warnings`).
     */
    [[nodiscard]] ingestion::TensorCoreRecord decompose(
        const std::vector<float>& embedding,
        const std::string&        chunk_id,
        const std::string&        source_file_id,
        double                    epsilon   = 0.0,
        std::size_t               max_rank  = 0) override;

    /**
     * @brief κ-gate: returns true when estimated compression ratio ≥ `min_kappa`.
     *
     * Runs a pilot TT-decomposition with a coarser tolerance (pilot_eps =
     * epsilon / 5, min 0.05) to quickly estimate the achievable compression
     * ratio without the full cost.
     *
     * For embeddings with dim ≤ 1024, the full embedding is used for the
     * pilot.  For dim > 1024, a random projection to 1024 elements is used
     * to keep the pilot cost bounded.
     */
    [[nodiscard]] bool shouldDecompose(
        const std::vector<float>& embedding,
        double min_kappa = 1.3) const override;

    /// Returns true — `TensorTrainDecomposer` is always available.
    [[nodiscard]] bool isAvailable() const override { return true; }

    /// Human-readable description for logging.
    [[nodiscard]] std::string description() const override;

    // ── Diagnostics ───────────────────────────────────────────────────────

    /// Total decompositions attempted (includes κ-gated calls).
    [[nodiscard]] long long decomposeCount() const noexcept {
        return decompose_count_.load(std::memory_order_relaxed);
    }
    /// Total decompositions skipped by the κ-gate.
    [[nodiscard]] long long kappaSkipCount() const noexcept {
        return kappa_skip_count_.load(std::memory_order_relaxed);
    }

private:
    storage::TensorTrainDecomposer decomposer_;

    std::atomic<double>     default_epsilon_{0.01};
    std::atomic<std::size_t> default_max_rank_{0};
    std::atomic<double>     default_min_kappa_{1.3};

    mutable std::atomic<long long> decompose_count_{0};
    mutable std::atomic<long long> kappa_skip_count_{0};
    
    // Concurrent workload hardening (Block A1)
    mutable std::atomic<size_t> pending_decompositions_{0};  ///< Track in-flight decomposition operations

    /// Infer a balanced 2D mode-shape for a vector of length `n`.
    /// Returns {rows, cols} such that rows * cols == n and |rows - cols|
    /// is minimised.  Pads n to the next perfect product when necessary.
    static std::vector<std::size_t> inferModeShape(std::size_t n);
};

} // namespace tensor
} // namespace themis
