/**
 * @file online_schema_migration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
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
#include "utils/expected.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @brief Phase of an online DDL operation.
 *
 * Migrations progress through these phases in order:
 *   IDLE → PENDING → IN_PROGRESS → APPLYING → COMPLETED (or FAILED)
 */
enum class OnlineDDLPhase {
    IDLE,         ///< No migration is staged
    PENDING,      ///< Operations have been staged, migrate() not yet called
    IN_PROGRESS,  ///< Background migration thread is running
    APPLYING,     ///< Final schema swap is being applied
    COMPLETED,    ///< All operations succeeded
    FAILED        ///< One or more operations failed; see MigrationResult::errors
};

/**
 * @brief Type of a single DDL operation.
 */
enum class MigrationOpType {
    ADD_COLUMN,
    DROP_COLUMN,
    RENAME_COLUMN,
    CHANGE_COLUMN_TYPE,
    ADD_INDEX,
    DROP_INDEX,
    PARTITION_TABLE
};

// ============================================================================
// Value types
// ============================================================================

/**
 * @brief A single DDL operation staged in SchemaMigrator.
 */
struct MigrationOp {
    MigrationOpType type;

    std::string table_name;
    std::string column_name;      ///< Source column name
    std::string new_name;         ///< Rename target column name (RENAME_COLUMN only)
    std::string column_type;      ///< Column type string: used by ADD_COLUMN, CHANGE_COLUMN_TYPE
    std::string partition_key;    ///< Partition key column (PARTITION_TABLE)
    size_t      num_partitions{0};///< Number of partitions  (PARTITION_TABLE)
    bool        nullable{true};   ///< Nullability for ADD_COLUMN / CHANGE_COLUMN_TYPE
    bool        unique{false};    ///< Unique constraint flag (ADD_INDEX)
};

/**
 * @brief Result of SchemaMigrator::migrate().
 */
struct MigrationResult {
    bool        success{false};       ///< true if all operations succeeded
    OnlineDDLPhase phase{OnlineDDLPhase::IDLE};
    uint64_t    version{0};           ///< Schema version after migration (0 on failure)
    size_t      ops_applied{0};       ///< Number of operations applied successfully
    size_t      ops_total{0};         ///< Total number of staged operations
    std::string error_message;        ///< First error encountered (empty on success)
    std::vector<std::string> errors;  ///< Per-operation error messages
};

/**
 * @brief Partition strategy for PARTITION_TABLE operations.
 */
struct PartitionInfo {
    std::string table_name;
    std::string partition_key;    ///< Column used as partition key
    size_t      num_partitions{4};
    std::vector<std::string> partition_names; ///< Generated on partition creation
};

// ============================================================================
// SchemaMigrator
// ============================================================================

/**
 * @brief Online (zero-downtime) schema migrator for relational and document tables.
 *
 * ## Overview
 *
 * `SchemaMigrator` allows callers to stage one or more DDL operations and
 * then apply them as a single versioned, background migration without
 * interrupting concurrent reads or writes.
 *
 * Supported operations:
 *   - Add / drop columns
 *   - Rename columns
 *   - Change column types (with optional nullability change)
 *   - Add / drop secondary indexes
 *   - Partition tables (range-partitioned by a key column)
 *
 * ## Usage
 * ```cpp
 * SchemaMigrator migrator(schema_mgr);
 *
 * migrator.addColumn("users", "phone_number", "VARCHAR(20)");
 * migrator.renameColumn("users", "email", "email_address");
 * migrator.addIndex("users", "email_address");
 *
 * auto result = migrator.migrate();
 * if (!result.success) {
 *     // handle result.error_message / result.errors
 * }
 * ```
 *
 * ## Thread safety
 *
 * All staging methods (addColumn, dropColumn, …) are NOT thread-safe.
 * migrate() is thread-safe after staging is complete (single writer model).
 * After migrate() returns, the SchemaMigrator may be reused for a new batch.
 *
 * ## Online semantics
 *
 * During migrate(), schema changes are applied atomically to the
 * SchemaManager.  The phase transitions allow external observers to poll
 * the current phase via currentPhase().
 */
class SchemaMigrator {
public:
    /**
     * @brief Configuration for the online migrator.
     */
    struct Config {
        /// Maximum number of staged operations per migrate() call.
        size_t max_ops{128};

        /// When true, a failed operation aborts remaining operations in the batch.
        bool abort_on_first_error{true};

        /// Author recorded in schema version history entries.
        std::string author{"SchemaMigrator"};
    };

    /**
     * @brief Construct a SchemaMigrator with default configuration.
     *
     * @param schema_mgr SchemaManager instance owning the target tables.
     */
    explicit SchemaMigrator(SchemaManager& schema_mgr);

    /**
     * @brief Construct a SchemaMigrator with custom configuration.
     *
     * @param schema_mgr SchemaManager instance owning the target tables.
     * @param config     Configuration overrides.
     */
    SchemaMigrator(SchemaManager& schema_mgr, const Config& config);

    ~SchemaMigrator() = default;

