/**
 * @file negative_keyword_filter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
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
 * @brief Negative keyword filtering for the search pipeline (`NOT` operator).
 *
 * `NegativeKeywordFilter` implements the `NOT` / minus-prefix operator for
 * full-text search queries, enabling callers to exclude documents that contain
 * specific terms from their search results.
 *
 * ### Supported query syntax
 *
 * | Syntax              | Example                             | Meaning                        |
 * |---------------------|-------------------------------------|--------------------------------|
 * | Minus prefix (`-`)  | `"machine learning -neural"`        | Search for "machine learning", exclude "neural" |
 * | `NOT` keyword       | `"machine learning NOT neural"`     | Equivalent to minus prefix     |
 * | Mixed               | `"database -slow NOT crash"`        | Exclude "slow" and "crash"     |
 * | Multiple            | `"python -java -c++"`               | Exclude "java" and "c"         |
 *
 * ### Typical usage in a search pipeline
 * ```cpp
 * // 1. Parse the raw user query
 * auto pq = NegativeKeywordFilter::parseQuery("machine learning -neural");
 * // pq.positive_query == "machine learning"
 * // pq.negative_terms == {"neural"}
 *
 * // 2. Run the normal search on the positive part
 * auto results = hybrid_search.search(pq.positive_query, ...);
 *
 * // 3. Collect result PKs
 * std::vector<std::string> pks;
 * for (auto& r : results) pks.push_back(r.document_id);
 *
 * // 4. Apply the NOT filter
 * NegativeKeywordFilter nkf(&sec_index);
 * auto [status, filtered_pks] = nkf.filter(
 *     "documents", "content", pks, pq.negative_terms);
 *
 * // 5. Retain only results whose PK survived the filter
 * std::unordered_set<std::string> keep(filtered_pks.begin(), filtered_pks.end());
 * results.erase(
 *     std::remove_if(results.begin(), results.end(),
 *         [&keep](const auto& r){ return keep.find(r.document_id) == keep.end(); }),
 *     results.end());
 * ```
 *
 * ### Null-index safety
 * When constructed with a null SecondaryIndexManager pointer, `filter()` returns
 * an error Status and the original `candidate_pks` unchanged — the caller can
 * choose whether to treat this as a fatal or degraded-mode condition.
 *
 * @note Thread Safety: `NegativeKeywordFilter` is itself stateless (it holds only a
 *   non-owning pointer to the SecondaryIndexManager).  `parseQuery()` is a pure
 *   static function and is fully thread-safe.  `filter()` is safe to call from
 *   multiple threads as long as the underlying SecondaryIndexManager is also
 *   thread-safe for concurrent reads — consult its documentation for details.
 *   The standard SecondaryIndexManager implementation does not guarantee concurrent
 *   read safety, so external synchronization is required if filter() is called
 *   from multiple threads on the same manager instance.
 * @note Exception Safety: `filter()` and `parseQuery()` never throw.
 *
 * v2.2.0 Feature: initial delivery (Issue #2003).
 */
class NegativeKeywordFilter {
public:
    /**
     * @brief Configuration for NegativeKeywordFilter.
     */
    struct Config {
        /// Maximum number of documents fetched per negative term when scanning
        /// the secondary index.  0 means no limit.  Default 100'000.
        ///
        /// For common terms this bound prevents unbounded memory use, but it
        /// may result in *incomplete* exclusion when a negative term matches
        /// more documents than `max_exclude_scan`.  Set to 0 to disable the
        /// limit and guarantee complete exclusion at the cost of higher memory.
        size_t max_exclude_scan = 100'000;
    };
    /**
     * @brief A query split into a positive search string and excluded terms.
     *
     * Produced by `parseQuery()`.  Callers use `positive_query` for the
     * underlying BM25 / hybrid search call, and `negative_terms` for the
     * subsequent `filter()` call.
     */
    struct ParsedQuery {
        std::string positive_query;             ///< Query without negated tokens
        std::vector<std::string> negative_terms;///< Lower-case terms to exclude
    };

    /**
     * @brief Construct with a (possibly null) secondary index.
     *
     * @param index  Non-owning pointer to the SecondaryIndexManager used to
     *               look up documents containing excluded terms.  May be null;
     *               all filter() calls return an error in that case.
     */
    explicit NegativeKeywordFilter(SecondaryIndexManager* index = nullptr);
    /**
     * @brief Construct with a (possibly null) secondary index and optional config.
     *
     * @param index  Non-owning pointer to the SecondaryIndexManager used to
     *               look up documents containing excluded terms.  May be null;
     *               all filter() calls return an error in that case.
     * @param config Engine configuration.
     */
    NegativeKeywordFilter(SecondaryIndexManager* index,
          const Config& config);

    // -----------------------------------------------------------------------
    // Static helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Parse a raw query string for positive and negative terms.
     *
     * Splits the query on whitespace.  A token is treated as a negative term
     * when it starts with a minus (`-`) or when the previous token was the
     * keyword `NOT` (case-insensitive).  All other tokens form the positive
     * query.
     *
     * Minus tokens must be at least two characters long (e.g. `-word`); a
     * lone `-` is treated as a regular positive token.
     *
     * `NOT` is consumed as an operator and does not appear in the positive
     * query or in the negative terms list itself.
     *
     * Examples:
     * - `"machine learning -neural"` →
     *   `{ positive: "machine learning", negatives: ["neural"] }`
     * - `"database NOT crash NOT slow"` →
     *   `{ positive: "database", negatives: ["crash", "slow"] }`
     * - `"search -engine NOT index"` →
     *   `{ positive: "search", negatives: ["engine", "index"] }`
     * - `"hello"` →
     *   `{ positive: "hello", negatives: [] }`
     *
     * @param raw_query  User-supplied query string (UTF-8).
     * @return ParsedQuery with `positive_query` and `negative_terms`.
     */
    static ParsedQuery parseQuery(const std::string& raw_query);

    // -----------------------------------------------------------------------
    // Filter operation
    // -----------------------------------------------------------------------

    /**
     * @brief Remove from `candidate_pks` any document that contains an excluded term.
     *
     * For each term in `negative_terms`, looks up the set of documents in
     * `table.column` that contain that term via the secondary index, then
     * removes those documents from `candidate_pks`.
     *
     * An empty `negative_terms` list returns all `candidate_pks` unchanged
     * with an OK Status.
     *
     * @param table           Table name.
     * @param column          Fulltext-indexed column to search for excluded terms.
     * @param candidate_pks   PKs to filter (the search results before filtering).
     * @param negative_terms  Lower-case terms that must NOT appear in matching docs.
     * @return Pair of Status and the filtered PK list.  On index errors the
     *         Status is Error and the returned vector may be a partial result
     *         (terms that were successfully looked up are still filtered).
     */
    std::pair<SecondaryIndexManager::Status, std::vector<std::string>> filter(
        const std::string& table,
        const std::string& column,
        const std::vector<std::string>& candidate_pks,
        const std::vector<std::string>& negative_terms
    ) const;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    SecondaryIndexManager* getIndex() const { return index_; }
    const Config& getConfig() const { return config_; }

private:
    SecondaryIndexManager* index_;
    Config config_;
};

} // namespace themis
