/**
 * @file tensor_rag_cost_model.h
 * @brief 5-phase cost model for Tensor-RAG query planning.
 *
 * Models the end-to-end latency of a Tensor-RAG pipeline as the sum of five
 * independently tunable phases:
 *
 *   C_RAG = C_embed + C_retrieve + C_rerank + C_assemble + C_generate
 *
 * @note TensorWorkloadClassifier is not present in the current codebase; the
 *       WorkloadType::TENSOR_RAG enum value should be added there when it is
 *       introduced (forward-declaration note kept here for integration).
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include <string>

namespace themis {
namespace rag {

// ─────────────────────────────────────────────────────────────────────────────
// TensorRagConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Per-query configuration passed to TensorRagCostModel::estimate().
 */
struct TensorRagConfig {
    /// Number of candidate chunks retrieved from the ANN index.
    std::size_t num_chunks{32};

    /// Embedding vector dimensionality (informational; reserved for future use).
    std::size_t embedding_dim{768};

    /// When true, the cross-encoder reranker phase is active.
    bool reranker_enabled{true};

    /// Fraction of retrieval results served from cache (0.0 = cold, 1.0 = full).
    float cache_hit_rate{0.0f};

    /// LLM time-to-first-token baseline (ms) — midpoint of 150–400 ms range.
    float llm_baseline_ttft_ms{275.0f};

    /// LLM time-to-first-token when KV-cache is warm — midpoint of 40–90 ms range.
    float cached_ttft_ms{65.0f};
};

// ─────────────────────────────────────────────────────────────────────────────
// CostEstimate
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Per-phase and aggregate latency estimate returned by TensorRagCostModel.
 */
struct CostEstimate {
    /// Sum of all five phase estimates (ms).
    float total_ms{0.0f};

    /// Phase 1 — text embedding latency (ms).
    float embed_ms{0.0f};

    /// Phase 2 — ANN / vector retrieval latency (ms).
    float retrieve_ms{0.0f};

    /// Phase 3 — cross-encoder reranking latency (ms).
    float rerank_ms{0.0f};

    /// Phase 4 — context assembly latency (ms).
    float assemble_ms{0.0f};

    /// Phase 5 — LLM generation / TTFT (ms).
    float generate_ms{0.0f};

    /**
     * @brief Model confidence in the estimate [0.0, 1.0].
     *
     * 0.8 when all TensorRagConfig fields use their defaults.
     * 0.5 when cache_hit_rate == 0.0 (cold path, higher variance).
     */
    float confidence{0.8f};
};

// ─────────────────────────────────────────────────────────────────────────────
// TensorRagCostModel
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Linear 5-phase cost model for Tensor-RAG query latency estimation.
 *
 * All phase coefficients are tunable at construction time.  The defaults
 * reflect empirical measurements on the ThemisDB reference hardware profile.
 *
 * @code{.cpp}
 * themis::rag::TensorRagCostModel model;
 * themis::rag::TensorRagConfig    cfg;
 * cfg.num_chunks       = 20;
 * cfg.reranker_enabled = true;
 * cfg.cache_hit_rate   = 0.6f;
 * auto est = model.estimate("SELECT * FROM knowledge WHERE topic = 'RAG'", cfg);
 * // est.total_ms ≈ embed + retrieve + rerank + assemble + generate
 * @endcode
 *
 * @note WorkloadType::TENSOR_RAG should be registered in TensorWorkloadClassifier
 *       (forward-declared) once that classifier is introduced in the codebase.
 */
class TensorRagCostModel {
public:
    /**
     * @brief Constructs a cost model with tuneable per-phase coefficients.
     *
     * @param embed_coeff    ms per query character for embedding   (default 0.02).
     * @param retrieve_coeff ms per chunk for ANN retrieval          (default 0.5).
     * @param rerank_coeff   ms per chunk for cross-encoder rerank   (default 1.2).
     */
    explicit TensorRagCostModel(float embed_coeff    = 0.02f,
                                float retrieve_coeff = 0.5f,
                                float rerank_coeff   = 1.2f) noexcept;

    /**
     * @brief Estimates the 5-phase cost of processing @p query with @p config.
     *
     * @param query  The raw query string (UTF-8).
     * @param config Per-query pipeline configuration.
     * @return CostEstimate with per-phase breakdown and aggregate total.
     */
    CostEstimate estimate(const std::string&    query,
                          const TensorRagConfig& config) const noexcept;

private:
    float embed_coeff_;
    float retrieve_coeff_;
    float rerank_coeff_;
};

} // namespace rag
} // namespace themis