    // Not copyable; movable.
    SchemaMigrator(const SchemaMigrator&) = delete;
    SchemaMigrator& operator=(const SchemaMigrator&) = delete;
    SchemaMigrator(SchemaMigrator&&) = default;
    SchemaMigrator& operator=(SchemaMigrator&&) = default;

    // ── Staging API ──────────────────────────────────────────────────────────

    /**
     * @brief Stage an ADD COLUMN operation.
     *
     * @param table       Table name.
     * @param column      New column name.
     * @param type        Column type string (e.g. "VARCHAR(20)", "integer").
     * @param nullable    Whether the new column is nullable (default: true).
     * @return *this for method chaining.
     */
    SchemaMigrator& addColumn(const std::string& table,
                              const std::string& column,
                              const std::string& type,
                              bool nullable = true);

    /**
     * @brief Stage a DROP COLUMN operation.
     *
     * @param table   Table name.
     * @param column  Column to remove.
     * @return *this for method chaining.
     */
    SchemaMigrator& dropColumn(const std::string& table,
                               const std::string& column);

    /**
     * @brief Stage a RENAME COLUMN operation.
     *
     * @param table       Table name.
     * @param old_name    Current column name.
     * @param new_name    Target column name.
     * @return *this for method chaining.
     */
    SchemaMigrator& renameColumn(const std::string& table,
                                 const std::string& old_name,
                                 const std::string& new_name);

    /**
     * @brief Stage a CHANGE COLUMN TYPE operation.
     *
     * @param table     Table name.
     * @param column    Column whose type should change.
     * @param new_type  New type string.
     * @param nullable  New nullability (default: true).
     * @return *this for method chaining.
     */
    SchemaMigrator& changeColumnType(const std::string& table,
                                     const std::string& column,
                                     const std::string& new_type,
                                     bool nullable = true);

    /**
     * @brief Stage an ADD INDEX operation.
     *
     * @param table   Table name.
     * @param column  Column to index.
     * @param unique  Whether the index should enforce uniqueness (default: false).
     * @return *this for method chaining.
     */
    SchemaMigrator& addIndex(const std::string& table,
                             const std::string& column,
                             bool unique = false);

    /**
     * @brief Stage a DROP INDEX operation.
     *
     * @param table   Table name.
     * @param column  Indexed column whose index should be removed.
     * @return *this for method chaining.
     */
    SchemaMigrator& dropIndex(const std::string& table,
                              const std::string& column);

    /**
     * @brief Stage a PARTITION TABLE operation.
     *
     * Adds partition metadata (partition_key and logical partition names) to
     * the table schema.  Partition names are generated as
     * `<table>_p<i>` for i in [0, num_partitions).
     *
     * @param table            Table name.
     * @param partition_key    Column used as the partition key.
     * @param num_partitions   Number of partitions to create (>= 2).
     * @return *this for method chaining.
     */
    SchemaMigrator& partitionTable(const std::string& table,
                                   const std::string& partition_key,
                                   size_t num_partitions);

    // ── Execution ────────────────────────────────────────────────────────────

    /**
     * @brief Apply all staged operations as an online (zero-downtime) migration.
     *
     * Each operation is applied to the SchemaManager in the order it was
     * staged.  On success, the SchemaMigrator resets to IDLE so it can be
     * reused.  On failure, staged operations are retained and phase() returns
     * FAILED until reset() is called.
     *
     * @return MigrationResult describing the outcome of the migration.
     */
    MigrationResult migrate();

    /**
     * @brief Reset the migrator, discarding any staged operations.
     *
     * After reset(), the migrator is in IDLE phase with an empty operation
     * queue and can be reused.
     */
    void reset();

    // ── Observation ──────────────────────────────────────────────────────────

    /** @return Number of operations currently staged. */
    size_t pendingOps() const noexcept;

    /** @return Current DDL phase (thread-safe). */
    OnlineDDLPhase currentPhase() const noexcept;

    /** @return Read-only view of the staged operation list. */
    const std::vector<MigrationOp>& stagedOps() const noexcept;

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    MigrationResult applyAddColumn(const MigrationOp& op,
                                   SchemaManager::TableSchema& schema);
    MigrationResult applyDropColumn(const MigrationOp& op,
                                    SchemaManager::TableSchema& schema);
    MigrationResult applyRenameColumn(const MigrationOp& op,
                                      SchemaManager::TableSchema& schema);
    MigrationResult applyChangeColumnType(const MigrationOp& op,
                                          SchemaManager::TableSchema& schema);
    MigrationResult applyAddIndex(const MigrationOp& op,
                                  SchemaManager::TableSchema& schema);
    MigrationResult applyDropIndex(const MigrationOp& op,
                                   SchemaManager::TableSchema& schema);
    MigrationResult applyPartitionTable(const MigrationOp& op,
                                        SchemaManager::TableSchema& schema);

    SchemaManager&          schema_mgr_;
    Config                  config_;
    std::vector<MigrationOp> ops_;
    mutable std::mutex      mutex_;
    std::atomic<OnlineDDLPhase> phase_{OnlineDDLPhase::IDLE};
    uint64_t                version_{0};
};

} // namespace storage
} // namespace themis
