/**
 * @file compliance_reporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=54, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/compliance_reporter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== File-local PDF/HTML helpers ==========

namespace {

static std::string reporter_escapePDFString(const std::string &s) {
    std::string result;
    result.reserve(s.size());
    for (unsigned char c : s) {
        if (c == '(') {
            result += "\\(";
        } else if (c == ')') {
            result += "\\)";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c >= 32 && c <= 126) {
            result += static_cast<char>(c);
        } else {
            result += ' ';
        }
    }
    return result;
}

static std::string buildComplianceReportHTML(const ComplianceReport &report) {
    std::ostringstream html;
    std::time_t ts    = static_cast<std::time_t>(report.generated_at);
    char time_buf[32] = {};
    std::tm *tm_info  = std::localtime(&ts);
    if (tm_info) {
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    html << "<!DOCTYPE html><html><head>"
         << "<meta charset='UTF-8'>"
         << "<title>ThemisDB Compliance Report</title>"
         << "<style>"
         << "body{font-family:Arial,sans-serif;margin:30px;color:#333;}"
         << "h1{color:#2c3e50;border-bottom:2px solid #4CAF50;padding-bottom:10px;}"
         << "h2{color:#34495e;margin-top:20px;}"
         << "table{border-collapse:collapse;width:100%;margin:10px 0;}"
         << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}"
         << "th{background-color:#4CAF50;color:white;}"
         << "tr:nth-child(even){background-color:#f9f9f9;}"
         << ".score{font-size:1.5em;font-weight:bold;color:#27ae60;}"
         << ".gap-critical{color:#c0392b;} .gap-high{color:#e74c3c;}"
         << ".gap-medium{color:#f39c12;} .gap-low{color:#27ae60;}"
         << "@media print{body{margin:15px;}}"
         << "</style></head><body>";

    html << "<h1>ThemisDB Compliance Report</h1>";
    html << "<p><strong>Report ID:</strong> " << report.report_id << "</p>";
    html << "<p><strong>Type:</strong> " << report.report_type << "</p>";
    html << "<p><strong>Generated:</strong> " << time_buf << "</p>";

    html << "<h2>Summary</h2>"
         << "<table><tr><th>Metric</th><th>Value</th></tr>"
         << "<tr><td>Total Rules</td><td>" << report.total_rules << "</td></tr>"
         << "<tr><td>Active Rules</td><td>" << report.active_rules << "</td></tr>"
         << "<tr><td>Inactive Rules</td><td>" << report.inactive_rules << "</td></tr>"
         << "<tr><td>Compliance Score</td><td class='score'>" << std::fixed << std::setprecision(1)
         << report.compliance_score << "%</td></tr>"
         << "</table>";

    if (!report.gaps.empty()) {
        html << "<h2>Compliance Gaps (" << report.gaps.size() << ")</h2>"
             << "<table><tr><th>Type</th><th>Severity</th>"
             << "<th>Description</th><th>Affected Resources</th></tr>";
        for (const auto &gap : report.gaps) {
            html << "<tr><td>" << gap.gap_type << "</td>"
                 << "<td class='gap-" << gap.severity << "'>" << gap.severity << "</td>"
                 << "<td>" << gap.description << "</td>"
                 << "<td>" << gap.affected_resources.size() << " resource(s)</td></tr>";
        }
        html << "</table>";
    } else {
        html << "<p><em>No compliance gaps detected.</em></p>";
    }

    if (!report.details.empty()) {
        html << "<h2>Details</h2><pre style='background:#f5f5f5;padding:10px;'>" << report.details.dump(2) << "</pre>";
    }

    html << "</body></html>";
    return html.str();
}

static std::string buildComplianceReportPDF(const ComplianceReport &report) {
    constexpr double PAGE_W     = 612.0;
    constexpr double PAGE_H     = 792.0;
    constexpr double MARGIN     = 50.0;
    constexpr double TITLE_SIZE = 14.0;
    constexpr double BODY_SIZE  = 9.0;
    constexpr double LINE_H     = 12.0;

    std::time_t ts    = static_cast<std::time_t>(report.generated_at);
    char time_buf[32] = {};
    std::tm *tm_info  = std::localtime(&ts);
    if (tm_info) {
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    // Collect text lines
    std::vector<std::string> lines;
    lines.push_back("Report ID:   " + report.report_id);
    lines.push_back("Type:        " + report.report_type);
    lines.push_back("Generated:   " + std::string(time_buf));
    lines.push_back("");
    lines.push_back("--- Summary ---");
    lines.push_back("Total Rules:   " + std::to_string(report.total_rules));
    lines.push_back("Active Rules:  " + std::to_string(report.active_rules));
    lines.push_back("Inactive Rules:" + std::to_string(report.inactive_rules));

    std::ostringstream score_ss;
    score_ss << std::fixed << std::setprecision(1) << report.compliance_score;
    lines.push_back("Compliance Score: " + score_ss.str() + "%");
    lines.push_back("");

    if (!report.gaps.empty()) {
        lines.push_back("--- Compliance Gaps ---");
        for (const auto &gap : report.gaps) {
            lines.push_back("[" + gap.severity + "] " + gap.gap_type + ": " + gap.description);
            lines.push_back("  Affected: " + std::to_string(gap.affected_resources.size()) + " resource(s)");
        }
    } else {
        lines.push_back("No compliance gaps detected.");
    }

    // Build PDF content stream
    std::vector<std::string> page_streams;
    page_streams.push_back("");

    const std::string title = "ThemisDB Compliance Report";

    double y = PAGE_H - MARGIN;
    page_streams.back() += "BT\n";
    page_streams.back() += "/F1 " + std::to_string(static_cast<int>(TITLE_SIZE)) + " Tf\n";
    page_streams.back()
        += std::to_string(static_cast<int>(MARGIN)) + " " + std::to_string(static_cast<int>(y)) + " Td\n";
    page_streams.back() += "(" + reporter_escapePDFString(title) + ") Tj\n";
    y -= TITLE_SIZE * 1.8;

    page_streams.back() += "ET\n";
    page_streams.back() += std::to_string(static_cast<int>(MARGIN)) + " " + std::to_string(static_cast<int>(y + 4))
                           + " " + std::to_string(static_cast<int>(PAGE_W - MARGIN * 2)) + " 1 re f\n";
    y -= LINE_H;

    page_streams.back() += "BT\n";
    page_streams.back() += "/F2 " + std::to_string(static_cast<int>(BODY_SIZE)) + " Tf\n";

    for (const auto &line : lines) {
        if (y < MARGIN) {
            page_streams.back() += "ET\n";
            page_streams.push_back("");
            y = PAGE_H - MARGIN;
            page_streams.back() += "BT\n";
            page_streams.back() += "/F2 " + std::to_string(static_cast<int>(BODY_SIZE)) + " Tf\n";
        }
        page_streams.back()
            += std::to_string(static_cast<int>(MARGIN)) + " " + std::to_string(static_cast<int>(y)) + " Td\n";
        std::string display = line.size() > 100 ? line.substr(0, 97) + "..." : line;
        page_streams.back() += "(" + reporter_escapePDFString(display) + ") Tj\n";
        y -= LINE_H;
    }
    page_streams.back() += "ET\n";

    // Assemble PDF binary
    std::string pdf;
    pdf.reserve(4096);
    pdf += "%PDF-1.4\n";
    pdf += "%\xE2\xE3\xCF\xD3\n";

    int P           = static_cast<int>(page_streams.size());
    int base_page   = 3;
    int base_stream = base_page + P;
    int font_f1_id  = base_stream + P;
    int font_f2_id  = font_f1_id + 1;
    int total_objs  = font_f2_id + 1;

    std::vector<size_t> offsets(static_cast<size_t>(total_objs) + 1, 0);

    auto appendObj = [&](int id, const std::string &dict, const std::string &stream_data = "") {
        offsets[static_cast<size_t>(id)] = pdf.size();
        pdf += std::to_string(id) + " 0 obj\n";
        if (stream_data.empty()) {
            pdf += dict + "\n";
        } else {
            std::string d = dict;
            auto pos      = d.rfind(">>");
            if (pos != std::string::npos) {
                d.insert(pos, " /Length " + std::to_string(stream_data.size()));
            }
            pdf += d + "\nstream\n";
            pdf += stream_data;
            pdf += "\nendstream\n";
        }
        pdf += "endobj\n";
    };

    appendObj(1, "<< /Type /Catalog /Pages 2 0 R >>");

    {
        std::string kids = "[";
        for (int i = 0; i < P; ++i) {
            kids += std::to_string(base_page + i) + " 0 R ";
        }
        kids += "]";
        appendObj(2, "<< /Type /Pages /Kids " + kids + " /Count " + std::to_string(P) + " >>");
    }

    std::string font_res
        = "<< /F1 " + std::to_string(font_f1_id) + " 0 R" + " /F2 " + std::to_string(font_f2_id) + " 0 R >>";

    for (int i = 0; i < P; ++i) {
        int page_id   = base_page + i;
        int stream_id = base_stream + i;
        appendObj(page_id, "<< /Type /Page /Parent 2 0 R"
                           " /MediaBox [0 0 "
                               + std::to_string(static_cast<int>(PAGE_W)) + " "
                               + std::to_string(static_cast<int>(PAGE_H))
                               + "]"
                                 " /Contents "
                               + std::to_string(stream_id)
                               + " 0 R"
                                 " /Resources << /Font "
                               + font_res + " >> >>");
    }

    for (int i = 0; i < P; ++i) {
        appendObj(base_stream + i, "<<>>", page_streams[static_cast<size_t>(i)]);
    }

    appendObj(font_f1_id, "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>");
    appendObj(font_f2_id, "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");

    size_t xref_offset = pdf.size();
    pdf += "xref\n";
    pdf += "0 " + std::to_string(total_objs) + "\n";
    pdf += "0000000000 65535 f \n";
    for (int i = 1; i < total_objs; ++i) {
        char buf[22];
        std::snprintf(buf, sizeof(buf), "%010zu 00000 n \n", offsets[static_cast<size_t>(i)]);
        pdf += buf;
    }

    pdf += "trailer\n";
    pdf += "<< /Size " + std::to_string(total_objs) + " /Root 1 0 R >>\n";
    pdf += "startxref\n";
    pdf += std::to_string(xref_offset) + "\n";
    pdf += "%%EOF\n";

    return pdf;
}

} // anonymous namespace

// ========== BiasFieldStats Implementation ==========

nlohmann::json BiasFieldStats::toJson() const {
    nlohmann::json j;
    j["field_name"]               = field_name;
    j["total_count"]              = total_count;
    j["representation_ratio"]     = representation_ratio;
    j["demographic_parity_score"] = demographic_parity_score;
    j["group_counts"]             = nlohmann::json::object();
    for (const auto &[label, count] : group_counts) {
        j["group_counts"][label] = count;
    }
    return j;
}

// ========== BiasAuditReport Implementation ==========

nlohmann::json BiasAuditReport::toJson() const {
    nlohmann::json j;
    j["report_id"]          = report_id;
    j["adapter_id"]         = adapter_id;
    j["dataset_id"]         = dataset_id;
    j["generated_at"]       = generated_at;
    j["overall_bias_score"] = overall_bias_score;
    j["status"]             = status;
    j["recommendations"]    = recommendations;
    nlohmann::json fs_arr   = nlohmann::json::array();
    for (const auto &fs : field_stats) {
        fs_arr.push_back(fs.toJson());
    }
    j["field_stats"] = std::move(fs_arr);
    return j;
}

// ========== CoverageAnalysis Implementation ==========

nlohmann::json CoverageAnalysis::toJson() const {
    nlohmann::json j;
    j["total_resources_analyzed"]   = total_resources_analyzed;
    j["resources_with_policies"]    = resources_with_policies;
    j["resources_without_policies"] = resources_without_policies;
    j["coverage_percentage"]        = coverage_percentage;
    j["uncovered_resources"]        = uncovered_resources;
    j["overlapping_rules"]          = overlapping_rules;
    return j;
}

// ========== ComplianceGap Implementation ==========

nlohmann::json ComplianceGap::toJson() const {
    nlohmann::json j;
    j["gap_type"]           = gap_type;
    j["severity"]           = severity;
    j["description"]        = description;
    j["affected_resources"] = affected_resources;
    j["recommendations"]    = recommendations;
    return j;
}

// ========== ComplianceReport Implementation ==========

nlohmann::json ComplianceReport::toJson() const {
    nlohmann::json j;
    j["report_id"]      = report_id;
    j["generated_at"]   = generated_at;
    j["report_type"]    = report_type;
    j["total_rules"]    = total_rules;
    j["active_rules"]   = active_rules;
    j["inactive_rules"] = inactive_rules;

    nlohmann::json gaps_array = nlohmann::json::array();
    for (const auto &gap : gaps) {
        gaps_array.push_back(gap.toJson());
    }
    j["gaps"]             = gaps_array;
    j["compliance_score"] = compliance_score;
    j["details"]          = details;

    return j;
}

// ========== ComplianceReporter Implementation ==========

ComplianceReporter::ComplianceReporter(std::shared_ptr<PolicyManager> policy_manager)
    : policy_manager_(std::move(policy_manager)) {
    if (!policy_manager_) {
        THEMIS_WARN("ComplianceReporter created with null PolicyManager");
    }
}

CoverageAnalysis ComplianceReporter::analyzeCoverage(const std::vector<std::string> &resources) const {
    CoverageAnalysis analysis;
    analysis.total_resources_analyzed = static_cast<int>(resources.size());

    auto rules = policy_manager_->listRules();

    for (const auto &resource : resources) {
        bool has_policy = false;

        for (const auto &rule : rules) {
            if (rule.appliesTo(resource, "*")) {
                has_policy = true;
                break;
            }
        }

        if (has_policy) {
            analysis.resources_with_policies++;
        } else {
            analysis.resources_without_policies++;
            analysis.uncovered_resources.push_back(resource);
        }
    }

    if (analysis.total_resources_analyzed > 0) {
        analysis.coverage_percentage
            = (static_cast<double>(analysis.resources_with_policies) / analysis.total_resources_analyzed) * 100.0;
    }

    // Detect overlapping rules
    auto overlaps = detectOverlappingRules();
    for (const auto &[rule1, rule2] : overlaps) {
        analysis.overlapping_rules.push_back(rule1 + " <-> " + rule2);
    }

    THEMIS_INFO("Coverage analysis: {:.1f}% ({}/{})", analysis.coverage_percentage, analysis.resources_with_policies,
                analysis.total_resources_analyzed);

    return analysis;
}

std::vector<std::pair<std::string, std::string>> ComplianceReporter::detectOverlappingRules() const {
    std::vector<std::pair<std::string, std::string>> overlaps;

    auto rules = policy_manager_->listRules();

    // Check each pair of rules for overlap
    for (size_t i = 0; i < rules.size(); i++) {
        for (size_t j = i + 1; j < rules.size(); j++) {
            const auto &rule1 = rules[i];
            const auto &rule2 = rules[j];

            // Simple overlap check: same resources and actions
            bool resource_overlap = false;
            for (const auto &r1_res : rule1.resources) {
                for (const auto &r2_res : rule2.resources) {
                    if (r1_res == r2_res || r1_res == "*" || r2_res == "*") {
                        resource_overlap = true;
                        break;
                    }
                }
                if (resource_overlap) {
                    break;
                }
            }

            if (resource_overlap) {
                bool action_overlap = false;
                for (const auto &r1_act : rule1.actions) {
                    for (const auto &r2_act : rule2.actions) {
                        if (r1_act == r2_act || r1_act == "*" || r2_act == "*") {
                            action_overlap = true;
                            break;
                        }
                    }
                    if (action_overlap) {
                        break;
                    }
                }

                if (action_overlap) {
                    overlaps.push_back({rule1.id, rule2.id});
                }
            }
        }
    }

    return overlaps;
}

std::vector<ComplianceGap> ComplianceReporter::detectGaps() const {
    std::vector<ComplianceGap> gaps;

    auto rules = policy_manager_->listRules();

    // Check for rules without encryption
    std::vector<std::string> no_encryption;
    std::vector<std::string> no_audit;
    std::vector<std::string> overly_permissive;

    for (const auto &rule : rules) {
        if (rule.enabled) {
            // Check encryption requirement
            if (!rule.require_encryption && rule.classification_level != "offen") {
                no_encryption.push_back(rule.id);
            }

            // Check audit requirements
            if (!rule.audit_access && !rule.audit_changes) {
                no_audit.push_back(rule.id);
            }

            // Check for overly permissive rules
            if (rule.allow_export && rule.allow_cache
                && (rule.classification_level == "geheim" || rule.classification_level == "streng-geheim")) {
                overly_permissive.push_back(rule.id);
            }
        }
    }

    // Create gap entries
    if (!no_encryption.empty()) {
        ComplianceGap gap;
        gap.gap_type           = "missing_encryption";
        gap.severity           = "high";
        gap.description        = "Rules without encryption requirements for sensitive data";
        gap.affected_resources = no_encryption;
        gap.recommendations    = {"Enable 'require_encryption' for sensitive data rules",
                                  "Review classification levels and ensure appropriate encryption"};
        gaps.push_back(gap);
    }

    if (!no_audit.empty()) {
        ComplianceGap gap;
        gap.gap_type           = "missing_audit";
        gap.severity           = "medium";
        gap.description        = "Rules without audit logging enabled";
        gap.affected_resources = no_audit;
        gap.recommendations    = {"Enable 'audit_access' or 'audit_changes' for compliance",
                                  "Ensure audit trail for sensitive operations"};
        gaps.push_back(gap);
    }

    if (!overly_permissive.empty()) {
        ComplianceGap gap;
        gap.gap_type           = "overly_permissive";
        gap.severity           = "critical";
        gap.description        = "Highly classified data with permissive export/cache settings";
        gap.affected_resources = overly_permissive;
        gap.recommendations
            = {"Disable 'allow_export' for classified data", "Disable 'allow_cache' for highly sensitive information",
               "Review and tighten access controls"};
        gaps.push_back(gap);
    }

    return gaps;
}

ComplianceReport ComplianceReporter::generateSummaryReport() const {
    ComplianceReport report;
    report.report_id    = "summary_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    report.report_type  = "summary";

    auto stats            = policy_manager_->getStats();
    report.total_rules    = stats.total_rules;
    report.active_rules   = stats.enabled_rules;
    report.inactive_rules = stats.disabled_rules;

    report.gaps             = detectGaps();
    report.compliance_score = calculateComplianceScore(report.gaps);

    // Add classification breakdown
    report.details["rules_by_classification"] = nlohmann::json::object();
    for (const auto &[level, count] : stats.rules_by_classification) {
        report.details["rules_by_classification"][level] = count;
    }

    return report;
}

ComplianceReport ComplianceReporter::generateComplianceReport(const std::string &framework) const {
    ComplianceReport report;
    report.report_id
        = "compliance_" + framework + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    report.report_type  = "compliance";

    auto stats            = policy_manager_->getStats();
    report.total_rules    = stats.total_rules;
    report.active_rules   = stats.enabled_rules;
    report.inactive_rules = stats.disabled_rules;

    report.gaps             = detectGaps();
    report.compliance_score = calculateComplianceScore(report.gaps);

    // Framework-specific analysis
    if (!framework.empty()) {
        report.details["framework"]              = framework;
        report.details["framework_requirements"] = nlohmann::json::array();

        // Add framework-specific requirements
        if (framework == "GDPR") {
            report.details["framework_requirements"].push_back("Data encryption at rest and in transit");
            report.details["framework_requirements"].push_back("Audit trail for data access");
            report.details["framework_requirements"].push_back("Data retention policies");
        } else if (framework == "SOX") {
            report.details["framework_requirements"].push_back("Access control and segregation of duties");
            report.details["framework_requirements"].push_back("Audit logging of all changes");
            report.details["framework_requirements"].push_back("Regular policy reviews");
        } else if (framework == "CCPA") {
            report.details["framework_requirements"].push_back("Opt-out of sale of personal information");
            report.details["framework_requirements"].push_back(
                "Right to know categories and specific pieces of personal information");
            report.details["framework_requirements"].push_back("Right to delete personal information");
            report.details["framework_requirements"].push_back("Right to portability of personal information");
            report.details["framework_requirements"].push_back("Non-discrimination for exercising CCPA rights");
        } else if (framework == "PCI-DSS") {
            report.details["framework_requirements"].push_back("Install and maintain network controls (Req 1)");
            report.details["framework_requirements"].push_back("Protect stored account data with encryption (Req 3)");
            report.details["framework_requirements"].push_back(
                "Protect cardholder data in transit with strong cryptography (Req 4)");
            report.details["framework_requirements"].push_back(
                "Restrict access by business need to know - least privilege (Req 7)");
            report.details["framework_requirements"].push_back(
                "Log and monitor all access to cardholder data (Req 10)");
            report.details["framework_requirements"].push_back("Retain audit logs for at least 12 months (Req 10.7)");
        }
    }

    return report;
}

nlohmann::json ComplianceReporter::generateCcpaReport(const CcpaRuleSet &rule_set, int64_t window_start_ms,
                                                      int64_t window_end_ms) const {
    nlohmann::json j = nlohmann::json::object();

    auto rules = policy_manager_->listRules();

    std::unordered_set<std::string> categories_seen;
    std::vector<std::string> third_party_disclosure_rule_ids;
    nlohmann::json missing_by_check = nlohmann::json::object();

    int compliant_rules     = 0;
    int non_compliant_rules = 0;

    for (const auto &rule : rules) {
        if (!rule.enabled) {
            continue;
        }

        if (!rule.classification_level.empty()) {
            categories_seen.insert(rule.classification_level);
        }

        if (rule.allow_export) {
            third_party_disclosure_rule_ids.push_back(rule.id);
        }

        const auto evals    = rule_set.evaluateRule(rule);
        bool rule_compliant = true;
        for (const auto &eval : evals) {
            if (!eval.compliant) {
                rule_compliant = false;
                if (!missing_by_check.contains(eval.ccpa_check_id)) {
                    missing_by_check[eval.ccpa_check_id] = nlohmann::json::array();
                }
                missing_by_check[eval.ccpa_check_id].push_back(eval.rule_id);
            }
        }

        if (rule_compliant) {
            ++compliant_rules;
        } else {
            ++non_compliant_rules;
        }
    }

    std::vector<std::string> data_categories(categories_seen.begin(), categories_seen.end());
    std::sort(data_categories.begin(), data_categories.end());

    std::sort(third_party_disclosure_rule_ids.begin(), third_party_disclosure_rule_ids.end());
    third_party_disclosure_rule_ids.erase(
        std::unique(third_party_disclosure_rule_ids.begin(), third_party_disclosure_rule_ids.end()),
        third_party_disclosure_rule_ids.end());

    j["framework"]       = "CCPA/CPRA";
    j["window_start_ms"] = window_start_ms;
    j["window_end_ms"]   = window_end_ms;
    j["generated_at"]
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    j["total_subjects"]                  = static_cast<int64_t>(rule_set.optOutCount());
    j["opted_out_of_sale"]               = rule_set.countOptOutRequests(window_start_ms, window_end_ms);
    j["ccpa_compliant_rules"]            = compliant_rules;
    j["ccpa_non_compliant_rules"]        = non_compliant_rules;
    j["data_categories"]                 = data_categories;
    j["third_party_disclosure_rule_ids"] = third_party_disclosure_rule_ids;
    j["missing_by_check"]                = missing_by_check;

    // Enrich with policy-level gap information specific to CCPA
    std::vector<nlohmann::json> gaps;

    // Check whether any active policy rule covers CCPA-required resources
    const std::vector<std::string> ccpa_resources = {"personal_information", "data_subject", "consumer_data"};
    std::vector<std::string> uncovered;
    for (const auto &res : ccpa_resources) {
        bool covered = false;
        for (const auto &rule : rules) {
            if (rule.enabled && rule.appliesTo(res, "*")) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            uncovered.push_back(res);
        }
    }

    if (!uncovered.empty()) {
        nlohmann::json gap;
        gap["gap_type"]           = "missing_ccpa_policy";
        gap["severity"]           = "high";
        gap["description"]        = "No active policy rule covers required CCPA-protected resources";
        gap["affected_resources"] = uncovered;
        gap["recommendations"] = {"Add a policy rule that covers 'personal_information' and 'consumer_data' resources",
                                  "Ensure the rule enforces encryption and audit logging"};
        gaps.push_back(gap);
    }

    j["policy_gaps"] = gaps;
    j["framework"]   = "CCPA/CPRA";

    THEMIS_INFO("CCPA compliance report generated: {} subjects, {} opt-outs", j["total_subjects"].get<int64_t>(),
                j["opted_out_of_sale"].get<int>());

    return j;
}

nlohmann::json ComplianceReporter::generateAccessControlMatrix() const {
    nlohmann::json matrix = nlohmann::json::object();

    auto rules = policy_manager_->listRules();

    // Build matrix: resource -> action -> required_roles
    for (const auto &rule : rules) {
        if (!rule.enabled) {
            continue;
        }

        for (const auto &resource : rule.resources) {
            if (!matrix.contains(resource)) {
                matrix[resource] = nlohmann::json::object();
            }

            for (const auto &action : rule.actions) {
                if (!matrix[resource].contains(action)) {
                    matrix[resource][action] = nlohmann::json::array();
                }

                for (const auto &role : rule.required_roles) {
                    // Add role if not already in list
                    bool found = false;
                    for (const auto &existing : matrix[resource][action]) {
                        if (existing.get<std::string>() == role) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        matrix[resource][action].push_back(role);
                    }
                }
            }
        }
    }

    return matrix;
}

ComplianceReport ComplianceReporter::generateRiskAssessmentReport() const {
    ComplianceReport report;
    report.report_id    = "risk_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    report.report_type  = "risk_assessment";

    auto stats            = policy_manager_->getStats();
    report.total_rules    = stats.total_rules;
    report.active_rules   = stats.enabled_rules;
    report.inactive_rules = stats.disabled_rules;

    report.gaps             = detectGaps();
    report.compliance_score = calculateComplianceScore(report.gaps);

    // Calculate risk metrics
    int critical_gaps = 0, high_gaps = 0, medium_gaps = 0, low_gaps = 0;
    for (const auto &gap : report.gaps) {
        if (gap.severity == "critical") {
            critical_gaps++;
        } else if (gap.severity == "high") {
            high_gaps++;
        } else if (gap.severity == "medium") {
            medium_gaps++;
        } else if (gap.severity == "low") {
            low_gaps++;
        }
    }

    report.details["risk_summary"]
        = {{"critical_gaps", critical_gaps},
           {"high_gaps", high_gaps},
           {"medium_gaps", medium_gaps},
           {"low_gaps", low_gaps},
           {"overall_risk_level", critical_gaps > 0 ? "critical" : (high_gaps > 0 ? "high" : "medium")}};

    return report;
}

std::string ComplianceReporter::exportReport(const ComplianceReport &report, const std::string &format) const {
    if (format == "json") {
        return report.toJson().dump(2);
    } else if (format == "csv") {
        return reportToCSV(report);
    } else if (format == "html") {
        THEMIS_INFO("Generating HTML compliance report (type={})", report.report_type);
        return buildComplianceReportHTML(report);
    } else if (format == "pdf") {
        THEMIS_INFO("Generating PDF compliance report (type={})", report.report_type);
        return buildComplianceReportPDF(report);
    }

    return report.toJson().dump(2); // Default to JSON
}

double ComplianceReporter::calculateComplianceScore(const std::vector<ComplianceGap> &gaps) const {
    if (gaps.empty()) {
        return 100.0;
    }

    // Deduct points based on severity
    double score = 100.0;
    for (const auto &gap : gaps) {
        if (gap.severity == "critical") {
            score -= 20.0;
        } else if (gap.severity == "high") {
            score -= 15.0;
        } else if (gap.severity == "medium") {
            score -= 10.0;
        } else if (gap.severity == "low") {
            score -= 5.0;
        }
    }

    return std::max(0.0, score);
}

std::string ComplianceReporter::reportToCSV(const ComplianceReport &report) const {
    std::ostringstream csv;

    // Header
    csv << "Report ID,Generated At,Type,Total Rules,Active Rules,Compliance Score\n";

    // Data
    csv << report.report_id << "," << report.generated_at << "," << report.report_type << "," << report.total_rules
        << "," << report.active_rules << "," << report.compliance_score << "\n";

    // Gaps section
    csv << "\nGap Type,Severity,Description,Affected Count\n";
    for (const auto &gap : report.gaps) {
        csv << gap.gap_type << "," << gap.severity << ","
            << "\"" << gap.description << "\"," << gap.affected_resources.size() << "\n";
    }

    return csv.str();
}

bool ComplianceReporter::hasRequiredControls(const PolicyRule &rule) const {
    // Check if rule has minimum required controls
    return rule.require_encryption && (rule.audit_access || rule.audit_changes) && !rule.required_roles.empty();
}

BiasAuditReport ComplianceReporter::generateBiasAuditReport(
    const std::string &adapter_id, const std::string &dataset_id,
    const std::unordered_map<std::string, std::unordered_map<std::string, size_t>> &raw_field_stats) const {
    BiasAuditReport report;
    report.report_id  = "bias_" + adapter_id + "_"
                        + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::system_clock::now().time_since_epoch())
                                             .count());
    report.adapter_id = adapter_id;
    report.dataset_id = dataset_id;
    report.generated_at
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count();

    double score_sum  = 0.0;
    int scored_fields = 0;

    for (const auto &[field_name, group_map] : raw_field_stats) {
        if (group_map.empty()) {
            continue;
        }

        BiasFieldStats fs;
        fs.field_name   = field_name;
        fs.group_counts = group_map;

        // Compute total and min/max group counts
        size_t total     = 0;
        size_t min_count = SIZE_MAX;
        size_t max_count = 0;
        for (const auto &[label, cnt] : group_map) {
            total += cnt;
            if (cnt < min_count)
                min_count = cnt;
            if (cnt > max_count)
                max_count = cnt;
        }
        fs.total_count = total;

        if (max_count > 0) {
            fs.representation_ratio = static_cast<double>(min_count) / static_cast<double>(max_count);
        }

        // Demographic parity: entropy-based normalization.
        // Compute actual entropy and compare to maximum entropy (uniform distribution).
        double actual_entropy = 0.0;
        for (const auto &[label, cnt] : group_map) {
            if (cnt > 0 && total > 0) {
                double p = static_cast<double>(cnt) / static_cast<double>(total);
                actual_entropy -= p * std::log(p);
            }
        }
        const double max_entropy    = (group_map.size() > 1) ? std::log(static_cast<double>(group_map.size())) : 1.0;
        fs.demographic_parity_score = (max_entropy > 0.0) ? (actual_entropy / max_entropy) : 1.0;

        // Clamp to [0, 1]
        fs.demographic_parity_score = std::max(0.0, std::min(1.0, fs.demographic_parity_score));

        // Per-field recommendation when representation is below threshold
        if (fs.representation_ratio < 0.8) {
            report.recommendations.push_back(
                "Field '" + field_name
                + "': representation_ratio=" + std::to_string(fs.representation_ratio).substr(0, 5)
                + " is below 0.8 — consider resampling or augmenting underrepresented groups.");
        }

        score_sum += fs.demographic_parity_score;
        ++scored_fields;

        report.field_stats.push_back(std::move(fs));
    }

    // Overall bias score is the mean demographic parity score across all fields
    report.overall_bias_score = (scored_fields > 0) ? (score_sum / static_cast<double>(scored_fields))
                                                    : 1.0; // No fields to audit: treat as unbiased

    // Assign status
    if (report.overall_bias_score >= 0.8) {
        report.status = "PASSED";
    } else if (report.overall_bias_score >= 0.5) {
        report.status = "FLAGGED";
    } else {
        report.status = "FAILED";
    }

    if (report.status != "PASSED") {
        report.recommendations.push_back("Overall bias score " + std::to_string(report.overall_bias_score).substr(0, 5)
                                         + ": review training data composition before proceeding with model training.");
    }

    THEMIS_INFO("BiasAuditReport generated for adapter='{}' dataset='{}': "
                "status={} score={:.3f} fields={}",
                adapter_id, dataset_id, report.status, report.overall_bias_score, scored_fields);

    return report;
}

// ============================================================================
// RuleEvaluationEntry
// ============================================================================

RuleEvaluationEntry RuleEvaluationEntry::fromJson(const nlohmann::json &j) {
    RuleEvaluationEntry e;
    if (j.contains("timestamp") && j["timestamp"].is_number()) {
        e.timestamp_ms = j["timestamp"].get<int64_t>();
    }
    if (j.contains("route") && j["route"].is_string()) {
        e.route = j["route"].get<std::string>();
    }
    if (j.contains("classification") && j["classification"].is_string()) {
        e.classification = j["classification"].get<std::string>();
    }
    if (j.contains("mode") && j["mode"].is_string()) {
        e.mode = j["mode"].get<std::string>();
    }
    if (j.contains("require_content_encryption") && j["require_content_encryption"].is_boolean()) {
        e.require_content_encryption = j["require_content_encryption"].get<bool>();
    }
    if (j.contains("ccpa_opted_out") && j["ccpa_opted_out"].is_boolean()) {
        e.ccpa_opted_out = j["ccpa_opted_out"].get<bool>();
    }
    if (j.contains("export_allowed") && j["export_allowed"].is_boolean()) {
        e.export_allowed = j["export_allowed"].get<bool>();
    }
    if (j.contains("user_id") && j["user_id"].is_string()) {
        e.user_id = j["user_id"].get<std::string>();
    }
    return e;
}

// ============================================================================
// TimeWindowReport serialization
// ============================================================================

nlohmann::json TimeWindowReport::toJson() const {
    nlohmann::json j;
    j["window_start_ms"]           = window_start_ms;
    j["window_end_ms"]             = window_end_ms;
    j["generated_at"]              = generated_at;
    j["framework"]                 = framework;
    j["total_evaluations"]         = total_evaluations;
    j["enforce_mode_evaluations"]  = enforce_mode_evaluations;
    j["observe_mode_evaluations"]  = observe_mode_evaluations;
    j["ccpa_opted_out_count"]      = ccpa_opted_out_count;
    j["encryption_required_count"] = encryption_required_count;
    j["export_blocked_count"]      = export_blocked_count;
    j["compliance_score"]          = compliance_score;

    nlohmann::json by_cls = nlohmann::json::object();
    for (const auto &kv : evaluations_by_classification) {
        by_cls[kv.first] = kv.second;
    }
    j["evaluations_by_classification"] = by_cls;

    nlohmann::json by_route = nlohmann::json::object();
    for (const auto &kv : evaluations_by_route) {
        by_route[kv.first] = kv.second;
    }
    j["evaluations_by_route"] = by_route;

    nlohmann::json gaps_arr = nlohmann::json::array();
    for (const auto &g : gaps) {
        gaps_arr.push_back(g.toJson());
    }
    j["gaps"] = gaps_arr;

    return j;
}

std::string TimeWindowReport::toCSV() const {
    std::ostringstream csv;
    csv << "window_start_ms,window_end_ms,generated_at,framework,"
        << "total_evaluations,enforce_mode_evaluations,observe_mode_evaluations,"
        << "ccpa_opted_out_count,encryption_required_count,export_blocked_count,"
        << "compliance_score\n";
    csv << window_start_ms << "," << window_end_ms << "," << generated_at << "," << framework << ","
        << total_evaluations << "," << enforce_mode_evaluations << "," << observe_mode_evaluations << ","
        << ccpa_opted_out_count << "," << encryption_required_count << "," << export_blocked_count << "," << std::fixed
        << std::setprecision(1) << compliance_score << "\n";
    return csv.str();
}

// ============================================================================
// generateTimeWindowReport
// ============================================================================

TimeWindowReport ComplianceReporter::generateTimeWindowReport(const std::vector<RuleEvaluationEntry> &entries,
                                                              int64_t window_start_ms, int64_t window_end_ms,
                                                              const std::string &framework) const {
    TimeWindowReport report;
    report.window_start_ms = window_start_ms;
    report.window_end_ms   = window_end_ms;
    report.generated_at
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    report.framework = framework;

    for (const auto &e : entries) {
        if (e.timestamp_ms < window_start_ms || e.timestamp_ms > window_end_ms) {
            continue;
        }

        ++report.total_evaluations;

        if (e.mode == "enforce") {
            ++report.enforce_mode_evaluations;
        } else {
            ++report.observe_mode_evaluations;
        }

        if (e.ccpa_opted_out) {
            ++report.ccpa_opted_out_count;
        }

        if (e.require_content_encryption) {
            ++report.encryption_required_count;
        }

        if (!e.export_allowed) {
            ++report.export_blocked_count;
        }

        if (!e.classification.empty()) {
            ++report.evaluations_by_classification[e.classification];
        }

        if (!e.route.empty()) {
            ++report.evaluations_by_route[e.route];
        }
    }

    // Attach policy-level compliance gaps and score from the current rule set
    report.gaps             = detectGaps();
    report.compliance_score = calculateComplianceScore(report.gaps);

    THEMIS_INFO("TimeWindowReport generated: window=[{}, {}] total_evaluations={} "
                "score={:.1f} framework='{}'",
                window_start_ms, window_end_ms, report.total_evaluations, report.compliance_score, framework);

    return report;
}

} // namespace governance
} // namespace themis
