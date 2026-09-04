/**
 * @file scraper_metadata_writer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scraper/scraper_metadata_writer.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <functional>
#include <algorithm>

namespace themis {
namespace scraper {

// ============================================================================
// Plugin version constant
// ============================================================================

/*static*/ const char* ScraperRecordBuilder::kPluginVersion = "1.0.0";

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// FNV-1a 64-bit hash for quick doc-id generation (no libssl needed).
uint64_t fnv1a64(const std::string& s) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string toHex16([[maybe_unused]] uint64_t v) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << v;
    return ss.str();
}

std::string iso8601Now() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

} // anonymous namespace

// ============================================================================
// ScraperRecordBuilder
// ============================================================================

/*static*/ std::string ScraperRecordBuilder::computeDocId(
        const std::string& url,
        const std::string& text) {
    const std::string combined = url + "|" + text.substr(0, 512);
    return toHex16(fnv1a64(combined));
}

/*static*/ std::string ScraperRecordBuilder::currentIso8601() {
    return iso8601Now();
}

/*static*/ ScraperRelationalRecord ScraperRecordBuilder::buildRelational(
        const std::string& url,
        const std::string& title,
        const std::string& text,
        const std::string& source_name,
        const std::string& gov_source_id,
        const EvaluationResult& eval,
        const GapContext& gap,
        const std::string& plugin_version) {
    ScraperRelationalRecord r;
    r.doc_id       = computeDocId(url, text);
    r.url          = url;
    r.title        = title;
    r.source_name  = source_name;
    r.gov_source_id = gov_source_id;
    r.gap_id       = gap.gap_id;
    r.text_snippet = (text.size() > 500) ? text.substr(0, 500) + "…" : text;
    r.quality_score = eval.quality_score;
    r.gap_relevance = eval.gap_relevance;
    r.scraped_at   = iso8601Now();

    // ── Provenance stamp (MANDATORY – must never be skipped) ─────────────
    r.is_scraper_ingested      = true;
    r.ingestion_source_type    = "SCRAPER";
    r.ingestion_plugin_version = plugin_version.empty() ? kPluginVersion
                                                        : plugin_version;
    return r;
}

/*static*/ ScraperGraphNode ScraperRecordBuilder::buildNode(
        const ScraperRelationalRecord& rel) {
    ScraperGraphNode node;
    node.node_id = rel.doc_id;
    node.label   = "ScrapedDocument";
    node.properties = {
        {"url",                       rel.url},
        {"title",                     rel.title},
        {"source",                    rel.source_name},
        {"gov_source_id",             rel.gov_source_id},
        {"gap_id",                    rel.gap_id},
        {"quality_score",             std::to_string(rel.quality_score)},
        {"gap_relevance",             std::to_string(rel.gap_relevance)},
        {"scraped_at",                rel.scraped_at},
        {"document_type",             rel.document_type},
        {"date_issued",               rel.date_issued},
        // Provenance properties — copied from the relational record so that
        // graph queries can filter by ingestion origin without a join.
        {"is_scraper_ingested",       rel.is_scraper_ingested ? "true" : "false"},
        {"ingestion_source_type",     rel.ingestion_source_type},
        {"ingestion_plugin_version",  rel.ingestion_plugin_version},
    };
    return node;
}

/*static*/ std::vector<ScraperGraphEdge> ScraperRecordBuilder::buildEdges(
        const ScraperRelationalRecord& rel,
        const EvaluationResult& eval) {
    std::vector<ScraperGraphEdge> edges;

    // Doc → Gap relationship
    if (!rel.gap_id.empty()) {
        ScraperGraphEdge e;
        e.from_id = rel.doc_id;
        e.to_id   = "GAP:" + rel.gap_id;
        e.rel     = "FILLS_GAP";
        e.properties = {{"relevance", std::to_string(rel.gap_relevance)}};
        edges.push_back(std::move(e));
    }

    // Doc → Source
    if (!rel.gov_source_id.empty()) {
        ScraperGraphEdge e;
        e.from_id = rel.doc_id;
        e.to_id   = "SOURCE:" + rel.gov_source_id;
        e.rel     = "PUBLISHED_BY";
        edges.push_back(std::move(e));
    }

    // Entity nodes from LLM
    for (const auto& entity : eval.key_entities) {
        if (entity.empty()) continue;
        ScraperGraphEdge e;
        e.from_id = rel.doc_id;
        e.to_id   = "ENTITY:" + entity;
        e.rel     = "REFERENCES";
        e.properties = {{"entity", entity}};
        edges.push_back(std::move(e));
    }

    return edges;
}

/*static*/ ScraperVectorRecord ScraperRecordBuilder::buildVector(
        const ScraperRelationalRecord& rel) {
    ScraperVectorRecord v;
    v.doc_id        = rel.doc_id;
    v.gap_id        = rel.gap_id;
    v.source_url    = rel.url;
    v.quality_score = rel.quality_score;

    // Provenance stamp — mirrors the relational record
    v.is_scraper_ingested      = true;
    v.ingestion_source_type    = rel.ingestion_source_type;
    v.ingestion_plugin_version = rel.ingestion_plugin_version;

    // Embedding is populated externally by an embedding model
    return v;
}

// ============================================================================
// InMemoryScraperMetadataWriter
// ============================================================================

WriteResult InMemoryScraperMetadataWriter::write(
        const ScraperRelationalRecord& rel,
        const ScraperGraphNode&        node,
        const std::vector<ScraperGraphEdge>& edges,
        const ScraperVectorRecord&     vec) {
    relational_.push_back(rel);
    nodes_.push_back(node);
    edges_.insert(edges_.end(), edges.begin(), edges.end());
    vectors_.push_back(vec);

    WriteResult r;
    r.success           = true;
    r.doc_id            = rel.doc_id;
    r.relational_written = true;
    r.graph_written     = true;
    r.vector_written    = true;
    return r;
}

} // namespace scraper
} // namespace themis
