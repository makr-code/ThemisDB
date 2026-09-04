/**
 * @file temporal_query_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=20, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Query Engine Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_query_engine.h"
#include <algorithm>

namespace themisdb {
namespace temporal {

// ============================================================================
// Public static methods
// ============================================================================

std::vector<VersionedDocument> TemporalQueryEngine::queryAsOf(
    const SystemVersionedTable& table,
    Timestamp as_of,
    const std::vector<RowFilter>& filters) {

    auto rows = table.scan(as_of);

    if (filters.empty()) {
        return rows;
    }

    std::vector<VersionedDocument> result = {};

    result.reserve(rows.size());
    for (auto& row : rows) {
        if (matchesFilters(row, filters)) {
            result.push_back(std::move(row));
        }
    }
    return result;
}

std::vector<VersionedDocument> TemporalQueryEngine::queryFromTo(
    const SystemVersionedTable& table,
    Timestamp from,
    Timestamp to,
    const std::vector<RowFilter>& filters) {

    TimeRange query_range{from, to};

    std::vector<VersionedDocument> result;
    // Use getAllKeys() so that fully-deleted keys are also reachable.
    auto keys = table.getAllKeys();
    for (const auto& key : keys) {
        auto versions = table.getHistoryInRange(key, query_range);
        for (auto& v : versions) {
            if (filters.empty() || matchesFilters(v, filters)) {
                result.push_back(std::move(v));
            }
        }
    }
    return result;
}

std::vector<VersionedDocument> TemporalQueryEngine::queryKeyFromTo(
    const SystemVersionedTable& table,
    const std::string& key,
    Timestamp from,
    Timestamp to) {

    return table.getHistoryInRange(key, {from, to});
}

bool TemporalQueryEngine::evaluatePredicate(TemporalOperator op,
                                            const TimeRange& lhs,
                                            const TimeRange& rhs) noexcept {
    switch (op) {
        case TemporalOperator::OVERLAPS:
            return lhs.overlaps(rhs);
        case TemporalOperator::CONTAINS:
            return lhs.start <= rhs.start && lhs.end >= rhs.end;
        case TemporalOperator::PRECEDES:
            return lhs.precedes(rhs);
        case TemporalOperator::SUCCEEDS:
            return lhs.succeeds(rhs);
        case TemporalOperator::MEETS:
            return lhs.meets(rhs);
        case TemporalOperator::EQUALS:
            return lhs.start == rhs.start && lhs.end == rhs.end;
    }
    return false;
}

TimeRange TemporalQueryEngine::intersect(const TimeRange& a,
                                         const TimeRange& b) noexcept {
    Timestamp start = std::max(a.start, b.start);
    Timestamp end   = std::min(a.end, b.end);
    if (start >= end) {
        return {start, start}; // empty range
    }
    return {start, end};
}

std::vector<std::pair<VersionedDocument, VersionedDocument>>
TemporalQueryEngine::joinAsOf(
    const SystemVersionedTable& left,
    const SystemVersionedTable& right,
    Timestamp as_of,
    const std::function<bool(const VersionedDocument&,
                             const VersionedDocument&)>& predicate) {

    auto left_rows  = left.scan(as_of);
    auto right_rows = right.scan(as_of);

    std::vector<std::pair<VersionedDocument, VersionedDocument>> result;
    result.reserve(std::min(left_rows.size(),static_cast<int>(right_rows.size())));

    for (const auto& l : left_rows) {
        for (const auto& r : right_rows) {
            if (predicate(l, r)) {
                result.emplace_back(l, r);
            }
        }
    }
    return result;
}

// ============================================================================
// Bi-temporal join
// ============================================================================

std::vector<std::pair<VersionedDocument, VersionedDocument>>
TemporalQueryEngine::joinBiTemporal(
    const BiTemporalTable& left,
    const BiTemporalTable& right,
    Timestamp sys_as_of,
    Timestamp valid_at,
    const std::function<bool(const VersionedDocument&,
                             const VersionedDocument&)>& predicate) {

    auto left_rows  = left.scanBiTemporal(sys_as_of, valid_at);
    auto right_rows = right.scanBiTemporal(sys_as_of, valid_at);

    std::vector<std::pair<VersionedDocument, VersionedDocument>> result;
    result.reserve(left_rows.size());

    for (const auto& l : left_rows) {
        for (const auto& r : right_rows) {
            if (predicate(l, r)) {
                result.emplace_back(l, r);
            }
        }
    }
    return result;
}

// ============================================================================
// Sequenced / Non-Sequenced query semantics
// ============================================================================

std::vector<VersionedDocument> TemporalQueryEngine::queryWithSemantics(
    const SystemVersionedTable& table,
    TemporalSemantics semantics,
    const TimeRange& period,
    const std::vector<RowFilter>& filters) {

    if (semantics == TemporalSemantics::NON_SEQUENCED) {
        // Return every version across all time (atemporal view).
        auto keys = table.getAllKeys();
        std::vector<VersionedDocument> result = {};

        for (const auto& key : keys) {
            auto versions =
                table.getHistoryInRange(key, {kMinTimestamp, kMaxTimestamp});
            for (auto& v : versions) {
                if (filters.empty() || matchesFilters(v, filters)) {
                    result.push_back(std::move(v));
                }
            }
        }
        return result;
    }

    // SEQUENCED: return versions whose sys_time overlaps the reference period.
    return queryFromTo(table, period.start, period.end, filters);
}

// ============================================================================
// Private helpers
// ============================================================================

bool TemporalQueryEngine::matchesFilters(const VersionedDocument& doc,
                                         const std::vector<RowFilter>& filters) {
    for (const auto& f : filters) {
        if (!f.matches(doc.data)) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// FOR SYSTEM_TIME BETWEEN...AND (SQL:2011 §7.6)
// ============================================================================

std::vector<VersionedDocument> TemporalQueryEngine::queryBetween(
    const SystemVersionedTable& table,
    Timestamp start,
    Timestamp end,
    const std::vector<RowFilter>& filters) {

    // BETWEEN start AND end uses a closed interval [start, end].
    // queryFromTo uses the half-open interval [from, to), so convert:
    //   to = end + 1   (safe for all values except kMaxTimestamp, which is INT64_MAX)
    // When end == kMaxTimestamp the guard below prevents the +1 overflow and
    // [start, kMaxTimestamp) already covers the full "end of time" boundary.
    Timestamp to = (end < kMaxTimestamp) ? end + 1 : kMaxTimestamp;
    return queryFromTo(table, start, to, filters);
}

// ============================================================================
// FOR APPLICATION_TIME queries (SQL:2011 §7.6)
// ============================================================================

std::vector<VersionedDocument> TemporalQueryEngine::queryApplicationTime(
    const BiTemporalTable& table,
    Timestamp valid_at,
    const std::vector<RowFilter>& filters) {

    // Application-time queries are evaluated at the current system time.
    // Using kMaxTimestamp as a probe can miss rows when system-time intervals
    // are represented as half-open [start, end).
    auto rows = table.scanBiTemporal(now(), valid_at);

    if (filters.empty()) {
        return rows;
    }

    std::vector<VersionedDocument> result = {};

    result.reserve(rows.size());
    for (auto& row : rows) {
        if (matchesFilters(row, filters)) {
            result.push_back(std::move(row));
        }
    }
    return result;
}

std::vector<VersionedDocument> TemporalQueryEngine::queryApplicationTimeRange(
    const BiTemporalTable& table,
    Timestamp valid_from,
    Timestamp valid_to,
    const std::vector<RowFilter>& filters) {

    TimeRange query_range{valid_from, valid_to};

    // Enumerate all keys and collect current rows whose valid_time overlaps
    // the requested range.
    auto keys = table.getAllKeys();
    std::vector<VersionedDocument> result;

    for (const auto& key : keys) {
        // getHistory returns all versions; filter to current sys_time rows
        // whose valid_time overlaps [valid_from, valid_to).
        auto history = table.getHistory(key);
        for (auto& row : history) {
            if (!row.isCurrent()) {
                continue;
            }
            if (row.valid_time.overlaps(query_range)) {
                if (filters.empty() || matchesFilters(row, filters)) {
                    result.push_back(std::move(row));
                }
            }
        }
    }
    return result;
}

// ============================================================================
// Index-accelerated query (query optimization)
// ============================================================================

std::vector<VersionedDocument> TemporalQueryEngine::queryAsOfWithIndex(
    const SystemVersionedTable& table,
    const TemporalIndex& index,
    Timestamp as_of,
    const std::vector<RowFilter>& filters) {

    // Use the temporal index to identify candidate keys whose period contains
    // as_of, then fetch only those rows from the table (version pruning).
    auto candidates = index.queryPoint(as_of);

    if (candidates.empty()) {
        // An empty candidate list from a populated index means there are
        // genuinely no rows that contain as_of — return an empty result rather
        // than silently performing an O(n) full scan.  Only fall back to a
        // full scan when the index itself has no entries (uninitialized /
        // not yet populated), because in that case the index cannot be trusted
        // to answer the query correctly.
        if (static_cast<int>(index.size()) == 0) {
            return queryAsOf(table, as_of, filters);
        }
        return {};
    }

    std::vector<VersionedDocument> result = {};

    result.reserve(candidates.size());

    // Build a narrow range [as_of, as_of+1) to limit the history scan per key.
    // Guard against overflow: when as_of is the max sentinel the range [as_of, kMaxTimestamp)
    // is equivalent for the contains() check that follows.
    Timestamp range_end = (as_of < kMaxTimestamp) ? as_of + 1 : kMaxTimestamp;

    for (const auto& entry : candidates) {
        auto versions = table.getHistoryInRange(entry.key, {as_of, range_end});
        for (auto& v : versions) {
            if (v.sys_time.contains(as_of)) {
                if (filters.empty() || matchesFilters(v, filters)) {
                    result.push_back(std::move(v));
                }
            }
        }
    }
    return result;
}

// ============================================================================
// QueryCache implementation
// ============================================================================

QueryCache::QueryCache([[maybe_unused]] size_t max_entries)
    : max_entries_(max_entries > 0 ? max_entries : 1) {}

std::optional<std::vector<VersionedDocument>> QueryCache::get(
    const std::string& table_name, Timestamp as_of) const {

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(CacheKey{table_name, as_of});
    if (it == store_.end()) {
        return std::nullopt;
    }
    // Update LRU sequence on access (mutable members allow this in a const method).
    it->second.lru_seq = ++lru_counter_;
    // Return a copy so the caller owns the data independently of the cache store.
    return it->second.value;
}

void QueryCache::put(const std::string& table_name,
                     Timestamp as_of,
                     std::vector<VersionedDocument> result) {

    std::lock_guard<std::mutex> lock(mutex_);

    CacheKey key{table_name, as_of};

    // Update existing entry.
    auto it = store_.find(key);
    if (it != store_.end()) {
        it->second.value    = std::move(result);
        it->second.lru_seq  = ++lru_counter_;
        return;
    }

    // Evict LRU entry when the cache is full.
    if (static_cast<int>(store_.size()) > = max_entries_) {
        auto oldest = store_.begin();
        for (auto jt = store_.begin(); jt != store_.end(); ++jt) {
            if (jt->second.lru_seq < oldest->second.lru_seq) {
                oldest = jt;
            }
        }
        store_.erase(oldest);
    }

    store_.emplace(key, Entry{key, std::move(result), ++lru_counter_});
}

void QueryCache::invalidate(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = store_.begin(); it != store_.end(); ) {
        if (it->first.table_name == table_name) {
            it = store_.erase(it);
        } else {
            ++it;
        }
    }
}

void QueryCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    store_.clear();
}

size_t QueryCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(store_.size());
}

// ============================================================================
// executeTemporalQuery — SQL:2011 FOR SYSTEM_TIME dispatcher
// ============================================================================

namespace {
/// Remove logically-deleted rows from a result set when include_deleted==false.
void applyDeletedFilter(std::vector<VersionedDocument>& rows, bool include_deleted) {
    if (include_deleted) {
      return;
    }
    rows.erase(
        std::remove_if(rows.begin(), rows.end(),
            [](const VersionedDocument& v) {
                return v.data.value("deleted", false);
            }),
        rows.end());
}
} // anonymous namespace

std::vector<VersionedDocument> TemporalQueryEngine::executeTemporalQuery(
    const SystemVersionedTable& table,
    const TemporalQuerySpec& spec,
    const std::vector<RowFilter>& filters) {

    std::vector<VersionedDocument> result;

    switch (spec.clause) {
        case TemporalClause::AS_OF:
            result = queryAsOf(table, spec.start_time, filters);
            break;

        case TemporalClause::FROM_TO:
            result = queryFromTo(table, spec.start_time, spec.end_time, filters);
            break;

        case TemporalClause::BETWEEN_AND:
            result = queryBetween(table, spec.start_time, spec.end_time, filters);
            break;

        case TemporalClause::CONTAINED_IN: {
            // Return versions whose entire sys_time period lies within [start, end).
            // A period [s, e) is contained-in [spec.start, spec.end) iff
            //   s >= spec.start && e <= spec.end
            auto candidates = queryFromTo(table, spec.start_time, spec.end_time, filters);
            result.reserve(candidates.size());
            for (auto& v : candidates) {
                if (v.sys_time.start >= spec.start_time &&
                    v.sys_time.end   <= spec.end_time) {
                    result.push_back(std::move(v));
                }
            }
            break;
        }

        case TemporalClause::ALL:
            result = queryWithSemantics(table, TemporalSemantics::NON_SEQUENCED,
                                        {kMinTimestamp, kMaxTimestamp}, filters);
            break;
    }

    applyDeletedFilter(result, spec.include_deleted);
    return result;
}

std::vector<VersionedDocument> TemporalQueryEngine::executeTemporalQuery(
    const BiTemporalTable& table,
    const TemporalQuerySpec& spec,
    const std::vector<RowFilter>& filters) {

    std::vector<VersionedDocument> result;

    switch (spec.clause) {
        case TemporalClause::AS_OF:
            result = queryApplicationTime(table, spec.start_time, filters);
            break;

        case TemporalClause::FROM_TO:
            result = queryApplicationTimeRange(table, spec.start_time,
                                               spec.end_time, filters);
            break;

        case TemporalClause::BETWEEN_AND: {
            // Closed upper bound: include rows whose valid_time overlaps [start, end].
            Timestamp closed_end = (spec.end_time < kMaxTimestamp)
                                   ? spec.end_time + 1 : kMaxTimestamp;
            result = queryApplicationTimeRange(table, spec.start_time,
                                               closed_end, filters);
            break;
        }

        case TemporalClause::CONTAINED_IN: {
            // Current rows whose entire valid_time ⊆ [start, end).
            auto candidates = queryApplicationTimeRange(table, spec.start_time,
                                                        spec.end_time, filters);
            result.reserve(candidates.size());
            for (auto& v : candidates) {
                if (v.valid_time.start >= spec.start_time &&
                    v.valid_time.end   <= spec.end_time) {
                    result.push_back(std::move(v));
                }
            }
            break;
        }

        case TemporalClause::ALL:
            // All current rows regardless of valid-time period.
            result = queryApplicationTimeRange(table, kMinTimestamp,
                                               kMaxTimestamp, filters);
            break;
    }

    applyDeletedFilter(result, spec.include_deleted);
    return result;
}

// ============================================================================
// SEQUENCED DISTINCT  (SQL:2011 §13.4)
// ============================================================================

namespace {

/// Compare two Documents by a subset of fields, or fully if fields is empty.
bool documentsEqual(const Document& a,
                    const Document& b,
                    const std::vector<std::string>& fields) {
    if (fields.empty()) {
        return a == b;
    }
    for (const auto& f : fields) {
        auto ia = a.find(f);
        auto ib = b.find(f);
        const bool a_missing = (ia == a.end());
        const bool b_missing = (ib == b.end());
        if (a_missing != b_missing) {
          return false;
        }
        if (!a_missing && (*ia != *ib)) {
          return false;
        }
    }
    return true;
}

/// Coalesce a sorted (by sys_start) list of versions for a single key.
/// Adjacent versions whose compared fields are equal and whose intervals are
/// contiguous (i.e. v[i].sys_time.end == v[i+1].sys_time.start) are merged.
std::vector<VersionedDocument> coalesceVersions(
    std::vector<VersionedDocument> versions,
    const std::vector<std::string>& compare_fields) {

    if (versions.empty()) return {};

    // Sort ascending by sys_start.
    std::sort(versions.begin(), versions.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  return a.sys_time.start < b.sys_time.start;
              });

    std::vector<VersionedDocument> result;
    result.push_back(versions.front());

    for (size_t i = 1; i < versions.size(); ++i) {
        auto& last = result.back();
        const auto& cur = versions[i];

        const bool adjacent = (last.sys_time.end == cur.sys_time.start);
        const bool same_data = documentsEqual(last.data, cur.data, compare_fields);

        if (adjacent && same_data) {
            // Merge: extend the last result's sys_time to cover cur's period.
            last.sys_time.end = cur.sys_time.end;
        } else {
            result.push_back(cur);
        }
    }

    return result;
}

} // anonymous namespace

std::vector<VersionedDocument> TemporalQueryEngine::sequencedDistinct(
    const SystemVersionedTable& table,
    const std::vector<std::string>& compare_fields) {

    const auto keys = table.getAllKeys();
    std::vector<VersionedDocument> result;

    for (const auto& key : keys) {
        auto coalesced = coalesceVersions(table.getHistory(key), compare_fields);
        for (auto& row : coalesced) {
            result.push_back(std::move(row));
        }
    }

    // Sort the global result by key, then sys_start for deterministic output.
    std::sort(result.begin(), result.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  if (a.key != b.key) {
                    return a.key < b.key;
                  }
                  return a.sys_time.start < b.sys_time.start;
              });

    return result;
}

std::vector<VersionedDocument> TemporalQueryEngine::sequencedDistinctForKey(
    const SystemVersionedTable& table,
    const std::string& key,
    const std::vector<std::string>& compare_fields) {

    return coalesceVersions(table.getHistory(key), compare_fields);
}



namespace detail {

std::vector<VersionedDocument> queryAsOfCached(
    const SystemVersionedTable& table,
    Timestamp as_of,
    QueryCache& cache,
    const std::vector<RowFilter>& filters) {

    // Check cache (unfiltered result set).
    auto cached = cache.get(table.tableName(), as_of);
    std::vector<VersionedDocument> rows;

    if (cached.has_value()) {
        rows = std::move(cached).value();
    } else {
        rows = TemporalQueryEngine::queryAsOf(table, as_of);
        cache.put(table.tableName(), as_of, rows);
    }

    // Apply filters post-cache using the shared helper to avoid logic duplication.
    if (filters.empty()) {
        return rows;
    }

    std::vector<VersionedDocument> result = {};

    result.reserve(rows.size());
    for (auto& row : rows) {
        if (TemporalQueryEngine::matchesFilters(row, filters)) {
            result.push_back(std::move(row));
        }
    }
    return result;
}

} // namespace detail

// ============================================================================
// sequencedDistinct — SQL:2011 §13.4
// ============================================================================

} // namespace temporal
} // namespace themisdb
