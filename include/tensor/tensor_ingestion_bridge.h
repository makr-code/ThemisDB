/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_ingestion_bridge.h                          ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file tensor_ingestion_bridge.h
 * @brief Concrete `ITensorDecompositionBackend` that delegates to
 *        `TensorTrainDecomposer`.
 *
 * This is the **only** class that bridges the `ingestion` module and the
 * `storage` tensor infrastructure.  It lives in `tensor/` so the `ingestion/`
 * module never needs to include any `storage/` or `tensor/` headers.
 *
 * ## Dependency flow (SoC / DIP compliant)
 * @code
 *   ingestion/  →  ITensorDecompositionBackend   (abstract, ingestion/)
 *                         ↑ implemented by
 *   tensor/     →  TensorIngestionBridge          (concrete, tensor/)
 *                         ↓ calls
 *               →  storage::TensorTrainDecomposer::decompose()
 * @endcode
 *
 * ## Wiring (main.cpp / server bootstrap)
 * @code
 * #include "tensor/tensor_ingestion_bridge.h"
 * #include "ingestion/builtin_step_factories.h"
 *
 * auto bridge = std::make_shared<themis::tensor::TensorIngestionBridge>();
 * bridge->setEpsilon(0.01);          // optional: override global ε
 * bridge->setMaxRank(64);            // optional: cap bond dimension
 * bridge->setMinKappa(1.3);          // optional: override κ-gate
 *
 * auto step = ingestion::builtin::createChunkTtDecomposeStep(bridge);
 * // register step in WorkflowEngine profile AFTER builtin.chunk_embed
 * @endcode
 *
 * ## κ-gate implementation
 *
 * `shouldDecompose()` estimates the compressibility κ as:
 *   κ = dense_elements / pilot_tt_params
 * where `pilot_tt_params` is the total parameters of a TT-decomposition with
 * a very tight pilot_eps (= eps / 10, capped at 0.001) on the same embedding.
 * If the pilot decomposition itself would exceed the input size (κ < min_kappa),
 * we skip the full decomposition.
 *
 * For large embeddings (dim > 8192) the pilot is run on a randomly sampled
 * reshape to keep the pilot cost O(√dim) instead of O(dim).
 *
 * ## Thread safety
 *
 * `TensorTrainDecomposer` is stateless and thread-safe after construction.
 * `TensorIngestionBridge` adds no mutable state beyond configuration fields
 * set before any concurrent use.  All public methods are safe to call from
 * multiple ingestion threads simultaneously.
 *
 * ## Scientific basis
 *
 * - Oseledets (2011) TT-SVD: SIAM J. Sci. Comput. 33(5), 2295-2317
 * - Edge et al. (2024) GraphRAG — offline pre-computation pattern
 * - Jiang et al. (2023) FLARE — pre-computed cores needed for ≤90 ms retrieval
 * - research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md §κ-boundary
 * - research/ADALORA_TT_BRIDGE_RESEARCH.md §3.1 (zero-copy rationale)
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
    TensorIngestionBridge(TensorIngestionBridge&&)            noexcept = default;
    TensorIngestionBridge& operator=(TensorIngestionBridge&&) noexcept = default;

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

    double      default_epsilon_    = 0.01;
    std::size_t default_max_rank_   = 0;
    double      default_min_kappa_  = 1.3;

    mutable std::atomic<long long> decompose_count_{0};
    mutable std::atomic<long long> kappa_skip_count_{0};

    /// Infer a balanced 2D mode-shape for a vector of length `n`.
    /// Returns {rows, cols} such that rows * cols == n and |rows - cols|
    /// is minimised.  Pads n to the next perfect product when necessary.
    static std::vector<std::size_t> inferModeShape(std::size_t n);
};

} // namespace tensor
} // namespace themis
