/**
 * @file faceted_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/faceted_search.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include <algorithm>
#include <unordered_set>

namespace themis {

FacetedSearch::FacetedSearch(SecondaryIndexManager* index) : index_(index) {}

std::pair<SecondaryIndexManager::Status, FacetResult>
FacetedSearch::computeFacet(const std::string& table,
                              const std::string& column,
                              const std::vector<std::string>& candidate_pks,
                              size_t max_values) const {
    if (!index_) {
        return {SecondaryIndexManager::Status::Error("FacetedSearch: null index"), {}};
    }
    if (table.empty() || column.empty()) {
        return {SecondaryIndexManager::Status::Error(
            "FacetedSearch: table and column must not be empty"), {}};
    }

    FacetResult result;
    result.field = column;

    const bool has_filter = !candidate_pks.empty();

    std::vector<std::string> all_pks = {};

    if (!has_filter) {
        // No filter — enumerate all PKs from the range index.
        constexpr size_t kScanLimit = 10'000;
        auto [scan_status, scanned_pks] = index_->scanKeysRange(
            table, column,
            std::nullopt, std::nullopt,
            true, true,
            kScanLimit
        );
        if (!scan_status.ok) {
            THEMIS_DEBUG("FacetedSearch::computeFacet: no index for {}.{}: {}",
                         table, column, scan_status.message);
            return {SecondaryIndexManager::Status::OK(), result};
        }
        all_pks = std::move(scanned_pks);
    }

    // When a filter is active we iterate the caller-supplied PKs directly;
    // otherwise we iterate the PKs discovered via the range-index scan above.
    const std::vector<std::string>& working_pks = has_filter ? candidate_pks : all_pks;

    for (const auto& pk : working_pks) {

        // Fetch the entity by its primary key to read the column value
        auto [es, entities] = index_->scanEntitiesEqual(table, "id", pk);
        if (!es.ok || entities.empty()) {
          continue;
        }

        const auto& entity = entities[0];
        std::string field_value;
        auto str_opt = entity.getFieldAsString(column);
        if (str_opt.has_value()) {
            field_value = str_opt.value();
        } else {
            auto int_opt = entity.getFieldAsInt(column);
            if (int_opt.has_value()) {
                field_value = std::to_string(int_opt.value());
            } else {
                continue;
            }
        }

        result.value_counts[field_value]++;
        result.total_docs++;
        if (max_values > 0 && result.value_counts.size() >= max_values) {
          break;
        }
    }

    THEMIS_DEBUG("FacetedSearch::computeFacet({}.{}): {} distinct values, {} docs",
                 table, column, result.value_counts.size(), result.total_docs);
    return {SecondaryIndexManager::Status::OK(), result};
}

std::pair<SecondaryIndexManager::Status, std::vector<FacetResult>>
FacetedSearch::computeFacets(const std::string& table,
                               const std::vector<std::string>& columns,
                               const std::vector<std::string>& candidate_pks) const {
    std::vector<FacetResult> facets = {};

    for (const auto& col : columns) {
        auto [st, facet] = computeFacet(table, col, candidate_pks);
        if (!st.ok) return {st, {}};
        facets.push_back(std::move(facet));
    }
    return {SecondaryIndexManager::Status::OK(), facets};
}

std::pair<SecondaryIndexManager::Status, FacetResult>
FacetedSearch::computeRangeFacet(const std::string& table,
                                   const std::string& column,
                                   const std::vector<RangeBucket>& buckets,
                                   const std::vector<std::string>& candidate_pks) const {
    if (!index_) {
        return {SecondaryIndexManager::Status::Error("FacetedSearch: null index"), {}};
    }
    if (buckets.empty()) {
        return {SecondaryIndexManager::Status::Error(
            "FacetedSearch: buckets must not be empty"), {}};
    }

    FacetResult result;
    result.field = column;
    for (const auto& b : buckets) {
      result.value_counts[b.label] = 0;
    }

    const std::unordered_set<std::string> pk_filter(candidate_pks.begin(), candidate_pks.end());
    const bool has_filter = !candidate_pks.empty();

    for (const auto& bucket : buckets) {
        std::string low_str  = std::to_string(static_cast<long long>(bucket.low));
        std::string high_str = std::to_string(static_cast<long long>(bucket.high));

        auto [st, pks] = index_->scanKeysRange(
            table, column, low_str, high_str, true, false, 10'000);
        if (!st.ok) {
          continue;
        }

        for (const auto& pk : pks) {
            if (has_filter && !pk_filter.count(pk)) {
              continue;
            }
            result.value_counts[bucket.label]++;
            result.total_docs++;
        }
    }
    return {SecondaryIndexManager::Status::OK(), result};
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
FacetedSearch::applyFacetFilters(const std::string& table,
                                   const std::vector<std::string>& candidate_pks,
                                   const std::vector<ActiveFacet>& active_facets) const {
    if (!index_) {
        return {SecondaryIndexManager::Status::Error("FacetedSearch: null index"), {}};
    }
    if (active_facets.empty()) {
        return {SecondaryIndexManager::Status::OK(), candidate_pks};
    }

    std::unordered_set<std::string> remaining(candidate_pks.begin(), candidate_pks.end());

    for (const auto& facet : active_facets) {
        auto [st, matching_pks] = index_->scanKeysEqual(table, facet.field, facet.value);
        if (!st.ok) {
            THEMIS_WARN("FacetedSearch::applyFacetFilters: scanKeysEqual failed {}.{}={}",
                        table, facet.field, facet.value);
            continue;
        }
        const std::unordered_set<std::string> matching_set(matching_pks.begin(), matching_pks.end());
        std::unordered_set<std::string> intersected = {};

        for (const auto& pk : remaining) {
            if (matching_set.count(pk)) {
              intersected.insert(pk);
            }
        }
        remaining = std::move(intersected);
        if (remaining.empty()) {
          break;
        }
    }

    return {SecondaryIndexManager::Status::OK(),
            std::vector<std::string>(remaining.begin(), remaining.end())};
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
FacetedSearch::discoverFacetableColumns(const std::string& table) const {
    if (!index_) {
        return {SecondaryIndexManager::Status::Error("FacetedSearch: null index"), {}};
    }
    if (table.empty()) {
        return {SecondaryIndexManager::Status::Error(
            "FacetedSearch: table must not be empty"), {}};
    }

    auto all_stats = index_->getAllIndexStats(table);
    std::vector<std::string> columns = {};

    columns.reserve(all_stats.size());
    for (const auto& stats : all_stats) {
        // Only regular, range, and sparse indexes produce meaningful categorical
        // or numeric facets.  Geo, TTL, fulltext, and composite indexes are skipped.
        if (stats.type == "regular" || stats.type == "range" || stats.type == "sparse") {
            columns.push_back(stats.column);
        }
    }

    THEMIS_DEBUG("FacetedSearch::discoverFacetableColumns({}): {} facetable columns",
                 table, columns.size());
    return {SecondaryIndexManager::Status::OK(), std::move(columns)};
}

std::pair<SecondaryIndexManager::Status, std::vector<FacetResult>>
FacetedSearch::computeDynamicFacets(const std::string& table,
                                     const std::vector<std::string>& candidate_pks,
                                     size_t max_values) const {
    auto [st, columns] = discoverFacetableColumns(table);
    if (!st.ok) return {st, {}};

    std::vector<FacetResult> facets = {};

    facets.reserve(columns.size());
    for (const auto& col : columns) {
        auto [fst, facet] = computeFacet(table, col, candidate_pks, max_values);
        if (!fst.ok) return {fst, {}};
        facets.push_back(std::move(facet));
    }

    THEMIS_DEBUG("FacetedSearch::computeDynamicFacets({}): {} facets computed",
                 table, facets.size());
    return {SecondaryIndexManager::Status::OK(), std::move(facets)};
}

} // namespace themis
