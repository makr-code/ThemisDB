/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_expander.h                                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 10:58:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 1f4de1436  2026-02-21  Search module: v1.4.0 hardening + v1.5.0 feature set (7 n... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace themis {

/**
 * @brief Expanded query produced by QueryExpander::expand().
 */
struct ExpandedQuery {
    std::string original;                  ///< Original input query
    std::string corrected;                 ///< Spelling-corrected query (empty if no correction)
    std::vector<std::string> synonyms;     ///< Synonym terms added by expansion
    std::vector<std::string> relaxed_terms;///< Fallback tokens when query returns zero results
    std::vector<std::string> all_terms;    ///< Union of original tokens + synonyms
};

/**
 * @brief Query expansion, rewriting, spelling correction, and zero-result fallback.
 *
 * QueryExpander enriches a raw user query before it is handed to BM25 / hybrid
 * search.  It operates entirely in-memory using a configurable synonym dictionary
 * and a simple edit-distance spell-corrector against a word vocabulary.
 *
 * ### Usage
 * ```cpp
 * QueryExpander::Config cfg;
 * cfg.use_synonyms     = true;
 * cfg.correct_spelling = true;
 * cfg.max_expansions   = 5;
 *
 * QueryExpander expander(cfg);
 * expander.addSynonyms("ml", {"machine learning", "artificial intelligence"});
 * expander.addVocabulary({"machine", "learning", "algorithm", "database"});
 *
 * auto expanded = expander.expand("mashine lerning");
 * // expanded.corrected == "machine learning"
 * // expanded.synonyms contains "artificial intelligence"
 * ```
 *
 * @note Thread Safety: A single QueryExpander instance is NOT thread-safe.
 *   Callers sharing an instance across threads must synchronize externally.
 *   Building one instance per thread is the recommended pattern.
 *
 * @note Exception Safety: The constructor and all methods are noexcept-safe;
 *   no exceptions are thrown for bad input — methods return empty results instead.
 */
class QueryExpander {
public:
    struct Config {
        bool use_synonyms = true;        ///< Expand with synonym terms
        bool correct_spelling = true;    ///< Apply best-effort spelling correction
        bool detect_phrases = true;      ///< Preserve multi-word synonym phrases
        double synonym_weight = 0.8;     ///< Relative importance of synonym terms (informational)
        size_t max_expansions = 5;       ///< Maximum number of synonym terms to add
        int max_edit_distance = 2;       ///< Maximum edit distance for spelling correction
    };

    /**
     * @brief Construct a QueryExpander with the given config.
     * @throws std::invalid_argument if max_edit_distance < 0 or max_expansions == 0.
     */
    explicit QueryExpander(const Config& config = Config{});

    // Not copyable (synonym map can be large); movable
    QueryExpander(const QueryExpander&) = delete;
    QueryExpander& operator=(const QueryExpander&) = delete;
    QueryExpander(QueryExpander&&) = default;
    QueryExpander& operator=(QueryExpander&&) = default;

    // -----------------------------------------------------------------------
    // Dictionary management
    // -----------------------------------------------------------------------

    /**
     * @brief Register synonyms for a term.
     *
     * @param term      The canonical term (will be lowercased).
     * @param synonyms  Equivalent terms/phrases for `term`.
     */
    void addSynonyms(const std::string& term, const std::vector<std::string>& synonyms);

    /**
     * @brief Add words to the vocabulary used for spelling correction.
     *
     * Words already in the vocabulary are ignored (idempotent).
     */
    void addVocabulary(const std::vector<std::string>& words);

    // -----------------------------------------------------------------------
    // Core operations
    // -----------------------------------------------------------------------

    /**
     * @brief Expand a user query: tokenize, correct spelling, add synonyms.
     *
     * @param query  Raw user input.
     * @return Populated ExpandedQuery.  Never throws.
     */
    ExpandedQuery expand(const std::string& query) const;

    /**
     * @brief Return the best spelling correction for a single word.
     *
     * Searches the registered vocabulary for the closest word within
     * Config::max_edit_distance.  Returns the original word if no close
     * match is found.
     *
     * @param word  Single word to correct (must not contain spaces).
     * @return Corrected word, or `word` unchanged if no correction found.
     */
    std::string correctSpelling(const std::string& word) const;

    /**
     * @brief Suggest alternative phrasings for the whole query.
     *
     * Returns up to Config::max_expansions alternative queries formed by
     * replacing one token at a time with its synonyms.
     *
     * @param query  Original query string.
     * @return List of alternative query strings (may be empty).
     */
    std::vector<std::string> suggestAlternatives(const std::string& query) const;

    /**
     * @brief Build a relaxed (OR-logic) query for zero-result fallback.
     *
     * Drops the least-discriminative token from the query, returning a
     * shorter query that is more likely to match.
     *
     * @param query  Original query.
     * @return Relaxed query with one token fewer, or empty string for single-token input.
     */
    std::string relaxQuery(const std::string& query) const;

    const Config& getConfig() const { return config_; }

private:
    Config config_;

    /// term → list of synonym terms/phrases (keys are lowercase)
    std::unordered_map<std::string, std::vector<std::string>> synonyms_;

    /// vocabulary for spelling correction (lowercase)
    std::unordered_set<std::string> vocabulary_;

    // Helpers
    static std::vector<std::string> tokenize(const std::string& text);
    static std::string toLower(const std::string& s);
    static int editDistance(const std::string& a, const std::string& b);
};

} // namespace themis
