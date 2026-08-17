/**
 * @file schema_migration.h
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

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

class SchemaMigration;
struct MigrationContext;

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

/**
 * @brief Strategy used when a migration encounters an error.
 */
enum class RollbackStrategy {
    AUTOMATIC,  ///< Roll back all applied steps automatically on any failure.
    MANUAL,     ///< Leave the database in the failed state; caller handles rollback.
};

/**
 * @brief Phase of the online DDL algorithm.
 */
enum class OnlineDDLPhase {
    IDLE,               ///< No migration in progress.
    SHADOW_CREATE,      ///< Shadow table created with new schema.
    DUAL_WRITE,         ///< Writes replicated to both main and shadow tables.
    BACKFILL,           ///< Historical rows being copied to shadow table.
    CONSISTENCY_CHECK,  ///< Data consistency between tables being verified.
    ATOMIC_SWAP,        ///< Shadow table atomically renamed to replace main.
    CLEANUP,            ///< Old (main) table dropped; migration complete.
    ROLLED_BACK,        ///< Migration rolled back after failure.
};

// ---------------------------------------------------------------------------
// Configuration structures
// ---------------------------------------------------------------------------

/**
 * @brief Definition of a new column to be added to a table.
 */
struct ColumnDef {
    std::string name;                           ///< Column name (must be non-empty).
    std::string type;                           ///< SQL type string, e.g. "VARCHAR(20)".
    bool        nullable      = true;           ///< Whether NULL values are allowed.
    std::string default_value;                  ///< Default SQL expression (may be empty).
    std::string comment;                        ///< Optional human-readable comment.
};

/**
 * @brief Definition of an index to be created on a table.
 */
struct IndexDef {
    std::string              name;              ///< Index name.
    std::vector<std::string> columns;           ///< Ordered list of indexed column names.
    bool                     unique       = false;  ///< UNIQUE constraint on the index.
    bool                     build_online = true;   ///< Build the index in the background.
};

/**
 * @brief Options controlling how a column drop is performed.
 */
struct DropColumnOptions {
    /// Minimum time to keep the column before it is physically removed.
    /// The column is hidden immediately but data is not purged until after
    /// this period, allowing rollback or dependent queries to complete.
    std::chrono::hours grace_period{0};
};

// ---------------------------------------------------------------------------
// Result types
// ---------------------------------------------------------------------------

/**
 * @brief Result returned by SchemaMigration::apply().
 */
struct MigrationResult {
    bool        success       = false;  ///< true if all steps completed without error.
    std::string error_message;          ///< Non-empty when success == false.
    std::string version;                ///< Target migration version string.
    OnlineDDLPhase phase_reached = OnlineDDLPhase::IDLE;  ///< Last successfully completed phase.

    /// Names of columns added during backfill (subset of migration operations).
    std::vector<std::string> backfilled_columns;
    /// Names of indexes built online during the migration.
    std::vector<std::string> indexes_built_online;
};

/**
 * @brief Result returned by SchemaMigration::rollback().
 */
struct RollbackResult {
    bool        success       = false;  ///< true if rollback completed cleanly.
    std::string error_message;          ///< Non-empty when success == false.
    OnlineDDLPhase rolled_back_from = OnlineDDLPhase::IDLE;  ///< Phase that was unwound.
};

// ---------------------------------------------------------------------------
// Storage abstraction used by MigrationContext
// ---------------------------------------------------------------------------

/**
 * @brief Minimal key-value storage interface for custom migration callbacks.
 *
 * Implementations wrap the actual storage engine (e.g. RocksDBWrapper) so
 * that custom migration lambdas remain testable without a real database.
 */
class IMigrationStorage {
public:
    virtual ~IMigrationStorage() = default;

    /**
     * @brief Store or overwrite a key-value pair.
     * @param key   Record key.
     * @param value Serialised record value.
     * @return true on success.
     */
    [[nodiscard]] virtual bool put(const std::string& key, const std::string& value) = 0;

    /**
     * @brief Retrieve the value for a key.
     * @param key    Record key.
     * @param value  Output parameter filled on success.
     * @return true if the key exists, false if not found.
     */
    [[nodiscard]] virtual bool get(const std::string& key, std::string& value) = 0;

    /**
     * @brief Delete a key-value pair.
     * @param key Record key.
     * @return true on success (including when the key did not exist).
     */
    [[nodiscard]] virtual bool remove(const std::string& key) = 0;

    /**
     * @brief Enumerate all stored keys into @p out.
     *
     * This optional operation supports custom migration callbacks that need to
     * scan records via MigrationContext::createIterator().  Implementations
     * that do not support key enumeration may leave @p out unchanged and return
     * false; in that case, iterators over those implementations will yield no
     * records.
     *
     * @param out  Destination vector; keys are appended (not replaced).
     * @return true if enumeration succeeded and @p out is complete.
     */
    virtual bool listKeys([[maybe_unused]] std::vector<std::string>& out)
    {
        return false;
    }
};

