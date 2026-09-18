/**
 * @file entity_matcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

// ============================================================================
// Deterministic Matcher – exact key / unique constraint matching
// ============================================================================

class DeterministicMatcher {
public:
    struct MatchResult {
        std::string existing_entity_id;      ///< UUID of the matched ThemisDB entity (empty if no match)
        double      confidence_score = 0.0;  ///< 1.0 for exact match, 0.0 if no match
        std::vector<std::string> match_keys; ///< Field names that produced the match
        json        evidence;                ///< Key/value pairs that were compared
    };

    DeterministicMatcher() = default;

    /**
     * @brief Find Exact Matches.
     * @param[in] incoming_entity Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] key_fields Input parameter.
     * @return Return value.
     */
    std::vector<MatchResult> findExactMatches(
        const json&                      incoming_entity,
        const std::string&               collection_name,
        const std::vector<std::string>&  key_fields
    ) const;

    /**
     * @brief Find By Primary Key.
     * @param[in] incoming_entity Input parameter.
     * @param[in] collection_name Name of the collection.
     * @return Return value.
     */
    MatchResult findByPrimaryKey(
        const json&        incoming_entity,
        const std::string& collection_name
    ) const;

    /**
     * @brief Find By Unique Fields.
     * @param[in] incoming_entity Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] unique_field_names Input parameter.
     * @return Return value.
     */
    std::vector<MatchResult> findByUniqueFields(
        const json&                      incoming_entity,
        const std::string&               collection_name,
        const std::vector<std::string>&  unique_field_names
    ) const;

    /**
     * @brief Find By Custom Identifier.
     * @param[in] incoming_entity Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] identifier_mapping Input parameter.
     * @return Return value.
     */
    MatchResult findByCustomIdentifier(
        const json&        incoming_entity,
        const std::string& collection_name,
        const json&        identifier_mapping
    ) const;
};

// ============================================================================
// Semantic Matcher – fuzzy string distance & vector-based similarity
// ============================================================================

struct SemanticMatchConfig {
    double  overall_threshold    = 0.80;  ///< Minimum overall confidence to report a match
    size_t  max_results          = 10;    ///< Maximum number of candidate matches to return
    bool    use_embeddings       = false; ///< Enable vector-based similarity (requires embedding model)

    std::map<std::string, double> field_weights;

    std::map<std::string, std::string> field_algorithms;
};

struct SimilarityScore {
    std::string field_name;
    double      score  = 0.0;   ///< 0.0 – 1.0
    std::string method;          ///< Algorithm used ("jaro_winkler", "levenshtein", etc.)
    json        details;         ///< Optional extra evidence
};

struct EntityMatchScore {
    std::string entity_id;           ///< ThemisDB entity UUID
    double      overall_confidence = 0.0;
    std::vector<SimilarityScore> field_scores;
    std::string confidence_level;    ///< "low" | "medium" | "high" | "very_high"
};

class SemanticMatcher {
public:
    SemanticMatcher() = default;

    // -----------------------------------------------------------------------
    // String distance metrics
    // -----------------------------------------------------------------------

    /**
     * @brief Jaro Winkler Distance.
     * @param[in] s1 Input parameter.
     * @param[in] s2 Input parameter.
     * @return Return value.
     */
    static double jaroWinklerDistance(const std::string& s1, const std::string& s2);

    /**
     * @brief Levenshtein Similarity.
     * @param[in] s1 Input parameter.
     * @param[in] s2 Input parameter.
     * @return Return value.
     */
    static double levenshteinSimilarity(const std::string& s1, const std::string& s2);

    // -----------------------------------------------------------------------
    // Specialised field matchers
    // -----------------------------------------------------------------------

    /**
     * @brief Normalize Full Name.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    static std::string normalizeFullName(const std::string& name);

    /**
     * @brief Soundex Match.
     * @param[in] name1 Input parameter.
     * @param[in] name2 Input parameter.
     * @return Return value.
     */
    static double soundexMatch(const std::string& name1, const std::string& name2);

    /**
     * @brief Score Name Variations.
     * @param[in] n1 Input parameter.
     * @param[in] n2 Input parameter.
     * @return Return value.
     */
    static double scoreNameVariations(const std::string& n1, const std::string& n2);

