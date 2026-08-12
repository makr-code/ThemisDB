/**
 * @file rag_ingestion_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "toolbox/ingestion_toolbox.h"
#include "ingestion/base_entity.h"
#include "ingestion/ingestion_sinks.h"
#include "rag/rag_judge.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ─────────────────────────────────────────────────────────────────────────────
// IndexResult — return type of RAGIngestionBridge::indexDocument()
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result produced by `RAGIngestionBridge::indexDocument()`.
 */
struct IndexResult {
    bool        ok{false};          ///< True on success
    std::string doc_id;             ///< Assigned document identifier
    std::string collection;         ///< Target collection name
    std::size_t entity_count{0};    ///< Number of NER entities extracted
    std::size_t vector_count{0};    ///< Number of vector chunks written
    std::string error;              ///< Human-readable error when ok == false
};

// ─────────────────────────────────────────────────────────────────────────────
// RAGIngestionBridge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Bridge that connects the `toolbox::IngestionToolbox` to the RAG
 *        pipeline, enabling document indexing and entity-enriched retrieval.
 *
 * ## Motivation
 *
 * `AQLIngestionBridge` enriches in-flight AQL `INSERT` payloads at query time.
 * `RAGIngestionBridge` addresses the complementary use case: indexing documents
 * *before* retrieval so that the RAG pipeline can locate them.
 *
 *  1. **`indexDocument()`** — runs the full ingestion workflow on a text blob,
 *     writes extracted vector chunks to an `IVectorWriter` sink, optionally
 *     writes NER graph nodes/edges to an `IGraphWriter` sink, and returns a
 *     structured `IndexResult` that callers can use to track the document.
 *
 *  2. **`extractEntitiesForContext()`** — delegate to
 *     `IngestionToolbox::extractEntities()` for use in re-ranking and
 *     prompt-context construction.
 *
 *  3. **`buildEntityContext()`** — converts a `BaseEntity` list to a compact
 *     string for injection into an LLM prompt, exactly mirroring the
 *     `AQLIngestionBridge::buildEntityContext()` API.
 *
 *  4. **`enrichRetrievedDocuments()`** — for each document in a retrieved set,
 *     extracts NER entities from the document content and attaches them to the
 *     document's metadata map under the key `"_entities"` (compact string) so
 *     that downstream re-rankers and prompt builders can use them.
 *
 * ## Dependency direction
 * @code
 *   rag/  →  toolbox/  →  ingestion/
 * @endcode
 * `ingestion/` never imports `rag/` or `toolbox/` headers.
 *
 * ## Deduplication
 * `indexDocument()` derives a stable `doc_id` from `collection + "/" +
 * SHA-1-like prefix of the text` (deterministic hash).  When the same
 * document is re-indexed, the `IVectorWriter` sink replaces existing records
 * keyed by `chunk_id`, making the operation idempotent.
 *
 * ## Thread-safety
 * All public methods are thread-safe.  The bridge holds no mutable state
 * beyond the constructor-injected shared pointers.
 *
 * ## Usage
 * @code
 * auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
 * auto bridge  = std::make_shared<RAGIngestionBridge>(toolbox, vector_writer);
 *
 * // Index a document
 * auto result = bridge->indexDocument("This is the document text.", "legal-docs");
 * if (!result.ok) { LOG_ERROR("index failed: {}", result.error); }
 *
 * // Enrich retrieved documents before re-ranking
 * bridge->enrichRetrievedDocuments(retrieved_docs, query_text);
 * @endcode
 */
class RAGIngestionBridge {
public:
    /**
     * @brief Construct a bridge backed by @p toolbox.
     *
     * @param toolbox       Shared toolbox instance; must not be null.
     * @param vector_writer Optional vector-store sink.  When provided,
     *                      `indexDocument()` writes extracted vector chunks
     *                      to the vector index.  Pass `nullptr` to skip
     *                      vector writes (useful for pure NER use-cases).
     * @param graph_writer  Optional graph-store sink.  When provided,
     *                      `indexDocument()` writes extracted entities and
     *                      relations to the graph store.
     */
    explicit RAGIngestionBridge(
        std::shared_ptr<toolbox::IngestionToolbox>    toolbox,
        std::shared_ptr<ingestion::IVectorWriter>     vector_writer = nullptr,
        std::shared_ptr<ingestion::IGraphWriter>      graph_writer  = nullptr
    );

    ~RAGIngestionBridge() noexcept;

    // Non-copyable, movable
    RAGIngestionBridge(const RAGIngestionBridge&) = delete;
    RAGIngestionBridge& operator=(const RAGIngestionBridge&) = delete;
    RAGIngestionBridge(RAGIngestionBridge&&) noexcept;
    RAGIngestionBridge& operator=(RAGIngestionBridge&&) noexcept;

    // ── Core operations ───────────────────────────────────────────────────────

