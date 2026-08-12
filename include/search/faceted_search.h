/**
 * @file faceted_search.h
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
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace themis {

/**
 * @brief Result of a single facet computation.
 */
struct FacetResult {
    std::string field;                         ///< Field/column name this facet covers
    std::map<std::string, size_t> value_counts;///< value → number of documents with that value
    size_t total_docs = 0;                     ///< Total number of documents scanned
};

/**
 * @brief Multi-dimensional facet computation for drill-down navigation.
 *
 * FacetedSearch reads a secondary index to compute per-field value counts,
 * range bucket counts, and supports drill-down filtering through a list of
 * active facets.
 *
 * It does **not** perform a full-text search — it is intended to be used
 * alongside a search step:
 *
 * ```cpp
 * // 1. Run the search
 * auto results = hybrid_search.search("laptop");
 *
 * // 2. Collect matching PKs
 * std::vector<std::string> pks;
 * for (const auto& r : results) pks.push_back(r.document_id);
 *
 * // 3. Compute facets
 * FacetedSearch facets(&sec_index);
 * auto brand_facet = facets.computeFacet("products", "brand", pks);
 * ```
 *
 * @note Thread Safety: A single FacetedSearch instance is NOT thread-safe.
 * @note Exception Safety: All methods return Status; they never throw.
 */
class FacetedSearch {
public:
    /**
     * @brief A range bucket definition for numeric range facets.
     */
    struct RangeBucket {
        std::string label;  ///< Human-readable label, e.g. "$0-$100"
        double low;         ///< Inclusive lower bound
        double high;        ///< Exclusive upper bound
    };

    /**
     * @brief An active facet filter applied during drill-down.
     */
    struct ActiveFacet {
        std::string field;  ///< Column to filter on
        std::string value;  ///< Required value
    };

    /**
     * @param index  Non-owning pointer to a SecondaryIndexManager.  Must outlive this.
     */
    explicit FacetedSearch(SecondaryIndexManager* index);

    // -----------------------------------------------------------------------
    // Core operations
    // -----------------------------------------------------------------------

    /**
     * @brief Compute value counts for a single field (categorical facet).
     *
     * Scans the secondary index for `table.column` and counts the occurrences
     * of each distinct value, limited to documents in `candidate_pks` (if
     * non-empty).  If `candidate_pks` is empty all documents are counted.
     *
     * @param table          Table name.
     * @param column         Column to facet on.
     * @param candidate_pks  Optional set of PKs to restrict counting (search results).
     * @param max_values     Maximum number of distinct values to return (0 = no limit).
     * @return Pair of Status and FacetResult.
     */
    std::pair<SecondaryIndexManager::Status, FacetResult> computeFacet(
        const std::string& table,
        const std::string& column,
        const std::vector<std::string>& candidate_pks = {},
        size_t max_values = 100
    ) const;

    /**
     * @brief Compute multiple facets at once.
     *
     * Convenience wrapper for calling computeFacet() on several columns in
     * a single call.
     *
     * @param table          Table name.
     * @param columns        List of columns to compute facets for.
     * @param candidate_pks  Optional set of PKs to restrict counting.
     * @return Pair of Status and list of FacetResult (one per column).
     */
    std::pair<SecondaryIndexManager::Status, std::vector<FacetResult>> computeFacets(
        const std::string& table,
        const std::vector<std::string>& columns,
        const std::vector<std::string>& candidate_pks = {}
    ) const;

    /**
     * @brief Compute a numeric range facet.
     *
     * Counts documents whose field value falls into each provided bucket.
     *
     * @param table          Table name.
     * @param column         Numeric column.
     * @param buckets        Ordered list of range definitions.
     * @param candidate_pks  Optional set of PKs to restrict counting.
     * @return Pair of Status and FacetResult (bucket labels as value_counts keys).
     */
    std::pair<SecondaryIndexManager::Status, FacetResult> computeRangeFacet(
        const std::string& table,
        const std::string& column,
        const std::vector<RangeBucket>& buckets,
        const std::vector<std::string>& candidate_pks = {}
    ) const;

    /**
     * @brief Apply active facet filters to a candidate PK set.
     *
     * Given a set of candidate PKs and a list of required field=value
     * constraints, return only the PKs that satisfy all constraints.
     *
     * @param table          Table name.
     * @param candidate_pks  PKs to filter.
     * @param active_facets  Required field=value filters.
     * @return Filtered PK list (may be empty).
     */
    std::pair<SecondaryIndexManager::Status, std::vector<std::string>> applyFacetFilters(
        const std::string& table,
        const std::vector<std::string>& candidate_pks,
        const std::vector<ActiveFacet>& active_facets
    ) const;

    /**
     * @brief Discover all columns in a table that are suitable for faceting.
     *
     * Queries the index metadata to find columns with regular, range, or sparse
     * indexes.  Geo, TTL, fulltext, and composite indexes are excluded because
     * they do not produce meaningful categorical or numeric range facets.
     *
     * @param table  Table name.
     * @return Pair of Status and list of column names suitable for faceting.
     */
    std::pair<SecondaryIndexManager::Status, std::vector<std::string>> discoverFacetableColumns(
        const std::string& table
    ) const;

    /**
     * @brief Compute dynamic facets for all discoverable columns in a table.
     *
     * Automatically calls discoverFacetableColumns() and then computeFacet() for
     * each discovered column.  This provides "dynamic" facet counting — callers
     * do not need to know which columns are indexed in advance.
     *
     * @param table          Table name.
     * @param candidate_pks  Optional set of PKs to restrict counting (search results).
     * @param max_values     Maximum distinct values per facet (0 = no limit).
     * @return Pair of Status and list of FacetResult (one per discoverable column).
     */
    std::pair<SecondaryIndexManager::Status, std::vector<FacetResult>> computeDynamicFacets(
        const std::string& table,
        const std::vector<std::string>& candidate_pks = {},
        size_t max_values = 100
    ) const;

private:
    SecondaryIndexManager* index_;
};

} // namespace themis
