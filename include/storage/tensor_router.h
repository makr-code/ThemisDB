/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_router.h                                    ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 📋 Specification (Phase 3, Q1 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_router.h
 * @brief Tensor-Router: decides which multi-model data is lifted to TT-format.
 *
 * ## Purpose
 *
 * ThemisDB stores data in multiple formats (relational, document, graph,
 * vector, timeseries, spatial).  Not all data benefits equally from
 * Tensor-Train compression.  The TensorRouter analyses access patterns,
 * data characteristics, and compression effectiveness to decide:
 *
 *   1. **Lift** — Compress to TT-format (TensorNetworkStorageEngine).
 *   2. **Keep** — Store in the native format (RocksDB, BlobDB, HNSW, etc.).
 *   3. **Hybrid** — Store natively but maintain a TT-shadow for fast similarity
 *                   search (HNSW-on-TT-Cores pattern).
 *
 * ## The HNSW + TT Hybrid Pattern
 *
 * The router enables the "HNSW on TT-Cores" architecture:
 * - Tensor Train is the *storage layer* (compressed full vectors).
 * - HNSW is the *navigation layer* (index over first-core sketches only).
 * - Distance computation during HNSW search uses TT-domain cosine similarity
 *   (O(d·r²) instead of O(n^d)), keeping search fast even for 10K-dim vectors.
 *
 * ## Data-Type Routing Heuristics
 *
 * | Data Type        | TT Compressibility | Recommended Route |
 * |------------------|--------------------|-------------------|
 * | LLM attention weights (NxN, low-rank structure) | Very High | LIFT |
 * | Maxwell / PDE simulation fields (smooth)        | Very High | LIFT |
 * | Dense embeddings (768–4096 dim, normalised)     | High      | HYBRID |
 * | Geodata rasters (smooth spatial fields)         | High      | LIFT |
 * | Sparse text embeddings                          | Medium    | KEEP  |
 * | Random noise / truly unstructured data          | None      | KEEP  |
 * | Relational tables (structured, low-dim)         | Low       | KEEP  |
 * | Images (JPEG-compressed → lossy, pre-compressed)| Medium    | HYBRID |
 *
 * ## STUB/SIMULATION NOTE:
 * Purpose: This class is a specification for the Phase 3 routing layer.
 *          The heuristics return hard-coded rules in Phase 1;
 *          ML-based routing is planned for Phase 4 (Q2 2027).
 * Activation: Always compiled in; decisions default to KEEP when the
 *             TensorNetworkStorageEngine is not wired in.
 * Production Delta: ML routing model (lightweight XGBoost) replaces heuristics.
 * Removal Plan: Heuristic path removed after ML model validation (Q2 2027).
 *
 * ## References
 *
 * - Oseledets (2011) — TT-SVD: structural compressibility predicts low TT-rank.
 * - Dettmers (2023) — NF4: normal-distributed weights ideal for TT+NF4.
 * - Stoudenmire & Schwab (2016) — TT for images and structured ML data.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"
#include "tensor/hiss_structural_search.h"

#include <cstddef>
#include <functional>
#include <memory>
#include &lt;optional&gt;
#include <string>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// TensorRouteDecision — outcome of a routing decision
// ============================================================================

enum class TensorRouteDecision : uint8_t {
    /// Compress fully to TT-format; serve all reads from TensorNetworkStorageEngine.
    LIFT    = 0,

    /// Keep in native format; no TT-compression.
    KEEP    = 1,

    /// Store natively but maintain TT-shadow for similarity search.
    /// HNSW navigates first-core sketches; full reconstruction from native store.
    HYBRID  = 2,
};

std::string to_string(TensorRouteDecision d) noexcept;

// ============================================================================
// TensorRouteHint — caller-supplied hints for routing
// ============================================================================

struct TensorRouteHint {
    /// Expected data distribution (set by the caller when known).
    enum class Distribution {
        UNKNOWN      = 0,
        NORMAL       = 1,   ///< N(0, σ²) — best for NF4
        UNIFORM      = 2,   ///< Uniform — INT8 preferred
        SPARSE       = 3,   ///< Many near-zero values
        SMOOTH_FIELD = 4,   ///< PDE/Maxwell fields — very high TT-compressibility
    };

    /// Data type category
    enum class DataCategory {
        UNKNOWN      = 0,
        LLM_WEIGHTS  = 1,
        LLM_ADAPTER  = 2,   ///< LoRA / PEFT adapter
        EMBEDDING    = 3,
        IMAGE        = 4,
        GEODATA      = 5,
        SIMULATION   = 6,   ///< PDE / Maxwell field
        DOCUMENT     = 7,
        RELATIONAL   = 8,
        TIMESERIES   = 9,
    };

    Distribution  distribution  = Distribution::UNKNOWN;
    DataCategory  category      = DataCategory::UNKNOWN;

    /// Expected TT-rank (0 = unknown).
    std::size_t   expected_rank = 0;

    /// True if the data will be used in FLARE / RAG with an inference engine.
    bool          inference_use = false;

    /// True if the data is updated frequently (high-churn).
    bool          high_churn    = false;

    /// Minimum acceptable compression ratio (0.0 = no minimum).
    double        min_ratio     = 0.0;

    /**
     * @brief Optional domain tag used for TemplateCatalog lookup.
     *
     * When non-empty and a `TemplateCatalog` has been wired via
     * `TensorRouter::setTemplateCatalog()`, the router checks whether a
     * structural template is available for this domain.  A cache-hit
     * promotes the routing decision toward LIFT (TENSOR_TRAIN), because
     * an optimised TN topology is known — reducing structure-search cost
     * at query time.
     *
     * STUB/SIMULATION NOTE (STUB #253):
     * Purpose: Phase-6 template-aware routing.
     * Activation: When domain_tag is non-empty AND setTemplateCatalog() used.
     * Production Delta: Template presence promotes to LIFT but doesn't yet
     *   carry the concrete TN topology into the stored index.
     * Removal Plan: Q3 2028 — wire template graph into index construction
     *   so the stored TT-cores follow the template topology.
     */
    std::string   domain_tag;
};

// ============================================================================
// TensorRoutingPolicy — configures the routing thresholds
// ============================================================================

struct TensorRoutingPolicy {
    /// Minimum compression ratio to choose LIFT over KEEP.
    double min_lift_compression_ratio = 2.0;

    /// Minimum compression ratio to choose HYBRID over KEEP.
    double min_hybrid_compression_ratio = 1.2;

    /// Maximum TT-rank for a field to qualify for LIFT.
    /// Higher-rank data is unlikely to benefit from TT-storage.
    std::size_t max_lift_rank = 64;

    /// Probe sample size (elements) used to estimate compressibility.
    std::size_t probe_sample_elements = 4096;

    /// If inference_use is true and category == LLM_WEIGHTS|EMBEDDING,
    /// override decision to LIFT to enable zero-copy RAG.
    bool force_lift_for_inference = true;

    /// If true, use ML-based routing model instead of heuristics.
    /// Requires THEMIS_ENABLE_ROUTING_ML=ON (Q2 2027).
    bool use_ml_routing = false;
};

// ============================================================================
// TensorRouter
// ============================================================================

/**
 * @brief Decides routing of multi-model data to TT-storage.
 *
 * ### Usage Example
 *
 * ```cpp
 * TensorRouter router(engine, policy);
 *
 * // Before storing an LLM embedding:
 * TensorRouteHint hint;
 * hint.category      = TensorRouteHint::DataCategory::EMBEDDING;
 * hint.distribution  = TensorRouteHint::Distribution::NORMAL;
 * hint.inference_use = true;
 *
 * auto decision = router.route(data, mode_sizes, hint);
 *
 * switch (decision) {
 *     case TensorRouteDecision::LIFT:
 *         engine->put(key, data, mode_sizes);
 *         break;
 *     case TensorRouteDecision::HYBRID:
 *         native_store->put(key, data);
 *         engine->put(tt_shadow_key, data, mode_sizes);
 *         break;
 *     case TensorRouteDecision::KEEP:
 *         native_store->put(key, data);
 *         break;
 * }
 * ```
 */
class TensorRouter {
public:
    // -----------------------------------------------------------------------
    // Index-time routing (no data available — heuristic only)
    // -----------------------------------------------------------------------

    /**
     * @brief Routing result for index-creation time (no tensor data available).
     *
     * Used by `TensorIndexManager::routeFor()` which decides the index type
     * for a (tenant, collection, field) triple before any vectors are stored.
     * Values map to `TensorRouteDecision` as follows:
     *   TENSOR_TRAIN ↔ LIFT   — full TT compression
     *   HYBRID       ↔ HYBRID — TT shadow + native storage
     *   HNSW         ↔ KEEP   — standard vector index only
     */
    enum class Route : uint8_t {
        TENSOR_TRAIN = 0,  ///< Fully compress to TT (index on TT-cores)
        HYBRID       = 1,  ///< TT shadow for ANN + native storage
        HNSW         = 2,  ///< Standard vector index (no TT layer)
    };

    /**
     * @brief Lightweight profiling descriptor used at index-creation time.
     *
     * Provides enough information to apply heuristic routing thresholds
     * without running a pilot TT-SVD (which requires actual data).
     */
    struct DataProfile {
        std::size_t dim              = 0;    ///< Embedding / tensor dimension
        std::size_t num_vectors      = 0;    ///< Expected number of vectors
        /// Pre-computed κ compressibility estimate (from dimension heuristic
        /// or external measurement).  κ ≥ 1.7 → TENSOR_TRAIN,
        /// κ ≥ 1.3 → HYBRID, otherwise → HNSW.
        double      kappa_estimate   = 0.0;
    };

    /**
     * @brief Heuristic routing from a `DataProfile` (no pilot, no engine).
     *
     * Applies dimension and κ thresholds without running a TT-SVD pilot.
     * Used by `TensorIndexManager` at index-creation time when no concrete
     * data sample is available yet.
     *
     * Thresholds (from research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md §3.2):
     *   κ ≥ 1.7 AND dim ≥ 256 → TENSOR_TRAIN
     *   κ ≥ 1.3              → HYBRID
     *   otherwise             → HNSW
     */
    static Route decide(const DataProfile& p) noexcept;

    // -----------------------------------------------------------------------
    // Engine-backed routing (pilot TT-SVD on actual data)
    // -----------------------------------------------------------------------

    explicit TensorRouter(
        std::shared_ptr<TensorNetworkStorageEngine> engine,
        TensorRoutingPolicy                         policy = {});

    ~TensorRouter();

    /**
     * @brief Decide routing for a tensor before storage.
     *
     * Probes compressibility via a pilot TT-SVD on a data sample,
     * applies policy thresholds, and returns the routing decision.
     *
     * @param data       Flat tensor data (float32).
     * @param mode_sizes Shape.
     * @param hint       Optional caller-supplied hints.
     * @return Routing decision.
     *
     * STUB/SIMULATION NOTE:
     * Purpose: Heuristic routing using TT-SVD pilot on sample.
     * Activation: Always active. ML routing gated by policy.use_ml_routing.
     * Production Delta: ML model uses a gradient-boosted decision tree trained
     *   on historical compression ratios, access patterns, and data categories.
     * Removal Plan: Replace heuristic branch with ML inference Q2 2027.
     */
    TensorRouteDecision route(
        const std::vector<float>&       data,
        const std::vector<std::size_t>& mode_sizes,
        const TensorRouteHint&          hint = {}) const;

    /**
     * @brief Explain the routing decision in human-readable form.
     *
     * Returns a JSON object with the decision, reason, estimated compression
     * ratio, pilot TT-rank, and applied policy thresholds.
     */
    std::string explain(
        const std::vector<float>&       data,
        const std::vector<std::size_t>& mode_sizes,
        const TensorRouteHint&          hint = {}) const;

    struct RouterStats {
        std::size_t total_decisions    = 0;
        std::size_t lift_decisions     = 0;
        std::size_t hybrid_decisions   = 0;
        std::size_t keep_decisions     = 0;
        double      avg_pilot_ratio    = 0.0;
        double      avg_decision_us    = 0.0;
    };

    RouterStats stats() const noexcept;

    const TensorRoutingPolicy& policy() const noexcept;
    void setPolicy(TensorRoutingPolicy p);

    /**
     * @brief Callback used to apply a matched TemplateCatalog topology.
     *
     * @param domain_tag  Domain tag that produced the template hit.
     * @param graph       Matched template graph.
     * @param hint        Original route hint.
     * @return `true` if topology embedding succeeded and LIFT promotion is valid.
     *         `false` if embedding failed (router falls back to heuristic path).
     */
    using TemplateTopologyApplyFn =
        std::function<bool(const std::string&,
                           const tensor::TensorNetworkGraph&,
                           const TensorRouteHint&)>;

    /**
     * @brief Wire a TemplateCatalog for domain-tag-based routing promotion.
     *
     * When a non-null catalog is set, `route()` checks the catalog for a
     * matching template whenever `TensorRouteHint::domain_tag` is non-empty.
     * A catalog hit promotes the routing decision toward LIFT.
     *
     * Passing nullptr disables template-catalog lookups (default).
     *
     * STUB/SIMULATION NOTE (STUB #253):
     * Purpose: Enable domain-aware routing via TemplateCatalog.
     * Activation: When setTemplateCatalog(non-null) AND hint.domain_tag set.
     * Production Delta: Template presence promotes to LIFT only when
     *   TemplateTopologyApplyFn is installed and returns true; without a
     *   callback, the previous promotion fallback remains.
     * Removal Plan: Q3 2028 — replace callback bridge with integrated
     *   topology-aware index construction.
     */
    void setTemplateCatalog(std::shared_ptr<tensor::TemplateCatalog> catalog);

    /**
     * @brief Return the currently wired TemplateCatalog (may be nullptr).
     */
    std::shared_ptr<tensor::TemplateCatalog> templateCatalog() const noexcept;

    /**
     * @brief Install/remove/read the template-topology embedding callback.
     *
     * Thread-safe. The callback is invoked on TemplateCatalog hits.
     */
    void setTemplateTopologyApplyFn(TemplateTopologyApplyFn fn);
    void clearTemplateTopologyApplyFn();
    [[nodiscard]] TemplateTopologyApplyFn getTemplateTopologyApplyFn() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace storage
} // namespace themis
