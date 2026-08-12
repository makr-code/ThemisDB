/**
 * @file entity_matcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Exact-key entity matcher for deterministic deduplication.
 *
 * Matches incoming entities against existing ThemisDB records using
 * primary-key values, unique-constraint field values, or custom identifier
 * mappings.  All matches return confidence = 1.0.
 *
 * Thread-safety: all public methods are stateless and safe to call
 * concurrently from multiple worker threads.
 */
class DeterministicMatcher {
public:
    /**
     * @brief Result of a single deterministic match attempt.
     */
    struct MatchResult {
        std::string existing_entity_id;      ///< UUID of the matched ThemisDB entity (empty if no match)
        double      confidence_score = 0.0;  ///< 1.0 for exact match, 0.0 if no match
        std::vector<std::string> match_keys; ///< Field names that produced the match
        json        evidence;                ///< Key/value pairs that were compared
    };

    DeterministicMatcher() = default;

    /**
     * @brief Find all exact matches across a collection for the given entity.
     *
     * Checks primary keys AND all unique fields.  May return multiple
     * MatchResult entries when different key sets each yield a match against
     * different existing entities (conflict situation).
     *
     * @param incoming_entity   Incoming JSON entity from the import source.
     * @param collection_name   Target ThemisDB collection name.
     * @param key_fields        Ordered list of field names to use as the key.
     * @return                  Zero or more match results (deduplicated by entity ID).
     */
    std::vector<MatchResult> findExactMatches(
        const json&                      incoming_entity,
        const std::string&               collection_name,
        const std::vector<std::string>&  key_fields
    ) const;

    /**
     * @brief Match a single entity by its primary-key value(s).
     *
     * @param incoming_entity   Incoming JSON entity.
     * @param collection_name   Target collection.
     * @return                  MatchResult with confidence 1.0 if found, 0.0 otherwise.
     */
    MatchResult findByPrimaryKey(
        const json&        incoming_entity,
        const std::string& collection_name
    ) const;

    /**
     * @brief Match an entity against unique-constraint field values.
     *
     * Each field in @p unique_field_names is checked independently.
     *
     * @param incoming_entity       Incoming JSON entity.
     * @param collection_name       Target collection.
     * @param unique_field_names    Names of fields with unique constraints.
     * @return                      One MatchResult per matching field (may be empty).
     */
    std::vector<MatchResult> findByUniqueFields(
        const json&                      incoming_entity,
        const std::string&               collection_name,
        const std::vector<std::string>&  unique_field_names
    ) const;

    /**
     * @brief Match using a custom identifier mapping between source and target field names.
     *
     * @param incoming_entity       Incoming JSON entity.
     * @param collection_name       Target collection.
     * @param identifier_mapping    JSON object {"source_field": "target_field", …}
     * @return                      MatchResult (confidence 1.0 if matched, 0.0 otherwise).
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

/**
 * @brief Configuration for semantic (fuzzy) entity matching.
 */
struct SemanticMatchConfig {
    double  overall_threshold    = 0.80;  ///< Minimum overall confidence to report a match
    size_t  max_results          = 10;    ///< Maximum number of candidate matches to return
    bool    use_embeddings       = false; ///< Enable vector-based similarity (requires embedding model)

    /// Per-field weight overrides.  Key = field name, value = weight in [0.0, 1.0].
    /// Weights are normalised to sum to 1.0 before scoring.
    std::map<std::string, double> field_weights;

