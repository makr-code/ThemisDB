/**
 * @file aql_confidence_scorer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace aql {

struct AQLConfidenceScore {
    float overall_confidence = 0.0f;

    float structural_score = 0.0f;

    float completeness_score = 0.0f;

    float schema_match_score = 0.5f;

    bool has_required_keywords = false;

    std::string reasoning;
};

class AQLConfidenceScorer {
public:
    struct Config {
        float structural_weight   = 0.50f;
        float completeness_weight = 0.30f;
        float schema_match_weight = 0.20f;

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

        float no_schema_neutral = 0.5f;
        float zero_match_floor  = 0.1f;
    };

    AQLConfidenceScorer() = default;

    explicit AQLConfidenceScorer(Config config) : config_(std::move(config)) {}

    AQLConfidenceScore score(
        const std::string& aql_query,
        const std::string& nl_query = "",
        const std::string& schema_context = ""
    ) const;

    void calibrate(const std::vector<std::pair<std::string, float>>& labelled_pairs);

    const Config& config() const { return config_; }

private:
    Config config_;

    /**
     * @brief Score Structure.
     * @param[in] aql_lower Input parameter.
     * @return Return value.
     */
    float scoreStructure(const std::string& aql_lower) const;

    /**
     * @brief Score Completeness.
     * @param[in] aql_lower Input parameter.
     * @return Return value.
     */
    float scoreCompleteness(const std::string& aql_lower) const;

    /**
     * @brief Score Schema Match.
     * @param[in] aql_lower Input parameter.
     * @param[in] schema_context Input parameter.
     * @return Return value.
     */
    float scoreSchemaMatch(
        const std::string& aql_lower,
        const std::string& schema_context
    ) const;

    /**
     * @brief Extract Collections.
     * @param[in] schema_context Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractCollections(
        const std::string& schema_context
    ) const;

    /**
     * @brief Contains FOR.
     * @param[in] aql_lower Input parameter.
     * @return True when the operation succeeds.
     */
    static bool containsFOR(const std::string& aql_lower);

    /**
     * @brief Contains Keyword.
     * @param[in] aql_lower Input parameter.
     * @param[in] keyword Input parameter.
     * @return True when the operation succeeds.
     */
    static bool containsKeyword(const std::string& aql_lower, const std::string& keyword);

    /**
     * @brief To Lower.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::string toLower(const std::string& text);
};

} // namespace aql
} // namespace themis
