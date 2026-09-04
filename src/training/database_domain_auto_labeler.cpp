/**
 * @file database_domain_auto_labeler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "training/database_domain_auto_labeler.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Minimal JSON parsing helpers (no external dependency required for this unit)
// We use a lightweight key extraction approach to avoid adding nlohmann/json
// as a new dependency to this translation unit.  Full JSON parsing is used by
// callers that already link against nlohmann::json.
// ---------------------------------------------------------------------------
namespace {

/// Extract the value of a JSON string field (simple single-value parser).
/// Returns empty string if the field is not found or the value is not a string.
static std::string extractStringField(const std::string& json, const std::string& key) {
    // Look for "key":"value" or "key": "value"
    auto pos = json.find('"' + key + '"');
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + key.size() + 2);
    if (pos == std::string::npos) return {};
    // Skip whitespace
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) {
      ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') return {};
    ++pos; // skip opening quote
    auto end = json.find('"', pos);
    if (end == std::string::npos) return {};
    return json.substr(pos, end - pos);
}

/// Extract the value of a JSON number field.
/// Returns NaN if not found.
static double extractDoubleField(const std::string& json, const std::string& key) {
    auto pos = json.find('"' + key + '"');
    if (pos == std::string::npos) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    pos = json.find(':', pos + key.size() + 2);
    if (pos == std::string::npos) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) {
      ++pos;
    }
    if (pos >= json.size()) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    try {
        size_t consumed = 0;
        double val = std::stod(json.substr(pos), &consumed);
        return consumed > 0 ? val : std::numeric_limits<double>::quiet_NaN();
    } catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

} // anonymous namespace

namespace themis::training {

// ── Constructor ────────────────────────────────────────────────────────────

DatabaseDomainAutoLabeler::DatabaseDomainAutoLabeler(double sensitivity_ms)
    : sensitivity_ms_(sensitivity_ms > 0.0 ? sensitivity_ms : 10.0)
{}

// ── computeConfidence ──────────────────────────────────────────────────────

double DatabaseDomainAutoLabeler::computeConfidence([[maybe_unused]] double delta_p99_ms) const {
    // confidence = sigmoid(|delta_p99_ms| / sensitivity_ms_)
    // sigmoid(x) = 1 / (1 + exp(-x))
    const double x = std::abs(delta_p99_ms) / sensitivity_ms_;
    return 1.0 / (1.0 + std::exp(-x));
}

// ── buildSample ───────────────────────────────────────────────────────────

LabeledDbSample DatabaseDomainAutoLabeler::buildSample(
    const std::string& query,
    const std::string& plan_json,
    double             delta_p99_ms,
    const std::string& source) const
{
    LabeledDbSample s;
    s.query_text   = query;
    s.plan_json    = plan_json;
    s.label        = DomainType::DATABASE_OPTIMIZER;
    s.confidence   = computeConfidence(delta_p99_ms);
    s.source       = source;
    s.delta_p99_ms = delta_p99_ms;
    return s;
}

// ── labelFromBaoDecision ──────────────────────────────────────────────────

LabeledDbSample DatabaseDomainAutoLabeler::labelFromBaoDecision(
    const std::string& query,
    const std::string& bao_plan_json,
    double             delta_p99_ms) const
{
    return buildSample(query, bao_plan_json, delta_p99_ms, "bao_log");
}

// ── labelFromDBAFeedback ──────────────────────────────────────────────────

LabeledDbSample DatabaseDomainAutoLabeler::labelFromDBAFeedback(
    const FeedbackEntry& entry) const
{
    LabeledDbSample s = buildSample(entry.query_text, entry.plan_json,
                                    entry.delta_p99_ms, "dba_feedback");

    // Negative (rejected) feedback → high-confidence signal for the labeler.
    // Positive feedback uses the normal sigmoid; negative feedback is forced
    // to at least 0.9 to reflect the strong signal from explicit DBA rejection.
    if (!entry.is_positive) {
        s.confidence = std::max(s.confidence, 0.9);
    }
    return s;
}

// ── labelFromLogFile ──────────────────────────────────────────────────────

std::vector<LabeledDbSample> DatabaseDomainAutoLabeler::labelFromLogFile(
    const std::string& log_path,
    size_t             max_samples,
    double             min_confidence) const
{
    std::vector<LabeledDbSample> result;

    std::ifstream file(log_path);
    if (!file.is_open()) {
        // File not found or not readable — return empty vector per spec.
        return result;
    }

    std::string line = {};
    while (std::getline(file, line)) {
        if (line.empty() || line.front() != '{') continue;

        const std::string query        = extractStringField(line, "query");
        const std::string plan         = extractStringField(line, "plan");
        const double      delta_p99_ms = extractDoubleField(line, "delta_p99_ms");

        // Skip unparseable or incomplete lines
        if (query.empty()) {
          continue;
        }
        if (std::isnan(delta_p99_ms)) {
          continue;
        }

        auto sample = buildSample(query, plan, delta_p99_ms, "bao_log");

        if (sample.confidence < min_confidence) {
          continue;
        }

        result.push_back(std::move(sample));

        if (max_samples > 0 && result.size() >= max_samples) {
          break;
        }
    }

    return result;
}

// ── exportToJsonl ─────────────────────────────────────────────────────────

/*static*/
std::string DatabaseDomainAutoLabeler::exportToJsonl(
    const std::vector<LabeledDbSample>& samples)
{
    if (samples.empty()) return {};

    std::ostringstream oss = {};

    auto escape = [](const std::string& s) -> std::string {
        std::string out = {};
        out.reserve(s.size() + 4);
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;      break;
            }
        }
        return out;
    };

    for (const auto& s : samples) {
        oss << '{'
            << "\"query\":\"" << escape(s.query_text) << "\","
            << "\"explain_plan\":\"" << escape(s.plan_json) << "\","
            << "\"latency_delta_ms\":" << s.delta_p99_ms << ","
            << "\"confidence\":" << s.confidence << ","
            << "\"source\":\"" << escape(s.source) << "\""
            << "}\n";
    }

    return oss.str();
}

} // namespace themis::training

