/*
 * ThemisDB | File: rag_ingestion_bridge.cpp | Version: 0.1.0 | Last Modified: 2026-05-22 11:24:56
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 268
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=0, L=0
 * PR History (last 5): #4697 feat(rag,toolbox): RAGInges... (2026-04-16)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "rag/rag_ingestion_bridge.h"

#include "ingestion/workflow_engine.h"
#include "ingestion/extraction_context.h"

#include <spdlog/spdlog.h>

#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <functional>  // std::hash

namespace themis {
namespace rag {

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

RAGIngestionBridge::~RAGIngestionBridge() = default;

RAGIngestionBridge::RAGIngestionBridge(RAGIngestionBridge&&) noexcept = default;
RAGIngestionBridge& RAGIngestionBridge::operator=(RAGIngestionBridge&&) noexcept = default;

// ─────────────────────────────────────────────────────────────────────────────
// Core operations
// ─────────────────────────────────────────────────────────────────────────────

IndexResult RAGIngestionBridge::indexDocument(
    const std::string& text,
    const std::string& collection,
    const std::string& mime,
    const std::string& filename)
{
    spdlog::info(
        "RAGIngestionBridge::indexDocument start: collection='{}' mime='{}' filename='{}' text_chars={}",
        collection,
        mime,
        filename,
        text.size());

    if (text.empty()) {
        spdlog::warn("RAGIngestionBridge::indexDocument rejected: empty input");
        return IndexResult{
            .ok    = false,
            .error = "empty input"
        };
    }

    // Derive a stable document ID from collection + text
    const std::string doc_hash = computeDocHash(text);
    const std::string doc_id   = collection + "/" + doc_hash;

    // Build an ExtractionContext and run the workflow
    ingestion::ExtractionContext ctx;
    ctx.manifest.detected_mime  = mime;
    ctx.manifest.filename_stem  = filename;
    ctx.manifest.extension      = "";
    ctx.raw_text                = text;

    ingestion::BaseEntitySet entity_set;
    bool used_workflow_fallback = false;

    if (auto engine = toolbox_->workflowEngine()) {
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

    if (used_workflow_fallback) {
        spdlog::info(
            "RAGIngestionBridge::indexDocument using fallback workflow path for collection='{}'",
            collection);
        entity_set.source_file_id = doc_id;
        entity_set.quality_score = 0.0;

        auto entities = toolbox_->extractEntities(text);
        for (auto& entity : entities) {
            if (entity.source_file_id.empty()) {
                entity.source_file_id = doc_id;
            }
        }
        entity_set.nodes = std::move(entities);

        ingestion::VectorRecord fallback_chunk;
        fallback_chunk.chunk_id = doc_id + "#0";
        fallback_chunk.source_file_id = doc_id;
        fallback_chunk.text_snippet = text;
        fallback_chunk.metadata["collection"] = collection;
        fallback_chunk.metadata["source"] = doc_id;
        entity_set.chunks.push_back(std::move(fallback_chunk));
    }

    std::size_t vector_count = 0;
    std::size_t entity_count = entity_set.nodes.size();

    // Write vector chunks (stamp each chunk's source_file_id with our doc_id)
    if (vector_writer_ && !entity_set.chunks.empty()) {
        std::vector<ingestion::VectorRecord> stamped_chunks = entity_set.chunks;
        for (auto& chunk : stamped_chunks) {
            if (chunk.source_file_id.empty()) {
                chunk.source_file_id = doc_id;
            }
            // Inject collection into metadata for downstream retrieval routing
            chunk.metadata["collection"] = collection;
        }

        auto write_result = vector_writer_->writeVectors(stamped_chunks);
        if (write_result) {
            vector_count = stamped_chunks.size();
            spdlog::debug(
                "RAGIngestionBridge::indexDocument vector write ok: chunk_count={}",
                vector_count);
        } else {
            spdlog::warn(
                "RAGIngestionBridge::indexDocument vector write failed for collection='{}'",
                collection);
        }
        // Write failure is non-fatal: we still return a partial result
    }

    // Write graph entities / relations
    if (graph_writer_) {
        if (!entity_set.nodes.empty()) {
            static_cast<void>(graph_writer_->writeEntities(entity_set.nodes));
        }
        if (!entity_set.edges.empty()) {
            static_cast<void>(graph_writer_->writeRelations(entity_set.edges));
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
        .collection   = collection,
        .entity_count = entity_count,
        .vector_count = vector_count
    };
}

std::size_t RAGIngestionBridge::enrichRetrievedDocuments(
    std::vector<judge::RetrievedDocument>& docs)
{
    spdlog::info(
        "RAGIngestionBridge::enrichRetrievedDocuments start: docs={}",
        docs.size());

    std::size_t enriched = 0;
    for (auto& doc : docs) {
        if (doc.content.empty() || doc.id.empty()) {
            continue;
        }
        if (doc.metadata["content"].empty()) {
            doc.metadata["content"] = doc.content;
        }
        if (doc.metadata["source"].empty()) {
            doc.metadata["source"] = doc.id;
        }
        auto entities = toolbox_->extractEntities(doc.content);
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
}

std::vector<ingestion::BaseEntity>
RAGIngestionBridge::extractEntitiesForContext(const std::string& text) {
    return toolbox_->extractEntities(text);
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

    std::ostringstream oss;
    oss << "Extracted entities:";
    bool first = true;
    for (const auto& e : entities) {
        if (!first) {
            oss << " |";
        }
        oss << " " << entityTypeName(e.entity_type);
        if (!e.id.empty()) {
            oss << " " << e.id;
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
    std::ostringstream oss;
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

    std::ostringstream doc;
    doc << "optimizer_log\n"
        << "query_id: " << query_id << "\n"
        << "latency_ms: " << latency_ms << "\n"
        << "plan: " << plan_snippet << "\n";

    const std::string filename = "optimizer-log-" + query_id + ".txt";
    return indexDocument(doc.str(), collection, "text/plain", filename);
}

} // namespace rag
} // namespace themis
