#pragma once

#include <string>
#include <vector>

namespace themis {
namespace aql {

/**
 * @brief Multi-dimensional confidence score for a generated AQL query
 */
struct AQLConfidenceScore {
    /// Combined weighted confidence in [0.0, 1.0]
    float overall_confidence = 0.0f;

    /// Structural validity: presence of required keywords (FOR / RETURN) [0.0, 1.0]
    float structural_score = 0.0f;

    /// Completeness: usage of optional AQL keywords (FILTER, SORT, …) [0.0, 1.0]
    float completeness_score = 0.0f;

    /// Schema alignment: how many schema collections appear in the query [0.0, 1.0].
    /// Defaults to 0.5 when no schema context is provided.
    float schema_match_score = 0.5f;

    /// True when both FOR and RETURN are present in the query
    bool has_required_keywords = false;

    /// Human-readable explanation of the individual scores
    std::string reasoning;
};

/**
 * @brief Scores a generated AQL query without requiring a live LLM
 *
 * Analyses an AQL query string against known AQL syntax rules, an optional
 * database schema context, and the originating natural-language query to
 * produce a lightweight, deterministic confidence score.
 *
 * Scoring dimensions:
 *  - structural  (50 %): presence and ordering of FOR / IN / RETURN
 *  - completeness (30 %): optional keywords (FILTER, SORT, LIMIT, …)
 *  - schema match (20 %): collection names from schema_context appear in query
 */
class AQLConfidenceScorer {
public:
    AQLConfidenceScorer() = default;

    /**
     * @brief Score a generated AQL query
     * @param aql_query       The AQL query string to evaluate
     * @param nl_query        Original natural-language query (optional, unused in
     *                        current scoring but reserved for future alignment checks)
     * @param schema_context  Database schema description (optional).  When
     *                        provided, collection names are extracted and checked
     *                        against the query.
     * @return AQLConfidenceScore with per-dimension and overall scores
     */
    AQLConfidenceScore score(
        const std::string& aql_query,
        const std::string& nl_query = "",
        const std::string& schema_context = ""
    ) const;

private:
    /// Evaluate structural validity (FOR + IN + RETURN keywords)
    float scoreStructure(const std::string& aql_lower) const;

    /// Evaluate optional keyword completeness (FILTER, SORT, LIMIT, …)
    float scoreCompleteness(const std::string& aql_lower) const;

    /// Evaluate schema alignment by matching extracted collection names
    float scoreSchemaMatch(
        const std::string& aql_lower,
        const std::string& schema_context
    ) const;

    /// Extract collection identifiers from a schema context string
    std::vector<std::string> extractCollections(
        const std::string& schema_context
    ) const;

    /// Return true when the lowercase AQL text contains a FOR keyword
    /// (i.e. "for" followed by whitespace or a parenthesis)
    static bool containsFOR(const std::string& aql_lower);

    /// Return a lowercase copy of text
    static std::string toLower(const std::string& text);
};

} // namespace aql
} // namespace themis
