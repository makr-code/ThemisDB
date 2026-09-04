/**
 * @file schema_consistency_checker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/schema_consistency_checker.h"
#include "metadata/statistics_collector.h"
#include "metadata/schema_constraints.h"
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>

namespace themis {

// ============================================================================
// ConsistencyIssue serialization
// ============================================================================

json ConsistencyIssue::toJSON() const {
    return {
        {"issue_type",   issue_type},
        {"table_name",   table_name},
        {"column_name",  column_name},
        {"detail",       detail}
    };
}

// ============================================================================
// SchemaConsistencyChecker
// ============================================================================

SchemaConsistencyChecker::SchemaConsistencyChecker(
    RocksDBWrapper&      db,
    SchemaManager&       schema_mgr,
    StatisticsCollector* stats,
    SchemaConstraints*   constraints)
    : db_(db)
    , schema_mgr_(schema_mgr)
    , stats_(stats)
    , constraints_(constraints)
{
    spdlog::debug("SchemaConsistencyChecker: initialized");
}

SchemaConsistencyChecker::~SchemaConsistencyChecker() {
    stopBackgroundCheck();
}

// ============================================================================
// runCheck
// ============================================================================

std::vector<ConsistencyIssue> SchemaConsistencyChecker::runCheck() const {
    std::vector<ConsistencyIssue> issues;

    auto orphans = checkOrphanKeys_();
    issues.insert(issues.end(), orphans.begin(), orphans.end());

    if (stats_) {
        auto stale = checkStaleStats_();
        issues.insert(issues.end(), stale.begin(), stale.end());
    }

    if (constraints_) {
        auto missing = checkMissingConstraints_();
        issues.insert(issues.end(), missing.begin(), missing.end());
    }

    spdlog::info("SchemaConsistencyChecker: check complete – {} issue(s) found", issues.size());

    // Update the cached last results (results_mutex_ is mutable — no cast needed).
    {
        std::lock_guard<std::mutex> lk(results_mutex_);
        last_results_ = issues;
    }

    return issues;
}

// ============================================================================
// Sub-checks
// ============================================================================

std::vector<ConsistencyIssue> SchemaConsistencyChecker::checkOrphanKeys_() const {
    std::vector<ConsistencyIssue> issues;

    // Collect all known table names
    std::set<std::string> known_tables;
    auto all_tables = schema_mgr_.getAllTables();
    for (const auto& ts : all_tables) {
        known_tables.insert(ts.name);
    }

    // System key prefixes that are valid even without a registered table
    static const std::vector<std::string> kSystemPrefixes = {
        "stats:", "config:", "audit:", "index:", "meta:", "wal:"
    };

    auto is_system_key = [&]([[maybe_unused]] const std::string& key) -> bool {
        for (const auto& pfx : kSystemPrefixes) {
            if (key.rfind(pfx, 0) == 0) {
              return true;
            }
        }
        return false;
    };

    // Scan all keys and check for orphans.
    // kMaxScanKeys caps the scan at 5 000 entries to bound the O(n) cost on large
    // databases.  At a typical key size of ~100 bytes that is ~500 KB of metadata
    // reads per check run.  Increase this limit if your deployment has tables with
    // more than ~4 000 row keys that should be validated (the checker detects
    // orphaned table *prefixes*, so a single orphan prefix is found after just one
    // matching key, regardless of how many rows the table has).
    constexpr size_t kMaxScanKeys = 5000;
    std::set<std::string> orphan_prefixes;

    try {
        size_t scanned = 0;
        db_.scanAll([&](std::string_view key, std::string_view /*value*/) -> bool {
            if (scanned++ >= kMaxScanKeys) {
              return false;
            }

            std::string key_str(key);
            if (is_system_key(key_str)) {
              return true;
            }

            // Extract table prefix: key format is "table_name:row_id"
            auto colon = key_str.find(':');
            if (colon == std::string::npos) return true;  // no prefix

            std::string prefix = key_str.substr(0, colon);
            if (known_tables.find(prefix) == known_tables.end()) {
                orphan_prefixes.insert(prefix);
            }
            return true;
        });
    } catch (const std::exception& e) {
        spdlog::warn("SchemaConsistencyChecker: orphan key scan failed: {}", e.what());
    }

    for (const auto& pfx : orphan_prefixes) {
        ConsistencyIssue issue;
        issue.issue_type = "orphan_key";
        issue.table_name = pfx;
        issue.detail     = "Keys with prefix '" + pfx + "' found in storage but '"
                           + pfx + "' is not a registered table";
        issues.push_back(std::move(issue));
    }

    return issues;
}

