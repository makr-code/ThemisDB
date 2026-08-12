/**
 * @file importers_api_contract.h
 * @brief Frozen importers contract: idempotency, schema evolution, error handling, and ordering.
 *
 * This header defines the normative contract for the importers module.
 * All importer implementations (FlatFile, PostgreSQL, MySQL, MongoDB, Kafka, S3,
 * schema validators, conflict resolvers) must honour these semantics within v1.x.
 *
 * ## Import Ordering Contract
 *
 * Row ordering as supplied by the source is preserved during import.  The
 * consumer observes rows in the same sequence they appear in the source file
 * or stream.  Reordering by the importer is a contract violation.
 *
 * ## Visibility Contract
 *
 * Partial import state is NOT visible to readers before the import transaction
 * commits.  Readers see either zero rows (pre-commit) or all committed rows.
 *
 * ## Idempotency Contract
 *
 * Re-importing the same source with the same `import_id` is idempotent.
 * A second import of the same data must produce no additional rows.
 * The `import_id` uniqueness is enforced at the import coordinator level.
 *
 * ## Schema Evolution Contract
 *
 * Additive column changes (new nullable columns) are supported and passed
 * through transparently.  Breaking changes (column removal, type changes,
 * renamed primary key) require an explicit migration step declared via the
 * schema migration API.  Attempting a breaking change without migration
 * surfaces IMPORT_SCHEMA_MISMATCH.
 *
 * ## Error Handling Contract
 *
 * Bad row disposition is configurable per import job:
 *   - SKIP:  the row is excluded; its index appears in the error response.
 *   - FAIL:  the entire import is aborted; no rows are committed.
 * The error response MUST include the exact count of bad rows encountered.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump.
 *
 * @see src/importers/ROADMAP.md — Phase 1 item
 * @see include/importers/importer_interface.h
 * @see include/importers/schema_validator.h
 * @see include/importers/conflict_resolver.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace importers {

// ============================================================================
// § 1  Import ID contract
// ============================================================================

/// Maximum length of an import_id string in bytes.
inline constexpr std::size_t kMaxImportIdBytes = 256;

/// Maximum number of concurrent import jobs per node.
inline constexpr std::size_t kMaxConcurrentImports = 32;

// ============================================================================
// § 2  Schema evolution contract
// ============================================================================

/**
 * @brief Classification of a schema change relative to the existing schema.
 */
enum class SchemaChangeKind : int {
    /// No change detected.
    NoChange  = 0,

    /// Additive only: new nullable columns added; no existing column affected.
    Additive  = 1,

    /// Breaking: existing columns removed, renamed, or type-changed.
    /// Requires explicit migration before import proceeds.
    Breaking  = 2,
};

/// Returns true when the schema change requires an explicit migration step.
[[nodiscard]] inline constexpr bool requiresMigration(SchemaChangeKind kind) noexcept {
    return kind == SchemaChangeKind::Breaking;
}

// ============================================================================
// § 3  Row disposition contract
// ============================================================================

/**
 * @brief How the importer handles individual bad rows.
 */
enum class BadRowDisposition : int {
    /// Skip bad row and continue; include its index in the error summary.
    Skip = 0,
    /// Abort the entire import; no rows are committed.
    Fail = 1,
};

// ============================================================================
// § 4  Import sizing constraints
// ============================================================================

/// Default maximum rows per import batch (in-memory buffer).
inline constexpr std::uint64_t kDefaultImportBatchRows = 10'000u;

/// Maximum rows per import batch (operator-configurable upper bound).
inline constexpr std::uint64_t kMaxImportBatchRows = 10'000'000u;

/// Maximum import file size (bytes) accepted without explicit quota override.
inline constexpr std::uint64_t kDefaultMaxImportFileBytes = 10ULL * 1024 * 1024 * 1024; // 10 GiB

// ============================================================================
// § 5  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the importers module.
 */
enum class ImporterErrorCode : int {
    /// No error.
    OK = 0,

    /// Source schema does not match target schema (and no migration declared).
    IMPORT_SCHEMA_MISMATCH = 1,

    /// A row failed validation (type, constraint, or format error).
    IMPORT_ROW_INVALID = 2,

    /// A row violates a primary-key or unique constraint.
    IMPORT_DUPLICATE_KEY = 3,

    /// The source file or stream could not be located or opened.
    IMPORT_FILE_NOT_FOUND = 4,

    /// The import would exceed the configured row or byte quota.
    IMPORT_QUOTA_EXCEEDED = 5,

    /// The import_id has already been used; re-import rejected.
    IMPORT_DUPLICATE_ID = 6,

    /// The import timed out before completing.
    IMPORT_TIMEOUT = 7,

    /// The source connector (DB, S3, Kafka) is unreachable.
    IMPORT_CONNECTOR_UNAVAILABLE = 8,

    /// A partial import was rolled back due to a mid-import failure.
    IMPORT_ROLLBACK = 9,

    /// Internal importer error.
    INTERNAL_ERROR = 10,
};

/// Returns true for errors that allow the import to be retried unchanged.
[[nodiscard]] inline constexpr bool isRetryableImportError(ImporterErrorCode code) noexcept {
    return code == ImporterErrorCode::IMPORT_TIMEOUT
        || code == ImporterErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
}

// ============================================================================
// § 6  Commit visibility contract
//
// A committed import is visible to all subsequent read transactions.
// Visibility is guaranteed after the import coordinator returns OK.
// The read-after-write guarantee holds within a single node; cross-node
// visibility is bounded by the cluster replication lag.
// ============================================================================

/// Maximum expected cross-node replication lag after a commit (SLO reference).
inline constexpr std::chrono::seconds kImportReplicationLagSlo{5};

} // namespace importers
} // namespace themis
