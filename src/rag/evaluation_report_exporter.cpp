/**
 * @file evaluation_report_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=39, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/evaluation_report_exporter.h"
#include "utils/logger.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace themis::rag::judge {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string EvaluationReportExporter::escapeJSON(const std::string& s) {
    // Optimize: Use std::string with proper reserve to avoid reallocation
    // Worst-case: each character becomes \uXXXX (6 chars), but typically 1-2
    // Reserve to account for escaped sequences without runtime reallocation
    // Complexity: O(n) linear time, minimal allocations
    std::string out = {};
    // Reserve conservative estimate: assume average 30% growth for escaping
    out.reserve(s.size() + (s.size() / 3));
    
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out.append("\\\""); break;
            case '\\': out.append("\\\\"); break;
            case '\b': out.append("\\b");  break;
            case '\f': out.append("\\f");  break;
            case '\n': out.append("\\n");  break;
            case '\r': out.append("\\r");  break;
            case '\t': out.append("\\t");  break;
            default:
                if (c < 0x20) {
                    // control character → \uXXXX (6 chars)
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned>(c));
                    out.append(buf);
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
        }
    }
    return out;
}

std::string EvaluationReportExporter::escapeHTML(const std::string& s) {
    // Optimize: Use std::string with proper reserve to avoid reallocation
    // Common HTML entities: & (5 chars), <, >, ", ' (each 4-6 chars)
    // Reserve conservative estimate: assume average 40% growth for escaping
    // Complexity: O(n) linear time, minimal allocations
    std::string out = {};
    out.reserve(s.size() + (s.size() / 2));
    
    for (unsigned char c : s) {
        switch (c) {
            case '&':  out.append("&amp;");  break;
            case '<':  out.append("&lt;");   break;
            case '>':  out.append("&gt;");   break;
            case '"':  out.append("&quot;"); break;
            case '\'': out.append("&#39;");  break;
            default:
                out.push_back(static_cast<char>(c));
                break;
        }
    }
    return out;
}

std::string EvaluationReportExporter::scoreBarHTML(const std::string& label,
                                                   double score,
                                                   bool is_critical) {
    // Clamp to [0, 1]
    double clamped = std::max(0.0, std::min(1.0, score));
    int pct        = static_cast<int>(std::round(clamped * 100.0));

    // Choose colour: red for critical + low score, yellow for mid, green for good
    const char* colour = "#4caf50"; // green
    if (is_critical && clamped < 0.8) {
        colour = "#f44336"; // red
    } else if (clamped < 0.5) {
        colour = "#f44336"; // red
    } else if (clamped < 0.7) {
        colour = "#ff9800"; // orange
    }

    std::ostringstream os = {};
    os << "<div class=\"score-row\">"
       << "<span class=\"score-label\">" << escapeHTML(label) << "</span>"
       << "<div class=\"score-bar-bg\">"
       << "<div class=\"score-bar-fill\" style=\"width:" << pct
       << "%;background:" << colour << ";\"></div>"
       << "</div>"
       << "<span class=\"score-value\">" << std::fixed << std::setprecision(3)
       << clamped << "</span>"
       << "</div>\n";
    return os.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON export
// ─────────────────────────────────────────────────────────────────────────────

std::string EvaluationReportExporter::toJSON(const PerQueryReport& report) const {
    const EvaluationInput&  inp = report.input;
    const EvaluationResult& res = report.result;

    // Unix epoch milliseconds
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    std::ostringstream os = {};
    os << std::fixed << std::setprecision(6);

    os << "{\n";
    os << "  \"report_id\": \""   << escapeJSON(report.report_id) << "\",\n";
    os << "  \"timestamp_ms\": "  << now_ms << ",\n";
    os << "  \"query\": \""        << escapeJSON(inp.query) << "\",\n";
    os << "  \"generated_answer\": \"" << escapeJSON(inp.generated_answer) << "\",\n";

    // documents array
    os << "  \"documents\": [\n";
    for (size_t i = 0; i < inp.documents.size(); ++i) {
        const auto& doc = inp.documents[i];
        os << "    {"
           << "\"id\": \""       << escapeJSON(doc.id)      << "\","
           << "\"similarity_score\": " << doc.similarity_score << ","
           << "\"content\": \""  << escapeJSON(doc.content) << "\""
           << "}";
        if (i + 1 < inp.documents.size()) {
          os << ",";
        }
        os << "\n";
    }
    os << "  ],\n";

    // metadata object
    os << "  \"metadata\": {\n";
    {
        size_t idx = 0;
        for (const auto& kv : inp.metadata) {
            os << "    \"" << escapeJSON(kv.first)
               << "\": \""  << escapeJSON(kv.second) << "\"";
            if (++idx < inp.metadata.size()) {
              os << ",";
            }
            os << "\n";
        }
    }
    os << "  },\n";

    // scores
    os << "  \"scores\": {\n";
    os << "    \"faithfulness\": "        << res.faithfulness_score        << ",\n";
    os << "    \"relevance\": "           << res.relevance_score           << ",\n";
    os << "    \"completeness\": "        << res.completeness_score        << ",\n";
    os << "    \"coherence\": "           << res.coherence_score           << ",\n";
    os << "    \"ethical_compliance\": "  << res.ethical_compliance_score  << ",\n";
    os << "    \"overall\": "             << res.overall_score             << "\n";
    os << "  },\n";

    // quality metadata
    os << "  \"quality\": {\n";
    os << "    \"passed_threshold\": "   << (res.passed_quality_threshold ? "true" : "false") << ",\n";
    os << "    \"confidence\": "         << res.confidence << ",\n";
    os << "    \"evaluation_time_ms\": " << res.evaluation_time.count() << ",\n";
    os << "    \"judge_model\": \""      << escapeJSON(res.judge_model) << "\"\n";
    os << "  },\n";

    // verified claims
    os << "  \"verified_claims\": [\n";
    for (size_t i = 0; i < res.verified_claims.size(); ++i) {
        os << "    \"" << escapeJSON(res.verified_claims[i]) << "\"";
        if (i + 1 < res.verified_claims.size()) {
          os << ",";
        }
        os << "\n";
    }
    os << "  ],\n";

    // unverified claims
    os << "  \"unverified_claims\": [\n";
    for (size_t i = 0; i < res.unverified_claims.size(); ++i) {
        os << "    \"" << escapeJSON(res.unverified_claims[i]) << "\"";
        if (i + 1 < res.unverified_claims.size()) {
          os << ",";
        }
        os << "\n";
    }
    os << "  ],\n";

    // improvements
    os << "  \"improvements\": [\n";
    for (size_t i = 0; i < res.improvements.size(); ++i) {
        os << "    \"" << escapeJSON(res.improvements[i]) << "\"";
        if (i + 1 < res.improvements.size()) {
          os << ",";
        }
        os << "\n";
    }
    os << "  ],\n";

    // ethical
    os << "  \"ethical\": {\n";
    os << "    \"violations\": [\n";
    for (size_t i = 0; i < res.ethical_violations.size(); ++i) {
        os << "      \"" << escapeJSON(res.ethical_violations[i]) << "\"";
        if (i + 1 < res.ethical_violations.size()) {
          os << ",";
        }
        os << "\n";
    }
    os << "    ],\n";
    os << "    \"respects_human_autonomy\": " << (res.respects_human_autonomy ? "true" : "false") << ",\n";
    os << "    \"shows_moral_diversity\": "   << (res.shows_moral_diversity   ? "true" : "false") << ",\n";
    os << "    \"has_ethical_citations\": "   << (res.has_ethical_citations   ? "true" : "false") << "\n";
    os << "  },\n";

    // explanation (last – no trailing comma)
    os << "  \"explanation\": \"" << escapeJSON(res.explanation) << "\"\n";
    os << "}\n";

    return os.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// HTML export
// ─────────────────────────────────────────────────────────────────────────────

std::string EvaluationReportExporter::toHTML(const PerQueryReport& report) const {
    const EvaluationInput&  inp = report.input;
    const EvaluationResult& res = report.result;

    const char* pass_colour  = res.passed_quality_threshold ? "#4caf50" : "#f44336";
    const char* pass_label   = res.passed_quality_threshold ? "PASSED"  : "FAILED";

    std::ostringstream os = {};

    // ── head ──────────────────────────────────────────────────────────────
    os << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
       << "<meta charset=\"UTF-8\">\n"
       << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
       << "<title>RAG Evaluation Report";
    if (!report.report_id.empty())
        os << " – " << escapeHTML(report.report_id);
    os << "</title>\n"
       << "<style>\n"
       << "body{font-family:Arial,sans-serif;margin:24px;color:#333;background:#f9f9f9}\n"
       << "h1{color:#1565c0}\n"
       << "h2{color:#1976d2;border-bottom:1px solid #ccc;padding-bottom:4px}\n"
       << ".card{background:#fff;border:1px solid #ddd;border-radius:6px;"
          "padding:16px;margin-bottom:16px;box-shadow:0 1px 3px rgba(0,0,0,.1)}\n"
       << ".badge{display:inline-block;padding:4px 12px;border-radius:12px;"
          "color:#fff;font-weight:bold;font-size:0.9em}\n"
       << ".score-row{display:flex;align-items:center;margin:6px 0}\n"
       << ".score-label{width:200px;font-size:0.9em}\n"
       << ".score-bar-bg{flex:1;height:14px;background:#e0e0e0;border-radius:7px;overflow:hidden;margin:0 8px}\n"
       << ".score-bar-fill{height:100%;border-radius:7px;transition:width .3s}\n"
       << ".score-value{width:56px;text-align:right;font-size:0.85em;font-family:monospace}\n"
       << "ul.claims li.verified{color:#2e7d32}\n"
       << "ul.claims li.unverified{color:#c62828}\n"
       << ".meta{color:#777;font-size:0.82em}\n"
       << ".doc-block{background:#f5f5f5;border-left:3px solid #90caf9;"
          "padding:8px 12px;margin:4px 0;font-size:0.85em}\n"
       << ".explanation{white-space:pre-wrap;font-family:monospace;font-size:0.84em;"
          "background:#fafafa;padding:10px;border:1px solid #ddd;border-radius:4px}\n"
       << "</style>\n"
       << "</head>\n<body>\n";

    // ── header ────────────────────────────────────────────────────────────
    os << "<h1>RAG Evaluation Report";
    if (!report.report_id.empty())
        os << " <span class=\"meta\">(" << escapeHTML(report.report_id) << ")</span>";
    os << "</h1>\n";

    os << "<p class=\"meta\">Judge model: <strong>" << escapeHTML(res.judge_model)
       << "</strong> &nbsp;|&nbsp; Evaluation time: <strong>"
       << res.evaluation_time.count() << " ms</strong>"
       << " &nbsp;|&nbsp; Confidence: <strong>"
       << std::fixed << std::setprecision(3) << res.confidence << "</strong></p>\n";

    // quality badge
    os << "<p><span class=\"badge\" style=\"background:" << pass_colour << "\">"
       << pass_label << "</span></p>\n";

    // ── query & answer ────────────────────────────────────────────────────
    os << "<div class=\"card\">\n"
       << "<h2>Query</h2>\n"
       << "<p>" << escapeHTML(inp.query) << "</p>\n"
       << "<h2>Generated Answer</h2>\n"
       << "<p>" << escapeHTML(inp.generated_answer) << "</p>\n"
       << "</div>\n";

    // ── scores ────────────────────────────────────────────────────────────
    os << "<div class=\"card\">\n<h2>Dimension Scores</h2>\n";
    os << scoreBarHTML("Faithfulness",       res.faithfulness_score,       /*critical=*/true);
    os << scoreBarHTML("Relevance",          res.relevance_score);
    os << scoreBarHTML("Completeness",       res.completeness_score);
    os << scoreBarHTML("Coherence",          res.coherence_score);
    os << scoreBarHTML("Ethical Compliance", res.ethical_compliance_score);
    os << "<hr style=\"margin:10px 0\">\n";
    os << scoreBarHTML("Overall",            res.overall_score,            /*critical=*/true);
    os << "</div>\n";

    // ── claims ────────────────────────────────────────────────────────────
    os << "<div class=\"card\">\n<h2>Claims</h2>\n";

    if (!res.verified_claims.empty()) {
        os << "<p><strong>✓ Verified Claims</strong></p>\n<ul class=\"claims\">\n";
        for (const auto& c : res.verified_claims)
            os << "<li class=\"verified\">" << escapeHTML(c) << "</li>\n";
        os << "</ul>\n";
    }

    if (!res.unverified_claims.empty()) {
        os << "<p><strong>✗ Unverified Claims</strong></p>\n<ul class=\"claims\">\n";
        for (const auto& c : res.unverified_claims)
            os << "<li class=\"unverified\">" << escapeHTML(c) << "</li>\n";
        os << "</ul>\n";
    }

    if (res.verified_claims.empty() && res.unverified_claims.empty())
        os << "<p class=\"meta\">No claims extracted.</p>\n";

    os << "</div>\n";

    // ── ethical compliance ────────────────────────────────────────────────
    os << "<div class=\"card\">\n<h2>Ethical Compliance</h2>\n"
       << "<p>Respects human autonomy: <strong>"
       << (res.respects_human_autonomy ? "Yes" : "No") << "</strong></p>\n"
       << "<p>Shows moral diversity: <strong>"
       << (res.shows_moral_diversity ? "Yes" : "No") << "</strong></p>\n"
       << "<p>Has ethical citations: <strong>"
       << (res.has_ethical_citations ? "Yes" : "No") << "</strong></p>\n";

    if (!res.ethical_violations.empty()) {
        os << "<p><strong>Ethical Violations:</strong></p>\n<ul>\n";
        for (const auto& v : res.ethical_violations)
            os << "<li style=\"color:#c62828\">" << escapeHTML(v) << "</li>\n";
        os << "</ul>\n";
    }
    os << "</div>\n";

    // ── improvements ─────────────────────────────────────────────────────
    if (!res.improvements.empty()) {
        os << "<div class=\"card\">\n<h2>Suggested Improvements</h2>\n<ul>\n";
        for (const auto& imp : res.improvements)
            os << "<li>" << escapeHTML(imp) << "</li>\n";
        os << "</ul>\n</div>\n";
    }

    // ── explanation ───────────────────────────────────────────────────────
    if (!res.explanation.empty()) {
        os << "<div class=\"card\">\n<h2>Explanation</h2>\n"
           << "<div class=\"explanation\">" << escapeHTML(res.explanation)
           << "</div>\n</div>\n";
    }

    // ── retrieved documents ───────────────────────────────────────────────
    if (!inp.documents.empty()) {
        os << "<div class=\"card\">\n<h2>Retrieved Documents ("
           << inp.documents.size() << ")</h2>\n";
        for (const auto& doc : inp.documents) {
            os << "<div class=\"doc-block\">"
               << "<strong>" << escapeHTML(doc.id) << "</strong>"
               << " <span class=\"meta\">(similarity: "
               << std::fixed << std::setprecision(4) << doc.similarity_score
               << ")</span><br>"
               << escapeHTML(doc.content)
               << "</div>\n";
        }
        os << "</div>\n";
    }

    os << "</body>\n</html>\n";
    return os.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// File export helpers
// ─────────────────────────────────────────────────────────────────────────────

bool EvaluationReportExporter::exportJSON(const PerQueryReport& report,
                                          const std::string& filepath) const {
    std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        THEMIS_WARN("EvaluationReportExporter: cannot open '{}' for JSON export",
                    filepath);
        return false;
    }
    ofs << toJSON(report);
    THEMIS_INFO("EvaluationReportExporter: JSON report written to '{}'", filepath);
    return ofs.good();
}

bool EvaluationReportExporter::exportHTML(const PerQueryReport& report,
                                          const std::string& filepath) const {
    std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        THEMIS_WARN("EvaluationReportExporter: cannot open '{}' for HTML export",
                    filepath);
        return false;
    }
    ofs << toHTML(report);
    THEMIS_INFO("EvaluationReportExporter: HTML report written to '{}'", filepath);
    return ofs.good();
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<EvaluationReportExporter> EvaluationReportExporterFactory::create() {
    return std::make_unique<EvaluationReportExporter>();
}

} // namespace themis::rag::judge
