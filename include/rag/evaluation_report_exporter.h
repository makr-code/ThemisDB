/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            evaluation_report_exporter.h                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:08:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     191                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file evaluation_report_exporter.h
 * @brief Per-query evaluation report export (JSON / HTML)
 *
 * Serialises a single RAG evaluation (@c EvaluationInput + @c EvaluationResult)
 * to a self-contained JSON document or an HTML report page suitable for
 * human review.
 *
 * JSON schema (top-level keys):
 *  - "report_id"    : string – caller-supplied identifier (may be empty)
 *  - "timestamp_ms" : int    – Unix epoch milliseconds at export time
 *  - "query"        : string
 *  - "generated_answer" : string
 *  - "documents"    : array of { "id", "content", "similarity_score" }
 *  - "metadata"     : object of string key/value pairs
 *  - "scores"       : { "faithfulness", "relevance", "completeness",
 *                       "coherence", "ethical_compliance", "overall" }
 *  - "quality"      : { "passed_threshold", "confidence", "evaluation_time_ms",
 *                       "judge_model" }
 *  - "verified_claims"   : array of strings
 *  - "unverified_claims" : array of strings
 *  - "improvements"      : array of strings
 *  - "ethical"      : { "violations", "respects_human_autonomy",
 *                       "shows_moral_diversity", "has_ethical_citations" }
 *  - "explanation"  : string
 *
 * HTML output is a single self-contained page with inline CSS that renders
 * score bars and annotated claim lists.
 *
 * Integration:
 * @code
 *   #include "rag/evaluation_report_exporter.h"
 *   using namespace themis::rag::judge;
 *
 *   EvaluationReportExporter exporter;
 *
 *   EvaluationReportExporter::PerQueryReport report;
 *   report.input  = my_input;
 *   report.result = my_result;
 *   report.report_id = "run-42";
 *
 *   std::string json = exporter.toJSON(report);
 *   std::string html = exporter.toHTML(report);
 *
 *   exporter.exportJSON(report, "/tmp/report.json");
 *   exporter.exportHTML(report, "/tmp/report.html");
 * @endcode
 */

#pragma once

#include "rag/rag_judge.h"

#include <string>
#include <memory>

namespace themis::rag::judge {

// ---------------------------------------------------------------------------
// Supporting types
// ---------------------------------------------------------------------------

/**
 * @brief Bundles the input and result of a single RAG evaluation for export
 */
struct PerQueryReport {
    /// Evaluation input (query, documents, generated answer, metadata)
    EvaluationInput input;
    /// Evaluation result with all dimension scores and analysis
    EvaluationResult result;
    /// Optional caller-supplied identifier (e.g. "run-42", UUID, query hash)
    std::string report_id;
};

// ---------------------------------------------------------------------------
// Exporter class
// ---------------------------------------------------------------------------

/**
 * @brief Exports per-query RAG evaluation reports as JSON or HTML
 *
 * Thread-safe: all public methods are const and produce no shared-state side
 * effects.  The exporter instance itself holds no mutable state.
 */
class EvaluationReportExporter {
public:
    EvaluationReportExporter() = default;
    ~EvaluationReportExporter() = default;

    // Non-copyable, non-movable (stateless – just construct a new one)
    EvaluationReportExporter(const EvaluationReportExporter&)            = delete;
    EvaluationReportExporter& operator=(const EvaluationReportExporter&) = delete;

    /**
     * @brief Serialise @p report to a JSON string
     *
     * The output is a compact (no trailing whitespace) UTF-8 JSON document.
     * Special characters inside string values are escaped according to
     * RFC 8259.
     *
     * @param report  Evaluation input + result to serialise
     * @return        JSON string; never empty on success
     */
    std::string toJSON(const PerQueryReport& report) const;

    /**
     * @brief Serialise @p report to an HTML string
     *
     * The output is a self-contained HTML5 document with inline CSS and no
     * external dependencies.  Score bars are rendered as styled @c <div>
     * elements.
     *
     * @param report  Evaluation input + result to render
     * @return        HTML string; never empty on success
     */
    std::string toHTML(const PerQueryReport& report) const;

    /**
     * @brief Write the JSON representation of @p report to a file
     *
     * Creates or overwrites the file at @p filepath.
     *
     * @param report    Evaluation to export
     * @param filepath  Destination path (must be writable)
     * @return true on success, false if the file could not be opened/written
     */
    bool exportJSON(const PerQueryReport& report,
                    const std::string& filepath) const;

    /**
     * @brief Write the HTML representation of @p report to a file
     *
     * Creates or overwrites the file at @p filepath.
     *
     * @param report    Evaluation to export
     * @param filepath  Destination path (must be writable)
     * @return true on success, false if the file could not be opened/written
     */
    bool exportHTML(const PerQueryReport& report,
                    const std::string& filepath) const;

private:
    /// Escape a raw string value for safe embedding inside a JSON string literal
    static std::string escapeJSON(const std::string& s);
    /// Escape a raw string for safe embedding inside HTML text / attribute values
    static std::string escapeHTML(const std::string& s);
    /// Render a single score as an HTML progress-bar row
    static std::string scoreBarHTML(const std::string& label,
                                    double score,
                                    bool is_critical = false);
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Factory helpers for creating EvaluationReportExporter instances
 */
class EvaluationReportExporterFactory {
public:
    /**
     * @brief Create a default exporter
     */
    static std::unique_ptr<EvaluationReportExporter> create();
};

} // namespace themis::rag::judge
