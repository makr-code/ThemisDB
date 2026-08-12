/**
 * @file config_contract.h
 * @brief Frozen config-module semantics for the active v1.x release line.
 *
 * This header defines the normative contract for config resolution,
 * schema validation, file watching, and encrypted storage that all config
 * module components must honour in the current major release line.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB configuration pipeline:
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
// These limits are enforced at every inbound boundary (file paths, schema
// documents, config values, config keys).  Inputs exceeding these limits
// are rejected immediately; the caller receives a CONFIG_PARSE_ERROR or
// CONFIG_VALIDATION_ERROR.  Oversized inputs are logged and audited.
// ============================================================================

/// Maximum accepted config file size in bytes (100 MiB).
/// Files exceeding this limit are rejected before parsing.
inline constexpr std::size_t kMaxConfigFileSizeBytes = 100 * 1024 * 1024;

/// Maximum accepted config path length in UTF-8 bytes (4 KiB).
/// Paths exceeding this limit are rejected with CONFIG_INVALID_PATH.
inline constexpr std::size_t kMaxConfigPathBytes = 4096;

/// Maximum accepted JSON Schema document size in bytes (10 MiB).
/// Schemas exceeding this limit are rejected before validation.
inline constexpr std::size_t kMaxSchemaSizeBytes = 10 * 1024 * 1024;

/// Maximum accepted config key length in UTF-8 bytes (256 bytes).
/// Keys exceeding this limit cause CONFIG_INVALID_KEY errors.
inline constexpr std::size_t kMaxConfigKeyBytes = 256;

/// Maximum accepted config value length in UTF-8 bytes (1 MiB).
/// Values exceeding this limit cause CONFIG_VALUE_OVERFLOW errors.
inline constexpr std::size_t kMaxConfigValueBytes = 1024 * 1024;

/// Maximum number of top-level keys in a config document.
/// Documents exceeding this count cause CONFIG_STRUCTURE_TOO_LARGE.
inline constexpr std::size_t kMaxConfigTopLevelKeys = 10000;

/// Maximum nesting depth for config objects and arrays.
/// Nested structures exceeding this depth cause CONFIG_NESTING_TOO_DEEP.
inline constexpr std::size_t kMaxConfigNestingDepth = 100;

// ============================================================================
// § 2  Temporal contract
//
// All time measurements use UTC epoch.  File watcher and cache-expiration
// comparisons are consistent and monotonic within a single process lifetime.
// ============================================================================

/// File watcher polling interval (configurable; default 5 seconds).
/// Actual watcher implementations must honor a bounded polling interval
/// to prevent excessive system resource consumption.
inline constexpr std::chrono::seconds kFileWatcherDefaultPollInterval{5};

/// Absolute maximum watcher polling interval before raising a diagnostic.
inline constexpr std::chrono::seconds kFileWatcherMaxPollInterval{60};

/// Minimum watcher polling interval to prevent CPU thrashing.
inline constexpr std::chrono::milliseconds kFileWatcherMinPollInterval{100};

/// Encrypted-store key rotation check interval (default 24 hours).
/// Rotation candidates are evaluated every interval; no in-flight rotations block client access.
inline constexpr std::chrono::hours kEncryptedStoreRotationInterval{24};

/// Maximum staleness of cached config state that is still considered
/// authoritative when the primary source (file/store) is temporarily unavailable.
inline constexpr std::chrono::seconds kConfigCacheMaxStaleness{120};

// ============================================================================
// § 3  Failure classification
//
// All config components must map their internal error states to one of these
// canonical failure classes.  This enables uniform operator diagnostics and
// consistent fail-closed resolution behavior.
// ============================================================================

/**
 * @brief Canonical failure classes for config operations.
 *
 * Every ConfigException thrown by a config component must carry one of these
 * classes (encoded as the high-level ConfigErrorCode category) so that callers
 * can apply uniform policy regardless of the underlying adapter.
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
 * Use this predicate in catch blocks to decide whether to use a fallback or cache:
 *
 * @code
 *   try {
 *       return resolver->resolve(path);
 *   } catch (const ConfigException& ex) {
 *       if (isFailClosedClass(classifyError(ex.error().code()))) {
 *           // Hard denial — use last-known-good or deny request
 *           throw;
 *       }
 *       // Structurally-invalid input — also deny
 *       throw;
 *   }
 * @endcode
 */
