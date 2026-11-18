/**
 * @file rag_ingestion_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "rag/rag_ingestion_bridge.h"

#include "core/concerns/adapter_signing.h"
#include "ingestion/workflow_engine.h"
#include "ingestion/extraction_context.h"

#include <spdlog/spdlog.h>

#include <sstream>
#include <stdexcept>
#include <utility>
#include <algorithm>

namespace themis {
namespace rag {

namespace {

constexpr std::size_t kMaxDocumentChars = 5 * 1024 * 1024; // 5 MiB
constexpr std::size_t kMaxCollectionChars = 256;
constexpr std::size_t kMaxMimeChars = 128;
constexpr std::size_t kMaxFilenameChars = 512;
constexpr std::size_t kMaxChunkSnippetChars = 128 * 1024; // 128 KiB
constexpr std::size_t kMaxMetadataValueChars = 16 * 1024; // 16 KiB

/**
 * @brief Trim Copy.
 * @param[in] in Input parameter.
 * @return Return value.
 * @details Calls: find_first_not_of(), find_last_not_of(), substr().
 */
std::string trimCopy(const std::string& in) {
    const auto begin = in.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = in.find_last_not_of(" \t\r\n");
    return in.substr(begin, end - begin + 1);
}

/**
 * @brief Truncate Copy.
 * @param[in] in Input parameter.
 * @param[in] max_chars Input parameter.
 * @return Return value.
 * @details Calls: size(), substr().
 */
std::string truncateCopy(const std::string& in, std::size_t max_chars) {
    if (in.size() <= max_chars) {
        return in;
    }
    return in.substr(0, max_chars);
}

/**
 * @brief Has Control Characters.
 * @param[in] value Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: std::any_of(), begin(), end().
 */
bool hasControlCharacters(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char c) {
        return c < 32 && c != '\t' && c != '\r' && c != '\n';
    });
}

/**
 * @brief Bounded Metadata Value.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: truncateCopy(), trimCopy().
 */
std::string boundedMetadataValue(const std::string& value) {
    return truncateCopy(trimCopy(value), kMaxMetadataValueChars);
}

