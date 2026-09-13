/**
 * @file schema_version_manager.h
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
#include <chrono>
#include <nlohmann/json.hpp>
#include "metadata/schema_manager.h"
#include "metadata/schema_audit_log.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;

using json = nlohmann::json;

/**
 * @brief Immutable record of one persisted schema version change.
 */
struct SchemaChange {
    uint64_t version = 0;                                   ///< Version number (monotonically increasing)
    std::string table_name;                             ///< Affected table
    std::string change_type;                            ///< "create", "update", "delete"
    std::string author;                                 ///< Who made the change (empty = system)
    std::string description;                            ///< Human-readable description
    std::chrono::system_clock::time_point timestamp;    ///< When the change was made
    SchemaManager::TableSchema snapshot;                ///< Full schema snapshot at this version

    /**
     * @brief Serialise this schema-change record to JSON.
     * @return JSON object containing version, author, snapshot, and timestamps.
     */
    json toJSON() const;
    /**
     * @brief Parse a schema-change record from JSON.
     * @param j JSON object created by toJSON().
     * @return Parsed SchemaChange value.
     */
    static SchemaChange fromJSON(const json& j);
};

/// Error codes for schema versioning operations
enum class VersionErrorCode {
    OK = 0,
    TABLE_NOT_FOUND,
    VERSION_NOT_FOUND,
    STORAGE_ERROR,
    SERIALIZATION_ERROR,
    INVALID_VERSION,
};

/**
 * @brief Strongly typed result wrapper for schema versioning operations.
 * @tparam T Payload type returned on success.
 */
template<typename T>
struct VersionResult {
    bool ok = false;
    T value{};
    VersionErrorCode error = VersionErrorCode::OK;
    std::string error_message;

    /**
     * @brief Construct a successful result.
     * @param v Payload value received by value; callers can pass `std::move(...)`
     *          to avoid an extra copy for large payloads.
     * @return VersionResult with `ok == true`.
     */
    static VersionResult<T> success(T v) {
        VersionResult<T> r;
        r.ok    = true;
        r.value = std::move(v);
        return r;
    }

    /**
     * @brief Construct a failed result.
     * @param code Typed error code describing the failure.
     * @param msg Human-readable failure description stored in error_message.
     * @return VersionResult with `ok == false`.
     */
    static VersionResult<T> failure(VersionErrorCode code, std::string msg) {
        VersionResult<T> r;
        r.ok            = false;
        r.error         = code;
        r.error_message = std::move(msg);
        return r;
    }
};

/// SchemaVersionManager – schema versioning, change history and rollback
///
/// Tracks every schema change for every table.  Each change is stored as a
/// `SchemaChange` record in RocksDB under the key prefix
/// `config:schema_version:<table>:<version>` (zero-padded 10-digit version).
///
/// The current version for a table is kept at
/// `config:schema_version:<table>:current`.
///
/// Usage:
///   SchemaVersionManager svm(db, schema_mgr);
///
///   // Record the current schema as version 1
///   svm.createSchemaVersion("users", "admin", "initial schema");
///
///   // … schema changes are made via SchemaManager …
///
///   // Record version 2
///   svm.createSchemaVersion("users", "admin", "added email column");
///
///   // List history
///   auto history = svm.getChangeHistory("users");
///
///   // Roll back
///   svm.rollbackToVersion("users", 1, "admin");
///
/// Thread-safety: NOT thread-safe.  External synchronization required.
class SchemaVersionManager {
public:
    /// Constructor
    /// @param db         RocksDB wrapper used for persistence
    /// @param schema_mgr SchemaManager whose schema is being versioned
    SchemaVersionManager(RocksDBWrapper& db, SchemaManager& schema_mgr);

    ~SchemaVersionManager() = default;

    // Disable copy and move
    SchemaVersionManager(const SchemaVersionManager&) = delete;
    SchemaVersionManager& operator=(const SchemaVersionManager&) = delete;
    SchemaVersionManager(SchemaVersionManager&&) noexcept = delete;
    SchemaVersionManager& operator=(SchemaVersionManager&&) noexcept = delete;

    // ========================================================================
    // Public API
    // ========================================================================

    /// Snapshot the current schema for @p table_name and store it as a new version.
    /// @param table_name  Table whose schema should be versioned
    /// @param author      Identity of the change author (may be empty)
    /// @param description Human-readable description of the change
    /// @return The newly assigned version number, or an error result.
    VersionResult<uint64_t> createSchemaVersion(
        std::string_view table_name,
        std::string_view author      = "",
        std::string_view description = ""
    );

    /**
     * @brief Return the current highest schema version for a table.
     * @param table_name Table whose current version should be queried.
     * @return Latest version number, or VersionErrorCode::TABLE_NOT_FOUND.
     */
    VersionResult<uint64_t> getCurrentVersion(std::string_view table_name) const;

