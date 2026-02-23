/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            in_place_schema_migrator.cpp                       ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-23                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     116                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/in_place_schema_migrator.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)

#include <map>

namespace themis {
namespace updates {

// ============================================================================
// InPlaceSchemaMigrator
// ============================================================================

InPlaceSchemaMigrator::InPlaceSchemaMigrator(const Config& config)
    : config_(config) {}

// ----------------------------------------------------------------------------
// isAdditiveMigration (static)
// ----------------------------------------------------------------------------

bool InPlaceSchemaMigrator::isAdditiveMigration(
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema)
{
    // Build a map of existing columns for fast lookup
    std::map<std::string, const SchemaManager::PropertyInfo*> from_props;
    for (const auto& p : from_schema.properties) {
        from_props[p.name] = &p;
    }

    // Every column in from_schema must appear unchanged in to_schema
    std::map<std::string, const SchemaManager::PropertyInfo*> to_props;
    for (const auto& p : to_schema.properties) {
        to_props[p.name] = &p;
    }

    for (const auto& [name, from_p] : from_props) {
        auto it = to_props.find(name);
        if (it == to_props.end()) {
            // A column was removed – not additive
            return false;
        }
        const auto* to_p = it->second;
        if (to_p->type != from_p->type || to_p->nullable != from_p->nullable) {
            // A column was modified – not additive
            return false;
        }
    }

    // to_schema must add at least one new column
    return to_schema.properties.size() > from_schema.properties.size();
}

// ----------------------------------------------------------------------------
// findAddedColumns (static, private)
// ----------------------------------------------------------------------------

std::vector<std::string> InPlaceSchemaMigrator::findAddedColumns(
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema)
{
    std::map<std::string, bool> from_names;
    for (const auto& p : from_schema.properties) {
        from_names[p.name] = true;
    }

    std::vector<std::string> added;
    for (const auto& p : to_schema.properties) {
        if (from_names.find(p.name) == from_names.end()) {
            added.push_back(p.name);
        }
    }
    return added;
}

// ----------------------------------------------------------------------------
// apply
// ----------------------------------------------------------------------------

InPlaceMigrationResult InPlaceSchemaMigrator::apply(
    const std::string& table_name,
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema,
    SchemaManager& schema_mgr,
    SchemaVersionManager& version_mgr,
    const std::string& author)
{
    InPlaceMigrationResult result;

    if (table_name.empty()) {
        result.error_message = "table_name must not be empty";
        LOG_ERROR("InPlaceSchemaMigrator: {}", result.error_message);
        return result;
    }

    if (config_.strict_additive && !isAdditiveMigration(from_schema, to_schema)) {
        result.error_message =
            "Migration for table '" + table_name +
            "' is not purely additive; use SchemaMigrationTester for "
            "destructive or type-changing migrations";
        LOG_ERROR("InPlaceSchemaMigrator: {}", result.error_message);
        return result;
    }

    result.added_columns = findAddedColumns(from_schema, to_schema);

    // Apply the new schema metadata in-place (no data copy)
    if (!schema_mgr.setTableSchema(table_name, to_schema)) {
        result.error_message =
            "SchemaManager rejected the new schema for table '" + table_name + "'";
        LOG_ERROR("InPlaceSchemaMigrator: {}", result.error_message);
        return result;
    }

    // Record the change in version history
    auto ver_result = version_mgr.createSchemaVersion(
        table_name,
        author,
        "in-place additive migration: added " +
            std::to_string(result.added_columns.size()) + " column(s)");

    if (!ver_result.ok) {
        // Schema was already applied; attempt to report the version error
        result.error_message =
            "Schema applied but version recording failed: " + ver_result.error_message;
        LOG_ERROR("InPlaceSchemaMigrator: {}", result.error_message);
        return result;
    }

    result.schema_version = ver_result.value;
    result.success        = true;

    std::string cols_str;
    for (size_t i = 0; i < result.added_columns.size(); ++i) {
        if (i) cols_str += ", ";
        cols_str += result.added_columns[i];
    }
    LOG_INFO(
        "InPlaceSchemaMigrator: table '{}' migrated in-place to v{}; "
        "added {} column(s): {}",
        table_name, result.schema_version, result.added_columns.size(),
        cols_str);

    return result;
}

} // namespace updates
} // namespace themis
