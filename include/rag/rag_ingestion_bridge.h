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
 * ## Thread Safety
 * All public methods are **thread-safe**. The bridge holds no mutable state
 * beyond constructor-injected shared pointers (toolbox, vector_writer, graph_writer).
 * Concurrent calls to indexDocument(), enrichRetrievedDocuments(), or
 * extractEntitiesForContext() are safe and do not race.
 *
 * ## Complexity Analysis
 * - **indexDocument()**: O(m * e) where m = text.size(), e = entity extraction overhead
 *   - Delegates to IngestionToolbox (typically linear in document length)
 *   - IVectorWriter and IGraphWriter sinks are O(1) per chunk/entity
 * - **enrichRetrievedDocuments()**: O(d * e) where d = docs.size(), e = entity extraction
 *   - Single pass over document list
 *   - Entity extraction per document
 * - **extractEntitiesForContext()**: O(t) where t = text.size()
 *   - Direct delegation to IngestionToolbox::extractEntities()
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
 * bridge->enrichRetrievedDocuments(retrieved_docs);
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
     * ## Workflow
     *  1. Validates input size (text <= kMaxDocumentChars, collection <= kMaxCollectionChars).
     *  2. Runs `WorkflowEngine::execute()` on the text to produce a
     *     `BaseEntitySet` (NER nodes, vector chunks, quality score).
     *     If workflow execution is unavailable or fails, a minimal fallback
     *     entity set is generated from direct entity extraction plus one
     *     canonical retrieval chunk so indexing remains usable.
     *  3. Writes all `VectorRecord` chunks to the `IVectorWriter` sink
     *     (if configured).
     *  4. Writes all `BaseEntity` nodes and `EntityRelation` edges to the
     *     `IGraphWriter` sink (if configured).
     *  5. Returns an `IndexResult` describing the outcome.
     *
     * ## Idempotency
     * The returned `doc_id` has the form `"<collection>/<doc_hash>"` where
     * `doc_hash` is a stable 16-character hex digest of the input text.
     * Re-indexing the same text produces the same `doc_id` and overwrite
     * previous records in the vector and graph sinks.
     *
     * @param text        UTF-8 text to index.  Empty text is rejected with
     *                    `IndexResult{.ok=false, .error="empty input"}`.
     *                    Text larger than kMaxDocumentChars (5 MiB) is rejected.
     * @param collection  Logical collection name (e.g. "legal-docs", "default").
     *                    Must be <= kMaxCollectionChars (256 chars).
     *                    Default: "default".
     * @param mime        MIME type hint for workflow routing (e.g. "text/plain", "text/html").
     *                    Default: "text/plain".
     * @param filename    Filename hint for workflow routing (e.g. "document.txt").
     *                    Default: "document.txt".
     *
     * @return IndexResult with:
     *   - ok: true on success, false on failure (validation, I/O, etc.)
     *   - doc_id: assigned document identifier (format: "<collection>/<hash>")
     *   - collection: echoed collection name
     *   - entity_count: number of NER entities extracted
     *   - vector_count: number of vector chunks written
     *   - error: human-readable error message when ok == false
     *
     * @pre toolbox must not be null (checked in constructor)
     * @pre text.size() <= kMaxDocumentChars (5 MiB)
     * @pre collection.size() <= kMaxCollectionChars (256 chars)
     *
     * @post if ok == true: doc_id is non-empty and deterministic from text
     * @post if ok == false: error message is non-empty and human-readable
     * @post vector_count reflects records written to vector_writer (0 if null)
     * @post entity_count reflects records written to graph_writer (0 if null)
     *
     * @throws std::invalid_argument if text is empty (see ok == false in return)
     *
     * @note Complexity: O(m * e) where m = text.size(), e = extraction overhead
     * @note Thread-safe: no mutable shared state modified
     * @note Fail-closed: malformed input returns IndexResult.ok=false, not exception
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
     * ## Processing
     * For each document in @p docs:
     *  1. Canonicalizes retrieval metadata:
     *     - Sets `metadata["content"]` from `RetrievedDocument::content` (if empty)
     *     - Sets `metadata["source"]` from `RetrievedDocument::id` (if empty)
     *  2. Runs `extractEntitiesForContext()` on the document content
     *  3. If entities are found, appends a compact entity context string to
     *     `metadata["_entities"]` for downstream re-rankers and prompt builders
     *
     * ## Failure Handling (Fail-Closed)
     * Documents without content or stable id are skipped (not modified).
     * Documents that yield no entities still retain the canonical `content`/`source`
     * metadata backfill. This operation is safe to call on an empty vector (no-op).
     *
     * ## Complexity
     * - **Time**: O(d * e) where d = docs.size(), e = entity extraction overhead
     * - **Space**: O(1) auxiliary (modifies docs in-place)
     *
     * @param docs  Retrieved documents to enrich (modified in-place).
     *              Safe to pass empty vector (no-op).
     *
     * @return Number of documents that received at least one entity.
     *         (Note: all documents may still be enriched with content/source metadata)
     *
     * @pre docs not null (vector, not pointer)
     * @post docs[i].metadata["content"] is set if previously empty
     * @post docs[i].metadata["source"] is set if previously empty
     * @post docs[i].metadata["_entities"] is set if entities were found
     *
     * @note Complexity: O(d * e) where d = docs.size(), e = extraction overhead
     * @note Thread-safe: each document processed independently
     * @note Fail-closed: documents without content/id are silently skipped
     */
    std::size_t enrichRetrievedDocuments(
        std::vector<judge::RetrievedDocument>& docs
    );

    /**
     * @brief Extract entities from @p text for use as RAG prompt context.
     *
     * Thin wrapper around `IngestionToolbox::extractEntities()`.
     * Runs Named Entity Recognition on the text and returns normalized entity nodes.
     *
     * @param text  UTF-8 text to process. Empty text is handled gracefully
     *              and returns an empty vector (no error).
     *
     * @return Extracted and normalised entity nodes; empty vector when:
     *   - text is empty
     *   - extraction yields no results
     *   - extraction fails (fail-closed)
     *
     * @note Complexity: O(t) where t = text.size()
     * @note Thread-safe: stateless delegation to IngestionToolbox
     * @note Fail-closed: extraction failures return empty vector, not exception
     *
     * @see buildEntityContext() to format entities into LLM prompt context
     */
    std::vector<ingestion::BaseEntity> extractEntitiesForContext(
        const std::string& text
    );

    /**
     * @brief Build a compact LLM context string from a list of entities.
     *
     * Formats the entity list as a single line for injection into an LLM
     * prompt. Each entity is formatted as `TYPE display_name:entity_id`.
     *
     * ## Format Example
     * @code
     * "Extracted entities: LEGAL_PROVISION law:BGB:§823 | ORGANIZATION org:abc12"
     * @endcode
     *
     * ## Behavior
     * - Empty entity list returns empty string (no "Extracted entities:" prefix)
     * - Single entity: prefix + entity
     * - Multiple entities: prefix + entity1 | entity2 | ...
     * - Used by downstream re-rankers and prompt builders
     *
     * ## Complexity
     * - **Time**: O(n) where n = entities.size()
     * - **Space**: O(n) for output string
     *
     * @param entities  Entities to summarise. Empty vector returns empty string.
     *
     * @return Formatted context string suitable for LLM prompt injection;
     *         empty string when entities is empty or contains no displayable entities.
     *
     * @note Complexity: O(n) where n = entities.size()
     * @note Stateless function (no mutable state)
     * @note Thread-safe: pure function, no concurrency issues
     * @note Fail-closed: malformed entities result in skipping that entity, not error
     *
     * @see enrichRetrievedDocuments() uses this to populate metadata["_entities"]
     * @see extractEntitiesForContext() to extract entities before calling this
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
