/**
 * @file content_toolbox_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: content_toolbox_bridge.h | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

// ContentToolboxBridge bridges the content/ processing + storage layer
// with the toolbox/ ingestion pipeline.
//
// Dependency direction: toolbox/ → content/ (permitted).
//                       content/ → toolbox/ is FORBIDDEN.
//
// Both headers are included here because this bridge class sits in
// toolbox/ and explicitly wires the two subsystems together.

#include "toolbox/ingestion_toolbox.h"
#include "content/content_manager.h"
#include "ingestion/ingestion_sinks.h"
#include "ingestion/base_entity.h"
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ContentToolboxBridge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Unified ingest entry-point that combines ContentManager's
 *        security/storage layer with the IngestionToolbox enrichment pipeline.
 *
 * ## Motivation
 *
 * `ContentManager` handles:
 *  - Malware scanning & abuse detection (`ContentSecurity`)
 *  - Deduplication (`DeduplicationChecker`)
 *  - Blob storage to RocksDB with optional encryption
 *  - Chunking, embedding generation, VectorIndex storage
 *
 * `IngestionToolbox` handles:
 *  - NER (legal / generic)
 *  - Deontic extraction
 *  - Graph entity assembly
 *  - Graph store sink (`IGraphWriter`)
 *  - Vector store sink (`IVectorWriter`)
 *
 * A single `ingest()` call on this bridge runs both pipelines and returns
 * a combined result so callers never need to coordinate them manually.
 *
 * ## Dependency direction
 * @code
 *   toolbox/ContentToolboxBridge → content/ContentManager  (OK)
 *   toolbox/ContentToolboxBridge → toolbox/IngestionToolbox (OK)
 *   content/ → toolbox/  (FORBIDDEN — never introduce this)
 * @endcode
 *
 * ## Thread-safety
 * `ingest()` is thread-safe.  All internal state is protected by a mutex.
 * `ContentManager` and `IngestionToolbox` must themselves be thread-safe
 * (they are, as documented in their respective headers).
 *
 * ## Usage — server bootstrap
 * @code
 * auto bridge = std::make_shared<ContentToolboxBridge>(
 *     toolbox,           // already configured via ToolboxBuilder
 *     content_manager,   // already initialised with storage / index
 *     graph_writer,      // optional — writes NER entities to graph store
 *     vector_writer      // optional — writes embeddings to vector index
 * );
 *
 * auto result = bridge->ingest(raw_bytes, meta, "legal-docs");
 * if (!result.ok) { LOG_ERROR("ingest failed: {}", result.error); }
 * @endcode
 */
class ContentToolboxBridge {
public:
    /**
     * @brief Construct the bridge.
     *
     * @param toolbox         Configured `IngestionToolbox` for enrichment.
     *                        Must not be null.
     * @param content_manager Initialised `ContentManager` for storage.
     *                        Must not be null.
     * @param graph_writer    Optional sink for NER graph entities.
     * @param vector_writer   Optional sink for embedding vectors.
     */
    ContentToolboxBridge(
        std::shared_ptr<IngestionToolbox>               toolbox,
        std::shared_ptr<content::ContentManager>        content_manager,
        std::shared_ptr<ingestion::IGraphWriter>        graph_writer  = nullptr,
        std::shared_ptr<ingestion::IVectorWriter>       vector_writer = nullptr
    );

    ~ContentToolboxBridge();

    // Non-copyable, movable
    ContentToolboxBridge(const ContentToolboxBridge&) = delete;
    ContentToolboxBridge& operator=(const ContentToolboxBridge&) = delete;
    ContentToolboxBridge(ContentToolboxBridge&&) noexcept;
    ContentToolboxBridge& operator=(ContentToolboxBridge&&) noexcept;

    // ─────────────────────────────────────────────────────────────────────────
    // BridgeResult — combined outcome of a single ingest() call
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Combined outcome of ContentManager ingest + Toolbox enrichment.
     */
    struct BridgeResult {
        // ContentManager output
        std::string content_id;            ///< Primary content ID assigned by ContentManager
        std::vector<std::string> child_ids;///< Child content IDs (archive members)

        // IngestionToolbox output
        std::vector<ingestion::BaseEntity>   entities;   ///< NER / deontic entities
        std::vector<ingestion::VectorRecord> vectors;    ///< Embedding records

