/**
 * @file fuzzy_matcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/secondary_index.h"
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Single fuzzy-match result.
 */
struct FuzzyMatch {
    std::string document_id;   ///< Primary key of the matched document
    std::string matched_token; ///< The index token that matched the query
    double score = 0.0;        ///< Similarity score in [0,1] (higher is better)
    int edit_distance = 0;     ///< Levenshtein distance to query token (0 = exact)
};

/**
 * @brief Enhanced fuzzy search with Levenshtein, Soundex, and Metaphone algorithms.
 *
 * FuzzyMatcher is a thin, algorithm-aware wrapper around
 * SecondaryIndexManager::scanFulltextFuzzy().  It adds:
 *  - Phonetic pre-filtering (Soundex / Metaphone) to reduce the candidate set
 *    before computing full edit distance.
 *  - N-gram token overlap scoring as a complementary similarity signal.
 *  - A unified, normalized [0,1] similarity score.
 *
 * ### Usage
 * ```cpp
 * FuzzyMatcher::Config cfg;
 * cfg.algorithm     = FuzzyMatcher::Algorithm::LEVENSHTEIN;
 * cfg.max_distance  = 2;
 *
 * FuzzyMatcher matcher(&secondary_index_mgr, cfg);
 * auto [status, matches] = matcher.search("douments", "docs", "body");
 * ```
 *
 * @note Thread Safety: A single FuzzyMatcher instance is NOT thread-safe.
 * @note Exception Safety: search() never throws; errors are returned via Status.
 */
class FuzzyMatcher {
public:
    enum class Algorithm {
        LEVENSHTEIN, ///< Edit-distance (default)
        SOUNDEX,     ///< Phonetic – English; supplements Levenshtein pre-filter
        METAPHONE,   ///< Phonetic – better vowel handling than Soundex
        NGRAM        ///< Bigram overlap score; useful for very short strings
    };

    struct Config {
        Algorithm algorithm = Algorithm::LEVENSHTEIN;
        int max_distance = 2;         ///< Maximum edit distance (Levenshtein) or min overlap
        size_t ngram_size = 2;        ///< N-gram size when algorithm == NGRAM
        bool phonetic_prefilter = false; ///< Apply phonetic pre-filter before edit distance
    };

    /**
     * @param index  Non-owning pointer to a SecondaryIndexManager.  Must outlive this.
     * @throws std::invalid_argument on invalid Config.
     */
    explicit FuzzyMatcher(SecondaryIndexManager* index);
    /**
     * @param index  Non-owning pointer to a SecondaryIndexManager.  Must outlive this.
     * @param config  Fuzzy search configuration.
     * @throws std::invalid_argument on invalid Config.
     */
    FuzzyMatcher(SecondaryIndexManager* index, const Config& config);

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Run fuzzy search on a fulltext-indexed column.
     *
     * @param query   Query string (one or more tokens).
     * @param table   Table name (must have a fulltext index on `column`).
     * @param column  Column name.
     * @param limit   Maximum number of results.
     * @return Pair of Status and list of FuzzyMatch, sorted by score descending.
     */
    std::pair<SecondaryIndexManager::Status, std::vector<FuzzyMatch>> search(
        const std::string& query,
        const std::string& table,
        const std::string& column,
        size_t limit = 100
    ) const;

    // -----------------------------------------------------------------------
    // Static algorithm utilities (public for testability)
    // -----------------------------------------------------------------------

    /**
     * @brief Compute Levenshtein edit distance between two strings.
     */
    static int levenshtein(const std::string& a, const std::string& b);

    /**
     * @brief Compute the Soundex code for a word (American Soundex).
     */
    static std::string soundex(const std::string& word);

    /**
     * @brief Compute a simple Metaphone code for a word.
     *
     * Implements a simplified (single) Metaphone that handles common English
     * consonant transformations.
     */
    static std::string metaphone(const std::string& word);

    /**
     * @brief Compute bigram (or n-gram) overlap similarity in [0,1].
     *
     * Uses the Dice coefficient: 2 * |intersection| / (|ngrams_a| + |ngrams_b|).
     *
     * @param a       First string.
     * @param b       Second string.
     * @param n       N-gram size (default 2).
     * @return Dice coefficient in [0,1].
     */
    static double ngramSimilarity(const std::string& a, const std::string& b, size_t n = 2);

    const Config& getConfig() const { return config_; }

private:
    SecondaryIndexManager* index_;
    Config config_;

    // Convert edit distance to a [0,1] similarity score
    static double distanceToScore(int distance, size_t query_len);
};

} // namespace themis
