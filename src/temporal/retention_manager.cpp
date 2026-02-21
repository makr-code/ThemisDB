/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            retention_manager.cpp                              ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:43:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     335                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 308ab7d2f  2026-02-20  feat(temporal): SQL:2011 Temporal Module – Production Rea... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Retention Manager Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/retention_manager.h"
#include <algorithm>
#include <chrono>
#include <thread>

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
// Background Scheduler
// ============================================================================

RetentionManager::~RetentionManager() {
    stopScheduler();
}

void RetentionManager::scheduleTable(SystemVersionedTable& table,
                                     std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    ScheduledTable st;
    st.table    = &table;
    st.interval = interval < std::chrono::milliseconds(1)
                      ? std::chrono::milliseconds(1)
                      : interval;
    st.next_run = std::chrono::steady_clock::now(); // run immediately on first tick
    scheduled_tables_.push_back(std::move(st));
}

void RetentionManager::startScheduler() {
    bool expected = false;
    if (!scheduler_running_.compare_exchange_strong(expected, true)) {
        return; // already running
    }
    scheduler_stop_.store(false, std::memory_order_release);
    scheduler_thread_ = std::thread(&RetentionManager::schedulerLoop, this);
}

void RetentionManager::stopScheduler() {
    if (!scheduler_running_.load(std::memory_order_acquire)) {
        return;
    }
    scheduler_stop_.store(true, std::memory_order_release);
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    scheduler_running_.store(false, std::memory_order_release);
}

bool RetentionManager::schedulerRunning() const noexcept {
    return scheduler_running_.load(std::memory_order_acquire);
}

void RetentionManager::schedulerLoop() {
    while (!scheduler_stop_.load(std::memory_order_acquire)) {
        auto now_tp = std::chrono::steady_clock::now();

        std::vector<SystemVersionedTable*> due_tables;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& st : scheduled_tables_) {
                if (now_tp >= st.next_run) {
                    due_tables.push_back(st.table);
                    st.next_run = now_tp + st.interval;
                }
            }
        }

        for (auto* tbl : due_tables) {
            // enforceRetention acquires mutex_ internally; call outside the lock
            enforceRetention(*tbl);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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

    // Use getAllKeys() so that fully-deleted keys are also included.
    auto keys = table.getAllKeys();

    for (const auto& key : keys) {
        auto history = table.getHistory(key);
        stats.versions_examined += history.size();

        // Build the set of historical versions to delete
        std::vector<VersionedDocument> to_delete;

        // VERSION_COUNT_BASED: use the dedicated keep-latest-N API to avoid
        // timestamp-collision issues when all updates happen in the same ms.
        if (policy.type == RetentionType::VERSION_COUNT_BASED) {
            // Count historical versions
            size_t historical_count = 0;
            for (const auto& v : history) {
                if (!v.isCurrent()) ++historical_count;
            }

            if (historical_count > policy.max_versions_per_key) {
                size_t to_delete_count =
                    historical_count - policy.max_versions_per_key;
                stats.versions_deleted += to_delete_count;

                // Archive before purge if requested
                if (policy.archive_before_delete) {
                    std::vector<VersionedDocument> non_current;
                    for (const auto& v : history) {
                        if (!v.isCurrent()) non_current.push_back(v);
                    }
                    std::sort(non_current.begin(), non_current.end(),
                              [](const VersionedDocument& a, const VersionedDocument& b) {
                                  return a.sys_time.start > b.sys_time.start;
                              });
                    for (size_t i = policy.max_versions_per_key;
                         i < non_current.size(); ++i) {
                        ArchivedRecord ar;
                        ar.document    = non_current[i];
                        ar.archive_tag = policy.archive_tag.empty()
                                             ? table.tableName()
                                             : policy.archive_tag;
                        ar.archived_at = now();
                        std::lock_guard<std::mutex> lock(mutex_);
                        archive_.push_back(std::move(ar));
                        ++stats.versions_archived;
                    }
                }

                table.purgeHistoricalVersionsKeepLatestN(
                    key, policy.max_versions_per_key);
            }
        } else {
            // TIME_BASED or CUSTOM
            for (const auto& v : history) {
                if (v.isCurrent()) {
                    continue; // never touch the current version
                }
                bool should_keep = false;
                switch (policy.type) {
                    case RetentionType::TIME_BASED:
                        should_keep = (v.sys_time.end > cutoff);
                        break;
                    case RetentionType::CUSTOM:
                        should_keep = policy.should_keep ? policy.should_keep(v) : true;
                        break;
                    default:
                        should_keep = true;
                        break;
                }
                if (!should_keep) {
                    to_delete.push_back(v);
                }
            }

            // Archive versions before purging them
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

            // Physically remove the flagged versions from the table.
            // Build a set of (sys_start, sys_end) pairs for O(1) lookup.
            if (!to_delete.empty()) {
                std::vector<std::pair<Timestamp, Timestamp>> delete_set;
                delete_set.reserve(to_delete.size());
                for (const auto& v : to_delete) {
                    delete_set.emplace_back(v.sys_time.start, v.sys_time.end);
                }

                table.purgeHistoricalVersions(key, [&](const VersionedDocument& v) {
                    for (const auto& d : delete_set) {
                        if (v.sys_time.start == d.first &&
                            v.sys_time.end   == d.second) {
                            return true;
                        }
                    }
                    return false;
                });
            }
        }
    }

    auto t_end = std::chrono::steady_clock::now();
    stats.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_end - t_start);

    return stats;
}

} // namespace temporal
} // namespace themisdb
