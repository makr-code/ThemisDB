/**
 * @file tensor_rag_cost_model.cpp
 * @brief Implementation of the 5-phase Tensor-RAG cost model.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "rag/tensor_rag_cost_model.h"

#include <cstddef>

namespace themis {
namespace rag {

TensorRagCostModel::TensorRagCostModel(float embed_coeff,
                                       float retrieve_coeff,
                                       float rerank_coeff) noexcept
    : embed_coeff_(embed_coeff)
    , retrieve_coeff_(retrieve_coeff)
    , rerank_coeff_(rerank_coeff)
{}

CostEstimate TensorRagCostModel::estimate(const std::string&    query,
                                          const TensorRagConfig& config) const noexcept
{
    CostEstimate est;

    // ── Phase 1: Embedding ───────────────────────────────────────────────────
    est.embed_ms = static_cast<float>(query.size()) * embed_coeff_;

    // ── Phase 2: ANN Retrieval ───────────────────────────────────────────────
    // Cache hit rate reduces the number of chunks that must be fetched live.
    est.retrieve_ms = static_cast<float>(config.num_chunks)
                    * retrieve_coeff_
                    * (1.0f - config.cache_hit_rate);

    // ── Phase 3: Cross-Encoder Reranking ────────────────────────────────────
    est.rerank_ms = config.reranker_enabled
                  ? static_cast<float>(config.num_chunks) * rerank_coeff_
                  : 0.0f;

    // ── Phase 4: Context Assembly ────────────────────────────────────────────
    est.assemble_ms = 5.0f + static_cast<float>(config.num_chunks) * 0.1f;

    // ── Phase 5: LLM Generation (TTFT) ──────────────────────────────────────
    // Use the cached TTFT when more than half the context is cache-warm.
    est.generate_ms = (config.cache_hit_rate > 0.5f)
                    ? config.cached_ttft_ms
                    : config.llm_baseline_ttft_ms;

    // ── Aggregate ────────────────────────────────────────────────────────────
    est.total_ms = est.embed_ms + est.retrieve_ms + est.rerank_ms
                 + est.assemble_ms + est.generate_ms;

    // ── Confidence ───────────────────────────────────────────────────────────
    // Confidence is 0.8 for the default (non-zero cache) path; cold-path
    // (cache_hit_rate == 0.0) has higher variance, so we lower it to 0.5.
    est.confidence = (config.cache_hit_rate == 0.0f) ? 0.5f : 0.8f;

    return est;
}

} // namespace rag
} // namespace themis
