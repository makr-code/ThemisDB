/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            log_search_engine.h                                ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:07:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     230                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 08b62168b2  2026-04-12  feat(observability): add per-tenant metric namespacing an... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file log_search_engine.h
 * @brief Structured log search API -- query logs like data.
 *
 * LogSearchEngine provides a query interface over in-process LogEntry
 * buffers collected by LogAggregator.  It supports:
 *
 * - Field-based filtering: match on arbitrary fields key/value pairs.
 * - Level filtering: restrict results to entries at or above a severity.
 * - Time-range filtering: half-open interval [from, to).
 * - Message full-text search: substring match on the message field.
 * - Pagination: limit + offset for large result sets.
 * - Sorting: ascending or descending by timestamp.
 *
 * All filter conditions are combined with AND semantics.
 * The engine is stateless and side-effect-free.
 *
 * Usage:
 *   LogAggregator agg;
 *   agg.logStructured(ILogger::Level::WARN, "Slow query",
 *                     {{"query_id", "q-42"}, {"latency_ms", "850"}});
 *
 *   LogSearchEngine engine;
 *   LogSearchQuery q;
 *   q.min_level     = ILogger::Level::WARN;
 *   q.field_filters = {{"query_id", FieldMatchOp::EQUALS, "q-42"}};
 *   q.limit         = 50;
 *
 *   auto result = engine.search(agg.entries(), q);
 *   for (const auto& entry : result.entries) {
 *       std::cout << entry.toJson() << "\n";
 *   }
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "observability/log_aggregator.h"

#include <chrono>
#include <string>
#include <vector>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// Field match operator
// ---------------------------------------------------------------------------

/**
 * @brief Comparison operator used by a field filter predicate.
 */
enum class FieldMatchOp {
    EQUALS,       ///< Exact equality (case-sensitive)
    CONTAINS,     ///< Substring match (case-sensitive)
    STARTS_WITH,  ///< Prefix match (case-sensitive)
    NOT_EQUALS,   ///< Negated equality
};

// ---------------------------------------------------------------------------
// LogFieldFilter
// ---------------------------------------------------------------------------

/**
 * @brief A single field-based filter predicate.
 *
 * Matches a LogEntry when the entry's fields map contains key and the
 * value satisfies the given op comparison against value.
 */
struct LogFieldFilter {
    std::string key;
    std::string value;
    FieldMatchOp op{FieldMatchOp::EQUALS};

    /**
     * @brief Evaluate this filter against the given LogEntry.
     * @returns true when the entry satisfies the filter.
     */
    bool matches(const LogEntry& entry) const;
};

// ---------------------------------------------------------------------------
// LogSearchQuery
// ---------------------------------------------------------------------------

/**
 * @brief Describes a structured log search request.
 *
 * All filter conditions are combined with AND semantics: an entry is
 * included in the result only when ALL specified conditions are satisfied.
 */
struct LogSearchQuery {
    // --- Level filter ---
    /// Include only entries at or above this severity.
    /// Defaults to TRACE (include all levels).
    core::concerns::ILogger::Level min_level{
        core::concerns::ILogger::Level::TRACE};

    // --- Time range ---
    /// If has_from_time is true, include only entries with timestamp >= from_time.
    std::chrono::system_clock::time_point from_time{};
    bool has_from_time{false};

    /// If has_to_time is true, include only entries with timestamp < to_time.
    std::chrono::system_clock::time_point to_time{};
    bool has_to_time{false};

    // --- Message search ---
    /// If non-empty, include only entries whose message contains this substring.
    std::string message_contains;

    // --- Field filters ---
    /// All filters must match (AND semantics).
    std::vector<LogFieldFilter> field_filters;

    // --- Pagination ---
    /// Maximum number of entries to return.  0 = no limit.
    size_t limit{0};
    /// Number of matching entries to skip before collecting results.
    size_t offset{0};

    // --- Sort order ---
    /// When true, results are ordered oldest-first; when false, newest-first.
    bool ascending{true};
};

// ---------------------------------------------------------------------------
// LogSearchResult
// ---------------------------------------------------------------------------

/**
 * @brief The result of a LogSearchEngine::search() call.
 */
struct LogSearchResult {
    /// Matching log entries (after offset/limit applied).
    std::vector<LogEntry> entries;

    /// Total number of entries that matched the query (before offset/limit).
    size_t total_matched{0};

    /// Offset that was applied to produce this page.
    size_t offset{0};

    /// Limit that was applied (0 = unlimited).
    size_t limit{0};
};

// ---------------------------------------------------------------------------
// LogSearchEngine
// ---------------------------------------------------------------------------

/**
 * @brief Stateless search engine for structured log entries.
 *
 * LogSearchEngine operates purely in-memory on a snapshot of entries
 * provided by the caller.  Typical usage is to pass agg.entries() where
 * agg is a LogAggregator instance.
 *
 * Thread safety: LogSearchEngine itself holds no mutable state; all state
 * lives in the LogSearchQuery and the input vector.  It is therefore safe
 * to call search() concurrently from multiple threads on the same engine
 * instance without additional synchronisation.
 */
class LogSearchEngine {
public:
    LogSearchEngine() = default;
    ~LogSearchEngine() = default;

    /**
     * @brief Execute a structured search over the given log entries.
     *
     * @param entries  Snapshot of LogEntry objects to search (typically from
     *                 LogAggregator::entries()).  The vector is not modified.
     * @param query    Search criteria.
     * @returns        LogSearchResult containing matching entries and metadata.
     */
    LogSearchResult search(const std::vector<LogEntry>& entries,
                           const LogSearchQuery& query) const;

    /**
     * @brief Count how many entries in the snapshot match the query.
     *
     * Equivalent to search(...).total_matched but avoids copying entries.
     */
    size_t count(const std::vector<LogEntry>& entries,
                 const LogSearchQuery& query) const;

    /**
     * @brief Return the distinct values present in the given field key
     *        across all provided entries.
     *
     * Useful for building faceted search UIs (e.g. "all distinct component
     * values seen in this log buffer").
     */
    std::vector<std::string> distinctFieldValues(
        const std::vector<LogEntry>& entries,
        const std::string& field_key) const;

private:
    /// Returns true when the given entry satisfies ALL conditions in query.
    bool matchesQuery(const LogEntry& entry, const LogSearchQuery& query) const;
};

} // namespace observability
} // namespace themis
