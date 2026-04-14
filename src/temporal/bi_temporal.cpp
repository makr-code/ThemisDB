/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bi_temporal.cpp                                    ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:38:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     430                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6e698b5dbb  2026-03-12  fix(temporal): address PR review feedback on BiTemporalTable ║
    • bf380a1af8  2026-03-12  feat(temporal): add gap detection, uniqueness constraints... ║
    • 6e8942ed4f  2026-03-09  feat(temporal): implement bitemporal joins and SEQUENCED/... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Bi-Temporal Table Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/bi_temporal.h"
#include <algorithm>

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

    std::vector<VersionedDocument> result;
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

    std::vector<VersionedDocument> result;
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
    std::vector<const VersionedDocument*> current;
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

    std::vector<VersionedDocument> result;
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

    std::vector<std::string> keys;
    keys.reserve(rows_.size());
    for (const auto& [key, _] : rows_) {
        keys.push_back(key);
    }
    return keys;
}

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

} // namespace temporal
} // namespace themisdb
