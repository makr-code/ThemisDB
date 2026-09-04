/**
 * @file schema_version_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/schema_version_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <spdlog/spdlog.h>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace themis {

// ============================================================================
// SchemaChange serialization
// ============================================================================

json SchemaChange::toJSON() const {
    json j;
    j["version"]     = version;
    j["table_name"]  = table_name;
    j["change_type"] = change_type;
    j["author"]      = author;
    j["description"] = description;

    auto tt = std::chrono::system_clock::to_time_t(timestamp);
    char buf[64];
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    j["timestamp"] = buf;

    j["snapshot"] = snapshot.toJSON();

    return j;
}

SchemaChange SchemaChange::fromJSON(const json& j) {
    SchemaChange sc;
    sc.version     = j.value("version",     uint64_t{0});
    sc.table_name  = j.value("table_name",  std::string{});
    sc.change_type = j.value("change_type", std::string{});
    sc.author      = j.value("author",      std::string{});
    sc.description = j.value("description", std::string{});
    sc.timestamp   = std::chrono::system_clock::now();  // Approximate on reload

    if (j.contains("snapshot") && j["snapshot"].is_object()) {
        sc.snapshot = SchemaManager::parseTableSchema(j["snapshot"]);
    }

    return sc;
}

// ============================================================================
// SchemaVersionManager
// ============================================================================

SchemaVersionManager::SchemaVersionManager(
    RocksDBWrapper& db,
    SchemaManager&  schema_mgr)
    : db_(db)
    , schema_mgr_(schema_mgr)
{}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

VersionResult<uint64_t> SchemaVersionManager::createSchemaVersion(
    std::string_view table_name,
    std::string_view author,
    std::string_view description)
{
    if (table_name.empty()) {
        return VersionResult<uint64_t>::failure(
            VersionErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }

    // Fetch current schema snapshot
    auto maybe_schema = schema_mgr_.getTable(table_name);
    if (!maybe_schema.has_value()) {
        return VersionResult<uint64_t>::failure(
            VersionErrorCode::TABLE_NOT_FOUND,
            "Table '" + std::string(table_name) + "' not found in schema manager");
    }

    uint64_t prev_version = readCurrentVersion(table_name);
    uint64_t new_version  = prev_version + 1;

    SchemaChange change;
    change.version     = new_version;
    change.table_name  = std::string(table_name);
    change.change_type = (prev_version == 0) ? "create" : "update";
    change.author      = std::string(author);
    change.description = std::string(description);
    change.timestamp   = std::chrono::system_clock::now();
    change.snapshot    = *maybe_schema;

    if (!persistChange(change)) {
        return VersionResult<uint64_t>::failure(
            VersionErrorCode::STORAGE_ERROR,
            "Failed to persist schema version for '" + std::string(table_name) + "'");
    }

    // Emit audit log entry
    if (audit_log_) {
        audit_log_->record(table_name, change.change_type, author, description, new_version);
    }

    spdlog::info("SchemaVersionManager: Recorded version {} for table '{}'",
                 new_version, table_name);

    return VersionResult<uint64_t>::success(new_version);
}

VersionResult<uint64_t> SchemaVersionManager::getCurrentVersion(
    std::string_view table_name) const
{
    if (table_name.empty()) {
        return VersionResult<uint64_t>::failure(
            VersionErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }

    uint64_t v = readCurrentVersion(table_name);
    if (v == 0) {
        return VersionResult<uint64_t>::failure(
            VersionErrorCode::TABLE_NOT_FOUND,
            "No version history found for table '" + std::string(table_name) + "'");
    }

    return VersionResult<uint64_t>::success(v);
}

VersionResult<std::vector<SchemaChange>> SchemaVersionManager::getChangeHistory(
    std::string_view table_name) const
{
    if (table_name.empty()) {
        return VersionResult<std::vector<SchemaChange>>::failure(
            VersionErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }

    uint64_t current = readCurrentVersion(table_name);
    if (current == 0) {
        // No history yet; return empty list (not an error)
        return VersionResult<std::vector<SchemaChange>>::success({});
    }

    std::vector<SchemaChange> history;
    history.reserve(static_cast<size_t>(current));

    for (uint64_t v = 1; v <= current; ++v) {
        auto maybe = loadChange(table_name, v);
        if (maybe.has_value()) {
            history.push_back(std::move(*maybe));
        }
    }

    return VersionResult<std::vector<SchemaChange>>::success(std::move(history));
}

VersionResult<SchemaChange> SchemaVersionManager::getVersion(
    std::string_view table_name,
    uint64_t version) const
{
    if (table_name.empty()) {
        return VersionResult<SchemaChange>::failure(
            VersionErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }
    if (version == 0) {
        return VersionResult<SchemaChange>::failure(
            VersionErrorCode::INVALID_VERSION, "Version must be >= 1");
    }

    auto maybe = loadChange(table_name, version);
    if (!maybe.has_value()) {
        return VersionResult<SchemaChange>::failure(
            VersionErrorCode::VERSION_NOT_FOUND,
            "Version " + std::to_string(version) +
            " not found for table '" + std::string(table_name) + "'");
    }

    return VersionResult<SchemaChange>::success(std::move(*maybe));
}

VersionResult<bool> SchemaVersionManager::rollbackToVersion(
    std::string_view table_name,
    uint64_t version,
    std::string_view author)
{
    if (table_name.empty()) {
        return VersionResult<bool>::failure(
            VersionErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }
    if (version == 0) {
        return VersionResult<bool>::failure(
            VersionErrorCode::INVALID_VERSION, "Version must be >= 1");
    }

    // Load the target version
    auto maybe_change = loadChange(table_name, version);
    if (!maybe_change.has_value()) {
        return VersionResult<bool>::failure(
            VersionErrorCode::VERSION_NOT_FOUND,
            "Version " + std::to_string(version) +
            " not found for table '" + std::string(table_name) + "'");
    }

    // Apply the historical snapshot via SchemaManager
    bool applied = schema_mgr_.setTableSchema(table_name, maybe_change->snapshot);
    if (!applied) {
        return VersionResult<bool>::failure(
            VersionErrorCode::STORAGE_ERROR,
            "SchemaManager rejected the rollback schema for '" +
            std::string(table_name) + "'");
    }

    // Record the rollback as a new version entry so history is preserved
    std::string rollback_desc =
        "Rollback to version " + std::to_string(version);
    auto new_ver_result = createSchemaVersion(
        table_name, author, rollback_desc);
    if (!new_ver_result.ok) {
        spdlog::warn("SchemaVersionManager: Rollback applied but failed to record it: {}",
                     new_ver_result.error_message);
    }

    // Emit dedicated rollback audit entry
    if (audit_log_) {
        audit_log_->record(table_name, "rollback", author, rollback_desc,
                           new_ver_result.ok ? new_ver_result.value : 0,
                           json{{"rolled_back_to", version}});
    }

    spdlog::info("SchemaVersionManager: Rolled back table '{}' to version {}",
                 table_name, version);

    return VersionResult<bool>::success(true);
}

VersionResult<json> SchemaVersionManager::diffVersions(
    std::string_view table_name,
    uint64_t version_a,
    uint64_t version_b) const
{
    auto r_a = getVersion(table_name, version_a);
    if (!r_a.ok) {
      return VersionResult<json>::failure(r_a.error, r_a.error_message);
    }

    auto r_b = getVersion(table_name, version_b);
    if (!r_b.ok) {
      return VersionResult<json>::failure(r_b.error, r_b.error_message);
    }

    const auto& schema_a = r_a.value.snapshot;
    const auto& schema_b = r_b.value.snapshot;

    // Build property maps for comparison
    std::map<std::string, SchemaManager::PropertyInfo> props_a, props_b;
    for (const auto& p : schema_a.properties) {
      props_a[p.name] = p;
    }
    for (const auto& p : schema_b.properties) {
      props_b[p.name] = p;
    }

    json added   = json::array();
    json removed = json::array();
    json modified = json::array();

    for (const auto& [name, prop] : props_b) {
        if (props_a.find(name) == props_a.end()) {
            added.push_back(prop.toJSON());
        } else {
            const auto& old_prop = props_a.at(name);
            if (old_prop.type != prop.type ||
                old_prop.nullable != prop.nullable ||
                old_prop.indexed  != prop.indexed)
            {
                json change;
                change["column"]  = name;
                change["before"]  = old_prop.toJSON();
                change["after"]   = prop.toJSON();
                modified.push_back(change);
            }
        }
    }
    for (const auto& [name, prop] : props_a) {
        if (props_b.find(name) == props_b.end()) {
            removed.push_back(prop.toJSON());
        }
    }

    json diff;
    diff["table_name"] = std::string(table_name);
    diff["version_a"]  = version_a;
    diff["version_b"]  = version_b;
    diff["added"]      = added;
    diff["removed"]    = removed;
    diff["modified"]   = modified;

    return VersionResult<json>::success(std::move(diff));
}

json SchemaVersionManager::historyToJSON(std::string_view table_name) const {
    auto r = getChangeHistory(table_name);
    json arr = json::array();
    if (!r.ok) {
      return arr;
    }
    for (const auto& change : r.value) {
        arr.push_back(change.toJSON());
    }
    return arr;
}

// ----------------------------------------------------------------------------
// Internal helpers
// ----------------------------------------------------------------------------

std::string SchemaVersionManager::versionKey(
    std::string_view table_name, uint64_t version)
{
    // Zero-padded 10-digit version for lexicographic ordering
    std::ostringstream oss;
    oss << "config:schema_version:"
        << table_name << ":"
        << std::setw(10) << std::setfill('0') << version;
    return oss.str();
}

std::string SchemaVersionManager::currentVersionKey(std::string_view table_name) {
    return "config:schema_version:" + std::string(table_name) + ":current";
}

uint64_t SchemaVersionManager::readCurrentVersion(std::string_view table_name) const {
    try {
        std::string key = currentVersionKey(table_name);
        auto result = db_.get(key);
        if (!result.has_value() || result->empty()) {
            return 0;
        }
        std::string raw(result->begin(), result->end());
        return static_cast<uint64_t>(std::stoull(raw));
    } catch (...) {
        return 0;
    }
}

bool SchemaVersionManager::persistChange(const SchemaChange& change) {
    try {
        // Persist the version record
        std::string key   = versionKey(change.table_name, change.version);
        std::string value = change.toJSON().dump();
        std::vector<uint8_t> data(value.begin(), value.end());

        if (!db_.put(key, data)) {
            spdlog::error("SchemaVersionManager: Failed to write version record '{}'", key);
            return false;
        }

        // Update the "current version" counter
        std::string cur_key   = currentVersionKey(change.table_name);
        std::string cur_value = std::to_string(change.version);
        std::vector<uint8_t> cur_data(cur_value.begin(), cur_value.end());

        if (!db_.put(cur_key, cur_data)) {
            spdlog::error("SchemaVersionManager: Failed to update current version counter");
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        spdlog::error("SchemaVersionManager: Exception persisting change: {}", e.what());
        return false;
    }
}

std::optional<SchemaChange> SchemaVersionManager::loadChange(
    std::string_view table_name,
    uint64_t version) const
{
    try {
        std::string key = versionKey(table_name, version);
        auto result = db_.get(key);
        if (!result.has_value() || result->empty()) {
            return std::nullopt;
        }

        std::string raw(result->begin(), result->end());
        json j = json::parse(raw);
        return SchemaChange::fromJSON(j);

    } catch (const std::exception& e) {
        spdlog::warn("SchemaVersionManager: Failed to load version {} for '{}': {}",
                     version, table_name, e.what());
        return std::nullopt;
    }
}

// ============================================================================
// Dry-run validation
// ============================================================================

VersionResult<bool> SchemaVersionManager::validateMigration(
    std::string_view table_name,
    const SchemaManager::TableSchema& new_schema) const
{
    // 1. Schema must have a non-empty name
    if (new_schema.name.empty()) {
        return VersionResult<bool>::failure(
            VersionErrorCode::INVALID_VERSION,
            "Migration validation failed: schema name must not be empty");
    }

    // 2. Schema must define at least one column/property
    if (new_schema.properties.empty()) {
        return VersionResult<bool>::failure(
            VersionErrorCode::INVALID_VERSION,
            "Migration validation failed: schema must have at least one column");
    }

    // 3. Column names must be unique within the new schema
    std::set<std::string> seen_columns;
    for (const auto& col : new_schema.properties) {
        if (col.name.empty()) {
            return VersionResult<bool>::failure(
                VersionErrorCode::INVALID_VERSION,
                "Migration validation failed: column name must not be empty");
        }
        if (!seen_columns.insert(col.name).second) {
            return VersionResult<bool>::failure(
                VersionErrorCode::INVALID_VERSION,
                "Migration validation failed: duplicate column '" + col.name + "'");
        }
    }

    // 4. If a current version exists, the new schema must differ from it
    uint64_t current_ver = readCurrentVersion(table_name);
    if (current_ver > 0) {
        auto current_change = loadChange(table_name, current_ver);
        if (current_change.has_value()) {
            const auto& existing = current_change->snapshot;
            if (existing.name == new_schema.name &&
                existing.properties.size() == new_schema.properties.size())
            {
                bool identical = true;
                for (size_t i = 0; i < existing.properties.size() && identical; ++i) {
                    if (existing.properties[i].name != new_schema.properties[i].name ||
                        existing.properties[i].type != new_schema.properties[i].type) {
                        identical = false;
                    }
                }
                if (identical) {
                    return VersionResult<bool>::failure(
                        VersionErrorCode::INVALID_VERSION,
                        "Migration validation failed: new schema is identical to current version "
                        + std::to_string(current_ver));
                }
            }
        }
    }

    spdlog::info("SchemaVersionManager: dry-run validation passed for table '{}' ({} columns)",
                 table_name, new_schema.properties.size());
    return VersionResult<bool>::success(true);
}

// ============================================================================
// Migration script generation
// ============================================================================

/// Map a ThemisDB property type string to a SQL column type.
static std::string toSqlType(const std::string& themis_type) {
    if (themis_type == "string") {
      return "VARCHAR";
    }
    if (themis_type == "integer") {
      return "INTEGER";
    }
    if (themis_type == "double") {
      return "DOUBLE PRECISION";
    }
    if (themis_type == "boolean") {
      return "BOOLEAN";
    }
    if (themis_type == "vector") {
      return "VECTOR";
    }
    if (themis_type == "binary") {
      return "BYTEA";
    }
    return "TEXT";
}

VersionResult<std::string> SchemaVersionManager::generateMigrationScript(
    std::string_view table_name,
    uint64_t version_from,
    uint64_t version_to) const
{
    auto diff_result = diffVersions(table_name, version_from, version_to);
    if (!diff_result.ok) {
        return VersionResult<std::string>::failure(
            diff_result.error, diff_result.error_message);
    }

    const json& diff  = diff_result.value;
    const std::string tbl = std::string(table_name);
    std::ostringstream script;

    script << "-- Migration: " << tbl
           << " from version " << version_from
           << " to version "   << version_to << "\n";

    // ADD COLUMN statements
    for (const auto& col : diff["added"]) {
        const std::string col_name = col.value("name", std::string{});
        const std::string col_type = col.value("type", std::string{});
        const bool nullable        = col.value("nullable", true);

        script << "ALTER TABLE " << tbl
               << " ADD COLUMN " << col_name
               << " " << toSqlType(col_type);
        if (!nullable) {
            script << " NOT NULL";
        }
        script << ";\n";
    }

    // DROP COLUMN statements
    for (const auto& col : diff["removed"]) {
        const std::string col_name = col.value("name", std::string{});
        script << "ALTER TABLE " << tbl
               << " DROP COLUMN " << col_name << ";\n";
    }

    // ALTER COLUMN statements (type or nullability changes)
    for (const auto& change : diff["modified"]) {
        const std::string col_name    = change.value("column", std::string{});
        const json&       before      = change["before"];
        const json&       after       = change["after"];
        const std::string new_type    = after.value("type",     std::string{});
        const std::string old_type    = before.value("type",    std::string{});
        const bool        new_nullable = after.value("nullable", true);
        const bool        old_nullable = before.value("nullable", true);

        if (new_type != old_type) {
            script << "ALTER TABLE " << tbl
                   << " ALTER COLUMN " << col_name
                   << " TYPE " << toSqlType(new_type) << ";\n";
        }
        if (new_nullable != old_nullable) {
            if (!new_nullable) {
                script << "ALTER TABLE " << tbl
                       << " ALTER COLUMN " << col_name << " SET NOT NULL;\n";
            } else {
                script << "ALTER TABLE " << tbl
                       << " ALTER COLUMN " << col_name << " DROP NOT NULL;\n";
            }
        }
    }

    spdlog::info(
        "SchemaVersionManager: Generated migration script for '{}' v{} → v{} "
        "({} add, {} drop, {} modify)",
        table_name, version_from, version_to,
        diff["added"].size(), diff["removed"].size(), diff["modified"].size());

    return VersionResult<std::string>::success(script.str());
}

} // namespace themis

