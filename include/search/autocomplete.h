/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            autocomplete.h                                     ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-15 04:12:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 48fbf5b222  2026-03-21  Update search, temporal, and build artifacts ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "index/secondary_index.h"
#include "search/search_analytics.h"
#include <string>
#include <vector>

namespace themis {

/**
 * @brief A single autocomplete suggestion.
 */
struct Suggestion {
    std::string text;       ///< Suggested completion text
    double score = 0.0;     ///< Relevance score (higher is better)
    bool is_popular = false;///< True if derived from popular-query history
};

/**
 * @brief Real-time query completion: prefix-based index suggestions, popular-query
 *        suggestions, and context-aware (recent-query) completion.
 *
 * AutocompleteEngine is a lightweight, index-backed suggestion engine.  It
 * supports three complementary suggestion strategies:
 *
 * 1. **Prefix suggestions** – scans a secondary index for field values that
 *    begin with the supplied prefix (`suggestByPrefix`).
 * 2. **Popular suggestions** – surfaces the most-frequent past queries whose
 *    text starts with the prefix, using a `SearchAnalytics` instance
 *    (`suggestPopular`).
 * 3. **Context-aware suggestions** – combines both sources, deduplicated and
 *    ranked (`suggest`).
 *
 * ### Usage
 * ```cpp
 * AutocompleteEngine::Config cfg;
 * cfg.max_suggestions = 10;
 *
 * AutocompleteEngine ac(&secondary_index, &analytics, cfg);
 *
 * // Get up to 10 suggestions for the prefix "data"
 * auto suggestions = ac.suggest("data", "products", "name");
 * for (const auto& s : suggestions) {
 *     std::cout << s.text << " (score=" << s.score << ")\n";
 * }
 * ```
 *
 * @note Thread Safety: A single AutocompleteEngine instance is NOT thread-safe.
 *   Callers sharing an instance across threads must synchronize externally.
 *
 * @note Exception Safety: All methods return empty/error results instead of
 *   throwing.  The constructor throws `std::invalid_argument` on bad config.
 */
class AutocompleteEngine {
public:
    struct Config {
        size_t max_suggestions = 10;       ///< Maximum completions returned
        size_t min_prefix_length = 1;      ///< Minimum prefix length to trigger completion
        double popular_boost = 1.5;        ///< Score multiplier for popular-query suggestions
        bool include_popular = true;       ///< Include popular-query suggestions
        bool include_prefix = true;        ///< Include prefix-index suggestions
        bool deduplicate = true;           ///< Remove duplicate suggestion texts
    };

    /**
     * @param index      Non-owning pointer to a SecondaryIndexManager (may be null if
     *                   only popular-query suggestions are needed).
     * @param analytics  Optional non-owning pointer to SearchAnalytics for popular
     *                   query sourcing (may be null).
     * @param config     Engine configuration.
     * @throws std::invalid_argument if max_suggestions == 0 or min_prefix_length == 0.
     */
    explicit AutocompleteEngine(SecondaryIndexManager* index,
                  SearchAnalytics* analytics = nullptr);
    AutocompleteEngine(SecondaryIndexManager* index,
               SearchAnalytics* analytics,
               const Config& config);

    // -----------------------------------------------------------------------
    // Core suggestion methods
    // -----------------------------------------------------------------------

    /**
     * @brief Return combined prefix + popular suggestions for the given prefix.
     *
     * Results are sorted by score descending, deduplicated (if Config::deduplicate),
     * and capped at Config::max_suggestions.
     *
     * @param prefix  Incomplete query text entered by the user.
     * @param table   Table name for prefix-index scan (ignored when index is null).
     * @param column  Column to scan for prefix matches.
     * @return Ranked suggestion list.
     */
    std::vector<Suggestion> suggest(const std::string& prefix,
                                     const std::string& table = "",
                                     const std::string& column = "") const;

    /**
     * @brief Return prefix-based suggestions by scanning a secondary index column.
     *
     * Uses `SecondaryIndexManager::scanKeysRange` with the prefix as lower bound
     * and `prefix + '\xff'` as upper bound to collect field values.
     *
     * @param prefix  Prefix to match.
     * @param table   Table name.
     * @param column  Column to scan.
     * @param limit   Maximum results.
     * @return List of matching field values as Suggestion objects.
     */
    std::vector<Suggestion> suggestByPrefix(const std::string& prefix,
                                             const std::string& table,
                                             const std::string& column,
                                             size_t limit = 20) const;

    /**
     * @brief Return popular past queries that start with the given prefix.
     *
     * Uses the SearchAnalytics instance (if non-null) to retrieve the most
     * frequent queries whose text begins with `prefix`.
     *
     * @param prefix  Prefix to match.
     * @param limit   Maximum results.
     * @return List of Suggestion objects with is_popular == true.
     */
    std::vector<Suggestion> suggestPopular(const std::string& prefix,
                                            size_t limit = 20) const;

    const Config& getConfig() const { return config_; }

private:
    SecondaryIndexManager* index_;
    SearchAnalytics* analytics_;
    Config config_;
};

} // namespace themis
