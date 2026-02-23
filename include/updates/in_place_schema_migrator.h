/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            in_place_schema_migrator.h                         ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-23                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     161                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "metadata/schema_manager.h"
#include "metadata/schema_version_manager.h"
#include <string>
#include <vector>

namespace themis {
namespace updates {

/**
 * @brief Result of an in-place schema migration.
 */
struct InPlaceMigrationResult {
    bool success = false;         ///< true if the migration was applied successfully
    std::string error_message;    ///< Non-empty on failure
    std::vector<std::string> added_columns;  ///< Column names that were added
    uint64_t schema_version = 0;  ///< New schema version recorded in SchemaVersionManager
};

/**
 * @brief Applies additive schema migrations in-place without copying data.
 *
 * ### Concept
 * An *additive* migration adds one or more new columns to a table schema while
 * leaving all existing columns untouched.  Because no existing column is
 * modified or removed, currently stored records remain valid under the new
 * schema: they simply lack values for the newly added columns (treated as
 * null/default).  This means the schema metadata can be updated directly
 * without any data-copy step.
 *
 * ### What counts as "additive"
 * A migration from @p from_schema to @p to_schema is additive when:
 *   - Every column present in @p from_schema is still present in @p to_schema
 *     with the same name, type, and nullability.
 *   - @p to_schema contains at least one column not in @p from_schema.
 *
 * ### What is NOT supported (use SchemaMigrationTester instead)
 *   - Removing a column.
 *   - Renaming a column.
 *   - Changing a column's type or nullability.
 *   - Any migration that requires transforming existing data.
 *
 * ### Example
 * ```cpp
 * InPlaceSchemaMigrator migrator;
 *
 * auto from = currentSchema;            // {"id", "name"}
 * auto to   = newSchema;                // {"id", "name", "email"}
 *
 * if (InPlaceSchemaMigrator::isAdditiveMigration(from, to)) {
 *     auto result = migrator.apply("users", from, to,
 *                                  schema_mgr, version_mgr, "ci-bot");
 *     if (result.success)
 *         // schema updated; no data copy performed
 * }
 * ```
 *
 * ### Thread-safety
 * Not thread-safe.  The caller must ensure exclusive access to @p schema_mgr
 * and @p version_mgr during apply().
 *
 * ### Constraints
 * - `HotReloadEngine::Config` is stable from v1.x; this class is additive.
 * - Migration version numbering is managed by SchemaVersionManager; no
 *   re-sequencing is performed.
 */
class InPlaceSchemaMigrator {
public:
    /**
     * @brief Configuration for the in-place migrator.
     */
    struct Config {
        /// When true (default), apply() returns an error for non-additive
        /// migrations instead of proceeding.
        bool strict_additive = true;
    };

    explicit InPlaceSchemaMigrator(const Config& config = {});
    ~InPlaceSchemaMigrator() = default;

    // Non-copyable, movable
    InPlaceSchemaMigrator(const InPlaceSchemaMigrator&) = delete;
    InPlaceSchemaMigrator& operator=(const InPlaceSchemaMigrator&) = delete;
    InPlaceSchemaMigrator(InPlaceSchemaMigrator&&) = default;
    InPlaceSchemaMigrator& operator=(InPlaceSchemaMigrator&&) = default;

    /**
     * @brief Check whether a schema migration is purely additive.
     *
     * A migration is additive if:
     *   - Every column in @p from_schema is present in @p to_schema with the
     *     same name, type, and nullability.
     *   - @p to_schema contains at least one additional column.
     *
     * @param from_schema  Current schema.
     * @param to_schema    Proposed new schema.
     * @return true if the migration is additive and can be applied in-place.
     */
    static bool isAdditiveMigration(
        const SchemaManager::TableSchema& from_schema,
        const SchemaManager::TableSchema& to_schema
    );

    /**
     * @brief Apply an additive schema migration in-place (no data copy).
     *
     * Validates that the migration is additive, updates @p schema_mgr with
     * the new schema, and records the change in @p version_mgr.
     *
     * @param table_name   Name of the table being migrated.
     * @param from_schema  Current schema (used for additive validation).
     * @param to_schema    New schema to apply.
     * @param schema_mgr   SchemaManager for the target database.
     * @param version_mgr  SchemaVersionManager for the target database.
     * @param author       Identity of the person/system performing the migration.
     * @return InPlaceMigrationResult describing the outcome.
     */
    InPlaceMigrationResult apply(
        const std::string& table_name,
        const SchemaManager::TableSchema& from_schema,
        const SchemaManager::TableSchema& to_schema,
        SchemaManager& schema_mgr,
        SchemaVersionManager& version_mgr,
        const std::string& author = ""
    );

private:
    Config config_;

    /// Collect columns present in @p to_schema but absent in @p from_schema.
    static std::vector<std::string> findAddedColumns(
        const SchemaManager::TableSchema& from_schema,
        const SchemaManager::TableSchema& to_schema
    );
};

} // namespace updates
} // namespace themis
