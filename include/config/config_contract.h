/**
 * @file config_contract.h
 * @brief Intended config-module semantics for the active v1.x release line.
 *
 * This header records intended config-module policy. Constants and policy
 * statements are not proof of enforcement; implementation and test coverage
 * are tracked in src/config/MODULE_GAPS.md.
 *
 * ## Contract Scope
 *
 * The policy areas below apply to implementations in the config pipeline:
 *   - Path resolvers (ConfigPathResolver with fallback mapping)
 *   - Schema validators (ConfigSchemaValidator with JSON Schema Draft 7 subset)
 *   - File watchers (ConfigFileWatcher with bounded signaling)
 *   - Encrypted stores (ConfigEncryptedStore with AES-256-GCM)
 *   - Audit loggers (ConfigAuditLog with observability contracts)
 *   - Metrics exporters (ConfigMetricsExporter with bounded registration)
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/config/ROADMAP.md — Phase 1 item
 * @see include/config/README.md — Phase 6 documentation item
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace themis {
namespace config {

// ============================================================================
// § 1  Configuration size constraints
//
// These are intended policy limits. Current code does not enforce every limit;
// see src/config/MODULE_GAPS.md before relying on any bound not covered there.
// ============================================================================

/// Intended maximum config-file size in bytes (100 MiB); enforcement is not
/// currently implemented by ConfigSchemaValidator.
inline constexpr std::size_t kMaxConfigFileSizeBytes = 100 * 1024 * 1024;

/// Intended maximum config path length in UTF-8 bytes (4 KiB); the resolver
/// validates path contents but does not currently enforce this length.
inline constexpr std::size_t kMaxConfigPathBytes = 4096;

/// Intended maximum JSON Schema document size in bytes (10 MiB); enforcement
/// is not currently implemented by ConfigSchemaValidator.
inline constexpr std::size_t kMaxSchemaSizeBytes = 10 * 1024 * 1024;

/// Intended maximum config key length in UTF-8 bytes (256 bytes); enforcement
/// is not currently implemented.
inline constexpr std::size_t kMaxConfigKeyBytes = 256;

/// Intended maximum config value length in UTF-8 bytes (1 MiB); enforcement
/// is not currently implemented independently of schema constraints.
inline constexpr std::size_t kMaxConfigValueBytes = 1024 * 1024;

/// Intended maximum number of top-level keys; enforcement is not currently
/// implemented by ConfigSchemaValidator.
inline constexpr std::size_t kMaxConfigTopLevelKeys = 10000;

/// Intended maximum nesting depth; enforcement is not currently implemented
/// by ConfigSchemaValidator.
inline constexpr std::size_t kMaxConfigNestingDepth = 100;

// ============================================================================
// § 2  Temporal contract
//
// All time measurements use UTC epoch.  File watcher and cache-expiration
// comparisons are consistent and monotonic within a single process lifetime.
// ============================================================================

/// Reserved policy interval. The current watcher uses platform event APIs,
/// not configurable polling.
inline constexpr std::chrono::seconds kFileWatcherDefaultPollInterval{5};

/// Reserved policy maximum; the current watcher does not poll at this interval.
inline constexpr std::chrono::seconds kFileWatcherMaxPollInterval{60};

/// Reserved policy minimum; the current watcher does not poll at this interval.
inline constexpr std::chrono::milliseconds kFileWatcherMinPollInterval{100};

/// Reserved policy interval for rotation scheduling. ConfigEncryptedStore does
/// not schedule rotations; callers invoke rotateKey() explicitly.
inline constexpr std::chrono::hours kEncryptedStoreRotationInterval{24};

/// Reserved policy value; ConfigPathResolver does not implement last-known-good
/// config caching or use this staleness bound.
inline constexpr std::chrono::seconds kConfigCacheMaxStaleness{120};

// ============================================================================
// § 3  Failure classification
//
// These values are policy categories. Current ConfigException types do not
// carry this enum, and no module-wide classifier maps exceptions to it.
// ============================================================================

/**
 * @brief Canonical failure classes for config operations.
 *
 * This enum is not currently attached to ConfigException or ConfigErrorCode.
 * Callers must classify concrete exception types themselves.
 */
enum class ConfigFailureClass : int {
    /// Input is structurally malformed (bad format, size violation, encoding).
    MalformedInput = 1,

    /// File/store is structurally valid but config/schema is semantically invalid.
    /// This includes validation failures, schema mismatches, unsupported constraints.
    InvalidConfig = 2,

    /// The requested config path or key does not exist.
    /// May be treated as either an error or a structured absence depending on context.
    NotFound = 3,