const char* indexStatusName(IndexStatus status) {
    switch (status) {
        case IndexStatus::COMPLETE: return "complete";
        case IndexStatus::DEGRADED: return "degraded";
        case IndexStatus::PARTIAL: return "partial";
        case IndexStatus::FAILED: return "failed";
    }
    return "failed";
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

/**
 * @brief Thread-Safe: captures local copies of member pointers before use.
 * @param[in] text Input parameter.
 * @param[in] collection Input parameter.
 * @param[in] mime Input parameter.
 * @param[in] filename Input parameter.
 * @return Return value.
 * @details Workflow fallback is reported as DEGRADED. Configured sink failures
 * return PARTIAL or FAILED, never ok=true; unconfirmed write outcomes are
 * conservatively reported as PARTIAL. IDs use a full SHA-256 text digest.
 */

IndexResult RAGIngestionBridge::indexDocument(
    const std::string& text,
    const std::string& collection,
    const std::string& mime,
    const std::string& filename)
{
    const auto toolbox = toolbox_;
    const auto vector_writer = vector_writer_;
    const auto graph_writer = graph_writer_;
    IndexResult outcome;
    bool write_started = false;

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
        IndexResult invalid_result;
        invalid_result.error = "empty input";
        return invalid_result;
    }
    // Bound check: text <= kMaxDocumentChars (5 MiB)
    // Prevents memory exhaustion and ensures bounded ingestion time
    if (text.size() > kMaxDocumentChars) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: text too large ({})",text.size());
        IndexResult invalid_result;
        invalid_result.error = "input too large";
        return invalid_result;
    }
    // Collection name validation: must be non-empty, <= 256 chars, no control chars
    // Control char check prevents injection attacks and ensures clean metadata
    if (trimmed_collection.empty() || trimmed_collection.size() > kMaxCollectionChars ||
        hasControlCharacters(trimmed_collection)) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: invalid collection");
        IndexResult invalid_result;
        invalid_result.error = "invalid collection";
        return invalid_result;
    }
    // MIME type validation: must be non-empty, <= 128 chars, no control chars
    if (trimmed_mime.empty() || trimmed_mime.size() > kMaxMimeChars ||
        hasControlCharacters(trimmed_mime)) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: invalid mime");
        IndexResult invalid_result;
        invalid_result.error = "invalid mime";
        return invalid_result;
    }
    // Filename validation: must be non-empty, <= 512 chars, no control chars
    // Filename used for workflow routing (e.g., PDF vs plain text handling)
    if (trimmed_filename.empty() || trimmed_filename.size() > kMaxFilenameChars ||
        hasControlCharacters(trimmed_filename)) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: invalid filename");
        IndexResult invalid_result;
        invalid_result.error = "invalid filename";
        return invalid_result;
    }

    // ── Workflow execution with fallback path ────────────────────────────────
    // Idempotent doc_id: computed from collection + text hash, deterministic.
    // Enables re-indexing the same document (will overwrite previous records).
    
    // Derive a stable document ID from collection + text
    const std::string doc_hash = computeDocHash(text);
    const std::string doc_id   = trimmed_collection + "/" + doc_hash;
    outcome.doc_id = doc_id;
    outcome.collection = trimmed_collection;

    // Build an ExtractionContext and run the workflow
    ingestion::ExtractionContext ctx;
    ctx.manifest.file_id       = doc_id;
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
        outcome.used_fallback = true;
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
    outcome.entity_count = entity_set.nodes.size();
    std::vector<std::string> write_errors;

    // ── Vector writer section (failure handling) ────────────────────────────
    // Writes all extracted chunks to the vector index (if vector_writer configured).
    // Injects canonical RAG metadata: collection, source, content
    // Failure to write is returned to the caller; independent sinks still run.
    // The final status distinguishes partial progress from complete indexing.
    
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

        write_started = true;
        auto write_result = vector_writer->writeVectors(stamped_chunks);
        if (write_result) {
            outcome.vector_count = stamped_chunks.size();
            spdlog::debug(
                "RAGIngestionBridge::indexDocument vector write ok: chunk_count={}",
                outcome.vector_count);
        } else {
            spdlog::warn(
                "RAGIngestionBridge::indexDocument vector write failed for collection='{}'",
                trimmed_collection);
            write_errors.emplace_back("vector write failed");
        }
    }

    // ── Graph writer section (optional, explicit result) ─────────────────────
    // Relations are withheld when their entity batch failed to avoid creating
    // graph edges for nodes this operation could not confirm.
    
    // Write graph entities / relations
    if (graph_writer) {
        bool entities_written = entity_set.nodes.empty();
        if (!entity_set.nodes.empty()) {
            write_started = true;
            auto write_result = graph_writer->writeEntities(entity_set.nodes);
            if (write_result) {
                outcome.graph_entity_count = entity_set.nodes.size();
                entities_written = true;
            } else {
                spdlog::warn(
                    "RAGIngestionBridge::indexDocument graph entity write failed for collection='{}'",
                    trimmed_collection);
                write_errors.emplace_back("graph entity write failed");
            }
        }
        if (!entity_set.edges.empty()) {
            if (!entities_written) {
                write_errors.emplace_back("graph relation write skipped after entity write failure");
            } else {
                write_started = true;
                auto write_result = graph_writer->writeRelations(entity_set.edges);
                if (write_result) {
                    outcome.graph_relation_count = entity_set.edges.size();
                } else {
                    spdlog::warn(
                        "RAGIngestionBridge::indexDocument graph relation write failed for collection='{}'",
                        trimmed_collection);
                    write_errors.emplace_back("graph relation write failed");
                }
            }
        }
    }

    if (write_errors.empty()) {
        outcome.status = used_workflow_fallback
            ? IndexStatus::DEGRADED
            : IndexStatus::COMPLETE;
        outcome.ok = true;
    } else {
        outcome.status = write_started
            ? IndexStatus::PARTIAL
            : IndexStatus::FAILED;
        outcome.ok = false;
        for (const auto& error : write_errors) {
            if (!outcome.error.empty()) {
                outcome.error += "; ";
            }
            outcome.error += error;
        }
    }

    spdlog::info(
        "RAGIngestionBridge::indexDocument complete: doc_id='{}' entities={} vectors={} graph_entities={} graph_relations={} status={}",
        doc_id,
        outcome.entity_count,
        outcome.vector_count,
        outcome.graph_entity_count,
        outcome.graph_relation_count,
        indexStatusName(outcome.status));

    return outcome;
    } catch (const std::exception& ex) {
        spdlog::error("RAGIngestionBridge::indexDocument exception: {}", ex.what());
        outcome.ok = false;
        outcome.status = write_started ? IndexStatus::PARTIAL : IndexStatus::FAILED;
        outcome.error = write_started
            ? std::string("indexing failed; sink write outcome may be partial: ") + ex.what()
            : ex.what();
        return outcome;
    } catch (...) {
        spdlog::error("RAGIngestionBridge::indexDocument exception: unknown failure");
        outcome.ok = false;
        outcome.status = write_started ? IndexStatus::PARTIAL : IndexStatus::FAILED;
        outcome.error = write_started
            ? "indexing failed; sink write outcome may be partial"
            : "unknown indexing failure";
        return outcome;
    }
}

/**
 * @brief Enrich Retrieved Documents.
 * @param[in,out] docs Input/output parameter.
 * @return Return value.
 */
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

/**
 * @brief Build Entity Context.
 * @param[in] entities Input parameter.
 * @return Return value.
 */
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

/**
 * @brief Compute Doc Hash.
 * @param[in] text Input parameter.
 * @return Return value.
 * @details Computes a full SHA-256 digest so document IDs remain stable across
 * processes and have substantially lower collision risk than truncated hashes.
 */
std::string RAGIngestionBridge::computeDocHash(const std::string& text) {
    const std::string digest =
        themis::core::concerns::SignedAdapterValidator::sha256Hex(text);
    if (digest.empty()) {
        throw std::runtime_error("document identity hashing failed");
    }
    return digest;
}

/**
 * @brief Entity Type Name.
 * @param[in] et Input parameter.
 * @return Return value.
 * @details Implements entityTypeName without additional internal calls.
 */
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

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── RAGIngestionBridge::indexOptimizerLog (IMPL-A2 Phase 2) ─────────────────────────────────────────────────────────────────────────────
 * @param[in] query_id Identifier of the query.
 * @param[in] plan_json Input parameter.
 * @param[in] latency_ms Input parameter.
 * @param[in] collection Input parameter.
 * @return Return value.
 */

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
