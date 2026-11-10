/**
 * @file process_light_retriever.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * - @c retrieval_time_ms: Elapsed wall-clock time, measured with a monotonic clock.
 * - @c context_size_bytes: Actual byte size of the returned, possibly truncated context.
 * - @c resource_exhaustion_reason: If set, indicates graceful degradation occurred.
 *   Examples: "max_context_size_exceeded", "retrieval_timeout", and
 *   "max_result_elements_exceeded".
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
 * Concurrent retrievals do not mutate this façade. Configure resource limits
 * before sharing it; concurrent calls to @c setResourceLimits and @c retrieve
 * are not synchronized. Thread safety of underlying services is inherited.
 *
 * @section resource_limits Resource limits
 * Retrieval applies output byte and element caps, and clamps LOCAL traversal
 * depth to the configured maximum. A timeout is detected after synchronous
 * dependencies return; those dependencies cannot be preempted by this façade.
 * GLOBAL counts community reports as result elements; LOCAL caps returned
 * prompt lines. A zero limit permits no corresponding output/depth.
 *
 * Exceeding an output or depth limit sets @c degraded and records the relevant
 * reason. Timeout reporting is best-effort and does not guarantee a maximum
 * return latency.
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
        /// Maximum returned context size in bytes; truncation preserves UTF-8 boundaries.
        size_t max_context_bytes{1024 * 1024};
        /// Soft elapsed-time budget in milliseconds; synchronous calls are not preempted.
        int64_t max_retrieval_time_ms{5000};
        /// Maximum LOCAL graph traversal depth (default: 50).
        size_t max_traversal_depth{50};
        /// GLOBAL report count / LOCAL output line count (default: 1000).
        size_t max_result_elements{1000};
    };
    /**
     * @brief Construct a retriever over existing database and retrieval services.
     * @param db                  RocksDB instance used to resolve instance→model_id.
     * @param graph_rag           Existing GraphRAG engine (LOCAL retrieval).
     * @param community_detector  Community detector (GLOBAL retrieval).
     * @note The referenced services must outlive this retriever.
     */
    explicit ProcessLightRetriever(
        RocksDBWrapper&           db,
        ProcessGraphRag&          graph_rag,
        ProcessCommunityDetector& community_detector
    );

    /**
     * @brief Set per-instance output and LOCAL traversal limits.
     * @param limits Limits to apply; zero values produce empty output/depth, and
     *        a negative timeout is treated as an already-exceeded soft budget.
     * @note Changes are not synchronized with concurrent retrieve() calls.
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
     * @return Result with measured elapsed time and returned context byte size;
     *         output/depth truncation or a detected timeout marks it degraded.
     * @note Timeout detection occurs after synchronous database/GraphRAG calls
     *       complete; it does not cancel those calls or guarantee a hard deadline.
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

    // Phase 2: Resource limit configuration
    ResourceLimits resource_limits_;
};

} // namespace process
} // namespace themis