    /**
     * @brief Return the full schema-change history for a table.
     * @param table_name Table whose version history should be loaded.
     * @return Ordered list of schema changes in ascending version order.
     */
    VersionResult<std::vector<SchemaChange>> getChangeHistory(
        std::string_view table_name
    ) const;

    /**
     * @brief Return one schema version snapshot.
     * @param table_name Table whose version should be read.
     * @param version Version number to retrieve.
     * @return Matching SchemaChange snapshot, or an error result.
     */
    VersionResult<SchemaChange> getVersion(
        std::string_view table_name,
        uint64_t version
    ) const;

    /// Roll the live schema back to a specific version.
    /// Applies the schema snapshot stored at @p version via SchemaManager::setTableSchema.
    /// Records the rollback itself as a new version entry so history is preserved.
    /// @param table_name  Table to roll back
    /// @param version     Target version
    /// @param author      Identity of who initiated the rollback
    VersionResult<bool> rollbackToVersion(
        std::string_view table_name,
        uint64_t version,
        std::string_view author = ""
    );

    /**
     * @brief Compute a JSON diff between two schema versions.
     * @param table_name Table whose versions should be compared.
     * @param version_a Older or left-hand version to compare.
     * @param version_b Newer or right-hand version to compare.
     * @return JSON diff object with `added`, `removed`, and `modified` arrays.
     */
    VersionResult<json> diffVersions(
        std::string_view table_name,
        uint64_t version_a,
        uint64_t version_b
    ) const;

    /**
     * @brief Export all schema versions for a table as JSON.
     * @param table_name Table whose history should be exported.
     * @return JSON array of SchemaChange records.
     */
    json historyToJSON(std::string_view table_name) const;

    /**
     * @brief Generate a DDL migration script between two schema versions.
     *
     * Produces a sequence of ALTER TABLE statements that, when executed in
     * order, transform @p table_name from the schema at @p version_from to
     * the schema at @p version_to.
     *
     * Generated statement types:
     *   - ADD COLUMN   – for columns present in @p version_to but not in @p version_from
     *   - DROP COLUMN  – for columns present in @p version_from but not in @p version_to
     *   - ALTER COLUMN – for columns whose type or nullability changed
     *
     * Type mapping (ThemisDB → SQL):
     *   string  → VARCHAR, integer → INTEGER, double → DOUBLE PRECISION,
     *   boolean → BOOLEAN, vector  → VECTOR,  binary → BYTEA, * → TEXT
     *
     * @param table_name Table whose versions to compare.
     * @param version_from Source version (the "before" state).
     * @param version_to Target version (the "after" state).
     * @return VersionResult<std::string> containing the script on success.
     */
    VersionResult<std::string> generateMigrationScript(
        std::string_view table_name,
        uint64_t version_from,
        uint64_t version_to
    ) const;

    /**
     * @brief Dry-run whether a new schema can be applied to a table.
     *
     * Checks performed:
     *   - The new schema has a non-empty `name` field.
     *   - The new schema has a `columns` or `properties` array.
     *   - No column appears more than once in the new schema.
     *   - If the table already has a versioned schema the new schema is not identical.
     *
     * @param table_name Table to validate against.
     * @param new_schema Proposed new schema.
     * @return VersionResult<bool> where `ok=true` means the migration is valid.
     */
    VersionResult<bool> validateMigration(
        std::string_view table_name,
        const SchemaManager::TableSchema& new_schema
    ) const;

    /// Attach an audit log.  If set, every schema change is also recorded there.
    /// The pointer is non-owning; caller manages the lifetime.
    void setAuditLog(SchemaAuditLog* audit_log) noexcept { audit_log_ = audit_log; }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// Build the RocksDB key for a specific version entry.
    static std::string versionKey(std::string_view table_name, uint64_t version);

    /// Build the RocksDB key for the "current version" counter.
    static std::string currentVersionKey(std::string_view table_name);

    /// Read the raw "current version" counter from RocksDB (0 = none).
    uint64_t readCurrentVersion(std::string_view table_name) const;

    /// Persist a SchemaChange record to RocksDB.
    bool persistChange(const SchemaChange& change);

    /// Load a SchemaChange record from RocksDB by (table, version).
    std::optional<SchemaChange> loadChange(
        std::string_view table_name,
        uint64_t version
    ) const;

    // ========================================================================
    // Members
    // ========================================================================

    RocksDBWrapper& db_;
    SchemaManager&  schema_mgr_;
    SchemaAuditLog* audit_log_ = nullptr;  ///< Optional audit log (non-owning)
};

} // namespace themis
