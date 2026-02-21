/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_confidence_scorer.cpp                          ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 19:29:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     215                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "aql/aql_confidence_scorer.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace themis {
namespace aql {

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

AQLConfidenceScore AQLConfidenceScorer::score(
    const std::string& aql_query,
    const std::string& /*nl_query*/,
    const std::string& schema_context
) const {
    AQLConfidenceScore result;

    if (aql_query.empty()) {
        result.reasoning = "Empty query: no AQL was generated.";
        return result;
    }

    const std::string lower = toLower(aql_query);

    result.structural_score   = scoreStructure(lower);
    result.completeness_score = scoreCompleteness(lower);
    result.schema_match_score = scoreSchemaMatch(lower, schema_context);

    result.has_required_keywords =
        containsFOR(lower) && lower.find("return") != std::string::npos;

    // Weighted combination: structural 50 %, completeness 30 %, schema 20 %
    result.overall_confidence =
        result.structural_score   * 0.50f +
        result.completeness_score * 0.30f +
        result.schema_match_score * 0.20f;

    // Build human-readable reasoning
    std::ostringstream oss;
    oss << "structural=" << result.structural_score
        << " completeness=" << result.completeness_score
        << " schema_match=" << result.schema_match_score
        << " overall=" << result.overall_confidence;
    if (!result.has_required_keywords) {
        oss << " [WARNING: FOR or RETURN missing]";
    }
    result.reasoning = oss.str();

    return result;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

float AQLConfidenceScorer::scoreStructure(const std::string& aql_lower) const {
    // Required: FOR … IN … RETURN pattern
    bool has_for    = containsFOR(aql_lower);
    bool has_return = aql_lower.find("return") != std::string::npos;
    bool has_in     = aql_lower.find(" in ") != std::string::npos
                   || aql_lower.find(" in\n") != std::string::npos
                   || aql_lower.find(" in\t") != std::string::npos;

    if (has_for && has_return) {
        // Full credit when FOR … IN … RETURN structure is present
        return has_in ? 1.0f : 0.85f;
    }
    if (has_for)    return 0.40f;
    if (has_return) return 0.25f;
    return 0.0f;
}

float AQLConfidenceScorer::scoreCompleteness(const std::string& aql_lower) const {
    // Base credit for minimal FOR / RETURN structure (checked by structural scorer)
    float s = 0.40f;

    static const std::pair<const char*, float> KEYWORDS[] = {
        {"filter",  0.20f},
        {"sort ",   0.15f},
        {"limit ",  0.15f},
        {"let ",    0.10f},
        {"collect", 0.10f},
        {"insert",  0.10f},
        {"update",  0.10f},
        {"remove",  0.10f},
        {"upsert",  0.10f},
        {"graph",   0.10f},
    };

    for (const auto& [kw, weight] : KEYWORDS) {
        if (aql_lower.find(kw) != std::string::npos) {
            s += weight;
        }
    }

    return std::min(s, 1.0f);
}

float AQLConfidenceScorer::scoreSchemaMatch(
    const std::string& aql_lower,
    const std::string& schema_context
) const {
    if (schema_context.empty()) {
        return 0.5f; // Neutral: cannot evaluate without schema
    }

    auto collections = extractCollections(schema_context);
    if (collections.empty()) {
        return 0.5f;
    }

    int matched = 0;
    for (const auto& col : collections) {
        if (aql_lower.find(toLower(col)) != std::string::npos) {
            ++matched;
        }
    }

    if (matched == 0) {
        return 0.1f; // No collection name matched
    }

    // Partial match still scores well; full match → 1.0
    float ratio = static_cast<float>(matched) /
                  static_cast<float>(collections.size());
    return std::min(0.5f + ratio * 0.5f, 1.0f);
}

std::vector<std::string> AQLConfidenceScorer::extractCollections(
    const std::string& schema_context
) const {
    std::vector<std::string> collections;

    // Heuristic: lines of the form "  - <identifier>:" (common schema notation)
    std::istringstream stream(schema_context);
    std::string line;
    while (std::getline(stream, line)) {
        // Strip leading whitespace
        auto it = std::find_if(line.begin(), line.end(),
            [](unsigned char c) { return !std::isspace(c); });
        if (it == line.end()) continue;
        std::string stripped(it, line.end());

        if (stripped.size() > 2 && stripped[0] == '-' && stripped[1] == ' ') {
            std::string rest = stripped.substr(2);
            // Trim leading spaces after the dash
            rest.erase(rest.begin(),
                std::find_if(rest.begin(), rest.end(),
                    [](unsigned char c) { return !std::isspace(c); }));

            auto colon = rest.find(':');
            if (colon != std::string::npos) {
                std::string name = rest.substr(0, colon);
                // Trim trailing whitespace
                name.erase(
                    std::find_if(name.rbegin(), name.rend(),
                        [](unsigned char c) { return !std::isspace(c); }).base(),
                    name.end());
                if (!name.empty()) {
                    collections.push_back(name);
                }
            }
        }
    }

    return collections;
}

std::string AQLConfidenceScorer::toLower(const std::string& text) {
    // Intentional copy: callers retain ownership of the original string
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

bool AQLConfidenceScorer::containsFOR(const std::string& aql_lower) {
    for (char sep : {' ', '\n', '\t', '('}) {
        if (aql_lower.find(std::string("for") + sep) != std::string::npos)
            return true;
    }
    return false;
}

} // namespace aql
} // namespace themis
