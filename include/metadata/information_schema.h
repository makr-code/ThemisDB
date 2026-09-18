/**
 * @file information_schema.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

/**
 * @brief Row model for `INFORMATION_SCHEMA.TABLES`.
 */
struct ISTable {
    std::string table_catalog;   ///< Always "def" (SQL standard)
    std::string table_schema;    ///< Schema/database name
    std::string table_name;      ///< Table/collection name
    std::string table_type;      ///< "BASE TABLE", "VIEW", etc.
    size_t row_count = 0;        ///< Estimated row count
    std::string engine;          ///< Storage engine name ("ThemisDB")
    std::string create_time;     ///< ISO-8601 creation timestamp (if known)

    /**
     * @brief Serialise this TABLES row to JSON.
     * @return JSON object representing the row fields.
     */
    json toJSON() const;
};

/**
 * @brief Row model for `INFORMATION_SCHEMA.COLUMNS`.
 */
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

    /**
     * @brief Serialise this COLUMNS row to JSON.
     * @return JSON object representing the row fields.
     */
    json toJSON() const;
};

/**
 * @brief Row model for `INFORMATION_SCHEMA.STATISTICS`.
 */
struct ISStatistic {
    std::string table_catalog;
    std::string table_schema;
    std::string table_name;
    std::string index_name;
    std::string column_name;
    uint32_t seq_in_index = 1;      ///< Column position within composite index
    std::string index_type;         ///< "BTREE", "HASH", "FULLTEXT", …
    std::string non_unique;         ///< "0" (unique) or "1" (not unique)

    /**
     * @brief Serialise this STATISTICS row to JSON.
     * @return JSON object representing the row fields.
     */
    json toJSON() const;
};

/**
 * @brief Row model for `INFORMATION_SCHEMA.KEY_COLUMN_USAGE`.
 */
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

    /**
     * @brief Serialise this KEY_COLUMN_USAGE row to JSON.
     * @return JSON object representing the row fields.
     */
    json toJSON() const;
};

/**
 * @brief Row model for `INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS`.
 */
struct ISReferentialConstraint {
    std::string constraint_catalog = {};
    std::string constraint_schema;
    std::string constraint_name;           ///< FK constraint name
    std::string unique_constraint_catalog; ///< Catalog of referenced constraint
    std::string unique_constraint_schema;
    std::string unique_constraint_name;    ///< Name of the referenced unique/PK constraint
    std::string match_option;              ///< "NONE", "PARTIAL", "FULL"
    std::string update_rule;               ///< "RESTRICT", "CASCADE", "NO ACTION", etc.
    std::string delete_rule;

    /**
     * @brief Serialise this REFERENTIAL_CONSTRAINTS row to JSON.
     * @return JSON object representing the row fields.
     */
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
    /**
     * @brief Construct an INFORMATION_SCHEMA view provider over a live SchemaManager.
     * @param schema_mgr SchemaManager that owns the authoritative schema metadata.
     * @return Return value.
     */
    explicit InformationSchema(SchemaManager& schema_mgr);

    ~InformationSchema() = default;

    // Disable copy and move
    InformationSchema(const InformationSchema&) = delete;
    InformationSchema& operator=(const InformationSchema&) = delete;
    InformationSchema(InformationSchema&&) noexcept = delete;
    InformationSchema& operator=(InformationSchema&&) noexcept = delete;

    // ========================================================================
    // INFORMATION_SCHEMA views
    // ========================================================================

    /**
     * @brief Return the `INFORMATION_SCHEMA.TABLES` view.
     * @return One row per table or collection in the default schema.
     */
    std::vector<ISTable> getTables() const;

    /**
     * @brief Return the `INFORMATION_SCHEMA.COLUMNS` view.
     * @param table_name Optional table filter; when omitted, all tables are included.
     * @return Column metadata rows for the selected scope.
     */
    std::vector<ISColumn> getColumns(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    /**
     * @brief Return the `INFORMATION_SCHEMA.STATISTICS` view.
     * @param table_name Optional table filter; when omitted, all tables are included.
     * @return Index metadata rows for the selected scope.
     */
    std::vector<ISStatistic> getStatistics(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    /**
     * @brief Return the `INFORMATION_SCHEMA.KEY_COLUMN_USAGE` view.
     * @param table_name Optional table filter; when omitted, all tables are included.
     * @return Key-usage rows for the selected scope.
     */
    std::vector<ISKeyColumnUsage> getKeyColumnUsage(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    /**
     * @brief Return the `INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS` view.
     * @param table_name Optional referencing-table filter.
     * @return Referential-constraint rows for the selected scope.
     */
    std::vector<ISReferentialConstraint> getReferentialConstraints(
        std::optional<std::string_view> table_name = std::nullopt
    ) const;

    // ========================================================================
    // JSON export helpers (for REST API / AQL integration)
    // ========================================================================

    /**
     * @brief Serialise the full INFORMATION_SCHEMA surface to JSON.
     * @return JSON object with `tables`, `columns`, `statistics`,
     *         `key_column_usage`, and `referential_constraints` arrays.
     */
    json toJSON() const;

    /**
     * @brief Serialise only the TABLES view to JSON.
     * @return JSON array of TABLES rows.
     */
    json tablesToJSON() const;

    /**
     * @brief Serialise only the COLUMNS view for one table to JSON.
     * @param table_name Table whose column metadata should be exported.
     * @return JSON array of COLUMNS rows for the table.
     */
    json columnsToJSON(std::string_view table_name) const;

    /**
     * @brief Serialise only the REFERENTIAL_CONSTRAINTS view to JSON.
     * @return JSON array of referential-constraint rows.
     */
    json referentialConstraintsToJSON() const;

private:
    SchemaManager& schema_mgr_;
};

} // namespace themis