std::vector<ConsistencyIssue> SchemaConsistencyChecker::checkStaleStats_() const {
    std::vector<ConsistencyIssue> issues = {};

    if (!stats_) {
      return issues;
    }

    auto all_tables = schema_mgr_.getAllTables();
    auto now        = std::chrono::system_clock::now();

    for (const auto& ts : all_tables) {
        auto result = stats_->getStats(ts.name);
        if (!result.ok) {
            // No stats at all → report as stale
            ConsistencyIssue issue;
            issue.issue_type = "stale_stats";
            issue.table_name = ts.name;
            issue.detail     = "No statistics collected for table '" + ts.name + "'";
            issues.push_back(std::move(issue));
            continue;
        }

        auto age = std::chrono::duration_cast<std::chrono::hours>(
            now - result.value.last_updated);
        if (age > max_stats_age_) {
            ConsistencyIssue issue;
            issue.issue_type = "stale_stats";
            issue.table_name = ts.name;
            issue.detail     = "Statistics for '" + ts.name + "' are "
                               + std::to_string(age.count())
                               + "h old (max " + std::to_string(max_stats_age_.count()) + "h)";
            issues.push_back(std::move(issue));
        }
    }

    return issues;
}

std::vector<ConsistencyIssue> SchemaConsistencyChecker::checkMissingConstraints_() const {
    std::vector<ConsistencyIssue> issues = {};

    if (!constraints_) {
      return issues;
    }

    auto all_tables = schema_mgr_.getAllTables();
    for (const auto& ts : all_tables) {
        auto cols = constraints_->getTableConstraints(ts.name);
        if (cols.empty()) {
            ConsistencyIssue issue;
            issue.issue_type = "missing_constraint";
            issue.table_name = ts.name;
            issue.detail     = "Table '" + ts.name
                               + "' has no registered constraints (consider adding NOT NULL or UNIQUE)";
            issues.push_back(std::move(issue));
        }
    }

    return issues;
}

// ============================================================================
// Background checking
// ============================================================================

void SchemaConsistencyChecker::startBackgroundCheck(std::chrono::seconds interval) {
    stopBackgroundCheck();

    bg_interval_ = interval;
    if (interval.count() <= 0) {
        spdlog::debug("SchemaConsistencyChecker: background check disabled");
        return;
    }

    stop_bg_.store(false);
    bg_thread_ = std::thread([this] { bgLoop_(); });
    spdlog::info("SchemaConsistencyChecker: background check started (interval={}s)",
                 interval.count());
}

void SchemaConsistencyChecker::stopBackgroundCheck() noexcept {
    stop_bg_.store(true);
    bg_cv_.notify_all();
    if (bg_thread_.joinable()) {
        bg_thread_.join();
    }
}

void SchemaConsistencyChecker::bgLoop_() {
    while (!stop_bg_.load()) {
        std::unique_lock<std::mutex> lk(bg_mutex_);
        bg_cv_.wait_for(lk, bg_interval_,
                        [this] { return stop_bg_.load(); });

        if (stop_bg_.load()) {
          break;
        }
        try {
            runCheck();
        } catch (const std::exception& e) {
            spdlog::error("SchemaConsistencyChecker: background check threw: {}", e.what());
        }
    }
    spdlog::debug("SchemaConsistencyChecker: background thread exited");
}

// ============================================================================
// Results access
// ============================================================================

std::vector<ConsistencyIssue> SchemaConsistencyChecker::getLastCheckResults() const {
    std::lock_guard<std::mutex> lk(results_mutex_);
    return last_results_;
}

json SchemaConsistencyChecker::lastResultsToJSON() const {
    std::lock_guard<std::mutex> lk(results_mutex_);
    json arr = json::array();
    for (const auto& issue : last_results_) {
        arr.push_back(issue.toJSON());
    }
    return arr;
}

} // namespace themis
