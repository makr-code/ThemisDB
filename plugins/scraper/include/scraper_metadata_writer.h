#pragma once

#include "scraper_llm_evaluator.h"
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
 */
struct ScraperRelationalRecord {
    std::string doc_id;         ///< SHA-256 of URL + content hash (hex, 16 chars)
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
 */
struct ScraperVectorRecord {
    std::string doc_id;
    std::string gap_id;
    std::string source_url;
    double      quality_score = 0.0;
    /// Embedding vector (produced externally; empty until embedding is generated)
    std::vector<float> embedding;
};

// ============================================================================
// Write result
// ============================================================================

struct WriteResult {
    bool        success     = false;
    std::string doc_id;
    std::string error;
    bool        relational_written = false;
    bool        graph_written      = false;
    bool        vector_written     = false;
};

// ============================================================================
// Interface
// ============================================================================

class IScraperMetadataWriter {
public:
    virtual ~IScraperMetadataWriter() = default;

    virtual WriteResult write(
        const ScraperRelationalRecord& rel,
        const ScraperGraphNode&        node,
        const std::vector<ScraperGraphEdge>& edges,
        const ScraperVectorRecord&     vec) = 0;

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
 */
struct ScraperRecordBuilder {
    static ScraperRelationalRecord buildRelational(
        const std::string& url,
        const std::string& title,
        const std::string& text,
        const std::string& source_name,
        const std::string& gov_source_id,
        const EvaluationResult& eval,
        const GapContext& gap);

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
