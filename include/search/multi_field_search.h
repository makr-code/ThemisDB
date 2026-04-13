/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_field_search.h                               ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:19:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     176                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 48fbf5b222  2026-03-21  Update search, temporal, and build artifacts ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5aa6ea1324  2026-02-28  fix(search): promote normalizeScores to public static, ad... ║
    • bf06f54176  2026-02-28  feat(search): implement MultiFieldBoostedSearch for title... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "index/secondary_index.h"
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Multi-field boosted full-text search with per-field boost weights.
 *
 * Searches a query across multiple fields (e.g. title, body, tags) using
 * BM25 scoring, applies a configurable boost weight to each field's scores,
 * and returns documents ranked by their combined weighted score.
 *
 * The default boost ordering follows the "title > body > tags" precedence
 * (title_boost=3.0, body_boost=1.0, tags_boost=0.5).
 *
 * ### Score combination
 * For each document that appears in one or more field result lists the final
 * score is:
 * ```
 * score(doc) = sum_f( boost_f * normalized_bm25_score_f(doc) )
 * ```
 * where `normalized_bm25_score_f` is the BM25 score for field `f` linearly
 * rescaled to [0, 1] across all candidates returned for that field.
 *
 * ### Usage
 * ```cpp
 * MultiFieldBoostedSearch::Config cfg;
 * cfg.k = 10;
 *
 * MultiFieldBoostedSearch mfs(&sec_index, cfg);
 *
 * auto fields = MultiFieldBoostedSearch::defaultFields("articles");
 * auto results = mfs.search("database engine", fields);
 * for (auto& r : results) {
 *     std::cout << r.document_id << " score=" << r.score << "\n";
 * }
 * ```
 *
 * @note Thread Safety: A single instance is NOT thread-safe. Callers that
 *   share an instance across threads must provide their own synchronization.
 * @note Exception Safety: `search()` never throws; all exceptions from the
 *   index backend are caught internally and result in an empty or partial
 *   result list.
 */
class MultiFieldBoostedSearch {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Configuration for a single searchable field.
     */
    struct FieldConfig {
        std::string table;   ///< Table containing this field
        std::string column;  ///< Column (field) name to search
        double boost = 1.0;  ///< Boost multiplier (higher = more important)
    };

    /**
     * @brief Engine-level configuration.
     */
    struct Config {
        size_t k = 10;                    ///< Maximum results to return
        size_t candidates_per_field = 100; ///< BM25 candidates fetched per field
    };

    /**
     * @brief A single search result with the combined boosted score.
     */
    struct Result {
        std::string document_id;                            ///< Primary key
        double score = 0.0;                                 ///< Combined boosted score
        std::vector<std::pair<std::string, double>> field_scores; ///< Per-field (column, score) pairs
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @param index   Non-owning pointer to a SecondaryIndexManager. May be null
     *                (all searches will return empty results).
     * @param config  Engine configuration.
     * @throws std::invalid_argument on invalid config.
     */
    explicit MultiFieldBoostedSearch(SecondaryIndexManager* index);
    MultiFieldBoostedSearch(SecondaryIndexManager* index,
                const Config& config);

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Execute a boosted multi-field search.
     *
     * For each `FieldConfig` in @p fields, runs a BM25 fulltext query,
     * normalizes the scores to [0, 1], multiplies by the field's boost weight,
     * and accumulates into a per-document combined score.  Documents that do
     * not appear in a particular field's results contribute 0.0 for that field.
     *
     * @param query   Text query string.  Empty query returns empty results.
     * @param fields  Ordered list of fields to search with their boost weights.
     *                Empty list returns empty results.
     * @return Results sorted by combined score descending, limited to Config::k.
     */
    std::vector<Result> search(const std::string& query,
                               const std::vector<FieldConfig>& fields) const;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Build the default title/body/tags field list for a table.
     *
     * Returns three FieldConfig entries:
     *  - `<table>.title`   boost = 3.0
     *  - `<table>.body`    boost = 1.0
     *  - `<table>.tags`    boost = 0.5
     *
     * @param table  Table name.
     * @return Default field list ordered by descending boost.
     */
    static std::vector<FieldConfig> defaultFields(const std::string& table);

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

    /**
     * @brief Normalize a list of (doc_id, raw_score) pairs to [0, 1] in place.
     *
     * When all scores are equal (range == 0):
     *  - score > 0 → all normalized to 1.0
     *  - score == 0 → all normalized to 0.0
     *
     * Promoted to public static for direct unit testing (same pattern as
     * HybridSearch::normalizeScores).
     */
    static void normalizeScores(std::vector<std::pair<std::string, double>>& scored);

private:
    SecondaryIndexManager* index_;
    Config config_;
};

} // namespace themis
