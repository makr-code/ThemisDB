/*
 * ThemisDB | File: process_light_retriever.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_light_retriever.h
 * Module:  include/process/
 * Purpose: Dual-mode LOCAL/GLOBAL retrieval following the LightRAG approach
 *          (Guo et al., 2024, arXiv:2410.05779). P5 implementation.
 */

#pragma once

#include "process/process_community_detector.h"
#include "process/process_graph_rag.h"
#include "storage/rocksdb_wrapper.h"
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// RetrievalMode
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Selects the retrieval strategy used by @c ProcessLightRetriever.
 *
 * - @c LOCAL  – entity-centric BFS/PPR traversal via @c ProcessGraphRag.
 * - @c GLOBAL – community-report-based lookup via @c ProcessCommunityDetector.
 * - @c AUTO   – heuristic keyword classification routes to LOCAL or GLOBAL.
 */
enum class RetrievalMode {
    LOCAL,   ///< Specific entity traversal
    GLOBAL,  ///< Community report summarisation
    AUTO,    ///< Classify query and route automatically
};

// ─────────────────────────────────────────────────────────────────────────────
// LightRetrievalResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result produced by @c ProcessLightRetriever::retrieve().
 */
struct LightRetrievalResult {
    RetrievalMode used_mode;                      ///< Effective mode used
    std::string llm_context;                      ///< Assembled context for LLM
    std::vector<std::string> community_ids_used;  ///< Community IDs (GLOBAL mode)
    std::string instance_id_used;                 ///< Instance ID (LOCAL mode)
};

// ─────────────────────────────────────────────────────────────────────────────
// ProcessLightRetriever
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Dual-mode retrieval façade implementing the LightRAG pattern.
 *
 * @par AUTO routing heuristic (< 5 ms, no LLM required)
 * Classifies as GLOBAL if the query contains any of the following terms
 * (case-insensitive):
 *   "gesamte", "überblick", "alle", "prozess", "ablauf", "workflow",
 *   "beschreibe", "erklär", "summary", "overview"
 * Otherwise classifies as LOCAL.
 *
 * @par GLOBAL fallback
 * If no communities are persisted for the model, GLOBAL falls back to LOCAL
 * and logs a WARN so there is no silent failure.
 *
 * @par Thread safety
 * All operations are read-only; thread safety is inherited from the underlying
 * @c ProcessGraphRag and @c ProcessCommunityDetector instances.
 *
 * @see ProcessCommunityDetector, ProcessGraphRag
 */
class ProcessLightRetriever {
public:
    /**
     * @param db                  RocksDB instance used to resolve instance→model_id.
     * @param graph_rag           Existing GraphRAG engine (LOCAL retrieval).
     * @param community_detector  Community detector (GLOBAL retrieval).
     */
    explicit ProcessLightRetriever(
        RocksDBWrapper&           db,
        ProcessGraphRag&          graph_rag,
        ProcessCommunityDetector& community_detector
    );

    /**
     * @brief Retrieve LLM context for @p query using the selected mode.
     *
     * @param query        Free-text query string.
     * @param instance_id  Process instance ID (used for LOCAL retrieval and
     *                     model_id resolution).
     * @param mode         Retrieval mode (default: AUTO).
     * @param config       GraphRAG config forwarded to LOCAL retrieval.
     * @return @c LightRetrievalResult with assembled @c llm_context.
     */
    [[nodiscard]] LightRetrievalResult retrieve(
        std::string_view        query,
        std::string_view        instance_id,
        RetrievalMode           mode   = RetrievalMode::AUTO,
        const ProcessRagConfig& config = {}
    ) const;

private:
    /**
     * @brief Classify a query as LOCAL or GLOBAL using keyword heuristics.
     *
     * Comparison is case-insensitive.  Returns @c RetrievalMode::GLOBAL when
     * any global keyword is found; @c RetrievalMode::LOCAL otherwise.
     */
    [[nodiscard]] RetrievalMode classifyQuery(std::string_view query) const;

    ProcessGraphRag&          graph_rag_;
    ProcessCommunityDetector& community_detector_;
    RocksDBWrapper&           db_;
};

} // namespace process
} // namespace themis
