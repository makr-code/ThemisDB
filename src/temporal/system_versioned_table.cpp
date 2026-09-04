/**
 * @file system_versioned_table.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
      source_node_(std::move(source_node)) {
    // Initialise config with defaults; history_table_name follows convention.
    config_.history_table_name = table_name_ + "_history";
}

SystemVersionedTable::SystemVersionedTable(std::string table_name,
                                           Config config,
                                           std::string source_node)
    : table_name_(std::move(table_name)),
      source_node_(std::move(source_node)),
      config_(std::move(config)) {
    if (config_.history_table_name.empty()) {
        config_.history_table_name = table_name_ + "_history";
    }
}

SystemVersionedTable::SystemVersionedTable(SystemVersionedTable&& other) noexcept
    : table_name_(std::move(other.table_name_)),
      source_node_(std::move(other.source_node_)),
      config_(std::move(other.config_)),
      schema_(std::move(other.schema_)),
      rows_(std::move(other.rows_))
    // mutex_ is default-constructed (not moved).  This is safe for the
    // factory use-case where the object is moved before being shared.
{}

// static
SystemVersionedTable SystemVersionedTable::createVersionedTable(
    const std::string& table_name,
    const Document&    schema,
    Config             config,
    const std::string& source_node) {

    if (config.history_table_name.empty()) {
        config.history_table_name = table_name + "_history";
    }

    SystemVersionedTable tbl(table_name, std::move(config), source_node);
    tbl.schema_ = schema;
    return tbl;
}

// static
SystemVersionedTable SystemVersionedTable::createVersionedTable(
    const std::string& table_name,
    const Document&    schema) {
    return createVersionedTable(table_name, schema, Config{}, "local");
}

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

    Timestamp ts = now();
    versions.push_back(makeVersion(key, doc, ts));
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

    versions.push_back(makeVersion(key, std::move(merged), ts));
    return true;
}

bool SystemVersionedTable::upsert(const std::string& key, const Document& doc) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& versions = rows_[key];

    // Check if a current version exists
    VersionedDocument* current = nullptr;
    for (auto& v : versions) {
        if (v.sys_time.end == kMaxTimestamp) {
            current = &v;
            break;
        }
    }

    Timestamp ts = now();

    if (!current) {
        // Insert path
        versions.push_back(makeVersion(key, doc, ts));
        return true;
    }

    // Update path: close existing version and open a new merged one
    closeCurrentVersion(versions, ts);
    Document merged = current->data;
    for (auto& [k, val] : doc.items()) {
        merged[k] = val;
    }
    versions.push_back(makeVersion(key, std::move(merged), ts));
    return false;
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

    std::vector<VersionedDocument> result = {};

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

    std::vector<VersionedDocument> result = {};

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
    std::vector<std::string> keys = {};

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
                       [&]([[maybe_unused]] const VersionedDocument& v) {
                           // Never remove the current (open-ended) version
                           if (v.isCurrent()) {
                             return false;
                           }
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
    std::vector<VersionedDocument*> historical = {};

    for (auto& v : versions) {
        if (!v.isCurrent()) {
            historical.push_back(&v);
        }
    }

    if (static_cast<int>(historical.size()) <= keep_latest_n) {
        return 0;
    }

    // Sort descending by sys_start (newest first)
    std::sort(historical.begin(), historical.end(),
              [](const VersionedDocument* a, const VersionedDocument* b) {
                  return a->sys_time.start > b->sys_time.start;
              });

    // Collect raw pointers of the entries to delete (the oldest ones)
    std::vector<const VersionedDocument*> to_delete_ptrs = {};

    to_delete_ptrs.reserve(static_cast<int>(historical.size()) - keep_latest_n);
    for (size_t i = keep_latest_n; i <static_cast<int>(historical.size()); ++i) {
        to_delete_ptrs.push_back(historical[i]);
    }

    size_t before = versions.size();

    versions.erase(
        std::remove_if(versions.begin(), versions.end(),
                       [&]([[maybe_unused]] const VersionedDocument& v) {
                           if (v.isCurrent()) {
                             return false;
                           }
                           for (const auto* p : to_delete_ptrs) {
                               if (p == &v) {
                                 return true;
                               }
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
    return static_cast<int>(rows_.size());
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

    nlohmann::json stats = {
        {"table_name",       table_name_},
        {"history_table",    config_.history_table_name},
        {"key_count",static_cast<int>(rows_.size())},
        {"current_rows",     current_count},
        {"historical_rows",  historical_count},
        {"total_versions",   current_count + historical_count},
        {"compress_history", config_.compress_history},
        {"track_user_id",    config_.track_user_id},
        {"retention_period_ms",
             static_cast<int64_t>(config_.retention_period.count())}};

    if (!schema_.is_null() && !schema_.empty()) {
        stats["schema"] = schema_;
    }

    return stats;
}

// ============================================================================
// Retention
// ============================================================================

size_t SystemVersionedTable::enforceRetentionPolicy() {
    if (config_.retention_period.count() == 0) {
        return 0;
    }

    // retention_period is stored in milliseconds; Timestamp is also in ms.
    const Timestamp cutoff = now() - static_cast<Timestamp>(
        config_.retention_period.count());

    return purgeHistoricalVersions([cutoff](const VersionedDocument& v) {
        // Remove historical versions whose sys_end is before the cutoff
        return v.sys_time.end < cutoff;
    });
}

// ============================================================================
// Payload replacement (used by TemporalCompressor)
// ============================================================================

bool SystemVersionedTable::replaceHistoricalPayload(const std::string& key,
                                                     Timestamp sys_start,
                                                     const Document& new_data) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rows_.find(key);
    if (it == rows_.end()) {
        return false;
    }

    for (auto& v : it->second) {
        if (v.sys_time.start == sys_start) {
            if (v.isCurrent()) {
                // Never replace the payload of the live current version
                return false;
            }
            v.data = new_data;
            return true;
        }
    }
    return false;
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

VersionedDocument SystemVersionedTable::makeVersion(const std::string& key,
                                                    Document data,
                                                    Timestamp ts) const {
    VersionedDocument vdoc;
    vdoc.key        = key;
    vdoc.data       = std::move(data);
    vdoc.sys_time   = {ts, kMaxTimestamp};
    vdoc.valid_time = {ts, kMaxTimestamp};
    if (config_.track_user_id) {
        vdoc.modified_by = source_node_;
    }
    return vdoc;
}

} // namespace temporal
} // namespace themisdb
