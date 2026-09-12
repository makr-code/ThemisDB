/**
 * @file wiki_rag_source.cpp
 * @brief WikiRagSource — RAG stage handler backed by the wiki chunk index.
 *
 * Converts `WikiChunk` query results into `RAGCandidate` objects and wraps
 * them in a `StageResult` compatible with `ModularRAGPipeline`.
 *
 * Error handling follows the `fail_open` contract: exceptions are caught and
 * surfaced as `StageStatus::Skipped` (fail-open) or `StageStatus::Error`
 * (fail-closed).
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "llm/wiki_rag_source.h"

#include <chrono>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

WikiRagSource::WikiRagSource(IWikiIndexReader&   reader,
                             WikiRagSourceConfig config)
    : reader_(reader)
    , config_(std::move(config))
{}

// ─────────────────────────────────────────────────────────────────────────────
// Provenance tag determination
// ─────────────────────────────────────────────────────────────────────────────

std::string WikiRagSource::provenanceTag() const {
    // Detect reader type at runtime via dynamic_cast for precise tagging.
    if (dynamic_cast<const WikiIndexStore*>(&reader_) != nullptr) {
        // Full hybrid store: uses both BM25 and vector
        return "retrieve:wiki-hybrid";
    }
    if (dynamic_cast<const JsonWikiIndexReader*>(&reader_) != nullptr) {
        return "retrieve:wiki-json";
    }
    // Unknown implementation — tag generically
    return "retrieve:wiki";
}

// ─────────────────────────────────────────────────────────────────────────────
// retrieveFromWiki
// ─────────────────────────────────────────────────────────────────────────────

rag::StageResult WikiRagSource::retrieveFromWiki(rag::ModularRAGContext& ctx) {
    const auto t0 = std::chrono::steady_clock::now();

    rag::StageResult result;
    result.stage = rag::RAGStageId::Retrieve;

    if (!reader_.isReady()) {
        result.status     = config_.fail_open ? rag::StageStatus::Skipped
                                              : rag::StageStatus::Error;
        result.diagnostic = "[WikiRagSource] index reader is not ready";
        spdlog::warn(result.diagnostic);
        result.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0);
        return result;
    }

    try {
        const std::string& prov_tag = provenanceTag();

        std::vector<WikiChunk> chunks =
            reader_.query(ctx.query, config_.top_k, config_.min_score);

        result.candidates.reserve(chunks.size());
        for (const auto& chunk : chunks) {
            rag::RAGCandidate cand;
            cand.doc_id           = chunk.chunk_id;
            cand.content          = chunk.text;
            cand.score            = chunk.score;
            cand.source_namespace = config_.source_namespace;
            cand.provenance_tags.push_back(prov_tag);
            // Additional provenance: source file and section
            if (!chunk.source_path.empty()) {
                cand.provenance_tags.push_back("source:" + chunk.source_path);
            }
            if (!chunk.section_title.empty()) {
                cand.provenance_tags.push_back("section:" + chunk.section_title);
            }
            result.candidates.push_back(std::move(cand));
        }

        result.status = rag::StageStatus::Success;
        spdlog::debug("[WikiRagSource] retrieved {} candidates for query '{}'",
                      result.candidates.size(),
                      ctx.query.size() > 80 ? ctx.query.substr(0, 80) + "..." : ctx.query);

    } catch (const std::exception& ex) {
        result.diagnostic = std::string("[WikiRagSource] query failed: ") + ex.what();
        spdlog::error(result.diagnostic);
        result.status = config_.fail_open ? rag::StageStatus::Skipped
                                          : rag::StageStatus::Error;
    } catch (...) {
        result.diagnostic = "[WikiRagSource] query failed: unknown exception";
        spdlog::error(result.diagnostic);
        result.status = config_.fail_open ? rag::StageStatus::Skipped
                                          : rag::StageStatus::Error;
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0);
    return result;
}

} // namespace llm
} // namespace themis
