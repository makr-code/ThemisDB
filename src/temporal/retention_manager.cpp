/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            retention_manager.cpp                              ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:00:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     330                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
#include <limits>
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

    int attempts = 0;
    int max_attempts = 1 + std::max(0, policy.max_retries);
    RetentionStats result;

    while (attempts < max_attempts) {
        result = applyPolicy(table, policy);
        if (result.errors.empty()) {
            break;
        }
        ++attempts;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        total_deleted_          += result.versions_deleted;
        total_archived_         += result.versions_archived;
        total_space_freed_bytes_ += result.space_freed_bytes;
    }
    return result;
}

RetentionStats RetentionManager::enforceRetention(SystemVersionedTable& table,
                                                   const RetentionPolicy& policy) {
    int attempts = 0;
    int max_attempts = 1 + std::max(0, policy.max_retries);
    RetentionStats result;

    while (attempts < max_attempts) {
        result = applyPolicy(table, policy);
        if (result.errors.empty()) {
            break;
        }
        ++attempts;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    total_deleted_           += result.versions_deleted;
    total_archived_          += result.versions_archived;
    total_space_freed_bytes_ += result.space_freed_bytes;
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
            {"total_space_freed_bytes", total_space_freed_bytes_},
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

/// Estimate the serialised storage footprint of a single versioned document.
static uint64_t estimateVersionSize(const VersionedDocument& v) {
    return static_cast<uint64_t>(v.key.size()) +
           static_cast<uint64_t>(v.data.dump().size()) +
           32u; // overhead: timestamps + metadata fields
}

RetentionStats RetentionManager::applyPolicy(SystemVersionedTable& table,
                                              const RetentionPolicy& policy) {
    auto t_start = std::chrono::steady_clock::now();
    RetentionStats stats;

    Timestamp cutoff = now();
    if (policy.type == RetentionType::TIME_BASED) {
        cutoff -= policy.retention_period.count();
    }

    // Minimum retention guard: versions younger than this are always kept.
    Timestamp min_keep_before = now();
    if (policy.minimum_retention_period.count() > 0) {
        min_keep_before -= policy.minimum_retention_period.count();
    }

    // Use getAllKeys() so that fully-deleted keys are also included.
    auto keys = table.getAllKeys();

    // ── STORAGE_BASED ──────────────────────────────────────────────────────
    if (policy.type == RetentionType::STORAGE_BASED) {
        if (policy.max_storage_bytes == 0) {
            // No limit configured – nothing to do.
            auto t_end = std::chrono::steady_clock::now();
            stats.execution_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
            return stats;
        }

        // Collect all historical (non-current) versions across all keys
        // together with their estimated sizes.
        struct HistEntry {
            std::string key;
            VersionedDocument doc;
            uint64_t size_bytes;
        };
        std::vector<HistEntry> all_historical;
        uint64_t total_size = 0;

        for (const auto& key : keys) {
            auto history = table.getHistory(key);
            stats.versions_examined += history.size();
            for (const auto& v : history) {
                if (v.isCurrent()) continue;
                uint64_t sz = estimateVersionSize(v);
                total_size += sz;
                all_historical.push_back({key, v, sz});
            }
        }

        if (total_size <= policy.max_storage_bytes) {
            auto t_end = std::chrono::steady_clock::now();
            stats.execution_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
            return stats;
        }

        // Sort oldest first (by sys_start ascending) – delete oldest first.
        std::sort(all_historical.begin(), all_historical.end(),
                  [](const HistEntry& a, const HistEntry& b) {
                      return a.doc.sys_time.start < b.doc.sys_time.start;
                  });

        size_t batch_remaining = (policy.incremental_batch_size > 0)
                                     ? policy.incremental_batch_size
                                     : std::numeric_limits<size_t>::max();

        for (auto& entry : all_historical) {
            if (total_size <= policy.max_storage_bytes) break;
            if (batch_remaining == 0) break;

            // Compliance minimum: skip versions that are too young to delete.
            if (policy.minimum_retention_period.count() > 0 &&
                entry.doc.sys_time.start > min_keep_before) {
                continue;
            }

            if (policy.archive_before_delete) {
                ArchivedRecord ar;
                ar.document    = entry.doc;
                ar.archive_tag = policy.archive_tag.empty()
                                     ? (policy.compliance_tag.empty()
                                            ? table.tableName()
                                            : policy.compliance_tag)
                                     : policy.archive_tag;
                ar.archived_at = now();
                std::lock_guard<std::mutex> lock(mutex_);
                archive_.push_back(std::move(ar));
                ++stats.versions_archived;
            }

            Timestamp del_start = entry.doc.sys_time.start;
            Timestamp del_end   = entry.doc.sys_time.end;
            table.purgeHistoricalVersions(
                entry.key, [del_start, del_end](const VersionedDocument& v) {
                    return v.sys_time.start == del_start &&
                           v.sys_time.end == del_end;
                });

            total_size -= entry.size_bytes;
            stats.space_freed_bytes += entry.size_bytes;
            ++stats.versions_deleted;
            --batch_remaining;
        }

        auto t_end = std::chrono::steady_clock::now();
        stats.execution_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
        return stats;
    }

    // ── VERSION_COUNT_BASED ────────────────────────────────────────────────
    // Use the dedicated keep-latest-N API to avoid timestamp-collision issues
    // when all updates happen in the same millisecond.
    if (policy.type == RetentionType::VERSION_COUNT_BASED) {
        size_t batch_remaining = (policy.incremental_batch_size > 0)
                                     ? policy.incremental_batch_size
                                     : std::numeric_limits<size_t>::max();

        for (const auto& key : keys) {
            if (batch_remaining == 0) break;

            auto history = table.getHistory(key);
            stats.versions_examined += history.size();

            // Count historical versions
            size_t historical_count = 0;
            for (const auto& v : history) {
                if (!v.isCurrent()) ++historical_count;
            }

            if (historical_count <= policy.max_versions_per_key) continue;

            size_t eligible_to_delete = historical_count - policy.max_versions_per_key;
            size_t count_to_delete    = std::min(eligible_to_delete, batch_remaining);
            batch_remaining -= count_to_delete;

            // Sort historical versions oldest-first
            std::vector<VersionedDocument> non_current;
            non_current.reserve(historical_count);
            for (const auto& v : history) {
                if (!v.isCurrent()) non_current.push_back(v);
            }
            std::sort(non_current.begin(), non_current.end(),
                      [](const VersionedDocument& a, const VersionedDocument& b) {
                          return a.sys_time.start < b.sys_time.start;
                      });

            // Archive and account for the versions that will be deleted
            for (size_t i = 0; i < count_to_delete && i < non_current.size(); ++i) {
                if (policy.archive_before_delete) {
                    ArchivedRecord ar;
                    ar.document    = non_current[i];
                    ar.archive_tag = policy.archive_tag.empty()
                                         ? (policy.compliance_tag.empty()
                                                ? table.tableName()
                                                : policy.compliance_tag)
                                         : policy.archive_tag;
                    ar.archived_at = now();
                    std::lock_guard<std::mutex> lock(mutex_);
                    archive_.push_back(std::move(ar));
                    ++stats.versions_archived;
                }
                stats.space_freed_bytes += estimateVersionSize(non_current[i]);
                ++stats.versions_deleted;
            }

            // Purge using a countdown predicate that removes the oldest
            // count_to_delete historical versions (safe against timestamp collisions).
            size_t purge_count = 0;
            table.purgeHistoricalVersions(key, [&](const VersionedDocument& v) -> bool {
                if (purge_count >= count_to_delete) return false;
                // The predicate is called in vector order (oldest-first after insert).
                // We want to remove the oldest count_to_delete versions.
                // Accept any non-current version until quota is met.
                ++purge_count;
                return true;
            });
        }

        auto t_end = std::chrono::steady_clock::now();
        stats.execution_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
        return stats;
    }

    // ── TIME_BASED or CUSTOM ───────────────────────────────────────────────
    size_t batch_remaining = (policy.incremental_batch_size > 0)
                                 ? policy.incremental_batch_size
                                 : std::numeric_limits<size_t>::max();

    for (const auto& key : keys) {
        if (batch_remaining == 0) break;

        auto history = table.getHistory(key);
        stats.versions_examined += history.size();

        // Collect eligible-to-delete versions (respects compliance guard and policy type)
        std::vector<VersionedDocument> eligible;
        for (const auto& v : history) {
            if (v.isCurrent()) continue;
            // Compliance minimum retention guard
            if (policy.minimum_retention_period.count() > 0 &&
                v.sys_time.start > min_keep_before) {
                continue;
            }
            bool should_del = false;
            switch (policy.type) {
                case RetentionType::TIME_BASED:
                    should_del = !(v.sys_time.end > cutoff);
                    break;
                case RetentionType::CUSTOM:
                    should_del = !(policy.should_keep ? policy.should_keep(v) : true);
                    break;
                default:
                    should_del = false;
                    break;
            }
            if (should_del) {
                eligible.push_back(v);
            }
        }

        // Respect incremental batch limit
        size_t count_to_delete = std::min(eligible.size(), batch_remaining);
        if (count_to_delete == 0) continue;
        batch_remaining -= count_to_delete;

        // Archive the first count_to_delete eligible versions
        for (size_t i = 0; i < count_to_delete; ++i) {
            if (policy.archive_before_delete) {
                ArchivedRecord ar;
                ar.document    = eligible[i];
                ar.archive_tag = policy.archive_tag.empty()
                                     ? (policy.compliance_tag.empty()
                                            ? table.tableName()
                                            : policy.compliance_tag)
                                     : policy.archive_tag;
                ar.archived_at = now();
                std::lock_guard<std::mutex> lock(mutex_);
                archive_.push_back(std::move(ar));
                ++stats.versions_archived;
            }
            stats.space_freed_bytes += estimateVersionSize(eligible[i]);
            ++stats.versions_deleted;
        }

        // Physically remove exactly count_to_delete eligible versions.
        // Use a countdown predicate so that versions with identical timestamps
        // (which can occur when multiple updates happen within the same ms) are
        // not over-deleted.
        size_t purge_count = 0;
        table.purgeHistoricalVersions(key, [&](const VersionedDocument& v) -> bool {
            if (purge_count >= count_to_delete) return false;
            if (policy.minimum_retention_period.count() > 0 &&
                v.sys_time.start > min_keep_before) {
                return false;
            }
            bool should_del = false;
            switch (policy.type) {
                case RetentionType::TIME_BASED:
                    should_del = !(v.sys_time.end > cutoff);
                    break;
                case RetentionType::CUSTOM:
                    should_del = !(policy.should_keep ? policy.should_keep(v) : true);
                    break;
                default:
                    should_del = false;
                    break;
            }
            if (should_del) {
                ++purge_count;
                return true;
            }
            return false;
        });
    }

    auto t_end = std::chrono::steady_clock::now();
    stats.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_end - t_start);

    return stats;
}

} // namespace temporal
} // namespace themisdb
