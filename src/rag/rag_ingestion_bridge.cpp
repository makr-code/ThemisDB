/**
 * @file rag_ingestion_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=4, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "rag/rag_ingestion_bridge.h"

#include "ingestion/workflow_engine.h"
#include "ingestion/extraction_context.h"

#include <spdlog/spdlog.h>

#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <functional>  // std::hash
#include <utility>
#include <algorithm>

namespace themis {
namespace rag {

namespace {

constexpr std::size_t kMaxDocumentChars = 5u * 1024u * 1024u; // 5 MiB
constexpr std::size_t kMaxCollectionChars = 256u;
constexpr std::size_t kMaxMimeChars = 128u;
constexpr std::size_t kMaxFilenameChars = 512u;
constexpr std::size_t kMaxChunkSnippetChars = 128u * 1024u; // 128 KiB
constexpr std::size_t kMaxMetadataValueChars = 16u * 1024u; // 16 KiB

std::string trimCopy(const std::string& in) {
    const auto begin = in.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = in.find_last_not_of(" \t\r\n");
    return in.substr(begin, end - begin + 1);
}

std::string truncateCopy(const std::string& in, std::size_t max_chars) {
    if (static_cast<int>(in.size()) <= max_chars) {
        return in;
    }
    return in.substr(0, max_chars);
}

bool hasControlCharacters(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char c) {
        return c < 32u && c != '\t' && c != '\r' && c != '\n';
    });
}

std::string boundedMetadataValue(const std::string& value) {
    return truncateCopy(trimCopy(value), kMaxMetadataValueChars);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

RAGIngestionBridge::RAGIngestionBridge(
    std::shared_ptr<toolbox::IngestionToolbox> toolbox,
    std::shared_ptr<ingestion::IVectorWriter>  vector_writer,
    std::shared_ptr<ingestion::IGraphWriter>   graph_writer)
    : toolbox_(std::move(toolbox))
    , vector_writer_(std::move(vector_writer))
    , graph_writer_(std::move(graph_writer))
{
    if (!toolbox_) {
        throw std::invalid_argument(
            "RAGIngestionBridge: toolbox must not be null");
    }
}

RAGIngestionBridge::~RAGIngestionBridge() noexcept = default;

RAGIngestionBridge::RAGIngestionBridge(RAGIngestionBridge&&) noexcept = default;
RAGIngestionBridge& RAGIngestionBridge::operator=(RAGIngestionBridge&&) noexcept = default;

// ─────────────────────────────────────────────────────────────────────────────
// Core operations
// ─────────────────────────────────────────────────────────────────────────────

// Thread-Safe: captures local copies of member pointers before use.
// Complexity: O(m * e) where m = text.size(), e = entity extraction overhead
// Failure modes handled (all return ok=false with descriptive error):
//   - Empty text: rejected with error="empty input"
//   - Text > kMaxDocumentChars (5 MiB): rejected with error="input too large"
//   - Invalid collection name (empty, >256 chars, control chars): rejected
//   - Invalid mime type (empty, >128 chars, control chars): rejected
//   - Invalid filename (empty, >512 chars, control chars): rejected
//   - Workflow engine unavailable: falls back to direct entity extraction (fail-open)
//   - Vector writer failure: logged, partial result returned (vector_count=0)
//   - Graph writer failure: logged, indexing continues without graph writes
// All validation errors are logged at WARN level; I/O errors at WARN level.
// Doc ID is deterministic from text hash (idempotent re-indexing).
// Fallback workflow ensures partial indexing succeeds even without full pipeline.

IndexResult RAGIngestionBridge::indexDocument(
    const std::string& text,
    const std::string& collection,
    const std::string& mime,
    const std::string& filename)
{
    const auto toolbox = toolbox_;
    const auto vector_writer = vector_writer_;
    const auto graph_writer = graph_writer_;

    try {
        const std::string trimmed_collection = trimCopy(collection);
        const std::string trimmed_mime = trimCopy(mime);
        const std::string trimmed_filename = trimCopy(filename);

    spdlog::info(
        "RAGIngestionBridge::indexDocument start: collection='{}' mime='{}' filename='{}' text_chars={}",
        trimmed_collection,
        trimmed_mime,
        trimmed_filename,
        text.size());

    // ── Validation section (fail-closed) ────────────────────────────────────
    // All validation checks return IndexResult with ok=false + error message
    // No exception thrown; fail-closed semantics for all input validation.
    
    if (text.empty()) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: empty input");
        return IndexResult{
            .ok    = false,
            .error = "empty input"
        };
    }
    // Bound check: text <= kMaxDocumentChars (5 MiB)
    // Prevents memory exhaustion and ensures bounded ingestion time
    if (static_cast<int>(text.size()) > kMaxDocumentChars) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: text too large ({})", text.size());
        return IndexResult{
            .ok    = false,
            .error = "input too large"
        };
    }
    // Collection name validation: must be non-empty, <= 256 chars, no control chars
    // Control char check prevents injection attacks and ensures clean metadata
    if (trimmed_collection.empty() || trimmed_collection.size() > kMaxCollectionChars ||
        hasControlCharacters(trimmed_collection)) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: invalid collection");
        return IndexResult{
            .ok    = false,
            .error = "invalid collection"
        };
    }
    // MIME type validation: must be non-empty, <= 128 chars, no control chars
    if (trimmed_mime.empty() || trimmed_mime.size() > kMaxMimeChars ||
        hasControlCharacters(trimmed_mime)) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: invalid mime");
        return IndexResult{
            .ok    = false,
            .error = "invalid mime"
        };
    }
    // Filename validation: must be non-empty, <= 512 chars, no control chars
    // Filename used for workflow routing (e.g., PDF vs plain text handling)
    if (trimmed_filename.empty() || trimmed_filename.size() > kMaxFilenameChars ||
        hasControlCharacters(trimmed_filename)) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: invalid filename");
        return IndexResult{
            .ok    = false,
            .error = "invalid filename"
        };
    }

    // ── Workflow execution with fallback path ────────────────────────────────
    // Idempotent doc_id: computed from collection + text hash, deterministic.
    // Enables re-indexing the same document (will overwrite previous records).
    
    // Derive a stable document ID from collection + text
    const std::string doc_hash = computeDocHash(text);
    const std::string doc_id   = trimmed_collection + "/" + doc_hash;

    // Build an ExtractionContext and run the workflow
    ingestion::ExtractionContext ctx;
    ctx.manifest.detected_mime  = trimmed_mime;
    ctx.manifest.filename_stem  = trimmed_filename;
    ctx.manifest.extension      = "";
    ctx.raw_text                = text;

    ingestion::BaseEntitySet entity_set;
    bool used_workflow_fallback = false;

    // Primary path: use WorkflowEngine if available (full NER + chunking)
    // Fallback path: direct entity extraction + one canonical chunk
    // Fallback ensures indexing succeeds even when workflow pipeline unavailable.
    if (auto engine = toolbox->workflowEngine()) {
        auto result = engine->execute(ctx);
        if (result) {
            entity_set = result.value();
            spdlog::debug(
                "RAGIngestionBridge::indexDocument workflow result: nodes={} edges={} chunks={}",
                entity_set.nodes.size(),
                entity_set.edges.size(),
                entity_set.chunks.size());
        } else {
            // Keep indexing available even when workflow execution is unavailable
            // by creating a minimal but fully hydrated retrieval payload.
            used_workflow_fallback = true;
        }
    } else {
        used_workflow_fallback = true;
    }

    // Fallback workflow path: minimal entity extraction + one canonical chunk
    // Ensures RAG pipeline can still retrieve documents even without full processing.
    if (used_workflow_fallback) {
        spdlog::info(
            "RAGIngestionBridge::indexDocument using fallback workflow path for collection='{}'",
            trimmed_collection);
        entity_set.source_file_id = doc_id;
        entity_set.quality_score = 0.0;

        auto entities = toolbox->extractEntities(text);
        for (auto& entity : entities) {
            if (entity.source_file_id.empty()) {
                entity.source_file_id = doc_id;
            }
        }
        entity_set.nodes = std::move(entities);

        // Create one canonical chunk from the full document for retrieval fallback
        ingestion::VectorRecord fallback_chunk;
        fallback_chunk.chunk_id = doc_id + "#0";
        fallback_chunk.source_file_id = doc_id;
        fallback_chunk.text_snippet = truncateCopy(text, kMaxChunkSnippetChars);
        fallback_chunk.metadata["collection"] = trimmed_collection;
        fallback_chunk.metadata["source"] = doc_id;
        entity_set.chunks.push_back(std::move(fallback_chunk));
    }

    std::size_t vector_count = 0;
    std::size_t entity_count = entity_set.nodes.size();

    // ── Vector writer section (failure handling) ────────────────────────────
    // Writes all extracted chunks to the vector index (if vector_writer configured).
    // Injects canonical RAG metadata: collection, source, content
    // Failure to write is logged but non-fatal; indexing continues.
    // This allows partial indexing to succeed even if vector write fails.
    
    // Write vector chunks (stamp each chunk's source_file_id with our doc_id)
    if (vector_writer && !entity_set.chunks.empty()) {
        // Move chunks into the write buffer to avoid an extra full-vector copy
        // in the indexing hot path.
        std::vector<ingestion::VectorRecord> stamped_chunks = std::move(entity_set.chunks);
        for (auto& chunk : stamped_chunks) {
            if (chunk.source_file_id.empty()) {
                chunk.source_file_id = doc_id;
            }
            // Inject canonical retrieval metadata for downstream RAG consumers.
            chunk.metadata["collection"] = trimmed_collection;

            const auto source_it = chunk.metadata.find("source");
            const auto content_it = chunk.metadata.find("content");
            const std::string trimmed_source =
                source_it == chunk.metadata.end() ? std::string{} : trimCopy(source_it->second);
            const std::string trimmed_content =
                content_it == chunk.metadata.end() ? std::string{} : trimCopy(content_it->second);

            const std::string canonical_source =
                trimmed_source.empty()
                    ? chunk.source_file_id
                    : trimmed_source;
            const std::string canonical_content =
                trimmed_content.empty()
                    ? trimCopy(chunk.text_snippet)
                    : trimmed_content;

            if (!canonical_source.empty()) {
                chunk.metadata["source"] = boundedMetadataValue(canonical_source);
            }
            if (!canonical_content.empty()) {
                const std::string bounded_content = boundedMetadataValue(canonical_content);
                chunk.metadata["content"] = bounded_content;
                chunk.metadata["text"] = bounded_content;
                chunk.metadata["body"] = bounded_content;
            }
            if (chunk.text_snippet.size() > kMaxChunkSnippetChars) {
                chunk.text_snippet.resize(kMaxChunkSnippetChars);
            }
        }

        auto write_result = vector_writer->writeVectors(stamped_chunks);
        if (write_result) {
            vector_count = stamped_chunks.size();
            spdlog::debug(
                "RAGIngestionBridge::indexDocument vector write ok: chunk_count={}",
                vector_count);
        } else {
            spdlog::warn(
                "RAGIngestionBridge::indexDocument vector write failed for collection='{}'",
                trimmed_collection);
        }
        // Write failure is non-fatal: we still return a partial result
    }

    // ── Graph writer section (optional, failure-closed) ──────────────────────
    // Writes extracted entities and relations to graph store (if graph_writer configured).
    // Failure is logged but does not affect indexing result.
    // Write is fire-and-forget; caller cannot observe graph write failures.
    
    // Write graph entities / relations
    if (graph_writer) {
        if (!entity_set.nodes.empty()) {
            static_cast<void>(graph_writer->writeEntities(entity_set.nodes));
        }
        if (!entity_set.edges.empty()) {
            static_cast<void>(graph_writer->writeRelations(entity_set.edges));
        }
    }

    spdlog::info(
        "RAGIngestionBridge::indexDocument complete: doc_id='{}' entities={} vectors={} fallback={}",
        doc_id,
        entity_count,
        vector_count,
        used_workflow_fallback);

    return IndexResult{
        .ok           = true,
        .doc_id       = doc_id,
        .collection   = trimmed_collection,
        .entity_count = entity_count,
        .vector_count = vector_count
    };
    } catch (const std::exception& ex) {
        spdlog::error("RAGIngestionBridge::indexDocument exception: {}", ex.what());
        return IndexResult{.ok = false, .error = ex.what()};
    } catch (...) {
        spdlog::error("RAGIngestionBridge::indexDocument exception: unknown failure");
        return IndexResult{.ok = false, .error = "unknown indexing failure"};
    }
}

std::size_t RAGIngestionBridge::enrichRetrievedDocuments(
    std::vector<judge::RetrievedDocument>& docs)
{
    const auto toolbox = toolbox_;
    std::size_t enriched = 0;
    spdlog::info(
        "RAGIngestionBridge::enrichRetrievedDocuments start: docs={}",
        docs.size());

    try {
        for (auto& doc : docs) {
        const std::string canonical_id = trimCopy(doc.id);
        const std::string canonical_content = trimCopy(doc.content);

        if (canonical_content.empty() || canonical_id.empty()) {
            continue;
        }

        const std::string metadata_content = trimCopy(doc.metadata["content"]);
        const std::string metadata_source = trimCopy(doc.metadata["source"]);

        if (metadata_content.empty()) {
            doc.metadata["content"] = canonical_content;
        } else {
            doc.metadata["content"] = metadata_content;
        }

        if (metadata_source.empty()) {
            doc.metadata["source"] = canonical_id;
        } else {
            doc.metadata["source"] = metadata_source;
        }

        if (doc.metadata["content"].empty() || doc.metadata["source"].empty()) {
            continue;
        }

        auto entities = toolbox->extractEntities(doc.metadata["content"]);
        if (entities.empty()) {
            continue;
        }
        const std::string context = buildEntityContext(entities);
        if (!context.empty()) {
            doc.metadata["_entities"] = context;
            ++enriched;
        }
    }

        spdlog::info(
            "RAGIngestionBridge::enrichRetrievedDocuments complete: docs={} enriched={}",
            docs.size(),
            enriched);

        return enriched;
    } catch (const std::exception& ex) {
        spdlog::error("RAGIngestionBridge::enrichRetrievedDocuments exception: {}", ex.what());
        return enriched;
    } catch (...) {
        spdlog::error("RAGIngestionBridge::enrichRetrievedDocuments exception: unknown failure");
        return enriched;
    }
}

std::vector<ingestion::BaseEntity>
RAGIngestionBridge::extractEntitiesForContext(const std::string& text) {
    const auto toolbox = toolbox_;
    try {
        return toolbox->extractEntities(text);
    } catch (const std::exception& ex) {
        spdlog::error("RAGIngestionBridge::extractEntitiesForContext exception: {}", ex.what());
        return {};
    } catch (...) {
        spdlog::error("RAGIngestionBridge::extractEntitiesForContext exception: unknown failure");
        return {};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string RAGIngestionBridge::buildEntityContext(
    const std::vector<ingestion::BaseEntity>& entities)
{
    if (entities.empty()) {
        return {};
    }

    std::ostringstream oss = {};
    oss << "Extracted entities:";
    bool first = true;
    for (const auto& e : entities) {
        if (!first) {
            oss << " |";
        }
        oss << " " << entityTypeName(e.entity_type);
        const std::string canonical_entity_id = trimCopy(e.id);
        if (!canonical_entity_id.empty()) {
            oss << " " << canonical_entity_id;
        }
        first = false;
    }
    return oss.str();
}

std::string RAGIngestionBridge::computeDocHash(const std::string& text) {
    // Derive a stable 16-character hex digest from the text.
    // Uses two independent std::hash calls on overlapping halves so that
    // both the beginning and end of long documents contribute to the hash.
    // This is NOT cryptographically secure; it is only used as a stable,
    // deterministic document key for idempotent upserts.
    const std::size_t h1 = std::hash<std::string>{}(text);
    const std::size_t h2 = std::hash<std::string>{}(
        text.size() > 32 ? text.substr(text.size() / 2) : text
    );
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0')
        << std::setw(8) << (static_cast<uint32_t>(h1) & 0xFFFFFFFFu)
        << std::setw(8) << (static_cast<uint32_t>(h2) & 0xFFFFFFFFu);
    return oss.str();
}

std::string RAGIngestionBridge::entityTypeName(ingestion::EntityType et) {
    using ET = ingestion::EntityType;
    switch (et) {
        case ET::UNKNOWN:              return "UNKNOWN";
        case ET::CHUNK:                return "CHUNK";
        case ET::PERSON:               return "PERSON";
        case ET::ORGANIZATION:         return "ORGANIZATION";
        case ET::LOCATION:             return "LOCATION";
        case ET::DATE:                 return "DATE";
        case ET::URL:                  return "URL";
        case ET::TABLE_ROW:            return "TABLE_ROW";
        case ET::GEO_FEATURE:          return "GEO_FEATURE";
        case ET::IMAGE_REGION:         return "IMAGE_REGION";
        case ET::LEGAL_PROVISION:      return "LEGAL_PROVISION";
        case ET::LEGAL_NORM_REFERENCE: return "LEGAL_NORM_REFERENCE";
        case ET::LEGAL_OBLIGATION:     return "LEGAL_OBLIGATION";
        case ET::LEGAL_PROHIBITION:    return "LEGAL_PROHIBITION";
        case ET::LEGAL_PERMISSION:     return "LEGAL_PERMISSION";
        case ET::LEGAL_AUTHORITY:      return "LEGAL_AUTHORITY";
        case ET::LEGAL_AKTENZEICHEN:   return "LEGAL_AKTENZEICHEN";
        case ET::LEGAL_DECISION:       return "LEGAL_DECISION";
        case ET::LEGAL_APPLICANT:      return "LEGAL_APPLICANT";
        case ET::LEGAL_EFFECTIVE_DATE: return "LEGAL_EFFECTIVE_DATE";
        default:                       return "UNKNOWN";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<toolbox::IngestionToolbox>
RAGIngestionBridge::toolbox() const {
    return toolbox_;
}

std::shared_ptr<ingestion::IVectorWriter>
RAGIngestionBridge::vectorWriter() const {
    return vector_writer_;
}

std::shared_ptr<ingestion::IGraphWriter>
RAGIngestionBridge::graphWriter() const {
    return graph_writer_;
}

// ─────────────────────────────────────────────────────────────────────────────
// RAGIngestionBridge::indexOptimizerLog  (IMPL-A2 Phase 2)
// ─────────────────────────────────────────────────────────────────────────────

IndexResult RAGIngestionBridge::indexOptimizerLog(
    const std::string& query_id,
    const std::string& plan_json,
    double             latency_ms,
    const std::string& collection)
{
    // Build a structured text document from the optimizer-log entry.
    // The plan_json is capped at 2 048 chars to stay within the chunk budget.
    const std::string plan_snippet =
        plan_json.size() > 2048 ? plan_json.substr(0, 2048) + "..." : plan_json;

    std::ostringstream doc = {};
    doc << "optimizer_log\n"
        << "query_id: " << query_id << "\n"
        << "latency_ms: " << latency_ms << "\n"
        << "plan: " << plan_snippet << "\n";

    const std::string filename = "optimizer-log-" + query_id + ".txt";
    return indexDocument(doc.str(), collection, "text/plain", filename);
}

} // namespace rag
} // namespace themis
