/**
 * @file bi_temporal.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Bi-Temporal Table Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/bi_temporal.h"
#include <algorithm>
#include <limits>

namespace themisdb {
namespace temporal {

// ============================================================================
// Construction
// ============================================================================

BiTemporalTable::BiTemporalTable(std::string table_name,
                                 std::string source_node)
    : table_name_(std::move(table_name)),
      source_node_(std::move(source_node)) {}

// ============================================================================
// TemporalForeignKey
// ============================================================================

bool TemporalForeignKey::validate(const BiTemporalTable& parent_table,
                                   const std::string& parent_key,
                                   const TimeRange& child_period) const {
    // Guard: reject if the caller passed the wrong parent table.
    if (!parent_table_name.empty() &&
        parent_table.tableName() != parent_table_name) {
        return false;
    }

    // A current parent row must exist whose valid-time period CONTAINS the
    // entire child_period, i.e. parent.valid_time.start <= child_period.start
    // AND parent.valid_time.end >= child_period.end.
    auto parent_rows = parent_table.queryCurrentByValidTime(
        parent_key, child_period.start);

    for (const auto& row : parent_rows) {
        if (row.valid_time.start <= child_period.start &&
            row.valid_time.end   >= child_period.end) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// DML
// ============================================================================

bool BiTemporalTable::insertWithValidTime(const std::string& key,
                                          const Document& doc,
                                          const TimeRange& valid_time) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& versions = rows_[key];

    // Check for valid-time overlap with existing current rows
    for (const auto& v : versions) {
        if (v.isCurrent() && v.valid_time.overlaps(valid_time)) {
            return false; // Overlap detected – reject insert
        }
    }

    VersionedDocument vdoc;
    vdoc.key         = key;
    vdoc.data        = doc;
    vdoc.sys_time    = {now(), kMaxTimestamp};
    vdoc.valid_time  = valid_time;
    vdoc.modified_by = source_node_;

    versions.push_back(std::move(vdoc));
    return true;
}

bool BiTemporalTable::updateForValidTime(const std::string& key,
                                          const Document& updates,
                                          Timestamp valid_at) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return false;
    }

    auto& versions = it->second;
    VersionedDocument* target = nullptr;

    for (auto& v : versions) {
        if (v.isCurrent() && v.valid_time.contains(valid_at)) {
            target = &v;
            break;
        }
    }

    if (!target) {
        return false;
    }

    Timestamp ts = now();

    // Preserve the valid_time of the old version
    TimeRange old_valid = target->valid_time;

    // Close the old system-time period
    target->sys_time.end = ts;

    // Create a new version with merged data
    Document merged = target->data;
    for (auto& [k, val] : updates.items()) {
        merged[k] = val;
    }

    VersionedDocument vdoc;
    vdoc.key         = key;
    vdoc.data        = std::move(merged);
    vdoc.sys_time    = {ts, kMaxTimestamp};
    vdoc.valid_time  = old_valid;
    vdoc.modified_by = source_node_;

    versions.push_back(std::move(vdoc));
    return true;
}

size_t BiTemporalTable::deleteForValidTime(const std::string& key,
                                            Timestamp valid_at) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return 0;
    }

    Timestamp ts = now();
    return closeCurrentRows(
        it->second, ts,
        [valid_at](const VersionedDocument& v) {
            return v.valid_time.contains(valid_at);
        });
}

// ============================================================================
// Queries
// ============================================================================

std::vector<VersionedDocument> BiTemporalTable::queryBiTemporal(
    const std::string& key,
    Timestamp sys_as_of,
    Timestamp valid_at) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return {};
    }

    std::vector<VersionedDocument> result = {};

    for (const auto& v : it->second) {
        if (v.sys_time.contains(sys_as_of) &&
            v.valid_time.contains(valid_at)) {
            result.push_back(v);
        }
    }
    return result;
}

std::vector<VersionedDocument> BiTemporalTable::queryCurrentByValidTime(
    const std::string& key, Timestamp valid_at) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return {};
    }

    std::vector<VersionedDocument> result = {};

    for (const auto& v : it->second) {
        if (v.isCurrent() && v.valid_time.contains(valid_at)) {
            result.push_back(v);
        }
    }
    return result;
}

std::vector<std::pair<VersionedDocument, VersionedDocument>>
BiTemporalTable::findOverlaps(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return {};
    }

    std::vector<std::pair<VersionedDocument, VersionedDocument>> overlaps;
    const auto& versions = it->second;

    // Only inspect current rows
    std::vector<const VersionedDocument*> current = {};

    for (const auto& v : versions) {
        if (v.isCurrent()) {
            current.push_back(&v);
        }
    }

    for (size_t i = 0; i < current.size(); ++i) {
        for (size_t j = i + 1; j < current.size(); ++j) {
            if (current[i]->valid_time.overlaps(current[j]->valid_time)) {
                overlaps.emplace_back(*current[i], *current[j]);
            }
        }
    }
    return overlaps;
}

std::vector<TimeRange> BiTemporalTable::findGaps(const std::string& key,
                                                   Timestamp from,
                                                   Timestamp to) const {
    if (from >= to) {
        return {};
    }

    // Limit the critical section to extracting the covered intervals;
    // sort and merge are independent of shared state and run unlocked.
    std::vector<TimeRange> covered;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = rows_.find(key);
        if (it == rows_.end()) {
            // No rows at all → the entire interval is a gap
            return {{from, to}};
        }

        for (const auto& v : it->second) {
            if (v.isCurrent() && v.valid_time.overlaps({from, to})) {
                // Clamp to [from, to).  The overlaps() pre-check guarantees
                // that the clamped range is non-empty (start < end).
                Timestamp cs = std::max(v.valid_time.start, from);
                Timestamp ce = std::min(v.valid_time.end, to);
                covered.push_back({cs, ce});
            }
        }
    } // lock released here

    if (covered.empty()) {
        return {{from, to}};
    }

    // Sort by start
    std::sort(covered.begin(), covered.end(),
              [](const TimeRange& a, const TimeRange& b) {
                  return a.start < b.start;
              });

    // Merge overlapping covered intervals, then compute complement in [from, to)
    std::vector<TimeRange> merged;
    merged.push_back(covered[0]);
    for (size_t i = 1; i < covered.size(); ++i) {
        if (covered[i].start <= merged.back().end) {
            merged.back().end = std::max(merged.back().end, covered[i].end);
        } else {
            merged.push_back(covered[i]);
        }
    }

    // Gaps are the intervals in [from, to) not in merged
    std::vector<TimeRange> gaps;
    Timestamp cursor = from;
    for (const auto& m : merged) {
        if (cursor < m.start) {
            gaps.push_back({cursor, m.start});
        }
        cursor = std::max(cursor, m.end);
    }
    if (cursor < to) {
        gaps.push_back({cursor, to});
    }
    return gaps;
}

bool BiTemporalTable::hasUniquenessConflict(const std::string& key,
                                             const TimeRange& period) const {
    // An empty or invalid period cannot overlap anything.
    if (period.start >= period.end) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return false;
    }

    for (const auto& v : it->second) {
        if (v.isCurrent() && v.valid_time.overlaps(period)) {
            return true;
        }
    }
    return false;
}

std::vector<VersionedDocument> BiTemporalTable::getHistory(
    const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return {};
    }
    return it->second;
}

std::vector<VersionedDocument> BiTemporalTable::scanBiTemporal(
    Timestamp sys_as_of, Timestamp valid_at) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<VersionedDocument> result = {};

    for (const auto& [key, versions] : rows_) {
        for (const auto& v : versions) {
            if (v.sys_time.contains(sys_as_of) &&
                v.valid_time.contains(valid_at)) {
                result.push_back(v);
            }
        }
    }
    return result;
}

std::vector<std::string> BiTemporalTable::getAllKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> keys = {};

    keys.reserve(rows_.size());
    for (const auto& [key, _] : rows_) {
        keys.push_back(key);
    }
    return keys;
}

// ============================================================================
// Cross-node reconciliation
// ============================================================================

// ============================================================================
// Metadata
// ============================================================================

size_t BiTemporalTable::keyCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rows_.size();
}

size_t BiTemporalTable::versionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [k, v] : rows_) {
        count += v.size();
    }
    return count;
}

nlohmann::json BiTemporalTable::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t current_count = 0;
    size_t historical_count = 0;
    for (const auto& [k, versions] : rows_) {
        for (const auto& v : versions) {
            if (v.isCurrent()) {
                ++current_count;
            } else {
                ++historical_count;
            }
        }
    }

    return {{"table_name", table_name_},
            {"key_count", rows_.size()},
            {"current_rows", current_count},
            {"historical_rows", historical_count},
            {"total_versions", current_count + historical_count}};
}

// ============================================================================
// Private helpers
// ============================================================================

size_t BiTemporalTable::closeCurrentRows(
    VersionList& versions,
    Timestamp close_time,
    const std::function<bool(const VersionedDocument&)>& pred) {

    size_t closed = 0;
    for (auto& v : versions) {
        if (v.isCurrent() && pred(v)) {
            v.sys_time.end = close_time;
            ++closed;
        }
    }
    return closed;
}

// ============================================================================
// BiTemporalTable::merge — cross-node LWW reconciliation (v1.9.0)
// ============================================================================

BiTemporalTable::MergeResult BiTemporalTable::merge(const BiTemporalTable& other) {
    // Reject cross-table merges to avoid mixing unrelated entity histories.
    MergeResult result = {};
    if (table_name_ != other.table_name_) {
        return result;
    }

    // Take a snapshot of the other table's rows under its own mutex, then
    // merge into our own table under our mutex.  Never hold both locks
    // simultaneously to avoid potential deadlock.
    std::map<std::string, std::vector<VersionedDocument>> other_snapshot;
    {
        std::lock_guard<std::mutex> lk_other(other.mutex_);
        other_snapshot = other.rows_;
    }

    std::lock_guard<std::mutex> lk_self(mutex_);

    for (const auto& [key, other_versions] : other_snapshot) {
        auto& self_versions = rows_[key];  // creates empty VersionList if absent

        for (const auto& o_row : other_versions) {
            // Check whether an identical row already exists locally or whether
            // the row overlaps a current local row and must be reconciled via LWW.
            bool found_exact = false;
            std::size_t conflict_idx = std::numeric_limits<std::size_t>::max();

            for (std::size_t i = 0; i < self_versions.size(); ++i) {
                const auto& s_row = self_versions[i];

                if (s_row.valid_time == o_row.valid_time &&
                    s_row.sys_time.start == o_row.sys_time.start &&
                    s_row.data == o_row.data) {
                    found_exact = true;
                    break;
                }

                // Merge conflicts are defined over overlapping valid-time windows
                // on current local rows for the same key.
                if (!s_row.isCurrent()) {
                    continue;
                }
                if (!s_row.valid_time.overlaps(o_row.valid_time)) {
                    continue;
                }

                if (conflict_idx == std::numeric_limits<std::size_t>::max() ||
                    s_row.sys_time.start > self_versions[conflict_idx].sys_time.start) {
                    conflict_idx = i;
                }
            }

            if (found_exact) {
                ++result.rows_skipped;
                continue;
            }

            if (conflict_idx != std::numeric_limits<std::size_t>::max()) {
                // LWW: the row with the later sys_time.start wins.
                if (o_row.sys_time.start > self_versions[conflict_idx].sys_time.start) {
                    self_versions[conflict_idx] = o_row;
                    ++result.conflicts_lww;
                } else {
                    ++result.rows_skipped;
                }
                continue;
            }

            // No overlapping current valid-time found locally → insert row.
            self_versions.push_back(o_row);
            ++result.rows_inserted;
        }
    }

    return result;
}

} // namespace temporal
} // namespace themisdb