/**
 * @brief Iterator over records in a table, used by custom migration callbacks.
 */
class IMigrationIterator {
public:
    virtual ~IMigrationIterator() = default;

    [[nodiscard]] virtual bool        valid() const = 0;  ///< true while the iterator points at a record.
    [[nodiscard]] virtual std::string key()   const = 0;  ///< Current record key.
    [[nodiscard]] virtual std::string value() const = 0;  ///< Current record value.
    virtual void        next()        = 0;  ///< Advance to the next record.
};

// ---------------------------------------------------------------------------
// Migration context (passed to custom migration callbacks)
// ---------------------------------------------------------------------------

/**
 * @brief Context object provided to custom migration lambdas.
 *
 * Grants access to the underlying storage and metadata about the current
 * migration so that custom callbacks can read, transform, and re-write
 * records as needed.
 *
 * ### Example
 * ```cpp
 * migration.addCustomMigration([](MigrationContext& ctx) {
 *     auto it = ctx.createIterator("users");
 *     while (it->valid()) {
 *         auto val = it->value();
 *         // …transform val…
 *         ctx.storage->put(it->key(), val);
 *         it->next();
 *     }
 *     return true;
 * });
 * ```
 */
struct MigrationContext {
    std::string version;    ///< Target migration version (same as SchemaMigration version).
    IMigrationStorage* storage = nullptr;  ///< Pointer to the storage backend.

    /**
     * @brief Create a forward iterator over all records in @p table_name.
     * @param table_name Table to scan.
     * @return Iterator positioned at the first record, or an exhausted iterator for an empty table.
     */
    [[nodiscard]] virtual std::unique_ptr<IMigrationIterator> createIterator(
        const std::string& table_name) = 0;
    
    virtual ~MigrationContext() = default;
};

// ---------------------------------------------------------------------------
// SchemaMigration
// ---------------------------------------------------------------------------

/**
 * @brief Automatic schema migration framework with online DDL.
 *
 * ### Overview
 * `SchemaMigration` provides a fluent DSL for expressing zero-downtime schema
 * changes.  The implementation uses a six-phase online DDL algorithm:
 *
 * ```
 * 1. Create shadow table with new schema
 * 2. Start dual-write (write to both tables)
 * 3. Background copy old table to shadow table (backfill)
 * 4. Verify data consistency
 * 5. Atomic swap (rename shadow → main)
 * 6. Drop old table
 * ```
 *
 * ### Supported operations
 * | Method            | Description                                      |
 * |-------------------|--------------------------------------------------|
 * | addColumn()       | Add a column; existing rows are backfilled with the default value. |
 * | renameColumn()    | Rename a column; dual-write keeps both names live during cutover. |
 * | addIndex()        | Build an index; `build_online = true` runs the build in the background. |
 * | dropColumn()      | Schedule column removal after an optional grace period. |
 * | addCustomMigration() | Inject arbitrary migration logic via a callback. |
 *
 * ### Error handling
 * If any step fails and `RollbackStrategy::AUTOMATIC` is set (default), all
 * completed steps are unwound before returning the error result.  With
 * `RollbackStrategy::MANUAL` the database is left in its partially-migrated
 * state and the caller must call rollback() explicitly.
 *
 * ### Example
 * ```cpp
 * SchemaMigration migration("1.5.0");
 * migration.setRollbackStrategy(RollbackStrategy::AUTOMATIC);
 *
 * migration.addColumn("users", {
 *     .name          = "phone_number",
 *     .type          = "VARCHAR(20)",
 *     .nullable      = true,
 *     .default_value = "NULL"
 * });
 * migration.renameColumn("users", "email", "email_address");
 * migration.addIndex("users", {
 *     .name         = "idx_email",
 *     .columns      = {"email_address"},
 *     .unique       = false,
 *     .build_online = true
 * });
 * migration.dropColumn("users", "old_column",
 *     {.grace_period = std::chrono::hours(24 * 7)});
 *
 * auto result = migration.apply(storage);
 * if (!result.success) {
 *     // AUTOMATIC strategy already rolled back
 * }
 * ```
 *
 * ### Thread-safety
 * A single `SchemaMigration` instance must not be used concurrently from
 * multiple threads.  Concurrent migrations against **different** tables on
 * the same storage engine are safe provided the storage engine itself is
 * thread-safe.
 */
class SchemaMigration {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Create a migration for a specific database version.
     * @param version  Semantic version string identifying this migration
     *                 (e.g. "1.5.0").  Written to storage under the key
     *                 `__schema__:version` on successful apply(), making
     *                 the last applied version durable.
     */
    explicit SchemaMigration(const std::string& version);