    /// File/store backend is unreachable or returned an unexpected error.
    /// Treated as fail-closed: callers MUST use last-known-good config or deny access.
    StorageDegraded = 4,

    /// The config module received an internally inconsistent state that prevents
    /// a deterministic resolution or validation outcome.
    /// Example: circular schema references, conflicting watcher signals.
    ConfigurationError = 5,

    /// Unclassified internal error; always fail-closed.
    InternalError = 6,

    /// Caller lacks sufficient permissions to access the requested config.
    /// This covers both filesystem permissions and encrypted-store authentication.
    PermissionDenied = 7,
};

// ============================================================================
// § 4  Resolver fail-closed contract
//
// All config resolution paths MUST default to denial (fail-closed) when:
//   a) The failure class is StorageDegraded or InternalError.
//   b) The requested path cannot be resolved through any mapped fallback.
//   c) The resolved path does not exist or cannot be opened.
//   d) The underlying file/store is unreadable due to permission errors.
//
// Callers MUST NOT treat a resolution failure as a positive outcome.
// Callers MAY use a last-known-good cached config only if the cache
// is within kConfigCacheMaxStaleness and the cache is explicitly trusted.
// ============================================================================

/**
 * @brief Returns true when the given failure class mandates fail-closed denial.
 *
 * This predicate only classifies the supplied value. The module does not yet
 * map its thrown exceptions to ConfigFailureClass or implement the cache policy.
 *
 * @code
 *   const bool deny = isFailClosedClass(ConfigFailureClass::StorageDegraded);
 * @endcode
 */
[[nodiscard]] inline constexpr bool isFailClosedClass(ConfigFailureClass fc) noexcept {
    return fc == ConfigFailureClass::StorageDegraded || fc == ConfigFailureClass::InternalError ||
           fc == ConfigFailureClass::ConfigurationError || fc == ConfigFailureClass::PermissionDenied;
}

// ============================================================================
// § 5  Schema validation contract
//
// The validator supports a bounded keyword subset and rejects external $ref.
// It collects diagnostics, but has no kMaxValidationErrorsCollected cap.
// Format checks are regex-based; RFC conformance is not established.
// ============================================================================

/// Intended validation-error cap. The current validator does not enforce it.
inline constexpr std::size_t kMaxValidationErrorsCollected = 1000;

/// Supported JSON Schema Draft version in use (Draft 7 subset).
inline constexpr const char* kJsonSchemaDraftVersion = "draft7-subset";

// ============================================================================
// § 6  File watcher availability contract
//
// The current watcher uses platform event APIs, not configurable polling.
// Interval constants do not establish or bound event latency.
// ============================================================================

/// Reserved policy timeout; current watcher code does not enforce it.
inline constexpr std::chrono::milliseconds kFileWatcherOperationTimeout{5000};

// ============================================================================
// § 7  Encrypted-store consistency contract
//
// ConfigEncryptedStore uses AES-256-GCM and validates IV/tag lengths. Rotation
// re-encrypts under an exclusive lock and blocks concurrent operations.
// get()/tryGet() propagate decryption errors; tryGet() returns nullopt only
// when absent. Snapshot/entry size bounds are not otherwise enforced.
// ============================================================================

/// Encryption algorithm in use: AES-256 in GCM mode (256-bit key).
inline constexpr const char* kEncryptionAlgorithm = "AES-256-GCM";

/// GCM authentication tag length in bytes (must be 16 for full security).
inline constexpr std::size_t kGcmAuthTagBytes = 16;

/// GCM IV (Initialization Vector) length in bytes (typically 12 bytes, 96 bits).
inline constexpr std::size_t kGcmIvBytes = 12;

/// Policy ceiling for old key generations. Current rotation keeps no old
/// generations and overwrites the active key buffer before replacing it.
inline constexpr std::size_t kMaxEncryptionKeyRotationHistorySize = 3;

// ============================================================================
// § 8  Audit and observability contract
//
// Audit and metrics implementations are in-memory/locally exported surfaces.
// The constants below are policy ceilings; no event-cardinality aggregation
// based on these limits is implemented in the current code.
// ============================================================================

/// Intended maximum number of distinct audit event types tracked in metrics;
/// no overflow aggregation currently uses this value.
inline constexpr std::size_t kMaxAuditEventTypeCardinality = 100;

/// Intended maximum metrics label cardinality; no aggregation currently uses it.
inline constexpr std::size_t kMaxMetricsLabelCardinality = 10000;

} // namespace config
} // namespace themis
