/**
 * @file scraper_metadata_writer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "scraper/scraper_llm_evaluator.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace themis {
namespace scraper {

// ============================================================================
// Record types
// ============================================================================

/**
 * @brief Relational record written for each accepted scraped document.
 *
 * PROVENANCE GUARANTEE
 * Every record written by the scraper plugin MUST carry the three fields
 * below. They allow any consumer to trace data back to automated ingestion
 * and treat it accordingly (e.g. require manual review before using in a
 * legal proceeding).
 *
 *   is_scraper_ingested   – always `true` for records produced by this plugin
 *   ingestion_source_type – "SCRAPER" (constant literal)
 *   ingestion_plugin_version – semver of the scraper plugin that produced the record
 */
struct ScraperRelationalRecord {
    std::string doc_id;         ///< FNV-1a hash of URL + content (hex, 16 chars)
    std::string url;
    std::string title;
    std::string source_name;    ///< E.g. "openjur.de", "gesetze_im_internet"
    std::string gov_source_id;  ///< GovDataSource::id (may be empty)
    std::string gap_id;
    std::string text_snippet;   ///< First 500 chars of extracted text
    double      quality_score  = 0.0;
    double      gap_relevance  = 0.0;
    std::string scraped_at;     ///< ISO-8601 timestamp
    std::string document_type;  ///< Urteil, Beschluss, Gesetz, …
    std::string date_issued;    ///< Publication/decision date from page metadata

    // ── Provenance / ingestion traceability (MANDATORY) ────────────────────
    /// Always true for records produced by this plugin.
    bool        is_scraper_ingested      = true;
    /// Fixed literal "SCRAPER" — identifies automated web-scraping as origin.
    std::string ingestion_source_type    = "SCRAPER";
    /// Semver of the scraper plugin that produced this record.
    std::string ingestion_plugin_version = "1.0.0";
};

/**
 * @brief Graph node/edge records for the property graph layer.
 */
struct ScraperGraphNode {
    std::string node_id;    ///< Same as ScraperRelationalRecord::doc_id
    std::string label;      ///< "ScrapedDocument"
    std::map<std::string, std::string> properties;
};

struct ScraperGraphEdge {
    std::string from_id;
    std::string to_id;
    std::string rel;        ///< "FILLS_GAP", "REFERENCES", "PUBLISHED_BY"
    std::map<std::string, std::string> properties;
};

/**
 * @brief Vector record for the ANN index.
 *
 * Carries the same provenance fields as ScraperRelationalRecord so the
 * vector layer can also be audited independently.
 */
struct ScraperVectorRecord {
    std::string doc_id;
    std::string gap_id;
    std::string source_url;
    double      quality_score = 0.0;
    /// Embedding vector (produced externally; empty until embedding is generated)
    std::vector<float> embedding;

    // ── Provenance (MANDATORY) ──────────────────────────────────────────────
    bool        is_scraper_ingested      = true;
    std::string ingestion_source_type    = "SCRAPER";
    std::string ingestion_plugin_version = "1.0.0";
};

// ============================================================================
// Write result
// ============================================================================

/**
 * @brief Outcome of a single IScraperMetadataWriter::write() call.
 *
 * Inspect the individual `*_written` flags to determine which storage layers
 * succeeded.  When `success == false`, `error` contains a diagnostic message.
 */
struct WriteResult {
    bool        success            = false;  ///< True when all layers wrote without error
    std::string doc_id;                      ///< doc_id of the written record (from ScraperRelationalRecord)
    std::string error;                       ///< Non-empty when success == false
    bool        relational_written = false;  ///< Relational layer write succeeded
    bool        graph_written      = false;  ///< Property-graph layer write succeeded
    bool        vector_written     = false;  ///< Vector / ANN layer write succeeded
};

// ============================================================================
// Interface
// ============================================================================