    /// Per-field algorithm overrides: "jaro_winkler", "levenshtein", "soundex", "email", "phone"
    std::map<std::string, std::string> field_algorithms;
};

/**
 * @brief Per-field similarity score detail for a semantic match.
 */
struct SimilarityScore {
    std::string field_name;
    double      score  = 0.0;   ///< 0.0 – 1.0
    std::string method;          ///< Algorithm used ("jaro_winkler", "levenshtein", etc.)
    json        details;         ///< Optional extra evidence
};

/**
 * @brief Overall match score for a candidate existing entity.
 */
struct EntityMatchScore {
    std::string entity_id;           ///< ThemisDB entity UUID
    double      overall_confidence = 0.0;
    std::vector<SimilarityScore> field_scores;
    std::string confidence_level;    ///< "low" | "medium" | "high" | "very_high"
};

/**
 * @brief Fuzzy / semantic entity matcher.
 *
 * Provides string-distance metrics (Jaro-Winkler, Levenshtein, Soundex),
 * specialised matchers for names, emails, and phone numbers, and an
 * optional vector-embedding similarity path.
 *
 * Thread-safety: all public methods are stateless and safe to call
 * concurrently from multiple worker threads.
 */
class SemanticMatcher {
public:
    SemanticMatcher() = default;

    // -----------------------------------------------------------------------
    // String distance metrics
    // -----------------------------------------------------------------------

    /**
     * @brief Jaro-Winkler similarity between two strings.
     *
     * Returns a value in [0.0, 1.0].  Suitable for short strings and names.
     *
     * @param s1  First string.
     * @param s2  Second string.
     * @return    Similarity score (1.0 = identical).
     */
    static double jaroWinklerDistance(const std::string& s1, const std::string& s2);

    /**
     * @brief Levenshtein-based similarity: 1 – (edit_distance / max_length).
     *
     * Returns a value in [0.0, 1.0].  Handles typos well.
     *
     * @param s1  First string.
     * @param s2  Second string.
     * @return    Similarity score (1.0 = identical).
     */
    static double levenshteinSimilarity(const std::string& s1, const std::string& s2);

    // -----------------------------------------------------------------------
    // Specialised field matchers
    // -----------------------------------------------------------------------

    /**
     * @brief Normalise a full name to a canonical lower-case form.
     *
     * Handles "Alice SMITH", "Smith, Alice", "alice smith" → "alice smith".
     */
    static std::string normalizeFullName(const std::string& name);

    /**
     * @brief Soundex-based phonetic match score for two name strings.
     *
     * @return 1.0 if phonetic codes are equal, 0.5 for partial match, 0.0 otherwise.
     */
    static double soundexMatch(const std::string& name1, const std::string& name2);

    /**
     * @brief Score two name strings considering first/last-name reordering.
     */
    static double scoreNameVariations(const std::string& n1, const std::string& n2);

    /**
     * @brief Score two email addresses.
     *
     * Applies local-part similarity and requires identical domains for a
     * non-zero score.
     */
    static double scoreEmailPair(const std::string& e1, const std::string& e2);

    /**
     * @brief Determine whether two emails are likely to be a typo of each other.
     *
     * Returns true when Levenshtein edit distance ≤ 2 and domains are equal.
     */
    static bool isLikelyEmailTypo(const std::string& e1, const std::string& e2);

    /**
     * @brief Normalise a phone number to E.164-style digits only.
     *
     * Strips spaces, dashes, parentheses, and leading "+" / country codes.
     */
    static std::string normalizePhoneNumber(const std::string& phone);

    /**
     * @brief Score two phone number strings after normalisation.
     */
    static double scorePhonePair(const std::string& p1, const std::string& p2);

    // -----------------------------------------------------------------------
    // Vector similarity
    // -----------------------------------------------------------------------

    /**
     * @brief Compute cosine similarity between two embedding vectors.
     *
     * @param v1  First embedding.
     * @param v2  Second embedding.
     * @return    Cosine similarity in [−1.0, 1.0]; clamped to [0.0, 1.0].
     */
    static double vectorSimilarity(
        const std::vector<float>& v1,
        const std::vector<float>& v2
    );

    // -----------------------------------------------------------------------
    // Main scoring engine
    // -----------------------------------------------------------------------

    /**
     * @brief Compute an overall similarity score between two entities.
     *
     * Each field present in both entities is scored with the algorithm
     * configured in @p config.  Per-field scores are combined using the
     * configured weights.
     *
     * @param incoming_entity   Entity from the import source.
     * @param existing_entity   Entity stored in ThemisDB.
     * @param collection_name   Collection being imported into.
     * @param config            Semantic matching configuration.
     * @return                  Detailed match score.
     */
    EntityMatchScore scoreEntityMatch(
        const json&               incoming_entity,
        const json&               existing_entity,
        const std::string&        collection_name,
        const SemanticMatchConfig& config
    ) const;

