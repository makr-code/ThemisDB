/**
 * @file ingestion_toolbox.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/workflow_engine.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/inference_backend.h"
#include "ingestion/base_entity.h"
#include "ingestion/extraction_context.h"
#include "ingestion/file_manifest.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// IngestionToolbox — system-wide access point for ingestion infrastructure
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief System-wide injectable service that exposes the ingestion
 *        infrastructure (WorkflowEngine, StepRegistry,
 *        ITextGenerationBackend) to other ThemisDB modules.
 *
 * ## Motivation
 *
 * The ingestion pipeline contains generally useful infrastructure —
 * NER steps, entity assembly, canonical ID normalisation, sinks — that
 * other modules such as AQL query enrichment and the RAG subsystem can
 * exploit.  `IngestionToolbox` provides a single, injected handle to
 * this infrastructure without coupling any consumer to a specific
 * implementation class or creating a global singleton.
 *
 * ## Design constraints
 *
 * - **No singleton anti-pattern**: obtain an instance through
 *   `createDefault()` and inject it; tests create their own instances.
 * - **Dependency direction**: `toolbox/` depends on `ingestion/`;
 *   `ingestion/` must never depend on `toolbox/`.  Consumers (`aql/`,
 *   `rag/`) depend on `toolbox/`; they must never import `ingestion/`
 *   sinks or workflow internals directly.
 * - **Opt-in / additive**: all integration paths in consumer modules
 *   are optional (guard with `if (toolbox)` checks).
 *
 * ## Usage — wiring code (server bootstrap)
 * @code
 * auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
 * auto bridge  = std::make_shared<themis::aql::AQLIngestionBridge>(toolbox);
 * llm_handler.setIngestionBridge(bridge);
 * @endcode
 *
 * ## Usage — test isolation
 * @code
 * auto toolbox = std::make_shared<IngestionToolbox>();
 * // inject custom engine / backend as needed
 * toolbox->setWorkflowEngine(custom_engine);
 * @endcode
 *
 * Thread-safety: all public methods are thread-safe.  The underlying
 * `WorkflowEngine::execute()` is also thread-safe; see its documentation.
 */
class IngestionToolbox {
public:
    IngestionToolbox();
    ~IngestionToolbox();

    // Non-copyable, movable
    IngestionToolbox(const IngestionToolbox&) = delete;
    IngestionToolbox& operator=(const IngestionToolbox&) = delete;
    IngestionToolbox(IngestionToolbox&&) noexcept;
    IngestionToolbox& operator=(IngestionToolbox&&) noexcept;

    // ── Factory ──────────────────────────────────────────────────────────────

    /**
     * @brief Create an `IngestionToolbox` pre-configured with all built-in
     *        steps and a `NullTextGenerationBackend`.
     *
     * The returned toolbox is ready for immediate use.  Inject a real
     * `ITextGenerationBackend` via `setTextBackend()` to enable
     * LLM-backed NER and entity extraction.
     */
    static std::shared_ptr<IngestionToolbox> createDefault();

    // ── Dependency injection ──────────────────────────────────────────────────

    /**
     * @brief Replace the `WorkflowEngine`.
     *
     * @param engine  Must not be null.
     */
    void setWorkflowEngine(std::shared_ptr<ingestion::WorkflowEngine> engine);

    /**
     * @brief Inject or replace the text-generation backend.
     *
     * When @p backend is `nullptr`, a `NullTextGenerationBackend` is
     * reinstated.  The new backend is propagated to all NER/LLM steps
     * that have been registered in the `StepRegistry`.
     */
    void setTextBackend(std::shared_ptr<ingestion::ITextGenerationBackend> backend);

    // ── Accessors ─────────────────────────────────────────────────────────────

    /**
     * @brief Access the `WorkflowEngine` for profile loading and execution.
     *
     * @return Always non-null.
     */
    std::shared_ptr<ingestion::WorkflowEngine> workflowEngine() const;

    /**
     * @brief Access the `StepRegistry` for custom step registration.
     *
     * @return Reference to the registry owned by `workflowEngine()`.
     */
    ingestion::StepRegistry& stepRegistry();

    /**
     * @brief Access the currently active text-generation backend.
     *
     * @return Always non-null (falls back to `NullTextGenerationBackend`).
     */
    std::shared_ptr<ingestion::ITextGenerationBackend> textBackend() const;

    // ── High-level convenience ────────────────────────────────────────────────

    /**
     * @brief Extract entities from a plain text string.
     *
     * Constructs a minimal `ExtractionContext` from @p text, selects the
     * best matching workflow profile (using MIME type @p mime and filename
     * hint @p filename), runs the workflow, and returns the assembled
     * entity nodes.
     *
     * This is the primary entry point for consumers that need entity
     * extraction without managing `ExtractionContext` directly.
     *
     * @param text      UTF-8 text to process (may be empty — returns {}).
     * @param mime      Detected MIME type hint (default: "text/plain").
     * @param filename  Filename hint used for profile selection
     *                  (default: "input.txt").
     * @return Extracted and normalised entity nodes; empty on workflow
     *         failure (failure details are logged at warn level).
     */
    std::vector<ingestion::BaseEntity> extractEntities(
        const std::string& text,
        const std::string& mime     = "text/plain",
        const std::string& filename = "input.txt"
    );

    /**
     * @brief Extract the full `BaseEntitySet` from a plain text string.
     *
     * Like `extractEntities()` but returns the complete `BaseEntitySet`
     * including `nodes`, `edges`, and `chunks` (vector index entries).
     * Consumers that need both the graph entities **and** the embedding
     * chunks (e.g. `ContentToolboxBridge` for `BridgeResult::vectors`)
     * should call this method instead of `extractEntities()`.
     *
     * @param text      UTF-8 text to process (may be empty — returns {}).
     * @param mime      Detected MIME type hint (default: "text/plain").
     * @param filename  Filename hint used for profile selection
     *                  (default: "input.txt").
     * @return Full assembled entity set; empty `BaseEntitySet` on failure.
     */
    ingestion::BaseEntitySet extractEntitySet(
        const std::string& text,
        const std::string& mime     = "text/plain",
        const std::string& filename = "input.txt"
    );

    // ── Prometheus metrics ────────────────────────────────────────────────────

    /**
     * @brief Record one completed `extractEntities()` call in the metrics
     *        counters.
     *
     * Call this after every `extractEntities()` / `extractEntitySet()`
     * invocation (including error paths).  Thread-safe; uses `std::atomic`.
     *
     * @param entity_count Number of entities returned (0 on failure).
     * @param latency_ms   Wall-clock latency of the call in milliseconds.
     * @param success      Whether the workflow completed without error.
     */
    void recordExtraction(std::size_t entity_count,
                          uint64_t    latency_ms,
                          bool        success) noexcept;

    /**
     * @brief Produce Prometheus text-format metrics.
     *
     * Emits the following metric families (Prometheus text v0.0.4):
     * - `toolbox_extract_calls_total`      counter — total `extractEntities()` calls
     * - `toolbox_extract_errors_total`     counter — failed calls
     * - `toolbox_extract_entities_total`   counter — cumulative entity count
     * - `toolbox_extract_latency_ms_total` counter — cumulative latency
     *
     * Returns an empty string when no calls have been recorded.
     *
     * @return Prometheus text payload, or "" if unused.
     */
    std::string getMetricsText() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace toolbox
} // namespace themis