        // Status
        bool ok{false};
        std::string error;

        /// true when entities or vectors were written to external sinks
        bool sinks_written{false};
    };

    // ─────────────────────────────────────────────────────────────────────────
    // ingest() — main entry point
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Ingest raw bytes through both ContentManager and IngestionToolbox.
     *
     * Steps performed in order:
     *  1. `ContentManager::ingestRawBlob()` — security scan, dedup, store blob,
     *     generate chunks + embeddings.
     *  2. Retrieve extracted text from ContentManager result.
     *  3. `IngestionToolbox::extractEntities()` — NER/deontic/graph assembly.
     *  4. Write entities to `graph_writer` if set.
     *  5. Write vector records to `vector_writer` if set.
     *  6. Return combined `BridgeResult`.
     *
     * @param data         Raw binary content to ingest (any format).
     * @param filename     Original filename hint (used for MIME detection and
     *                     archive member naming), e.g. "report.pdf".
     * @param mime_type    Optional MIME type override.  When empty, ContentManager
     *                     auto-detects via magic bytes + extension.
     * @param collection   Target collection name for the document store sink.
     *                     Default: "default".
     * @param user_context User context string forwarded to ContentManager for
     *                     per-user encryption and access control.
     * @return BridgeResult with content_id, entities, vectors, and ok/error.
     */
    BridgeResult ingest(
        std::span<const std::byte> data,
        const std::string& filename,
        const std::string& mime_type    = "",
        const std::string& collection   = "default",
        const std::string& user_context = ""
    );

    /**
     * @brief Run the enrichment pipeline for content already stored in
     *        ContentManager (re-enrichment path).
     *
     * Retrieves the extracted text for @p content_id from ContentManager,
     * runs the Toolbox enrichment pipeline, and writes to the sinks.
     * Does NOT re-ingest the raw blob (no security re-scan, no dedup check).
     *
     * @param content_id    ID previously returned by `ingest()` or an earlier
     *                      `ContentManager::ingestRawBlob()` call.
     * @param collection    Target collection for the document store sink.
     * @return BridgeResult with entities, vectors, and ok/error.
     *         `content_id` in the result is set to the input @p content_id.
     */
    BridgeResult enrichExisting(
        const std::string& content_id,
        const std::string& collection = "default"
    );

    // ─────────────────────────────────────────────────────────────────────────
    // Accessors
    // ─────────────────────────────────────────────────────────────────────────

    std::shared_ptr<IngestionToolbox>        toolbox()        const;
    std::shared_ptr<content::ContentManager> contentManager() const;
    std::shared_ptr<ingestion::IGraphWriter> graphWriter()    const;
    std::shared_ptr<ingestion::IVectorWriter> vectorWriter()  const;

    // ─────────────────────────────────────────────────────────────────────────
    // Prometheus metrics (Phase 2.4)
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Return cumulative count of bridge ingest/enrichment failures.
     *
     * Used for Prometheus metric `bridge_failures_total`.
     * Incremented on ContentManager failures, null checks, or other
     * bridge-level errors. Does not count individual sink write failures
     * (those are tracked separately).
     */
    uint64_t failuresTotal() const noexcept;

    /**
     * @brief Return cumulative count of graph writer failures.
     *
     * Used for Prometheus metric `bridge_graph_write_failures_total`.
     * Incremented when an entity write to the graph sink fails, but the
     * bridge continues processing (soft-fail behavior).
     */
    uint64_t graphWriteFailuresTotal() const noexcept;

    /**
     * @brief Return cumulative count of vector writer failures.
     *
     * Used for Prometheus metric `bridge_vector_write_failures_total`.
     * Incremented when a vector record write to the vector sink fails, but the
     * bridge continues processing (soft-fail behavior).
     */
    uint64_t vectorWriteFailuresTotal() const noexcept;

    /**
     * @brief Export all bridge metrics in Prometheus text format.
     *
     * Returns empty string if no operations have been recorded yet.
     * Includes failure counters and latency histogram buckets.
     */
    std::string getMetricsText() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    /**
     * @brief Record operation latency for metrics histogram.
     *
     * Used internally by ingest() and enrichExisting() to populate latency buckets.
     * @param latency_ms Operation latency in milliseconds
     */
    void recordLatency(uint64_t latency_ms) noexcept;
};

} // namespace toolbox
} // namespace themis