/**
 * @brief Persistence interface for scraper output records.
 *
 * A single write() call persists one document to all three storage layers
 * (relational, property graph, and vector).  Partial-write failures are
 * reported via WriteResult without aborting the overall scraper run.
 */
class IScraperMetadataWriter {
public:
    virtual ~IScraperMetadataWriter() = default;

    /**
     * @brief Persist one scraped document to all storage layers.
     *
     * Implementations must set provenance fields before writing and must
     * not modify the `rel.is_scraper_ingested`,
     * `rel.ingestion_source_type`, or `rel.ingestion_plugin_version` fields.
     *
     * @param rel    Relational record with full document metadata.
     * @param node   Property-graph node derived from the document.
     * @param edges  Property-graph edges (e.g. FILLS_GAP, PUBLISHED_BY).
     * @param vec    Vector record for ANN indexing.
     * @return WriteResult indicating per-layer success and the assigned doc_id.
     */
    virtual WriteResult write(
        const ScraperRelationalRecord& rel,
        const ScraperGraphNode&        node,
        const std::vector<ScraperGraphEdge>& edges,
        const ScraperVectorRecord&     vec) = 0;

    /**
     * @brief Flush any buffered writes to durable storage.
     * @return true when all buffered writes were persisted successfully.
     */
    virtual bool flush() = 0;
};

// ============================================================================
// In-memory implementation (tests + default)
// ============================================================================

/**
 * @brief In-memory writer that stores all records for test assertions.
 */
class InMemoryScraperMetadataWriter : public IScraperMetadataWriter {
public:
    InMemoryScraperMetadataWriter() = default;

    WriteResult write(
        const ScraperRelationalRecord& rel,
        const ScraperGraphNode&        node,
        const std::vector<ScraperGraphEdge>& edges,
        const ScraperVectorRecord&     vec) override;

    bool flush() override { flush_called_++; return true; }

    // Test inspection
    const std::vector<ScraperRelationalRecord>& relationalRecords() const {
        return relational_;
    }
    const std::vector<ScraperGraphNode>& graphNodes() const { return nodes_; }
    const std::vector<ScraperGraphEdge>& graphEdges() const { return edges_; }
    const std::vector<ScraperVectorRecord>& vectorRecords() const { return vectors_; }
    int flushCount() const { return flush_called_; }
    void clear() {
        relational_.clear(); nodes_.clear();
        edges_.clear(); vectors_.clear(); flush_called_ = 0;
    }

private:
    std::vector<ScraperRelationalRecord> relational_;
    std::vector<ScraperGraphNode>        nodes_;
    std::vector<ScraperGraphEdge>        edges_;
    std::vector<ScraperVectorRecord>     vectors_;
    int flush_called_ = 0;
};

// ============================================================================
// Factory helper
// ============================================================================

/**
 * @brief Build the relational, graph, and vector records from a document.
 *
 * All builder methods unconditionally set the three provenance fields
 * (`is_scraper_ingested`, `ingestion_source_type`, `ingestion_plugin_version`)
 * on every record they produce.  These fields MUST NOT be cleared or overridden
 * by the caller — they are the authoritative ingestion trail.
 */
struct ScraperRecordBuilder {
    /// Semver injected at plugin initialisation; default "1.0.0".
    static const char* kPluginVersion;

    static ScraperRelationalRecord buildRelational(
        const std::string& url,
        const std::string& title,
        const std::string& text,
        const std::string& source_name,
        const std::string& gov_source_id,
        const EvaluationResult& eval,
        const GapContext& gap,
        const std::string& plugin_version = "");

    static ScraperGraphNode buildNode(const ScraperRelationalRecord& rel);

    static std::vector<ScraperGraphEdge> buildEdges(
        const ScraperRelationalRecord& rel,
        const EvaluationResult& eval);

    static ScraperVectorRecord buildVector(const ScraperRelationalRecord& rel);

private:
    static std::string computeDocId(const std::string& url,
                                    const std::string& text);
    static std::string currentIso8601();
};

} // namespace scraper
} // namespace themis