    /**
     * @brief Index a document by running the full ingestion workflow and
     *        writing extracted chunks and entities to the configured sinks.
     *
     * Steps:
     *  1. Runs `WorkflowEngine::execute()` on the text to produce a
     *     `BaseEntitySet` (NER nodes, vector chunks, quality score).
     *     If workflow execution is unavailable or fails, a minimal fallback
     *     entity set is generated from direct entity extraction plus one
     *     canonical retrieval chunk so indexing remains usable.
     *  2. Writes all `VectorRecord` chunks to the `IVectorWriter` sink
     *     (if configured).
     *  3. Writes all `BaseEntity` nodes and `EntityRelation` edges to the
     *     `IGraphWriter` sink (if configured).
     *  4. Returns an `IndexResult` describing the outcome.
     *
     * The returned `doc_id` has the form `"<collection>/<doc_hash>"` where
     * `doc_hash` is a stable 16-character hex digest of the input text.
     *
     * @param text        UTF-8 text to index.  Empty text is a safe no-op
     *                    that returns `IndexResult{.ok=false, .error="empty input"}`.
     * @param collection  Logical collection name (e.g. "legal-docs", "default").
     * @param mime        MIME type hint (default: `"text/plain"`).
     * @param filename    Filename hint for workflow routing (default: `"document.txt"`).
     * @return `IndexResult` describing the operation outcome.
     */
    IndexResult indexDocument(
        const std::string& text,
        const std::string& collection = "default",
        const std::string& mime       = "text/plain",
        const std::string& filename   = "document.txt"
    );

    /**
     * @brief Index an optimizer-log entry from a query execution for RAG retrieval.
     *
     * Formats the optimizer-log entry as a structured text document and passes it
     * through `indexDocument()` so that the RAG pipeline can retrieve past
     * optimizer decisions during Loop 1–3 context assembly.
     *
     * The formatted document contains: `query_id`, `latency_ms`, and the
     * `explain_plan_json` snippet (first 2 048 chars to stay within chunk budget).
     *
     * @param query_id      Stable query fingerprint / request-id.
     * @param plan_json     JSON-serialised EXPLAIN / BaoOptimizer plan.
     * @param latency_ms    Observed end-to-end query latency in milliseconds.
     * @param collection    Target collection (default: `"optimizer-logs"`).
     * @return `IndexResult` from the underlying `indexDocument()` call.
     */
    IndexResult indexOptimizerLog(
        const std::string& query_id,
        const std::string& plan_json,
        double             latency_ms,
        const std::string& collection = "optimizer-logs"
    );

    /**
     * @brief Enrich a list of retrieved documents with NER entity context.
     *
     * For each document in @p docs, this method first canonicalises retrieval
     * metadata for downstream RAG stages:
     *  - `metadata["content"]` is backfilled from `RetrievedDocument::content`
     *  - `metadata["source"]` is backfilled from `RetrievedDocument::id`
     *
     * It then runs `extractEntitiesForContext()` on the document content and,
     * when entities are found, appends a compact entity context string to
     * `metadata["_entities"]`.
     *
     * Documents without content or stable id are skipped (fail-closed). Documents
     * that yield no entities still retain the canonical `content`/`source`
     * metadata backfill.
     * This operation is safe to call on an empty vector (no-op).
     *
     * @param docs  Retrieved documents to enrich (modified in place).
     * @return The number of documents that received at least one entity.
     */
    std::size_t enrichRetrievedDocuments(
        std::vector<judge::RetrievedDocument>& docs
    );

    /**
     * @brief Extract entities from @p text for use as RAG prompt context.
     *
     * Thin wrapper around `IngestionToolbox::extractEntities()`.
     *
     * @param text  UTF-8 text to process.
     * @return Extracted and normalised entity nodes; empty when extraction
     *         yields no results or the text is empty.
     */
    std::vector<ingestion::BaseEntity> extractEntitiesForContext(
        const std::string& text
    );

    /**
     * @brief Build a compact LLM context string from a list of entities.
     *
     * Formats the entity list as a single line for injection into an LLM
     * prompt.  Example output:
     * @code
     * "Extracted entities: LEGAL_PROVISION law:BGB:§823 | ORGANIZATION org:abc12"
     * @endcode
     * Returns an empty string when @p entities is empty.
     *
     * @param entities  Entities to summarise.
     * @return Formatted context string; empty when entities is empty.
     */
    static std::string buildEntityContext(
        const std::vector<ingestion::BaseEntity>& entities
    );

    // ── Accessors ─────────────────────────────────────────────────────────────

    /**
     * @brief Return the backing `IngestionToolbox`.
     */
    std::shared_ptr<toolbox::IngestionToolbox> toolbox() const;

    /**
     * @brief Return the configured vector-writer sink (may be null).
     */
    std::shared_ptr<ingestion::IVectorWriter> vectorWriter() const;

    /**
     * @brief Return the configured graph-writer sink (may be null).
     */
    std::shared_ptr<ingestion::IGraphWriter> graphWriter() const;

private:
    std::shared_ptr<toolbox::IngestionToolbox> toolbox_;
    std::shared_ptr<ingestion::IVectorWriter>  vector_writer_;
    std::shared_ptr<ingestion::IGraphWriter>   graph_writer_;

    /// Compute a stable 16-character hex doc hash from text.
    static std::string computeDocHash(const std::string& text);

    /// Map an EntityType enum value to a short display string.
    static std::string entityTypeName(ingestion::EntityType et);
};

} // namespace rag
} // namespace themis
