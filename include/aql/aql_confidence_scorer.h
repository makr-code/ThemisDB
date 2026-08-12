/**
 * @file aql_confidence_scorer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
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
 *
 * All weights and bonuses are runtime-configurable via @ref Config.
 */
class AQLConfidenceScorer {
public:
    /**
     * @brief Runtime-configurable scoring weights and constants.
     *
     * Default values replicate the original hard-coded behaviour so that
     * existing code using the default constructor is unaffected.
     */
    struct Config {
        /// Weight of the structural dimension (default 0.50)
        float structural_weight   = 0.50f;
        /// Weight of the completeness dimension (default 0.30)
        float completeness_weight = 0.30f;
        /// Weight of the schema-match dimension (default 0.20)
        float schema_match_weight = 0.20f;

        /**
         * @brief Per-keyword completeness bonuses.
         *
         * Maps lowercase AQL keyword (whole-word) to bonus score added
         * when the keyword appears in the query.  Default entries replicate
         * the original static table.
         */
        std::unordered_map<std::string, float> keyword_bonuses = {
            {"filter",  0.20f},
            {"sort",    0.15f},
            {"limit",   0.15f},
            {"let",     0.10f},
            {"collect", 0.10f},
            {"insert",  0.10f},
            {"update",  0.10f},
            {"remove",  0.10f},
            {"upsert",  0.10f},
            {"graph",   0.10f},
        };

        /// Schema-match score returned when no schema context is provided (default 0.5)
        float no_schema_neutral = 0.5f;
        /// Schema-match score returned when none of the schema collections match (default 0.1)
        float zero_match_floor  = 0.1f;
    };

    /// Default constructor – uses default @ref Config (backward-compatible behaviour)
    AQLConfidenceScorer() = default;

    /// Construct with a custom configuration
    explicit AQLConfidenceScorer(Config config) : config_(std::move(config)) {}

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

    /**
     * @brief Fit the three top-level scoring weights via least-squares regression.
     *
     * Given a set of (query, ground-truth-confidence) labelled pairs, this
     * method adjusts @c structural_weight, @c completeness_weight, and
     * @c schema_match_weight so that the predicted overall confidence
     * best matches the supplied ground-truth values in a least-squares sense.
     *
     * The three weights are normalised to sum to 1.0 and clamped to [0, 1]
     * after fitting.  All other @ref Config fields remain unchanged.
     *
     * @param labelled_pairs  Vector of (aql_query, ground_truth_confidence) pairs.
     *                        Pairs with empty query strings are ignored.
     *                        At least three non-degenerate samples are required;
     *                        if fewer are available the weights are left unchanged.
     */
    void calibrate(const std::vector<std::pair<std::string, float>>& labelled_pairs);

    /// Read-only access to the current configuration
    const Config& config() const { return config_; }

private:
    Config config_;

    /// Evaluate structural validity (FOR + IN + RETURN keywords)
    float scoreStructure(const std::string& aql_lower) const;

    /// Evaluate optional keyword completeness using config_.keyword_bonuses
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

    /// Return true when @p keyword appears as a whole word in @p aql_lower
    static bool containsKeyword(const std::string& aql_lower, const std::string& keyword);

    /// Return a lowercase copy of text
    static std::string toLower(const std::string& text);
};

} // namespace aql
} // namespace themis
