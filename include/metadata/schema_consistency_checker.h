/**
 * @file schema_consistency_checker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SchemaManager;
class StatisticsCollector;
class SchemaConstraints;

using json = nlohmann::json;

/// A single consistency issue discovered during a schema check
struct ConsistencyIssue {
    /// The category of problem
    std::string issue_type;    ///< "orphan_key", "stale_stats", "missing_constraint", "schema_mismatch"
    std::string table_name;    ///< Affected table (may be empty for global issues)
    std::string column_name;   ///< Affected column (may be empty)
    std::string detail;        ///< Human-readable description of the problem

    json toJSON() const;
};

/// SchemaConsistencyChecker – periodic background scan for schema health issues
///
/// Checks performed:
///   - **Orphan keys**: RocksDB keys whose prefix does not match any registered
///     table in SchemaManager.
///   - **Stale statistics**: Tables whose stats have not been refreshed for longer
///     than the configured maximum age (default: 24 hours).
///   - **Missing constraints**: Tables in SchemaManager that have no registered
///     constraints in SchemaConstraints (when a SchemaConstraints instance is
///     provided and its table list is non-empty).
///
/// Usage:
///   SchemaConsistencyChecker checker(db, schema_mgr, &stats, &constraints);
///   checker.startBackgroundCheck(std::chrono::hours(6));
///   // … later …
///   auto issues = checker.getLastCheckResults();
///
/// Thread-safety: `runCheck()` is thread-safe; `startBackgroundCheck()` and
/// `stopBackgroundCheck()` are NOT thread-safe with respect to each other and
/// must be called from the same owner thread.
class SchemaConsistencyChecker {
public:
    /// Default maximum statistics age before a "stale_stats" issue is raised.
    static constexpr std::chrono::hours kDefaultMaxStatsAge{24};

    /// Constructor
    /// @param db          RocksDB wrapper for key prefix scanning
    /// @param schema_mgr  SchemaManager whose table list defines the truth
    /// @param stats       Optional statistics collector (for stale-stats checks)
    /// @param constraints Optional constraints engine (for missing-constraint checks)
    explicit SchemaConsistencyChecker(
        RocksDBWrapper&      db,
        SchemaManager&       schema_mgr,
        StatisticsCollector* stats       = nullptr,
        SchemaConstraints*   constraints = nullptr
    );

    /// Destructor – stops the background thread if running.
    ~SchemaConsistencyChecker();

    // Disable copy
    SchemaConsistencyChecker(const SchemaConsistencyChecker&) = delete;
    SchemaConsistencyChecker& operator=(const SchemaConsistencyChecker&) = delete;

    // ========================================================================
    // Public API
    // ========================================================================

    /// Run a single synchronous consistency check and return all issues found.
    /// This method is thread-safe and can be called while the background thread
    /// is also running.
    std::vector<ConsistencyIssue> runCheck() const;

    /// Start a background thread that calls runCheck() every @p interval.
    /// Calling this a second time replaces the interval.
    /// Pass std::chrono::seconds(0) to stop the background thread.
    void startBackgroundCheck(std::chrono::seconds interval);

    /// Stop the background thread (blocking until it exits).
    /// Called automatically by the destructor.
    void stopBackgroundCheck() noexcept;

    /// Return the results of the most recent background (or manual) check.
    /// Returns an empty vector if no check has run yet.
    std::vector<ConsistencyIssue> getLastCheckResults() const;

    /// Serialise the last check results as a JSON array.
    json lastResultsToJSON() const;

    /// Configure the maximum acceptable statistics age.
    /// Issues are raised for any table whose stats are older than this.
    void setMaxStatsAge(std::chrono::hours max_age) noexcept { max_stats_age_ = max_age; }

private:
    // ========================================================================
    // Sub-checks
    // ========================================================================

    /// Check for orphaned RocksDB keys (keys whose prefix is not a known table).
    std::vector<ConsistencyIssue> checkOrphanKeys_() const;

    /// Check for tables with stale or missing statistics.
    std::vector<ConsistencyIssue> checkStaleStats_() const;

    /// Check for tables in the schema that have no constraint definitions.
    std::vector<ConsistencyIssue> checkMissingConstraints_() const;

    /// Background check loop.
    void bgLoop_();

    // ========================================================================
    // Members
    // ========================================================================

    RocksDBWrapper&      db_;
    SchemaManager&       schema_mgr_;
    StatisticsCollector* stats_{nullptr};
    SchemaConstraints*   constraints_{nullptr};

    std::chrono::hours max_stats_age_{kDefaultMaxStatsAge};

    mutable std::mutex                    results_mutex_;
    mutable std::vector<ConsistencyIssue> last_results_;

    std::chrono::seconds bg_interval_{0};
    std::thread          bg_thread_;
    std::atomic<bool>    stop_bg_{false};
    std::condition_variable bg_cv_;
    std::mutex           bg_mutex_;
};

} // namespace themis
