/**
 * @file schema_constraints.h
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
#include <optional>
#include <variant>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declaration
class RocksDBWrapper;

using json = nlohmann::json;

/// Constraint violation error details returned by enforcement methods
struct ConstraintViolation {
    std::string table_name;
    std::string column_name;
    std::string constraint_name;
    std::string constraint_type;  ///< "NOT_NULL", "UNIQUE", "CHECK", "DEFAULT", "FOREIGN_KEY"
    std::string message;

    json toJSON() const;
};

/// A scalar value that can be stored in a column (used for CHECK / DEFAULT)
using ColumnValue = std::variant<std::monostate, std::string, int64_t, double, bool>;

/// Describes a single schema constraint on a column
struct ColumnConstraint {
    /// Constraint kind (mirrors SQL constraint types)
    enum class Kind {
        NOT_NULL,       ///< Column must not be NULL
        UNIQUE,         ///< All non-NULL values must be distinct
        CHECK,          ///< Arbitrary expression evaluated as bool (stored as string)
        DEFAULT,        ///< Default value when none is supplied
        FOREIGN_KEY,    ///< References another table's column
    };

    Kind kind;
    std::string name;                           ///< Constraint name (may be auto-generated)
    std::optional<std::string> check_expr;      ///< CHECK expression (as SQL/AQL string)
    std::optional<ColumnValue> default_value;   ///< DEFAULT value
    std::optional<std::string> fk_table;        ///< Foreign-key referenced table
    std::optional<std::string> fk_column;       ///< Foreign-key referenced column

    json toJSON() const;
    static ColumnConstraint makeNotNull(std::string constraint_name);
    static ColumnConstraint makeUnique(std::string constraint_name);
    static ColumnConstraint makeCheck(std::string constraint_name, std::string expr);
    static ColumnConstraint makeDefault(std::string constraint_name, ColumnValue value);
    static ColumnConstraint makeForeignKey(
        std::string constraint_name,
        std::string ref_table,
        std::string ref_column
    );
};

/// Error codes returned by constraint enforcement
enum class ConstraintErrorCode {
    OK = 0,
    NOT_NULL_VIOLATION,
    UNIQUE_VIOLATION,
    CHECK_VIOLATION,
    FOREIGN_KEY_VIOLATION,
    UNKNOWN_TABLE,
    UNKNOWN_COLUMN,
};

/// SchemaConstraints - Constraint definition and enforcement engine
///
/// Stores per-column constraints (NOT NULL, UNIQUE, CHECK, DEFAULT, FK) and
/// provides enforcement methods that validate a proposed row value map against
/// those constraints.
///
/// Constraints are stored in-memory; callers are responsible for persistence
/// (e.g. by serialising via toJSON() and saving to RocksDB).
///
/// Thread-safety: NOT thread-safe; external synchronization required when
/// sharing a SchemaConstraints instance across threads.
///
/// Usage:
///   SchemaConstraints sc;
///   sc.addConstraint("users", "email",
///       ColumnConstraint::makeNotNull("users_email_not_null"));
///   sc.addConstraint("users", "email",
///       ColumnConstraint::makeUnique("users_email_unique"));
///
///   std::map<std::string, ColumnValue> row = {{"email", std::string("a@b.c")}};
///   auto violations = sc.enforce("users", row);
///   if (!violations.empty()) { /* handle */ }
class SchemaConstraints {
public:
    SchemaConstraints() = default;
    ~SchemaConstraints() = default;

    // Disable copy, allow move
    SchemaConstraints(const SchemaConstraints&) = delete;
    SchemaConstraints& operator=(const SchemaConstraints&) = delete;
    SchemaConstraints(SchemaConstraints&&) = default;
    SchemaConstraints& operator=(SchemaConstraints&&) = default;

    // ========================================================================
    // Constraint management
    // ========================================================================

    /// Add a constraint for a column on a table.
    void addConstraint(
        std::string_view table_name,
        std::string_view column_name,
        ColumnConstraint constraint
    );

    /// Remove all constraints on a specific column.
    void removeColumnConstraints(
        std::string_view table_name,
        std::string_view column_name
    );

    /// Remove all constraints for an entire table.
    void removeTableConstraints(std::string_view table_name);

    /// Retrieve all constraints for a column (empty vector if none).
    std::vector<ColumnConstraint> getColumnConstraints(
        std::string_view table_name,
        std::string_view column_name
    ) const;

    /// Retrieve all constraints for a table (all columns).
    std::vector<ColumnConstraint> getTableConstraints(
        std::string_view table_name
    ) const;

    // ========================================================================
    // Enforcement
    // ========================================================================

    /// Validate a row value map against all registered constraints for a table.
    /// @param table_name  Table the row belongs to
    /// @param row         Map of column_name -> value (missing key = NULL)
    /// @returns           List of constraint violations; empty means valid.
    std::vector<ConstraintViolation> enforce(
        std::string_view table_name,
        const std::map<std::string, ColumnValue>& row
    ) const;

    /// Apply DEFAULT values for any columns that are missing from the row.
    /// Returns a copy of @p row with defaults filled in where applicable.
    std::map<std::string, ColumnValue> applyDefaults(
        std::string_view table_name,
        std::map<std::string, ColumnValue> row
    ) const;

    // ========================================================================
    // Serialisation
    // ========================================================================

    /// Serialise all constraints to JSON.
    json toJSON() const;

    /// Parse constraints from a JSON object produced by toJSON().
    static SchemaConstraints fromJSON(const json& j);

    // ========================================================================
    // RocksDB persistence
    // ========================================================================

    /// Persist all constraints to RocksDB under "config:constraints:<table>" keys.
    /// @param db  RocksDB wrapper to write to
    /// @return    true if all tables were persisted successfully
    bool persistTo(RocksDBWrapper& db) const;

    /// Persist constraints for a single table.
    /// @param db          RocksDB wrapper to write to
    /// @param table_name  Table whose constraints should be persisted
    bool persistTableTo(RocksDBWrapper& db, std::string_view table_name) const;

    /// Load constraints for all tables whose keys are found in RocksDB
    /// under the "config:constraints:" prefix.
    /// Replaces existing in-memory state.
    /// @param db  RocksDB wrapper to read from
    /// @return    Number of tables loaded (0 = none found)
    size_t loadFrom(RocksDBWrapper& db);

    /// Load constraints for a single table from RocksDB.
    /// Merges with existing in-memory constraints for that table.
    /// @param db          RocksDB wrapper to read from
    /// @param table_name  Table to load
    /// @return            true if constraints were found and loaded
    bool loadTableFrom(RocksDBWrapper& db, std::string_view table_name);

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// Check a NOT NULL constraint for a single column value.
    std::optional<ConstraintViolation> checkNotNull(
        std::string_view table_name,
        std::string_view column_name,
        const ColumnConstraint& c,
        const ColumnValue& value
    ) const;

    /// Check a CHECK constraint expression (simple key=value string comparison
    /// for the initial implementation; real expression evaluation is a future
    /// enhancement).
    std::optional<ConstraintViolation> checkCheck(
        std::string_view table_name,
        std::string_view column_name,
        const ColumnConstraint& c,
        const ColumnValue& value
    ) const;

    // table_name -> (column_name -> [constraints])
    std::map<std::string, std::map<std::string, std::vector<ColumnConstraint>>> constraints_;
};

} // namespace themis
