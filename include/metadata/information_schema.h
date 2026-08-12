/**
 * @file information_schema.h
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
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class SchemaManager;

using json = nlohmann::json;

/// Row in INFORMATION_SCHEMA.TABLES
struct ISTable {
    std::string table_catalog;   ///< Always "def" (SQL standard)
    std::string table_schema;    ///< Schema/database name
    std::string table_name;      ///< Table/collection name
    std::string table_type;      ///< "BASE TABLE", "VIEW", etc.
    size_t row_count = 0;        ///< Estimated row count
    std::string engine;          ///< Storage engine name ("ThemisDB")
    std::string create_time;     ///< ISO-8601 creation timestamp (if known)

    json toJSON() const;
};

/// Row in INFORMATION_SCHEMA.COLUMNS
struct ISColumn {
    std::string table_catalog;      ///< Always "def"
    std::string table_schema;
    std::string table_name;
    std::string column_name;
    uint32_t ordinal_position = 1;  ///< 1-based column position
    std::string data_type;          ///< "string", "integer", "double", …
    std::string is_nullable;        ///< "YES" or "NO"
    std::optional<std::string> column_default; ///< Default value expression
    std::string extra;              ///< "auto_increment", "indexed", …

    json toJSON() const;
};

/// Row in INFORMATION_SCHEMA.STATISTICS (index info)
struct ISStatistic {
    std::string table_catalog;
    std::string table_schema;
    std::string table_name;
    std::string index_name;
    std::string column_name;
    uint32_t seq_in_index = 1;      ///< Column position within composite index
    std::string index_type;         ///< "BTREE", "HASH", "FULLTEXT", …
    std::string non_unique;         ///< "0" (unique) or "1" (not unique)

    json toJSON() const;
};

/// Row in INFORMATION_SCHEMA.KEY_COLUMN_USAGE
struct ISKeyColumnUsage {
    std::string constraint_catalog;
    std::string constraint_schema;
    std::string constraint_name;
    std::string table_schema;
    std::string table_name;
    std::string column_name;
    uint32_t ordinal_position = 1;
    std::optional<std::string> referenced_table_schema;
    std::optional<std::string> referenced_table_name;
    std::optional<std::string> referenced_column_name;

    json toJSON() const;
};

/// Row in INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS
struct ISReferentialConstraint {
    std::string constraint_catalog;
    std::string constraint_schema;
    std::string constraint_name;           ///< FK constraint name
    std::string unique_constraint_catalog; ///< Catalog of referenced constraint
    std::string unique_constraint_schema;
    std::string unique_constraint_name;    ///< Name of the referenced unique/PK constraint
    std::string match_option;              ///< "NONE", "PARTIAL", "FULL"
    std::string update_rule;               ///< "RESTRICT", "CASCADE", "NO ACTION", etc.
    std::string delete_rule;

    json toJSON() const;
};

/// InformationSchema - SQL-standard INFORMATION_SCHEMA views
///
/// Provides read-only metadata views modelled after the SQL:2003 standard.
/// Data is derived from the SchemaManager on demand (no separate persistence).
///
/// All methods return std::vector of the corresponding row type.
/// An empty vector is returned for an unknown schema/table.
///
/// Usage:
///   InformationSchema info_schema(schema_mgr);
///   auto tables = info_schema.getTables();
///   auto cols   = info_schema.getColumns("users");
class InformationSchema {
public:
    /// Constructor
    /// @param schema_mgr  SchemaManager that owns the live schema data
    explicit InformationSchema(SchemaManager& schema_mgr);

    ~InformationSchema() = default;

    // Disable copy, allow move
    InformationSchema(const InformationSchema&) = delete;
    InformationSchema& operator=(const InformationSchema&) = delete;
    InformationSchema(InformationSchema&&) = default;
    InformationSchema& operator=(InformationSchema&&) = default;

    // ========================================================================
    // INFORMATION_SCHEMA views
    // ========================================================================

    /// INFORMATION_SCHEMA.TABLES
    /// Returns one row per table/collection in the default schema.
    std::vector<ISTable> getTables() const;

    /// INFORMATION_SCHEMA.COLUMNS
    /// Returns one row per column across all tables, or only for a specific
    /// table when @p table_name is provided.
    std::vector<ISColumn> getColumns(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    /// INFORMATION_SCHEMA.STATISTICS
    /// Returns index metadata rows, optionally filtered by table.
    std::vector<ISStatistic> getStatistics(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    /// INFORMATION_SCHEMA.KEY_COLUMN_USAGE
    /// Returns foreign-key / unique-key usage rows for all (or one) table(s).
    std::vector<ISKeyColumnUsage> getKeyColumnUsage(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    /// INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS
    /// Returns foreign-key referential constraint metadata, optionally
    /// filtered by the referencing table name.
    std::vector<ISReferentialConstraint> getReferentialConstraints(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    // ========================================================================
    // JSON export helpers (for REST API / AQL integration)
    // ========================================================================

    /// Serialize the full INFORMATION_SCHEMA as a JSON object with keys
    /// "tables", "columns", "statistics", "key_column_usage", and
    /// "referential_constraints".
    json toJSON() const;

    /// Return only the TABLES view as a JSON array.
    json tablesToJSON() const;

    /// Return only the COLUMNS view for one table as a JSON array.
    json columnsToJSON(std::string_view table_name) const;

    /// Return only the REFERENTIAL_CONSTRAINTS view as a JSON array.
    json referentialConstraintsToJSON() const;

private:
    SchemaManager& schema_mgr_;
};

} // namespace themis
