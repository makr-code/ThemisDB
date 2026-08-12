/**
 * @file column_lineage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
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
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace metadata {

/// A reference to a specific column within a specific table/collection.
struct ColumnRef {
    std::string table_name;   ///< Table or collection name
    std::string column_name;  ///< Column or property name

    bool operator==(const ColumnRef& other) const noexcept {
        return table_name == other.table_name && column_name == other.column_name;
    }

    /// Returns "table_name.column_name"
    std::string toString() const;

    /// Serialize to JSON: {"table": "...", "column": "..."}
    nlohmann::json toJSON() const;

    /// Parse from JSON object
    static ColumnRef fromJSON(const nlohmann::json& j);
};

/// Hash support so ColumnRef can be used as an unordered_map key
struct ColumnRefHash {
    std::size_t operator()(const ColumnRef& ref) const noexcept;
};

/// Type of transformation that was applied to derive a target column
enum class TransformationType {
    DIRECT_COPY,    ///< Target is a direct (verbatim) copy of a single source column
    RENAME,         ///< Column was renamed; content identical to source
    CAST,           ///< Column value was type-cast from the source
    COMPUTED,       ///< Computed from one or more source columns (arithmetic, concat, etc.)
    AGGREGATION,    ///< Aggregation of source column values (SUM, AVG, COUNT, …)
    ANONYMIZATION,  ///< PII/PHI in source column was anonymized or pseudonymized
    ENRICHMENT,     ///< Source column was enriched with additional data from another source
    CUSTOM          ///< Any other transformation; description is in transformation_expression
};

/// Convert TransformationType to a string label
std::string transformationTypeToString(TransformationType t);

/// Convert a string label back to TransformationType (case-insensitive)
TransformationType transformationTypeFromString(const std::string& s);

/// A single derivation step recorded in the column lineage graph.
///
/// Each entry describes how one target column was produced from a set of
/// source columns, along with the transformation applied and optional
/// expression detail.  Entries are append-only: once recorded they are
/// never modified or deleted.
struct ColumnLineageEntry {
    std::string entry_id;                     ///< Unique identifier (auto-assigned if empty)
    ColumnRef target_column;                  ///< The column that was produced
    std::vector<ColumnRef> source_columns;    ///< Contributing source columns (may be empty for root columns)
    TransformationType transformation{TransformationType::DIRECT_COPY}; ///< How target was derived
    std::string transformation_expression;    ///< Optional expression detail, e.g. "first_name || ' ' || last_name"
    std::string performed_by;                 ///< User or service that performed the derivation
    int64_t timestamp_ms{0};                  ///< Unix epoch time in milliseconds (auto-assigned if 0)
    nlohmann::json metadata;                  ///< Arbitrary additional context

    /// Serialize to JSON
    nlohmann::json toJSON() const;
};

/// Full lineage record for a single column: all recorded derivation entries
/// and derived provenance information.
struct ColumnLineageRecord {
    ColumnRef column;
    std::vector<ColumnLineageEntry> entries;  ///< All derivation entries where this column is the target

    /// Serialize to JSON
    nlohmann::json toJSON() const;
};

/**
 * @brief Column-level lineage and data provenance tracker.
 *
 * ColumnLineageTracker is the single point for recording and querying
 * column-level derivation relationships.  It builds a directed acyclic graph
 * (DAG) where nodes are ColumnRef values and edges are ColumnLineageEntry
 * records.
 *
 * Design constraints
 * ------------------
 * - Append-only: recordDerivation() may never modify or delete an existing entry.
 * - All public methods are thread-safe.
 * - No persistence dependency: the tracker operates in-memory.  Callers that
 *   need durability should persist the JSON export via their preferred store.
 *
 * Integration points
 * ------------------
 * - SchemaManager: notify on column renames, table schema updates
 * - StatisticsCollector: link collected statistics to lineage origin
 * - governance::DataLineageTracker: column-level events complement dataset-level events
 */
class ColumnLineageTracker {
public:
    ColumnLineageTracker() = default;

    // Disable copy; allow move
    ColumnLineageTracker(const ColumnLineageTracker&) = delete;
    ColumnLineageTracker& operator=(const ColumnLineageTracker&) = delete;
    ColumnLineageTracker(ColumnLineageTracker&&) = default;
    ColumnLineageTracker& operator=(ColumnLineageTracker&&) = default;

    /**
     * @brief Record a derivation step for a target column.
     *
     * entry.entry_id is auto-assigned if empty.
     * entry.timestamp_ms is auto-assigned to the current wall-clock time if 0.
     *
     * @param entry  Fully or partially populated ColumnLineageEntry.
     */
    void recordDerivation(ColumnLineageEntry entry);

    /**
     * @brief Return all recorded derivation entries whose target is @p col.
     */
    ColumnLineageRecord getColumnLineage(const ColumnRef& col) const;

    /**
     * @brief Transitively return all source columns that contributed to @p col.
     *
     * Performs a breadth-first traversal of the lineage DAG from @p col
     * upward (toward roots).  The returned set does not include @p col itself.
     *
     * @return Unique ColumnRef values in BFS order (nearest ancestors first).
     */
    std::vector<ColumnRef> getUpstreamColumns(const ColumnRef& col) const;

    /**
     * @brief Transitively return all columns derived from @p col.
     *
     * Performs a breadth-first traversal of the lineage DAG from @p col
     * downward (toward leaves).  The returned set does not include @p col itself.
     *
     * @return Unique ColumnRef values in BFS order (nearest descendants first).
     */
    std::vector<ColumnRef> getDownstreamColumns(const ColumnRef& col) const;

    /**
     * @brief Return a structured provenance record for @p col.
     *
     * The returned JSON object contains:
     * - "column": the queried column
     * - "entries": direct derivation entries for this column
     * - "upstream_columns": all transitive source columns
     * - "downstream_columns": all transitive derived columns
     *
     * @param col Column to inspect.
     * @return JSON provenance object.
     */
    nlohmann::json getColumnProvenance(const ColumnRef& col) const;

    /**
     * @brief Export all lineage entries for every column in @p table_name.
     *
     * @param table_name Table/collection name.
     * @return JSON array of ColumnLineageRecord objects.
     */
    nlohmann::json exportTableLineage(const std::string& table_name) const;

    /**
     * @brief Export the full lineage graph as JSON.
     *
     * Returns a JSON object: {"entries": [...], "total_entries": N}
     */
    nlohmann::json exportAllLineage() const;

    /// Return the total number of lineage entries recorded
    size_t totalEntryCount() const;

private:
    mutable std::mutex mutex_;

    /// Primary store: target_column key → derivation entries
    std::unordered_map<ColumnRef, std::vector<ColumnLineageEntry>, ColumnRefHash> entries_by_target_;

    /// Secondary index: source_column key → set of target ColumnRefs
    ///   Used by getDownstreamColumns() for efficient traversal
    std::unordered_map<ColumnRef, std::vector<ColumnRef>, ColumnRefHash> targets_by_source_;

    /// All entries in insertion order (for exportAllLineage)
    std::vector<ColumnLineageEntry> all_entries_;

    std::atomic<uint64_t> next_entry_seq_{1};

    /// Generate a unique entry_id
    std::string assignEntryId();
};

} // namespace metadata
} // namespace themis
