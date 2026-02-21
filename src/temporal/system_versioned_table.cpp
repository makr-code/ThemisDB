/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            system_versioned_table.cpp                         ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     386                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB System-Versioned Table Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/system_versioned_table.h"
#include <algorithm>
#include <stdexcept>

namespace themisdb {
namespace temporal {

// ============================================================================
// Construction
// ============================================================================

SystemVersionedTable::SystemVersionedTable(std::string table_name,
                                           std::string source_node)
    : table_name_(std::move(table_name)),
      source_node_(std::move(source_node)) {}

// ============================================================================
// DML
// ============================================================================

bool SystemVersionedTable::insert(const std::string& key, const Document& doc) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& versions = rows_[key];
    // Reject if a current version already exists
    for (const auto& v : versions) {
        if (v.sys_time.end == kMaxTimestamp) {
            return false;
        }
    }

    VersionedDocument vdoc;
    vdoc.key         = key;
    vdoc.data        = doc;
    vdoc.sys_time    = {now(), kMaxTimestamp};
    vdoc.valid_time  = {now(), kMaxTimestamp};
    vdoc.modified_by = source_node_;

    versions.push_back(std::move(vdoc));
    return true;
}

bool SystemVersionedTable::update(const std::string& key,
                                  const Document& updates) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return false;
    }

    auto& versions = it->second;
    // Find the open-ended (current) version
    VersionedDocument* current = nullptr;
    for (auto& v : versions) {
        if (v.sys_time.end == kMaxTimestamp) {
            current = &v;
            break;
        }
    }
    if (!current) {
        return false;
    }

    Timestamp ts = now();

    // Close the old version
    current->sys_time.end = ts;

    // Create a new version with merged data
    Document merged = current->data;
    for (auto& [k, val] : updates.items()) {
        merged[k] = val;
    }

    VersionedDocument vdoc;
    vdoc.key         = key;
    vdoc.data        = std::move(merged);
    vdoc.sys_time    = {ts, kMaxTimestamp};
    vdoc.valid_time  = {ts, kMaxTimestamp};
    vdoc.modified_by = source_node_;

    versions.push_back(std::move(vdoc));
    return true;
}

bool SystemVersionedTable::deleteRow(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return false;
    }

    closeCurrentVersion(it->second, now());
    return true;
}

// ============================================================================
// Queries
// ============================================================================

std::optional<VersionedDocument> SystemVersionedTable::getCurrent(
    const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return std::nullopt;
    }
    for (const auto& v : it->second) {
        if (v.sys_time.end == kMaxTimestamp) {
            return v;
        }
    }
    return std::nullopt;
}

std::optional<VersionedDocument> SystemVersionedTable::getAsOf(
    const std::string& key, Timestamp as_of) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return std::nullopt;
    }
    for (const auto& v : it->second) {
        if (v.sys_time.contains(as_of)) {
            return v;
        }
    }
    return std::nullopt;
}

std::vector<VersionedDocument> SystemVersionedTable::getHistory(
    const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return {};
    }
    return it->second;
}

std::vector<VersionedDocument> SystemVersionedTable::getHistoryInRange(
    const std::string& key, const TimeRange& range) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return {};
    }

    std::vector<VersionedDocument> result;
    for (const auto& v : it->second) {
        if (v.sys_time.overlaps(range)) {
            result.push_back(v);
        }
    }
    return result;
}

std::vector<VersionedDocument> SystemVersionedTable::scan(
    Timestamp as_of) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<VersionedDocument> result;
    for (const auto& [key, versions] : rows_) {
        for (const auto& v : versions) {
            if (as_of == kMaxTimestamp) {
                if (v.sys_time.end == kMaxTimestamp) {
                    result.push_back(v);
                }
            } else {
                if (v.sys_time.contains(as_of)) {
                    result.push_back(v);
                }
            }
        }
    }
    return result;
}

// ============================================================================
// Purge / all-key APIs
// ============================================================================

std::vector<std::string> SystemVersionedTable::getAllKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> keys;
    keys.reserve(rows_.size());
    for (const auto& [k, _] : rows_) {
        keys.push_back(k);
    }
    return keys;
}

size_t SystemVersionedTable::purgeHistoricalVersions(
    const std::string& key,
    const std::function<bool(const VersionedDocument&)>& predicate) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return 0;
    }

    auto& versions = it->second;
    size_t before = versions.size();

    versions.erase(
        std::remove_if(versions.begin(), versions.end(),
                       [&](const VersionedDocument& v) {
                           // Never remove the current (open-ended) version
                           if (v.isCurrent()) return false;
                           return predicate(v);
                       }),
        versions.end());

    return before - versions.size();
}

size_t SystemVersionedTable::purgeHistoricalVersions(
    const std::function<bool(const VersionedDocument&)>& predicate) {

    // Collect all keys first (lock-free key list not needed – we hold the lock
    // inside the per-key call; call per-key which locks each time)
    std::vector<std::string> keys;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [k, _] : rows_) {
            keys.push_back(k);
        }
    }

    size_t total = 0;
    for (const auto& k : keys) {
        total += purgeHistoricalVersions(k, predicate);
    }
    return total;
}

size_t SystemVersionedTable::purgeHistoricalVersionsKeepLatestN(
    const std::string& key,
    size_t keep_latest_n) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return 0;
    }

    auto& versions = it->second;

    // Collect pointers to historical (closed) versions
    std::vector<VersionedDocument*> historical;
    for (auto& v : versions) {
        if (!v.isCurrent()) {
            historical.push_back(&v);
        }
    }

    if (historical.size() <= keep_latest_n) {
        return 0;
    }

    // Sort descending by sys_start (newest first)
    std::sort(historical.begin(), historical.end(),
              [](const VersionedDocument* a, const VersionedDocument* b) {
                  return a->sys_time.start > b->sys_time.start;
              });

    // Collect raw pointers of the entries to delete (the oldest ones)
    std::vector<const VersionedDocument*> to_delete_ptrs;
    to_delete_ptrs.reserve(historical.size() - keep_latest_n);
    for (size_t i = keep_latest_n; i < historical.size(); ++i) {
        to_delete_ptrs.push_back(historical[i]);
    }

    size_t before = versions.size();

    versions.erase(
        std::remove_if(versions.begin(), versions.end(),
                       [&](const VersionedDocument& v) {
                           if (v.isCurrent()) return false;
                           for (const auto* p : to_delete_ptrs) {
                               if (p == &v) return true;
                           }
                           return false;
                       }),
        versions.end());

    return before - versions.size();
}

// ============================================================================
// Metadata
// ============================================================================

size_t SystemVersionedTable::keyCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rows_.size();
}

size_t SystemVersionedTable::versionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [k, v] : rows_) {
        count += v.size();
    }
    return count;
}

nlohmann::json SystemVersionedTable::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t current_count = 0;
    size_t historical_count = 0;
    for (const auto& [k, versions] : rows_) {
        for (const auto& v : versions) {
            if (v.sys_time.end == kMaxTimestamp) {
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

void SystemVersionedTable::closeCurrentVersion(VersionList& versions,
                                               Timestamp close_time) {
    for (auto& v : versions) {
        if (v.sys_time.end == kMaxTimestamp) {
            v.sys_time.end = close_time;
        }
    }
}

} // namespace temporal
} // namespace themisdb