    /**
     * @brief Find all entities in a collection that are similar to the incoming entity.
     *
     * This is a reference implementation that scans a provided @p candidates
     * list.  Production deployments should replace this with an index-backed
     * lookup.
     *
     * @param incoming_entity       Entity from the import source.
     * @param candidates            Candidate existing entities to score.
     * @param config                Semantic matching configuration.
     * @return                      Sorted (descending by confidence) match results.
     */
    std::vector<EntityMatchScore> findSimilarEntities(
        const json&               incoming_entity,
        const std::vector<json>&  candidates,
        const SemanticMatchConfig& config
    ) const;

private:
    static std::string computeSoundex(const std::string& name);
    static size_t      levenshteinDistance(const std::string& s1, const std::string& s2);
    static double      jaroSimilarity(const std::string& s1, const std::string& s2);
};

// ============================================================================
// Hybrid Matcher – combines deterministic and semantic strategies
// ============================================================================

/**
 * @brief Characteristics of a field used to select the optimal match strategy.
 */
struct FieldCharacteristics {
    std::string name;
    std::string type;            ///< "email", "phone", "name", "numeric", "text", "id"
    double      uniqueness_ratio = 0.0;  ///< 0.0–1.0: fraction of distinct values in collection
    bool        is_primary_key   = false;
    bool        is_unique        = false;
};

/**
 * @brief Combined result from the hybrid matcher.
 */
struct HybridMatchResult {
    std::string entity_id;
    double      deterministic_score = 0.0;
    double      semantic_score      = 0.0;
    double      hybrid_score        = 0.0;   ///< Weighted combination
    std::string match_method;                 ///< "deterministic" | "semantic" | "ensemble"
    json        confidence_evidence;
};

/**
 * @brief Hybrid entity matcher that combines deterministic and semantic strategies.
 *
 * Supports three execution modes:
 *   - DETERMINISTIC_FIRST: exact matching first; fall back to semantic if no match.
 *   - SEMANTIC_FIRST:      semantic scoring first; exact keys used for confirmation.
 *   - WEIGHTED_ENSEMBLE:   both strategies run in parallel; scores are weighted.
 *
 * Thread-safety: all public methods are stateless.
 */
class HybridEntityMatcher {
public:
    enum class MatchStrategy {
        DETERMINISTIC_FIRST,  ///< Exact match first, then fuzzy fallback
        SEMANTIC_FIRST,       ///< Fuzzy first, then exact confirmation
        WEIGHTED_ENSEMBLE     ///< Both in parallel, weighted combination
    };

    HybridEntityMatcher() = default;

    /**
     * @brief Find matching existing entities using the specified strategy.
     *
     * @param incoming_entity   Entity from the import source.
     * @param existing_entities All existing entities to match against.
     * @param key_fields        Fields used for deterministic matching.
     * @param strategy          Matching strategy.
     * @param sem_config        Semantic matching configuration.
     * @param threshold         Minimum hybrid score to include a result.
     * @return                  Candidates sorted descending by hybrid_score.
     */
    std::vector<HybridMatchResult> findMatchingEntities(
        const json&               incoming_entity,
        const std::vector<json>&  existing_entities,
        const std::vector<std::string>& key_fields,
        MatchStrategy             strategy,
        const SemanticMatchConfig& sem_config,
        double                    threshold = 0.85
    ) const;

    /**
     * @brief Automatically select the best strategy for a collection.
     *
     * Uses field characteristics (uniqueness, type, key flags) to heuristically
     * choose between DETERMINISTIC_FIRST, SEMANTIC_FIRST, and WEIGHTED_ENSEMBLE.
     *
     * @param field_stats  Per-field characteristics for the collection.
     * @return             Recommended strategy.
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
