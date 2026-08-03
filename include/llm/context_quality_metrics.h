/**
 * @file context_quality_metrics.h
 * @brief ContextQualityBudget extension with state-quality metrics (Phase 1 PoC).
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL
 * @note Gap Summary: Context quality tracking for Agentic Memory transitions
 * @note Status: Phase 1 PoC
 * @note **Metrics/Configuration Header**: Defines metrics structures for context quality.
 *       No .cpp implementation needed. Used by consumers for state tracking.
 */

#pragma once

#include <cstdint>
#include <optional>

namespace themis::llm {

/**
 * @brief Context quality metrics for state-aware routing (P1-D06).
 *
 * Extends existing ContextWindowBudget with state-retention and drift signals:
 * - state_retention_score [0.0, 1.0]: HLC-based freshness of SSM state
 * - factual_drift_estimate [0.0, 1.0]: semantic coherence degradation
 * - tokens_since_last_retrieval: RAG refresh cadence metric
 *
 * Used by HybridContextRouter (P3-D01) for architecture selection.
 * Integration: ContextWindowBudget::compute() returns ContextQualityMetrics.
 */
struct ContextQualityMetrics {
    /// Total tokens in context window
    uint64_t total_tokens = 0;

    /// Tokens available in L1 (working memory)
    uint64_t l1_tokens = 0;

    /// Tokens compressed into L2 (episodic memory)
    uint64_t l2_tokens = 0;

    /// Estimated tokens in L3 (long-term memory via RAG)
    uint64_t l3_tokens = 0;

    /// State retention score from SSM plugin [0.0, 1.0]
    /// Reflects how much history is retained in SSM state
    /// 1.0 = fresh state with full history
    /// 0.0 = degraded state, loss of history
    double state_retention_score = 1.0;

    /// Factual drift estimate [0.0, 1.0]
    /// Measures semantic coherence degradation over context window
    /// 1.0 = coherent, no drift
    /// 0.0 = severe drift, likely hallucination risk
    double factual_drift_estimate = 1.0;

    /// Tokens processed since last RAG retrieval
    /// Used to decide when to refresh context via AgenticRAG
    uint64_t tokens_since_last_retrieval = 0;

    /// Maximum acceptable drift before forcing RAG refresh
    /// Config parameter: typically 0.3 (30% degradation threshold)
    double drift_threshold = 0.3;

    /// Check if quality metrics justify RAG refresh
    /// @return true if drift > threshold or retention < 0.5
    bool shouldRefreshRAG() const {
        return (1.0 - factual_drift_estimate) > drift_threshold ||
               state_retention_score < 0.5;
    }

    /// Check if context quality is sufficient for Transformer path
    /// @return true if drift < 0.1 and retention > 0.7
    bool isTransformerQuality() const {
        return (1.0 - factual_drift_estimate) < 0.1 &&
               state_retention_score > 0.7;
    }

    /// Check if quality is sufficient for Infini-attention path
    /// @return true if drift < 0.2 and retention > 0.5
    bool isInfiniQuality() const {
        return (1.0 - factual_drift_estimate) < 0.2 &&
               state_retention_score > 0.5;
    }

    /// Check if quality is sufficient for SSM hybrid path
    /// @return true (SSM compensates for drift via state compression)
    bool isSSMQuality() const {
        // SSM can operate with lower retention scores
        return state_retention_score > 0.3;
    }
};

}  // namespace themis::llm

