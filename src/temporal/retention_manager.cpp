/**
 * @file retention_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=20, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <stdexcept>
#include <thread>

namespace themisdb {
namespace temporal {

// ============================================================================
// RetentionRule — comparison operators
// ============================================================================

bool RetentionRule::operator==(const RetentionRule& rhs) const noexcept {
    return std::tie(type, period, max_versions, max_bytes, tag)
        == std::tie(rhs.type, rhs.period, rhs.max_versions, rhs.max_bytes, rhs.tag);
}

bool RetentionRule::operator<(const RetentionRule& rhs) const noexcept {
    return std::tie(type, period, max_versions, max_bytes, tag)
        < std::tie(rhs.type, rhs.period, rhs.max_versions, rhs.max_bytes, rhs.tag);
}

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
        try {
            result = applyPolicy(table, policy);
        } catch (const std::exception& e) {
            result.errors.push_back(std::string("applyPolicy exception: ") + e.what());
        } catch (...) {
            result.errors.push_back("applyPolicy: unknown exception");
        }
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
        try {
            result = applyPolicy(table, policy);
        } catch (const std::exception& e) {
            result.errors.push_back(std::string("applyPolicy exception: ") + e.what());
        } catch (...) {
            result.errors.push_back("applyPolicy: unknown exception");
        }
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
    std::vector<ArchivedRecord> result = {};

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

/// Resolve the archive tag:
///   1. Explicit archive_tag  → used as-is (backwards compatible).
///   2. compliance_tag only   → "<table_name>:<compliance_tag>" so that
///      getArchivedRecords("<table>") (which filters by tags containing the
///      table name) can still retrieve these records.
///   3. Neither set           → table name alone.
static std::string resolveArchiveTag(const RetentionPolicy& policy,
                                     const std::string& table_name) {
    if (!policy.archive_tag.empty()) {
      return policy.archive_tag;
    }
    if (!policy.compliance_tag.empty()) {
      return table_name + ":" + policy.compliance_tag;
    }
    return table_name;
}

/// Return the per-run deletion quota (unlimited when incremental_batch_size == 0).
static size_t batchLimit(const RetentionPolicy& policy) {
    return policy.incremental_batch_size > 0
               ? policy.incremental_batch_size
               : std::numeric_limits<size_t>::max();
}

RetentionStats RetentionManager::applyPolicy(SystemVersionedTable& table,
                                              const RetentionPolicy& policy) {
    auto t_start = std::chrono::steady_clock::now();
    RetentionStats stats;

    Timestamp cutoff = now();
    if (policy.type == RetentionType::TIME_BASED) {
        cutoff -= policy.retention_period.count();
    }

    // Minimum retention guard: versions younger than this threshold are always
    // kept regardless of the primary policy type.
    Timestamp min_keep_before = now();
    if (policy.minimum_retention_period.count() > 0) {
        min_keep_before -= policy.minimum_retention_period.count();
    }

    /// Returns true when version v must be kept due to compliance minimum retention.
    auto isProtected = [&]([[maybe_unused]] const VersionedDocument& v) -> bool {
        return policy.minimum_retention_period.count() > 0 &&
               v.sys_time.start > min_keep_before;
    };

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

        // Collect lightweight metadata for every non-current version so we can
        // sort and decide what to delete without holding copies of all documents.
        struct HistMeta {
            std::string   key = {};
            Timestamp     sys_start;
            Timestamp     sys_end;
            uint64_t      size_bytes = {};
        };
        std::vector<HistMeta> all_historical;
        uint64_t total_size = 0;

        for (const auto& key : keys) {
            auto history = table.getHistory(key);
            stats.versions_examined += history.size();
            for (const auto& v : history) {
                if (v.isCurrent()) {
                  continue;
                }
                uint64_t sz = estimateVersionSize(v);
                total_size += sz;
                all_historical.push_back({key, v.sys_time.start, v.sys_time.end, sz});
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
                  [](const HistMeta& a, const HistMeta& b) {
                      return a.sys_start < b.sys_start;
                  });

        size_t batch_remaining = batchLimit(policy);

        for (const auto& meta : all_historical) {
            if (total_size <= policy.max_storage_bytes) {
              break;
            }
            if (batch_remaining == 0) {
              break;
            }

            // Compliance minimum: skip versions that are too young to delete.
            // We use sys_start as the version birth time for the guard.
            if (policy.minimum_retention_period.count() > 0 &&
                meta.sys_start > min_keep_before) {
                continue;
            }

            // Archive the document before purging, if required.
            if (policy.archive_before_delete) {
                // Retrieve the full document only when we actually need it.
                auto history = table.getHistory(meta.key);
                for (const auto& v : history) {
                    if (v.sys_time.start == meta.sys_start &&
                        v.sys_time.end   == meta.sys_end) {
                        ArchivedRecord ar;
                        ar.document    = v;
                        ar.archive_tag = resolveArchiveTag(policy, table.tableName());
                        ar.archived_at = now();
                        std::lock_guard<std::mutex> lock(mutex_);
                        archive_.push_back(std::move(ar));
                        ++stats.versions_archived;
                        break;
                    }
                }
            }

            // Use a countdown predicate so that versions sharing identical
            // sys_time intervals (e.g. millisecond-resolution collisions) do not
            // cause more than one version to be removed per meta entry.
            // purgeHistoricalVersions holds the table mutex and evaluates the
            // predicate sequentially via std::remove_if, so the mutable
            // remaining_to_delete counter is safe without further synchronisation.
            size_t remaining_to_delete = 1;
            Timestamp del_start = meta.sys_start;
            Timestamp del_end   = meta.sys_end;
            size_t deleted = table.purgeHistoricalVersions(
                meta.key,
                [del_start, del_end, &remaining_to_delete](const VersionedDocument& v) -> bool {
                    if (remaining_to_delete == 0) {
                      return false;
                    }
                    if (v.sys_time.start == del_start && v.sys_time.end == del_end) {
                        --remaining_to_delete;
                        return true;
                    }
                    return false;
                });

            if (deleted > 0) {
                total_size -= meta.size_bytes;
                stats.space_freed_bytes += meta.size_bytes;
                ++stats.versions_deleted;
                --batch_remaining;
            }
        }

        auto t_end = std::chrono::steady_clock::now();
        stats.execution_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
        return stats;
    }

    // ── VERSION_COUNT_BASED ────────────────────────────────────────────────
    if (policy.type == RetentionType::VERSION_COUNT_BASED) {
        size_t batch_remaining = batchLimit(policy);

        for (const auto& key : keys) {
            if (batch_remaining == 0) {
              break;
            }

            auto history = table.getHistory(key);
            stats.versions_examined += history.size();

            // Separate historical versions into eligible (deletable) and
            // protected (compliance minimum retention must be honoured).
            std::vector<VersionedDocument> eligible;
            size_t protected_count = 0;
            for (const auto& v : history) {
                if (v.isCurrent()) {
                  continue;
                }
                if (isProtected(v)) {
                    ++protected_count;
                } else {
                    eligible.push_back(v);
                }
            }

            // How many eligible versions must we keep?  We want to retain
            // max_versions_per_key total historical versions.  Protected ones
            // always stay, so from eligible we keep the remainder.
            size_t keep_from_eligible = (policy.max_versions_per_key > protected_count)
                                            ? policy.max_versions_per_key - protected_count
                                            : 0;
            if (eligible.size() <= keep_from_eligible) {
              continue;
            }

            // Sort eligible oldest-first so that the oldest are removed first.
            std::sort(eligible.begin(), eligible.end(),
                      [](const VersionedDocument& a, const VersionedDocument& b) {
                          return a.sys_time.start < b.sys_time.start;
                      });

            size_t eligible_to_delete = eligible.size() - keep_from_eligible;
            size_t count_to_delete    = std::min(eligible_to_delete, batch_remaining);
            batch_remaining -= count_to_delete;

            // Archive and account for the versions about to be deleted.
            for (size_t i = 0; i < count_to_delete; ++i) {
                if (policy.archive_before_delete) {
                    ArchivedRecord ar;
                    ar.document    = eligible[i];
                    ar.archive_tag = resolveArchiveTag(policy, table.tableName());
                    ar.archived_at = now();
                    std::lock_guard<std::mutex> lock(mutex_);
                    archive_.push_back(std::move(ar));
                    ++stats.versions_archived;
                }
                stats.space_freed_bytes += estimateVersionSize(eligible[i]);
                ++stats.versions_deleted;
            }

            // Purge using a countdown predicate: skip protected versions and
            // remove exactly count_to_delete eligible (oldest) versions.
            // Using a countdown avoids timestamp-collision over-deletion.
            size_t purge_count = 0;
            table.purgeHistoricalVersions(key, [&]([[maybe_unused]] const VersionedDocument& v) -> bool {
                if (purge_count >= count_to_delete) {
                  return false;
                }
                if (isProtected(v)) {
                  return false;
                }
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
    size_t batch_remaining = batchLimit(policy);

    for (const auto& key : keys) {
        if (batch_remaining == 0) {
          break;
        }

        auto history = table.getHistory(key);
        stats.versions_examined += history.size();

        // Collect eligible-to-delete versions (honours compliance guard and policy).
        std::vector<VersionedDocument> eligible = {};

        for (const auto& v : history) {
            if (v.isCurrent()) {
              continue;
            }
            if (isProtected(v)) {
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

        // Respect incremental batch limit.
        size_t count_to_delete = std::min(eligible.size(), batch_remaining);
        if (count_to_delete == 0) {
          continue;
        }
        batch_remaining -= count_to_delete;

        // Archive the first count_to_delete eligible versions.
        for (size_t i = 0; i < count_to_delete; ++i) {
            if (policy.archive_before_delete) {
                ArchivedRecord ar;
                ar.document    = eligible[i];
                ar.archive_tag = resolveArchiveTag(policy, table.tableName());
                ar.archived_at = now();
                std::lock_guard<std::mutex> lock(mutex_);
                archive_.push_back(std::move(ar));
                ++stats.versions_archived;
            }
            stats.space_freed_bytes += estimateVersionSize(eligible[i]);
            ++stats.versions_deleted;
        }

        // Physically remove exactly count_to_delete eligible versions.
        // A countdown predicate avoids over-deletion when multiple versions
        // share the same timestamp (e.g. rapid updates within one millisecond).
        size_t purge_count = 0;
        table.purgeHistoricalVersions(key, [&]([[maybe_unused]] const VersionedDocument& v) -> bool {
            if (purge_count >= count_to_delete) {
              return false;
            }
            if (isProtected(v)) {
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


