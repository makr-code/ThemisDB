/**
 * @file in_place_schema_migrator.h
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
 * @brief Describes a single column whose type or nullability would change.
 */
struct ColumnModification {
    std::string column_name;   ///< Name of the column
    std::string old_type;      ///< Type in the current (from) schema
    std::string new_type;      ///< Type in the proposed (to) schema
    bool old_nullable = true;  ///< Nullability in the current schema
    bool new_nullable = true;  ///< Nullability in the proposed schema
};

/**
 * @brief Dry-run result: detailed preview of what a migration would change.
 *
 * Returned by InPlaceSchemaMigrator::preview().  No schema or version state
 * is modified when computing this result.
 */
struct MigrationChangePreview {
    bool is_valid    = false;  ///< true if apply() would succeed in strict mode
    bool is_additive = false;  ///< true if only new columns are introduced
    std::string error_message; ///< Non-empty when is_valid == false

    /// Columns present in to_schema but absent from from_schema
    std::vector<SchemaManager::PropertyInfo> added_columns;

    /// Columns present in from_schema but absent from to_schema
    std::vector<SchemaManager::PropertyInfo> removed_columns;

    /// Columns present in both schemas whose type or nullability changed
    std::vector<ColumnModification> modified_columns;

    /// Total number of schema changes (added + removed + modified)
    std::size_t changeCount() const noexcept {
        return added_columns.size() + removed_columns.size() + modified_columns.size();
    }
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

    InPlaceSchemaMigrator();
    explicit InPlaceSchemaMigrator(const Config& config);
    ~InPlaceSchemaMigrator() = default;

    // Non-copyable, movable
    InPlaceSchemaMigrator(const InPlaceSchemaMigrator&) = delete;
    InPlaceSchemaMigrator& operator=(const InPlaceSchemaMigrator&) = delete;
    InPlaceSchemaMigrator(InPlaceSchemaMigrator&&) noexcept = default;
    InPlaceSchemaMigrator& operator=(InPlaceSchemaMigrator&&) noexcept = default;

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
     * @brief Dry-run: compute a detailed change preview without applying or
     *        persisting anything.
     *
     * Unlike apply(), preview() never modifies a SchemaManager or
     * SchemaVersionManager.  It may be called safely with any combination of
     * schemas, including non-additive ones, to understand exactly what changes
     * would be required before committing to an apply().
     *
     * @param from_schema  Current (baseline) schema.
     * @param to_schema    Proposed new schema.
     * @return MigrationChangePreview describing every change that would be
     *         performed, plus validation flags indicating whether apply()
     *         would succeed in strict mode.
     */
    static MigrationChangePreview preview(
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
