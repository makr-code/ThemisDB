/**
 * @file aql_confidence_scorer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_confidence_scorer.h"

#include <algorithm>
#include <cctype>
#include <numeric>
#include <sstream>

namespace themis {
namespace aql {

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

AQLConfidenceScore AQLConfidenceScorer::score(const std::string &aql_query, const std::string & /*nl_query*/,
                                              const std::string &schema_context) const {
    AQLConfidenceScore result;

    if (aql_query.empty()) {
        result.reasoning = "Empty query: no AQL was generated.";
        return result;
    }

    const std::string lower = toLower(aql_query);

    result.structural_score   = scoreStructure(lower);
    result.completeness_score = scoreCompleteness(lower);
    result.schema_match_score = scoreSchemaMatch(lower, schema_context);

    result.has_required_keywords = containsFOR(lower) && lower.find("return") != std::string::npos;

    // Weighted combination driven by config
    result.overall_confidence = result.structural_score * config_.structural_weight
                                + result.completeness_score * config_.completeness_weight
                                + result.schema_match_score * config_.schema_match_weight;

    // Build human-readable reasoning
    std::ostringstream oss;
    oss << "structural=" << result.structural_score << " completeness=" << result.completeness_score
        << " schema_match=" << result.schema_match_score << " overall=" << result.overall_confidence;
    if (!result.has_required_keywords) {
        oss << " [WARNING: FOR or RETURN missing]";
    }
    result.reasoning = oss.str();

    return result;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

float AQLConfidenceScorer::scoreStructure(const std::string &aql_lower) const {
    // Required: FOR … IN … RETURN pattern
    bool has_for    = containsFOR(aql_lower);
    bool has_return = aql_lower.find("return") != std::string::npos;
    bool has_in     = aql_lower.find(" in ") != std::string::npos || aql_lower.find(" in\n") != std::string::npos
                      || aql_lower.find(" in\t") != std::string::npos;

    if (has_for && has_return) {
        // Full credit when FOR … IN … RETURN structure is present
        return has_in ? 1.0f : 0.85f;
    }
    if (has_for) {
        return 0.40f;
    }
    if (has_return) {
        return 0.25f;
    }
    return 0.0f;
}

float AQLConfidenceScorer::scoreCompleteness(const std::string &aql_lower) const {
    // Base credit for minimal FOR / RETURN structure (checked by structural scorer)
    float s = 0.40f;

    for (const auto &[kw, weight] : config_.keyword_bonuses) {
        if (containsKeyword(aql_lower, kw)) {
            s += weight;
        }
    }

    return std::min(s, 1.0f);
}

float AQLConfidenceScorer::scoreSchemaMatch(const std::string &aql_lower, const std::string &schema_context) const {
    if (schema_context.empty()) {
        return config_.no_schema_neutral; // Neutral: cannot evaluate without schema
    }

    auto collections = extractCollections(schema_context);
    if (collections.empty()) {
        return config_.no_schema_neutral;
    }

    int matched = 0;
    for (const auto &col : collections) {
        if (aql_lower.find(toLower(col)) != std::string::npos) {
            ++matched;
        }
    }

    if (matched == 0) {
        return config_.zero_match_floor; // No collection name matched
    }

    // Partial match still scores well; full match → 1.0
    float ratio = static_cast<float>(matched) / static_cast<float>(collections.size());
    return std::min(config_.no_schema_neutral + ratio * config_.no_schema_neutral, 1.0f);
}

std::vector<std::string> AQLConfidenceScorer::extractCollections(const std::string &schema_context) const {
    std::vector<std::string> collections;

    // Heuristic: lines of the form "  - <identifier>:" (common schema notation)
    std::istringstream stream(schema_context);
    std::string line;
    while (std::getline(stream, line)) {
        // Strip leading whitespace
        auto it = std::find_if(line.begin(), line.end(), [](unsigned char c) { return !std::isspace(c); });
        if (it == line.end()) {
            continue;
        }
        std::string stripped(it, line.end());

        if (stripped.size() > 2 && stripped[0] == '-' && stripped[1] == ' ') {
            std::string rest = stripped.substr(2);
            // Trim leading spaces after the dash
            rest.erase(rest.begin(),
                       std::find_if(rest.begin(), rest.end(), [](unsigned char c) { return !std::isspace(c); }));

            auto colon = rest.find(':');
            if (colon != std::string::npos) {
                std::string name = rest.substr(0, colon);
                // Trim trailing whitespace
                name.erase(
                    std::find_if(name.rbegin(), name.rend(), [](unsigned char c) { return !std::isspace(c); }).base(),
                    name.end());
                if (!name.empty()) {
                    collections.push_back(name);
                }
            }
        }
    }

    return collections;
}

std::string AQLConfidenceScorer::toLower(const std::string &text) {
    // Intentional copy: callers retain ownership of the original string
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

bool AQLConfidenceScorer::containsFOR(const std::string &aql_lower) {
    for (char sep : {' ', '\n', '\t', '('}) {
        if (aql_lower.find(std::string("for") + sep) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool AQLConfidenceScorer::containsKeyword(const std::string &aql_lower, const std::string &keyword) {
    // Search for all occurrences and verify word boundaries on each side.
    // A word boundary is a position where one side is an alphanumeric/underscore
    // character and the other is not (or is start/end of string).
    auto isWordChar = [](char c) -> bool { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };

    std::size_t pos = 0;
    while ((pos = aql_lower.find(keyword, pos)) != std::string::npos) {
        bool leftOk  = (pos == 0) || !isWordChar(aql_lower[pos - 1]);
        bool rightOk = (pos + keyword.size() >= aql_lower.size()) || !isWordChar(aql_lower[pos + keyword.size()]);
        if (leftOk && rightOk) {
            return true;
        }
        ++pos;
    }
    return false;
}

void AQLConfidenceScorer::calibrate(const std::vector<std::pair<std::string, float>> &labelled_pairs) {
    // Collect per-sample sub-scores and ground-truth values, skipping empty queries.
    std::vector<float> xs, ys, zs; // structural, completeness, schema sub-scores
    std::vector<float> targets;

    for (const auto &[query, truth] : labelled_pairs) {
        if (query.empty()) {
            continue;
        }
        const std::string lower = toLower(query);
        xs.push_back(scoreStructure(lower));
        ys.push_back(scoreCompleteness(lower));
        zs.push_back(scoreSchemaMatch(lower, ""));
        targets.push_back(truth);
    }

    const std::size_t n = targets.size();
    if (n < 3) {
        // Insufficient data: leave weights unchanged.
        return;
    }

    // Ordinary least-squares via normal equations for the model:
    //   y_hat = w0*xs[i] + w1*ys[i] + w2*zs[i]
    // Build X^T X (3×3) and X^T y (3×1).
    // Columns: [structural, completeness, schema]
    double XtX[3][3] = {};
    double Xty[3]    = {};

    for (std::size_t i = 0; i < n; ++i) {
        double row[3] = {xs[i], ys[i], zs[i]};
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                XtX[r][c] += row[r] * row[c];
            }
            Xty[r] += row[r] * targets[i];
        }
    }

    // Solve 3×3 system via Cramer's rule (small fixed size; no external deps).
    auto det3 = [](const double M[3][3]) -> double {
        return M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])
               + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
    };

    double D = det3(XtX);
    if (std::abs(D) < 1e-12) {
        // Singular matrix: leave weights unchanged.
        return;
    }

    double weights[3];
    for (int col = 0; col < 3; ++col) {
        double M[3][3];
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                M[r][c] = (c == col) ? Xty[r] : XtX[r][c];
            }
        }
        weights[col] = det3(M) / D;
    }

    // Clamp each weight to [0, 1] then normalise to sum = 1.
    for (double &w : weights) {
        w = std::max(0.0, std::min(1.0, w));
    }
    double sum = weights[0] + weights[1] + weights[2];
    if (sum < 1e-12) {
        // Degenerate result: leave weights unchanged.
        return;
    }
    config_.structural_weight   = static_cast<float>(weights[0] / sum);
    config_.completeness_weight = static_cast<float>(weights[1] / sum);
    config_.schema_match_weight = static_cast<float>(weights[2] / sum);
}

} // namespace aql
} // namespace themis
