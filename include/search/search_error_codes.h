/**
 * @file search_error_codes.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=0, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: search_error_codes.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=0, H=0, M=0, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace themis {

/**
 * @brief Explicit error taxonomy for search module failure modes.
 *
 * This enumeration defines all failure classes that can occur during search
 * retrieval, fusion, distributed merge, and utility operations. Each error
 * code maps to a deterministic failure mode and suggests recovery/fallback
 * behavior.
 *
 * Error codes are organized by category:
 * - 0x0000-0x0FFF: Retrieval errors (lexical/vector candidate deficits)
 * - 0x1000-0x1FFF: Fusion errors (score normalization, RRF failures)
 * - 0x2000-0x2FFF: Distributed merge errors (shard failures, merge conflicts)
 * - 0x3000-0x3FFF: Utility errors (expansion, reranking, faceting limits)
 * - 0x4000-0x4FFF: Analytics and observability errors
 *
 * @since v2.0.0 (Phase 1: Contract Freeze)
 */
enum class SearchErrorCode : uint32_t {
    // ========================================================================
    // 0x0000-0x0FFF: Retrieval Errors
    // ========================================================================
    
    /// No error; operation succeeded.
    SUCCESS = 0x0000,
    
    /// BM25 backend returned no candidates.
    BM25_NO_RESULTS = 0x0001,
    
    /// Vector backend returned no candidates.
    VECTOR_NO_RESULTS = 0x0002,
    
    /// Both BM25 and vector backends returned no candidates.
    BOTH_BACKENDS_EMPTY = 0x0003,
    
    /// BM25 backend timed out or became unavailable.
    BM25_BACKEND_UNAVAILABLE = 0x0004,
    
    /// Vector backend timed out or became unavailable.
    VECTOR_BACKEND_UNAVAILABLE = 0x0005,
    
    /// BM25 backend returned fewer candidates than requested (partial).
    BM25_PARTIAL_RESULTS = 0x0006,
    
    /// Vector backend returned fewer candidates than requested (partial).
    VECTOR_PARTIAL_RESULTS = 0x0007,
    
    // ========================================================================
    // 0x1000-0x1FFF: Fusion Errors
    // ========================================================================
    
    /// RRF fusion failed due to rank list corruption or underflow.
    RRF_FUSION_FAILED = 0x1001,
    
    /// Score normalization encountered invalid/infinite values.
    SCORE_NORMALIZATION_FAILED = 0x1002,
    
    /// Linear combination fusion failed due to weight validation.
    LINEAR_FUSION_FAILED = 0x1003,
    
    /// Fusion produced no results even with partial candidates.
    FUSION_PRODUCED_EMPTY_SET = 0x1004,
    
    /// Score normalization fallback to identity (no normalization applied).
    SCORE_NORMALIZATION_FALLBACK = 0x1005,
    
    // ========================================================================
    // 0x2000-0x2FFF: Distributed Merge Errors
    // ========================================================================
    
    /// Shard query timed out and was skipped.
    SHARD_TIMEOUT = 0x2001,
    
    /// Shard returned HTTP error or malformed response.
    SHARD_HTTP_ERROR = 0x2002,
    
    /// Shard was unreachable (network error).
    SHARD_UNREACHABLE = 0x2003,
    
    /// All shards failed; no results available.
    ALL_SHARDS_FAILED = 0x2004,
    
    /// Some (but not all) shards failed; result is partial.
    PARTIAL_SHARD_FAILURE = 0x2005,
    
    /// Distributed merge resulted in fewer results than k due to shard failures.
    MERGE_CANDIDATE_DEFICIT = 0x2006,
    
    /// High-cardinality overlap detected; merge behavior may diverge from expected.
    HIGH_CARDINALITY_OVERLAP = 0x2007,
    
    /// Shard result ordering conflict detected during merge.
    SHARD_RANKING_CONFLICT = 0x2008,
    
    // ========================================================================
    // 0x3000-0x3FFF: Utility Errors
    // ========================================================================
    
    /// Query expansion exceeded maximum expansion count.
    EXPANSION_LIMIT_EXCEEDED = 0x3001,
    