    ~SchemaMigration();

    // Non-copyable, movable
    SchemaMigration(const SchemaMigration&)            = delete;
    SchemaMigration& operator=(const SchemaMigration&) = delete;
    SchemaMigration(SchemaMigration&&)                 noexcept = default;
    SchemaMigration& operator=(SchemaMigration&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Set the rollback strategy.
     *
     * Defaults to `RollbackStrategy::AUTOMATIC`.
     *
     * @param strategy  The strategy to use on failure.
     * @return Reference to this object (fluent interface).
     */
    SchemaMigration& setRollbackStrategy(RollbackStrategy strategy);

    // -----------------------------------------------------------------------
    // DDL operations (fluent builder)
    // -----------------------------------------------------------------------

    /**
     * @brief Schedule adding a column to @p table.
     *
     * On apply(), the column is added to the shadow schema and existing rows
     * are backfilled with `column.default_value`.
     *
     * @param table   Target table name.
     * @param column  Column definition.
     * @return Reference to this object (fluent interface).
     */
    SchemaMigration& addColumn(const std::string& table, const ColumnDef& column);

    /**
     * @brief Schedule renaming a column in @p table.
     *
     * During the dual-write phase both the old and new column names are
     * written.  After the atomic swap the old name is no longer accessible.
     *
     * @param table     Target table name.
     * @param old_name  Current column name.
     * @param new_name  Desired column name.
     * @return Reference to this object (fluent interface).
     */
    SchemaMigration& renameColumn(const std::string& table,
                                  const std::string& old_name,
                                  const std::string& new_name);

    /**
     * @brief Schedule adding an index to @p table.
     *
     * When `index.build_online == true` the index is built in the background
     * without blocking reads or writes on the table.
     *
     * @param table  Target table name.
     * @param index  Index definition.
     * @return Reference to this object (fluent interface).
     */
    SchemaMigration& addIndex(const std::string& table, const IndexDef& index);

    /**
     * @brief Schedule dropping a column from @p table.
     *
     * The column is hidden immediately on apply() but the underlying data is
     * not physically purged until `opts.grace_period` has elapsed, allowing
     * rollback or dependent queries to complete safely.
     *
     * @param table   Target table name.
     * @param column  Name of the column to remove.
     * @param opts    Drop options (grace period).
     * @return Reference to this object (fluent interface).
     */
    SchemaMigration& dropColumn(const std::string& table,
                                const std::string& column,
                                const DropColumnOptions& opts = {});

    /**
     * @brief Register a custom migration callback.
     *
     * The callback receives a `MigrationContext` and must return `true` on
     * success or `false` to trigger rollback.
     *
     * @param migration  Callable `bool(MigrationContext&)`.
     * @return Reference to this object (fluent interface).
     */
    SchemaMigration& addCustomMigration(
        std::function<bool(MigrationContext&)> migration);

    // -----------------------------------------------------------------------
    // Execution
    // -----------------------------------------------------------------------

    /**
     * @brief Apply the migration against @p storage.
     *
     * Executes all queued operations using the online DDL algorithm.
     * On failure with `RollbackStrategy::AUTOMATIC` the migration is
     * automatically rolled back before returning.
     *
     * @param storage  Storage backend (must outlive this call).
     * @return MigrationResult describing the outcome.
     */
    MigrationResult apply(IMigrationStorage& storage);

    /**
     * @brief Manually roll back a failed (or in-progress) migration.
     *
     * Replays the undo log from the most recent `apply()` call in reverse
     * order, restoring any keys that were modified or removed.
     *
     * **Lifetime requirement:** This method uses the `IMigrationStorage`
     * reference that was passed to the most recent `apply()` call.  The
     * storage object **must still be alive** when `rollback()` is invoked.
     * If the storage may have been destroyed by the time a manual rollback
     * is needed, keep a reference to it and call `apply()` only while it is
     * guaranteed to outlive any subsequent `rollback()` call.
     *
     * **No-op cases:** Returns success immediately if:
     *  - `apply()` has not yet been called (no undo log to replay), or
     *  - `apply()` completed successfully (migration was committed and the
     *    undo log was cleared).
     *
     * @return RollbackResult describing the rollback outcome.
     */
    RollbackResult rollback();

    /**
     * @brief Return the version string this migration targets.
     */
    const std::string& version() const noexcept;

    /**
     * @brief Return the current online DDL phase.
     */
    OnlineDDLPhase currentPhase() const noexcept;

    /**
     * @brief Return the number of DDL operations (column/index/custom) queued.
     */
    std::size_t operationCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace updates
} // namespace themis