[[nodiscard]] inline constexpr bool isFailClosedClass(ConfigFailureClass fc) noexcept {
    return fc == ConfigFailureClass::StorageDegraded || fc == ConfigFailureClass::InternalError ||
           fc == ConfigFailureClass::ConfigurationError || fc == ConfigFailureClass::PermissionDenied;
}

// ============================================================================
// § 5  Schema validation contract
//
// All schema validators MUST satisfy:
//   - validate() is deterministic and idempotent for identical input.
//   - External $ref URIs are NOT resolved (prevent SSRF, restrict to document-internal #/paths).
//   - Unsupported JSON Schema keywords are silently ignored (graceful degradation).
//   - Validation failures are collected and reported together (not fail-first).
//   - Format validators (email, date, uri, ipv4, ipv6) use RFC-compliant implementations.
// ============================================================================

/// Maximum number of distinct validation errors collected before short-circuiting.
/// Validators that hit this limit must stop and report the collected errors.
inline constexpr std::size_t kMaxValidationErrorsCollected = 1000;

/// Supported JSON Schema Draft version in use (Draft 7 subset).
inline constexpr const char* kJsonSchemaDraftVersion = "draft7-subset";

// ============================================================================
// § 6  File watcher availability contract
//
// File watchers MUST satisfy all of the following:
//   - Polling interval is bounded and configurable within [kFileWatcherMinPollInterval, kFileWatcherMaxPollInterval].
//   - File modifications are signaled within kFileWatcherDefaultPollInterval + latency (typically < 10 seconds).
//   - If a watched file is deleted, the watcher signals a change event; callers MAY treat deletion as an error.
//   - Watcher does NOT block main application threads for file I/O; all I/O is non-blocking or threaded.
//   - Concurrent modifications during a watch interval are eventually consistent; no intermediate state
//     is guaranteed to be observed (races are resolved in favor of the final state).
// ============================================================================

/// File watcher timeout for internal operations (e.g., stat, open, read).
/// If an operation exceeds this timeout, it is logged and retried next cycle.
inline constexpr std::chrono::milliseconds kFileWatcherOperationTimeout{5000};

// ============================================================================
// § 7  Encrypted-store consistency contract
//
// Encrypted stores MUST obey all of the following:
//   - All stored values are encrypted with AES-256-GCM (authenticated encryption).
//   - Decryption failures return a CONFIG_DECRYPTION_ERROR (never silent failure).
//   - Key rotation is non-blocking to client reads/writes (old + new keys accepted during transition).
//   - Metadata (IV, nonce, ciphertext length) is validated before decryption attempt.
//   - Store does not support partial reads; entire value is decrypted or operation fails.
// ============================================================================

/// Encryption algorithm in use: AES-256 in GCM mode (256-bit key).
inline constexpr const char* kEncryptionAlgorithm = "AES-256-GCM";

/// GCM authentication tag length in bytes (must be 16 for full security).
inline constexpr std::size_t kGcmAuthTagBytes = 16;

/// GCM IV (Initialization Vector) length in bytes (typically 12 bytes, 96 bits).
inline constexpr std::size_t kGcmIvBytes = 12;

/// Maximum number of old encryption keys retained during rotation.
/// Older keys are securely zeroed after this many generations.
inline constexpr std::size_t kMaxEncryptionKeyRotationHistorySize = 3;

// ============================================================================
// § 8  Audit and observability contract
//
// All audit and metrics paths MUST satisfy:
//   - Audit logs NEVER block config resolution or validation on write failures.
//   - Metrics collection is always-on but bounded in cardinality (no unbounded label explosion).
//   - Failed config operations are logged with sufficient context for debugging
//     (path, schema context, error details) but WITHOUT sensitive values or secrets.
// ============================================================================

/// Maximum number of distinct audit event types tracked in metrics.
/// After this limit, overflow events are batched under a generic "audit_event_overflow" counter.
inline constexpr std::size_t kMaxAuditEventTypeCardinality = 100;

/// Maximum metrics label cardinality before aggregation to prevent memory exhaustion.
inline constexpr std::size_t kMaxMetricsLabelCardinality = 10000;

} // namespace config
} // namespace themis
