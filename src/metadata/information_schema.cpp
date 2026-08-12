/**
 * @file information_schema.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=24, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/information_schema.h"
#include "metadata/schema_manager.h"
#include <chrono>
#include <ctime>
#include <sstream>

namespace themis {

// ============================================================================
// ISTable
// ============================================================================

json ISTable::toJSON() const {
    return {
        {"table_catalog", table_catalog},
        {"table_schema",  table_schema},
        {"table_name",    table_name},
        {"table_type",    table_type},
        {"row_count",     row_count},
        {"engine",        engine},
        {"create_time",   create_time},
    };
}

// ============================================================================
// ISColumn
// ============================================================================

json ISColumn::toJSON() const {
    json j = {
        {"table_catalog",     table_catalog},
        {"table_schema",      table_schema},
        {"table_name",        table_name},
        {"column_name",       column_name},
        {"ordinal_position",  ordinal_position},
        {"data_type",         data_type},
        {"is_nullable",       is_nullable},
        {"extra",             extra},
    };
    if (column_default.has_value()) {
        j["column_default"] = *column_default;
    } else {
        j["column_default"] = nullptr;
    }
    return j;
}

// ============================================================================
// ISStatistic
// ============================================================================

json ISStatistic::toJSON() const {
    return {
        {"table_catalog", table_catalog},
        {"table_schema",  table_schema},
        {"table_name",    table_name},
        {"index_name",    index_name},
        {"column_name",   column_name},
        {"seq_in_index",  seq_in_index},
        {"index_type",    index_type},
        {"non_unique",    non_unique},
    };
}

// ============================================================================
// ISKeyColumnUsage
// ============================================================================

json ISKeyColumnUsage::toJSON() const {
    json j = {
        {"constraint_catalog", constraint_catalog},
        {"constraint_schema",  constraint_schema},
        {"constraint_name",    constraint_name},
        {"table_schema",       table_schema},
        {"table_name",         table_name},
        {"column_name",        column_name},
        {"ordinal_position",   ordinal_position},
    };
    if (referenced_table_schema.has_value()) {
        j["referenced_table_schema"] = *referenced_table_schema;
    } else {
        j["referenced_table_schema"] = nullptr;
    }
    if (referenced_table_name.has_value()) {
        j["referenced_table_name"] = *referenced_table_name;
    } else {
        j["referenced_table_name"] = nullptr;
    }
    if (referenced_column_name.has_value()) {
        j["referenced_column_name"] = *referenced_column_name;
    } else {
        j["referenced_column_name"] = nullptr;
    }
    return j;
}

// ============================================================================
// ISReferentialConstraint
// ============================================================================

json ISReferentialConstraint::toJSON() const {
    return {
        {"constraint_catalog",        constraint_catalog},
        {"constraint_schema",         constraint_schema},
        {"constraint_name",           constraint_name},
        {"unique_constraint_catalog", unique_constraint_catalog},
        {"unique_constraint_schema",  unique_constraint_schema},
        {"unique_constraint_name",    unique_constraint_name},
        {"match_option",              match_option},
        {"update_rule",               update_rule},
        {"delete_rule",               delete_rule},
    };
}

InformationSchema::InformationSchema(SchemaManager& schema_mgr)
    : schema_mgr_(schema_mgr)
{}

// Translate SchemaManager table type to SQL TABLE_TYPE string
static std::string tableTypeSQL(const std::string& type) {
    if (type == "graph_node" || type == "graph_edge") {
        return "BASE TABLE";
    }
    return "BASE TABLE";
}

// Map SchemaManager property type to SQL data type string
static std::string mapDataType(const std::string& prop_type) {
    if (prop_type == "integer")  return "BIGINT";
    if (prop_type == "double")   return "DOUBLE";
    if (prop_type == "boolean")  return "BOOLEAN";
    if (prop_type == "binary")   return "BLOB";
    if (prop_type == "vector")   return "ARRAY";
    if (prop_type == "null")     return "NULL";
    return "VARCHAR";  // string and unknown default
}

// Map index type to SQL INDEX_TYPE string
static std::string mapIndexType(const std::string& idx_type) {
    if (idx_type == "range")    return "BTREE";
    if (idx_type == "geo")      return "SPATIAL";
    if (idx_type == "fulltext") return "FULLTEXT";
    return "HASH";
}

std::vector<ISTable> InformationSchema::getTables() const {
    auto tables = schema_mgr_.getAllTables();
    std::vector<ISTable> result;
    result.reserve(tables.size());

    for (const auto& t : tables) {
        ISTable row;
        row.table_catalog = "def";
        row.table_schema  = "main";
        row.table_name    = t.name;
        row.table_type    = tableTypeSQL(t.type);
        row.row_count     = t.estimated_row_count;
        row.engine        = "ThemisDB";
        result.push_back(std::move(row));
    }
    return result;
}

std::vector<ISColumn> InformationSchema::getColumns(
    std::optional<std::string_view> table_filter) const
{
    auto tables = schema_mgr_.getAllTables();
    std::vector<ISColumn> result;

    for (const auto& t : tables) {
        if (table_filter.has_value() && t.name != *table_filter) {
            continue;
        }

        uint32_t pos = 1;
        for (const auto& prop : t.properties) {
            ISColumn col;
            col.table_catalog    = "def";
            col.table_schema     = "main";
            col.table_name       = t.name;
            col.column_name      = prop.name;
            col.ordinal_position = pos++;
            col.data_type        = mapDataType(prop.type);
            col.is_nullable      = prop.nullable ? "YES" : "NO";
            if (prop.indexed) {
                col.extra = "indexed";
            }
            result.push_back(std::move(col));
        }
    }
    return result;
}

std::vector<ISStatistic> InformationSchema::getStatistics(
    std::optional<std::string_view> table_filter) const
{
    auto tables = schema_mgr_.getAllTables();
    std::vector<ISStatistic> result;

    for (const auto& t : tables) {
        if (table_filter.has_value() && t.name != *table_filter) {
            continue;
        }

        for (const auto& idx : t.indexes) {
            uint32_t seq = 1;
            for (const auto& col : idx.columns) {
                ISStatistic row;
                row.table_catalog = "def";
                row.table_schema  = "main";
                row.table_name    = t.name;
                row.index_name    = idx.name;
                row.column_name   = col;
                row.seq_in_index  = seq++;
                row.index_type    = mapIndexType(idx.type);
                row.non_unique    = idx.unique ? "0" : "1";
                result.push_back(std::move(row));
            }
        }
    }
    return result;
}

std::vector<ISKeyColumnUsage> InformationSchema::getKeyColumnUsage(
    std::optional<std::string_view> table_filter) const
{
    auto tables = schema_mgr_.getAllTables();
    std::vector<ISKeyColumnUsage> result;

    for (const auto& t : tables) {
        if (table_filter.has_value() && t.name != *table_filter) {
            continue;
        }

        for (const auto& idx : t.indexes) {
            if (!idx.unique) {
                continue;  // KEY_COLUMN_USAGE only tracks key constraints
            }

            uint32_t pos = 1;
            for (const auto& col : idx.columns) {
                ISKeyColumnUsage row;
                row.constraint_catalog = "def";
                row.constraint_schema  = "main";
                row.constraint_name    = idx.name;
                row.table_schema       = "main";
                row.table_name         = t.name;
                row.column_name        = col;
                row.ordinal_position   = pos++;
                result.push_back(std::move(row));
            }
        }
    }
    return result;
}

std::vector<ISReferentialConstraint> InformationSchema::getReferentialConstraints(
    std::optional<std::string_view> table_filter) const
{
    auto tables = schema_mgr_.getAllTables();
    std::vector<ISReferentialConstraint> result;

    for (const auto& t : tables) {
        if (table_filter.has_value() && t.name != *table_filter) {
            continue;
        }

        for (const auto& idx : t.indexes) {
            // Only FK-like indexes carry referenced table info; in the current
            // data model we detect them by a naming convention "fk_*".
            // A full implementation would consult SchemaConstraints for FK definitions.
            if (idx.name.rfind("fk_", 0) != 0) {
                continue;
            }

            ISReferentialConstraint rc;
            rc.constraint_catalog        = "def";
            rc.constraint_schema         = "main";
            rc.constraint_name           = idx.name;
            rc.unique_constraint_catalog = "def";
            rc.unique_constraint_schema  = "main";
            rc.unique_constraint_name    = idx.name + "_pk";  // Inferred
            rc.match_option              = "NONE";
            rc.update_rule               = "NO ACTION";
            rc.delete_rule               = "NO ACTION";
            result.push_back(std::move(rc));
        }
    }
    return result;
}

json InformationSchema::toJSON() const {
    json j;

    json tables_arr = json::array();
    for (const auto& row : getTables()) {
        tables_arr.push_back(row.toJSON());
    }
    j["tables"] = tables_arr;

    json cols_arr = json::array();
    for (const auto& row : getColumns()) {
        cols_arr.push_back(row.toJSON());
    }
    j["columns"] = cols_arr;

    json stats_arr = json::array();
    for (const auto& row : getStatistics()) {
        stats_arr.push_back(row.toJSON());
    }
    j["statistics"] = stats_arr;

    json kcu_arr = json::array();
    for (const auto& row : getKeyColumnUsage()) {
        kcu_arr.push_back(row.toJSON());
    }
    j["key_column_usage"] = kcu_arr;

    json rc_arr = json::array();
    for (const auto& row : getReferentialConstraints()) {
        rc_arr.push_back(row.toJSON());
    }
    j["referential_constraints"] = rc_arr;

    return j;
}

json InformationSchema::tablesToJSON() const {
    json arr = json::array();
    for (const auto& row : getTables()) {
        arr.push_back(row.toJSON());
    }
    return arr;
}

json InformationSchema::columnsToJSON(std::string_view table_name) const {
    json arr = json::array();
    for (const auto& row : getColumns(table_name)) {
        arr.push_back(row.toJSON());
    }
    return arr;
}

json InformationSchema::referentialConstraintsToJSON() const {
    json arr = json::array();
    for (const auto& row : getReferentialConstraints()) {
        arr.push_back(row.toJSON());
    }
    return arr;
}

} // namespace themis
