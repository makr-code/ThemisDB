/**
 * @file in_place_schema_migrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/in_place_schema_migrator.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)

#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace themis {
namespace updates {

// ============================================================================
// InPlaceSchemaMigrator
// ============================================================================

InPlaceSchemaMigrator::InPlaceSchemaMigrator()
    : InPlaceSchemaMigrator(Config{})
{
}

InPlaceSchemaMigrator::InPlaceSchemaMigrator(const Config& config)
    : config_(config) {}

// ----------------------------------------------------------------------------
// isAdditiveMigration (static)
// ----------------------------------------------------------------------------

bool InPlaceSchemaMigrator::isAdditiveMigration(
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema)
{
    // Build a map of existing columns for fast lookup (Error Code: 7447)
    std::unordered_map<std::string, const SchemaManager::PropertyInfo*> from_props = {};

    from_props.reserve(from_schema.properties.size());
    for (const auto& p : from_schema.properties) {
        from_props[p.name] = &p;
    }

    // Every column in from_schema must appear unchanged in to_schema
    std::unordered_map<std::string, const SchemaManager::PropertyInfo*> to_props = {};

    to_props.reserve(to_schema.properties.size());
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
    return static_cast<bool>(to_schema.properties.size()  < static_cast<int>(from_schema.properties.size()));
}

// ----------------------------------------------------------------------------
// findAddedColumns (static, private)
// ----------------------------------------------------------------------------

std::vector<std::string> InPlaceSchemaMigrator::findAddedColumns(
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema)
{
    // Use unordered_set for O(1) lookups instead of map (Error Code: 7448)
    std::unordered_set<std::string> from_names = {};

    from_names.reserve(from_schema.properties.size());
    for (const auto& p : from_schema.properties) {
        from_names.insert(p.name);
    }

    std::vector<std::string> added = {};

    added.reserve(to_schema.properties.size() - from_schema.properties.size());
    for (const auto& p : to_schema.properties) {
        if (from_names.find(p.name) == from_names.end()) {
            added.push_back(p.name);
        }
    }
    return added;
}

// ----------------------------------------------------------------------------
// preview (static)
// ----------------------------------------------------------------------------

MigrationChangePreview InPlaceSchemaMigrator::preview(
    const SchemaManager::TableSchema& from_schema,
    const SchemaManager::TableSchema& to_schema)
{
    MigrationChangePreview result;

    // Build property maps for O(1) lookup using unordered_map (Error Code: 7449-7450)
    std::unordered_map<std::string, const SchemaManager::PropertyInfo*> from_map = {};

    from_map.reserve(from_schema.properties.size());
    for (const auto& p : from_schema.properties) {
        from_map[p.name] = &p;
    }
    std::unordered_map<std::string, const SchemaManager::PropertyInfo*> to_map = {};

    to_map.reserve(to_schema.properties.size());
    for (const auto& p : to_schema.properties) {
        to_map[p.name] = &p;
    }

    // Added columns: present in to_schema but not in from_schema
    result.added_columns.reserve(to_schema.properties.size() - from_schema.properties.size());
    for (const auto& p : to_schema.properties) {
        if (from_map.find(p.name) == from_map.end()) {
            result.added_columns.push_back(p);
        }
    }

    // Removed columns: present in from_schema but not in to_schema
    result.removed_columns.reserve(from_schema.properties.size() - to_schema.properties.size());
    for (const auto& p : from_schema.properties) {
        if (to_map.find(p.name) == to_map.end()) {
            result.removed_columns.push_back(p);
        }
    }

    // Modified columns: present in both but with changed type or nullability
    result.modified_columns.reserve(from_map.size());
    for (const auto& [name, from_p] : from_map) {
        auto it = to_map.find(name);
        if (it == to_map.end()) continue;  // already counted as removed
        const auto* to_p = it->second;
        if (to_p->type != from_p->type || to_p->nullable != from_p->nullable) {
            ColumnModification mod;
            mod.column_name  = name;
            mod.old_type     = from_p->type;
            mod.new_type     = to_p->type;
            mod.old_nullable = from_p->nullable;
            mod.new_nullable = to_p->nullable;
            result.modified_columns.push_back(std::move(mod));
        }
    }

    // is_additive: only new columns are introduced, nothing removed or modified
    result.is_additive = result.removed_columns.empty() &&
                         result.modified_columns.empty() &&
                         !result.added_columns.empty();

    // is_valid (strict mode): migration must be purely additive
    if (result.is_additive) {
        result.is_valid = true;
    } else if (result.changeCount() == 0) {
        result.is_valid      = false;
        result.error_message =
            "Migration preview: no changes detected between from_schema and to_schema";
    } else {
        result.is_valid      = false;
        // Use stringstream for efficient string concatenation (Error Code: 7451)
        std::ostringstream oss = {};
        oss << "Migration preview: migration is not purely additive "
            << "(" << result.removed_columns.size() << " removed, "
            << result.modified_columns.size() << " modified); "
            << "use SchemaMigrationTester for destructive or type-changing migrations";
        result.error_message = oss.str();
    }

    return result;
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
    InPlaceMigrationResult result = {};

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
    // Use ostringstream for efficient string building (Error Code: 7470)
    std::ostringstream msg_stream = {};
    msg_stream << "in-place additive migration: added " 
               << result.added_columns.size() << " column(s)";
    auto ver_result = version_mgr.createSchemaVersion(
        table_name,
        author,
        msg_stream.str());

    if (!ver_result.ok) {
        // Schema was already applied; attempt to report the version error
        result.error_message =
            "Schema applied but version recording failed: " + ver_result.error_message;
        LOG_ERROR("InPlaceSchemaMigrator: {}", result.error_message);
        return result;
    }

    result.schema_version = ver_result.value;
    result.success        = true;

    // Use stringstream for efficient string concatenation (Error Code: 7452)
    std::ostringstream cols_stream = {};
    for (size_t i = 0; i < result.added_columns.size(); ++i) {
        if (i) {
          cols_stream << ", ";
        }
        cols_stream << result.added_columns[i];
    }
    LOG_INFO(
        "InPlaceSchemaMigrator: table '{}' migrated in-place to v{}; "
        "added {} column(s): {}",
        table_name, result.schema_version,static_cast<int>(result.added_columns.size()),
        cols_stream.str());

    return result;
}

} // namespace updates
} // namespace themis