    /// Fuzzy matching failed; reverting to exact match.
    FUZZY_MATCHING_FALLBACK = 0x3002,
    
    /// Faceting aggregation exceeded cardinality limit.
    FACET_CARDINALITY_LIMIT = 0x3003,
    
    /// LLM reranker backend unavailable; using base ranking.
    RERANKER_BACKEND_UNAVAILABLE = 0x3004,
    
    /// LLM reranker timed out; using base ranking.
    RERANKER_TIMEOUT = 0x3005,
    
    /// LLM reranker returned invalid score format.
    RERANKER_INVALID_SCORES = 0x3006,
    
    /// Query rewriter failed; using original query.
    QUERY_REWRITER_FAILED = 0x3007,
    
    /// Result highlighting failed for subset of results.
    HIGHLIGHTING_PARTIAL_FAILURE = 0x3008,
    
    /// Streaming cursor exceeded bounds.
    STREAM_CURSOR_OUT_OF_BOUNDS = 0x3009,
    
    // ========================================================================
    // 0x4000-0x4FFF: Analytics and Observability Errors
    // ========================================================================
    
    /// Analytics event buffer full; discarding old events.
    ANALYTICS_BUFFER_FULL = 0x4001,
    
    /// Analytics recording failed; event not captured.
    ANALYTICS_RECORD_FAILED = 0x4002,
};

/**
 * @brief Convert SearchErrorCode to human-readable string.
 * @param code Error code to convert.
 * @return Descriptive string for the error code.
 */
inline std::string searchErrorCodeToString(SearchErrorCode code) {
    static const std::unordered_map<uint32_t, std::string> kErrorMap{
        { 0x0000, "SUCCESS" },
        { 0x0001, "BM25_NO_RESULTS" },
        { 0x0002, "VECTOR_NO_RESULTS" },
        { 0x0003, "BOTH_BACKENDS_EMPTY" },
        { 0x0004, "BM25_BACKEND_UNAVAILABLE" },
        { 0x0005, "VECTOR_BACKEND_UNAVAILABLE" },
        { 0x0006, "BM25_PARTIAL_RESULTS" },
        { 0x0007, "VECTOR_PARTIAL_RESULTS" },
        { 0x1001, "RRF_FUSION_FAILED" },
        { 0x1002, "SCORE_NORMALIZATION_FAILED" },
        { 0x1003, "LINEAR_FUSION_FAILED" },
        { 0x1004, "FUSION_PRODUCED_EMPTY_SET" },
        { 0x1005, "SCORE_NORMALIZATION_FALLBACK" },
        { 0x2001, "SHARD_TIMEOUT" },
        { 0x2002, "SHARD_HTTP_ERROR" },
        { 0x2003, "SHARD_UNREACHABLE" },
        { 0x2004, "ALL_SHARDS_FAILED" },
        { 0x2005, "PARTIAL_SHARD_FAILURE" },
        { 0x2006, "MERGE_CANDIDATE_DEFICIT" },
        { 0x2007, "HIGH_CARDINALITY_OVERLAP" },
        { 0x2008, "SHARD_RANKING_CONFLICT" },
        { 0x3001, "EXPANSION_LIMIT_EXCEEDED" },
        { 0x3002, "FUZZY_MATCHING_FALLBACK" },
        { 0x3003, "FACET_CARDINALITY_LIMIT" },
        { 0x3004, "RERANKER_BACKEND_UNAVAILABLE" },
        { 0x3005, "RERANKER_TIMEOUT" },
        { 0x3006, "RERANKER_INVALID_SCORES" },
        { 0x3007, "QUERY_REWRITER_FAILED" },
        { 0x3008, "HIGHLIGHTING_PARTIAL_FAILURE" },
        { 0x3009, "STREAM_CURSOR_OUT_OF_BOUNDS" },
        { 0x4001, "ANALYTICS_BUFFER_FULL" },
        { 0x4002, "ANALYTICS_RECORD_FAILED" },
    };
    
    auto it = kErrorMap.find(static_cast<uint32_t>(code));
    if (it != kErrorMap.end()) {
        return it->second;
    }
    return "UNKNOWN_ERROR_0x" + std::to_string(static_cast<uint32_t>(code));
}

}  // namespace themis
