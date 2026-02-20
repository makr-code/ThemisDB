/**
 * ThemisDB Retention Manager Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/retention_manager.h"
#include <algorithm>

namespace themisdb {
namespace temporal {

// ============================================================================
// Policy management
// ============================================================================

void RetentionManager::setPolicy(const std::string& table_name,
                                  const RetentionPolicy& policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    policies_[table_name] = policy;
}

std::optional<RetentionPolicy> RetentionManager::getPolicy(
    const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = policies_.find(table_name);
    if (it == policies_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ============================================================================
// Enforcement
// ============================================================================

RetentionStats RetentionManager::enforceRetention(SystemVersionedTable& table) {
    RetentionPolicy policy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = policies_.find(table.tableName());
        if (it == policies_.end()) {
            RetentionStats stats;
            stats.errors.push_back("No retention policy registered for table: " +
                                   table.tableName());
            return stats;
        }
        policy = it->second;
    }

    auto result = applyPolicy(table, policy);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        total_deleted_  += result.versions_deleted;
        total_archived_ += result.versions_archived;
    }
    return result;
}

RetentionStats RetentionManager::enforceRetention(SystemVersionedTable& table,
                                                   const RetentionPolicy& policy) {
    auto result = applyPolicy(table, policy);

    std::lock_guard<std::mutex> lock(mutex_);
    total_deleted_  += result.versions_deleted;
    total_archived_ += result.versions_archived;
    return result;
}

// ============================================================================
// Archive
// ============================================================================

std::vector<ArchivedRecord> RetentionManager::getArchivedRecords() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return archive_;
}

std::vector<ArchivedRecord> RetentionManager::getArchivedRecords(
    const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ArchivedRecord> result;
    for (const auto& r : archive_) {
        if (r.document.modified_by == table_name ||
            r.archive_tag.find(table_name) != std::string::npos) {
            result.push_back(r);
        }
    }
    return result;
}

void RetentionManager::clearArchive() {
    std::lock_guard<std::mutex> lock(mutex_);
    archive_.clear();
}

nlohmann::json RetentionManager::getCumulativeStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {{"total_deleted", total_deleted_},
            {"total_archived", total_archived_},
            {"archive_size", archive_.size()},
            {"registered_policies", policies_.size()}};
}

// ============================================================================
// Private helpers
// ============================================================================

RetentionStats RetentionManager::applyPolicy(SystemVersionedTable& table,
                                              const RetentionPolicy& policy) {
    auto t_start = std::chrono::steady_clock::now();
    RetentionStats stats;

    Timestamp cutoff = now();
    if (policy.type == RetentionType::TIME_BASED) {
        cutoff -= policy.retention_period.count();
    }

    // Gather all keys
    auto current_rows = table.scan(kMaxTimestamp);
    std::vector<std::string> keys;
    keys.reserve(current_rows.size());
    for (const auto& r : current_rows) {
        keys.push_back(r.key);
    }
    // Also handle deleted keys: we query all history across all keys
    // For efficiency we reconstruct unique keys from all history
    // (current_rows already has them if the key is still alive).
    // For keys that were deleted we cannot easily enumerate them without
    // a full scan API – in production this would be solved by a key-list store.
    // Here we restrict to currently-known keys for simplicity.

    for (const auto& key : keys) {
        auto history = table.getHistory(key);
        stats.versions_examined += history.size();

        // Separate current vs. historical versions
        std::vector<VersionedDocument> to_delete;
        std::vector<VersionedDocument> to_keep;

        for (const auto& v : history) {
            if (v.isCurrent()) {
                // Never delete the current version
                to_keep.push_back(v);
                continue;
            }

            bool should_keep = false;

            switch (policy.type) {
                case RetentionType::TIME_BASED:
                    // Keep if the version closed AFTER the cutoff (still within retention)
                    should_keep = (v.sys_time.end > cutoff);
                    break;

                case RetentionType::VERSION_COUNT_BASED:
                    // Handled after this loop
                    to_keep.push_back(v);
                    continue;

                case RetentionType::CUSTOM:
                    should_keep = policy.should_keep
                                      ? policy.should_keep(v)
                                      : true;
                    break;
            }

            if (should_keep) {
                to_keep.push_back(v);
            } else {
                to_delete.push_back(v);
            }
        }

        // VERSION_COUNT_BASED: sort non-current versions by sys_start
        // descending and keep only the top N
        if (policy.type == RetentionType::VERSION_COUNT_BASED) {
            // Separate current from historical
            std::vector<VersionedDocument> non_current;
            VersionedDocument* current_ver = nullptr;
            // Re-scan from to_keep which has all versions here
            for (auto& v : to_keep) {
                if (!v.isCurrent()) {
                    non_current.push_back(v);
                }
            }
            // Sort by sys_start descending (newest first)
            std::sort(non_current.begin(), non_current.end(),
                      [](const VersionedDocument& a, const VersionedDocument& b) {
                          return a.sys_time.start > b.sys_time.start;
                      });

            // Keep only max_versions_per_key historical versions
            to_delete.clear();
            to_keep.clear();
            // Re-add current
            for (auto& v : history) {
                if (v.isCurrent()) {
                    to_keep.push_back(v);
                }
            }
            for (size_t i = 0; i < non_current.size(); ++i) {
                if (i < policy.max_versions_per_key) {
                    to_keep.push_back(non_current[i]);
                } else {
                    to_delete.push_back(non_current[i]);
                }
            }
        }

        // Archive and delete
        for (const auto& v : to_delete) {
            if (policy.archive_before_delete) {
                ArchivedRecord ar;
                ar.document    = v;
                ar.archive_tag = policy.archive_tag.empty()
                                     ? table.tableName()
                                     : policy.archive_tag;
                ar.archived_at = now();
                std::lock_guard<std::mutex> lock(mutex_);
                archive_.push_back(std::move(ar));
                ++stats.versions_archived;
            }
            ++stats.versions_deleted;
        }
    }

    auto t_end = std::chrono::steady_clock::now();
    stats.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_end - t_start);

    // NOTE: Actual deletion from the table's storage would require the table
    // to expose a purgeVersion() API.  That API is intentionally omitted to
    // keep SystemVersionedTable append-only in v1.1.  The RetentionManager
    // therefore records what *would* be deleted, making the stats actionable
    // without mutating the authoritative table data.
    return stats;
}

} // namespace temporal
} // namespace themisdb