    /**
     * @brief Score Email Pair.
     * @param[in] e1 Input parameter.
     * @param[in] e2 Input parameter.
     * @return Return value.
     */
    static double scoreEmailPair(const std::string& e1, const std::string& e2);

    /**
     * @brief Is Likely Email Typo.
     * @param[in] e1 Input parameter.
     * @param[in] e2 Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isLikelyEmailTypo(const std::string& e1, const std::string& e2);

    /**
     * @brief Normalize Phone Number.
     * @param[in] phone Input parameter.
     * @return Return value.
     */
    static std::string normalizePhoneNumber(const std::string& phone);

    /**
     * @brief Score Phone Pair.
     * @param[in] p1 Input parameter.
     * @param[in] p2 Input parameter.
     * @return Return value.
     */
    static double scorePhonePair(const std::string& p1, const std::string& p2);

    // -----------------------------------------------------------------------
    // Vector similarity
    // -----------------------------------------------------------------------

    /**
     * @brief Vector Similarity.
     * @param[in] v1 Input parameter.
     * @param[in] v2 Input parameter.
     * @return Return value.
     */
    static double vectorSimilarity(
        const std::vector<float>& v1,
        const std::vector<float>& v2
    );

    // -----------------------------------------------------------------------
    // Main scoring engine
    // -----------------------------------------------------------------------

    /**
     * @brief Score Entity Match.
     * @param[in] incoming_entity Input parameter.
     * @param[in] existing_entity Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    EntityMatchScore scoreEntityMatch(
        const json&               incoming_entity,
        const json&               existing_entity,
        const std::string&        collection_name,
        const SemanticMatchConfig& config
    ) const;

    /**
     * @brief Find Similar Entities.
     * @param[in] incoming_entity Input parameter.
     * @param[in] candidates Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    std::vector<EntityMatchScore> findSimilarEntities(
        const json&               incoming_entity,
        const std::vector<json>&  candidates,
        const SemanticMatchConfig& config
    ) const;

private:
    /**
     * @brief Compute Soundex.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    static std::string computeSoundex(const std::string& name);
    /**
     * @brief Levenshtein Distance.
     * @param[in] s1 Input parameter.
     * @param[in] s2 Input parameter.
     * @return Return value.
     */
    static size_t      levenshteinDistance(const std::string& s1, const std::string& s2);
    /**
     * @brief Jaro Similarity.
     * @param[in] s1 Input parameter.
     * @param[in] s2 Input parameter.
     * @return Return value.
     */
    static double      jaroSimilarity(const std::string& s1, const std::string& s2);
};

// ============================================================================
// Hybrid Matcher – combines deterministic and semantic strategies
// ============================================================================

struct FieldCharacteristics {
    std::string name = {};
    std::string type;            ///< "email", "phone", "name", "numeric", "text", "id"
    double      uniqueness_ratio = 0.0;  ///< 0.0–1.0: fraction of distinct values in collection
    bool        is_primary_key   = false;
    bool        is_unique        = false;
};

struct HybridMatchResult {
    std::string entity_id;
    double      deterministic_score = 0.0;
    double      semantic_score      = 0.0;
    double      hybrid_score        = 0.0;   ///< Weighted combination
    std::string match_method;                 ///< "deterministic" | "semantic" | "ensemble"
    json        confidence_evidence;
};

class HybridEntityMatcher {
public:
    enum class MatchStrategy {
        DETERMINISTIC_FIRST,  ///< Exact match first, then fuzzy fallback
        SEMANTIC_FIRST,       ///< Fuzzy first, then exact confirmation
        WEIGHTED_ENSEMBLE     ///< Both in parallel, weighted combination
    };

    HybridEntityMatcher() = default;

    std::vector<HybridMatchResult> findMatchingEntities(
        const json&               incoming_entity,
        const std::vector<json>&  existing_entities,
        const std::vector<std::string>& key_fields,
        MatchStrategy             strategy,
        const SemanticMatchConfig& sem_config,
        double                    threshold = 0.85
    ) const;

    /**
     * @brief Select Optimal Strategy.
     * @param[in] field_stats Input parameter.
     * @return Return value.
     */
    static MatchStrategy selectOptimalStrategy(
        const std::vector<FieldCharacteristics>& field_stats
    );

private:
    DeterministicMatcher det_matcher_;
    SemanticMatcher       sem_matcher_;
};

} // namespace importers
} // namespace themis
