/**
 * @file process_light_retriever.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

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
#include <optional>
#include <cstdint>
#include <cstddef>

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
 *
 * @section resource_tracking Resource Tracking
 * The result includes metrics about resource consumption during retrieval:
 * - @c retrieval_time_ms: Time spent in retrieval operations.
 * - @c context_size_bytes: Actual size of the assembled context.
 * - @c resource_exhaustion_reason: If set, indicates graceful degradation occurred.
 *   Examples: "max_context_size_exceeded", "retrieval_timeout", "no_results_found".
 */
struct LightRetrievalResult {
    RetrievalMode used_mode;                      ///< Effective mode used
    std::string llm_context;                      ///< Assembled context for LLM
    std::vector<std::string> community_ids_used;  ///< Community IDs (GLOBAL mode)
    std::string instance_id_used;                 ///< Instance ID (LOCAL mode)

    // Phase 2: Resource tracking and stress scenario support
    int64_t retrieval_time_ms{0};                 ///< Milliseconds spent in retrieve()
    size_t context_size_bytes{0};                 ///< Actual byte size of llm_context
    bool degraded{false};                         ///< true if graceful degradation occurred
    std::optional<std::string> resource_exhaustion_reason;  ///< Why degradation occurred
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
 * @section phase2_stress_hardening Phase 2: Stress Scenario Hardening
 * ProcessLightRetriever now gracefully handles resource constraints:
 *
 * - **Bounded Retrieval Depth**: Maximum traversal depth is enforced; if exceeded,
 *   retrieval returns partial context with @c degraded=true.
 * - **Context Size Limits**: If accumulated context exceeds @c kMaxContextBytes,
 *   retrieval gracefully truncates and signals @c resource_exhaustion_reason.
 * - **Timeout Enforcement**: If retrieval exceeds @c kMaxRetrievalTimeMs,
 *   returns best-effort partial result.
 * - **Element Count Limits**: Parser respects maximum element count;
 *   excess elements trigger graceful degradation.
 *
 * @see ProcessCommunityDetector, ProcessGraphRag
 */
class ProcessLightRetriever {
public:
    /**
     * @brief Resource limits for stress scenario handling.
     *
     * These are configurable per-retriever instance via setResourceLimits().
     */
    struct ResourceLimits {
        /// Maximum context size in bytes before truncation (default: 1 MiB)
        size_t max_context_bytes{1024 * 1024};
        /// Maximum retrieval timeout in milliseconds (default: 5000 ms)
        int64_t max_retrieval_time_ms{5000};
        /// Maximum graph traversal depth (default: 50)
        size_t max_traversal_depth{50};
        /// Maximum number of results before graceful degradation (default: 1000)
        size_t max_result_elements{1000};
    };
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
     * @brief Set custom resource limits for stress scenario handling.
     * @param limits The resource limits to enforce.
     */
    void setResourceLimits(const ResourceLimits& limits);

    /**
     * @brief Get the current resource limits.
     * @return The active ResourceLimits.
     */
    [[nodiscard]] const ResourceLimits& getResourceLimits() const {
        return resource_limits_;
    }

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

    // Phase 2: Stress scenario helpers
    /**
     * @brief Check if current time is within the configured timeout window.
     * @param start_time_ms The time retrieval started.
     * @return true if within timeout; false if exceeded.
     */
    [[nodiscard]] bool isWithinTimeoutBudget(int64_t start_time_ms) const;

    /**
     * @brief Check if accumulated context size is within limits.
     * @param current_size_bytes Current accumulated size.
     * @return true if within limits; false if exceeded.
     */
    [[nodiscard]] bool isWithinSizeBudget(size_t current_size_bytes) const;

    /**
     * @brief Check if traversal depth is within limits.
     * @param current_depth The current traversal depth.
     * @return true if within limits; false if exceeded.
     */
    [[nodiscard]] bool isWithinDepthBudget(size_t current_depth) const;

    /**
     * @brief Gracefully degrade retrieval result when resources are exhausted.
     * @param reason Description of why degradation occurred.
     * @return LightRetrievalResult marked as degraded with partial context.
     */
    [[nodiscard]] LightRetrievalResult createDegradedResult(std::string_view reason) const;

    ProcessGraphRag&          graph_rag_;
    ProcessCommunityDetector& community_detector_;
    RocksDBWrapper&           db_;

    // Phase 2: Resource limit configuration
    ResourceLimits resource_limits_;
};

} // namespace process
} // namespace themis
