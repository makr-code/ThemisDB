/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bi_temporal.cpp                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:01:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     287                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 308ab7d2f  2026-02-20  feat(temporal): SQL:2011 Temporal Module – Production Rea... ║
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

std::vector<VersionedDocument> BiTemporalTable::getHistory(
    const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return {};
    }
    return it->second;
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
