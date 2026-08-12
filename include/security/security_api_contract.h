/**
 * @file security_api_contract.h
 * @brief Frozen security module API contracts for the active v1.x major line.
 *
 * This header defines the normative, binding contract for the ThemisDB security
 * module.  It covers:
 *   - Transport security (TLS/mTLS handshake, certificate validation, HSTS)
 *   - Key management lifecycle (generation → storage → rotation → revocation)
 *   - Access-control policy evaluation (fail-closed default, RBAC/ABAC order)
 *   - Audit logging (immutability, write-ordering, failure handling)
 *   - Threat detection (alert→response latency)
 *   - Canonical error taxonomy (12+ codes)
 *
 * ## Contract Scope
 *
 * These contracts are binding for all v1.x implementations that participate in
 * the ThemisDB security pipeline:
 *   - Transport security checkers (TransportSecurityChecker, mTLS adapters)
 *   - Key providers (KeyProvider, PKIKeyProvider, VaultKeyProvider, HSMKeyProvider)
 *   - Access-control managers (RBACManager, ABACPolicy, ZeroTrustPolicyEnforcer)
 *   - Audit loggers (StorageAuditLogger, SecurityEvidenceCollector)
 *   - Threat/anomaly detectors (BehavioralAnomalyDetector, MalwareScanner)
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/security/ROADMAP.md — Phase 1 frozen contract items
 * @see include/security/rbac.h          — RBAC types
 * @see include/security/access_control.h — Policy evaluation types
 * @see include/security/key_provider.h  — Key management interface
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace security {

// ============================================================================
// § 1  Transport Security Constraints
//
// These limits are enforced at every TLS/mTLS inbound boundary.  Connections
// that violate them are rejected immediately with CERT_VALIDATION_FAILED or
// TRANSPORT_SECURITY_ERROR before any application data is exchanged.
// ============================================================================

/// Minimum accepted TLS version.  TLS 1.0 and 1.1 are unconditionally refused.
inline constexpr int kMinTlsVersionMajor = 1;
inline constexpr int kMinTlsVersionMinor = 2;  ///< TLS 1.2 minimum; TLS 1.3 preferred.

/// Maximum certificate chain depth accepted during mTLS peer validation.
inline constexpr int kMaxCertChainDepth = 8;

/// Maximum distinguished-name (DN) field length in bytes (RFC 5280 §4.1.2.4).
inline constexpr std::size_t kMaxCertDnFieldBytes = 256;

/// HSTS max-age in seconds committed to clients on every HTTPS response.
/// Must not decrease across minor releases (ratchet-forward only).
inline constexpr std::chrono::seconds kHstsMaxAge{31536000};  ///< 365 days.

/// TLS handshake hard timeout.  Connections that do not complete the TLS
/// handshake within this window are aborted.
inline constexpr std::chrono::seconds kTlsHandshakeTimeout{10};

// ============================================================================
// § 2  Key Management Lifecycle Contract
//
// The lifecycle of every cryptographic key follows this state machine:
//
//   GENERATING → ACTIVE → ROTATING → ACTIVE (new generation) → REVOKED
//
// Implementations MUST:
//   a) Never return a REVOKED key for encryption or signing.
//   b) Honor KEY_ROTATION_IN_PROGRESS during the overlap window (§ 2.1).
//   c) Persist the revocation record durably before acknowledging revocation.
// ============================================================================

/// Minimum key size in bits for symmetric encryption (AES) keys.
inline constexpr int kMinSymmetricKeyBits = 256;

/// Minimum RSA key size in bits accepted by key-generation calls.
inline constexpr int kMinRsaKeyBits = 2048;

/// Maximum key overlap window during rotation: the old key remains ACTIVE for
/// at most this duration after the new generation is made ACTIVE.
inline constexpr std::chrono::hours kKeyRotationOverlapWindow{24};

/// Default key rotation period.  Keys older than this SHOULD be rotated by the
/// key-management subsystem without operator intervention.
inline constexpr std::chrono::days kDefaultKeyRotationPeriod{90};

/// Maximum time a key may remain in GENERATING state before the operation is
/// considered failed (KEY_GENERATION_TIMEOUT).
inline constexpr std::chrono::seconds kKeyGenerationTimeout{30};

// ============================================================================
// § 2.1  Key Rotation Overlap Semantics
//
// During key rotation the security layer MUST:
//   1. Generate the new key and persist it as PENDING.
//   2. Atomically transition: old key → ROTATING, new key → ACTIVE.
//   3. During the overlap window, accept ciphertexts encrypted with the old key.
//   4. After kKeyRotationOverlapWindow, mark the old key REVOKED.
//
// Callers that observe KEY_ROTATION_IN_PROGRESS SHOULD retry after a short
// backoff (≤ kKeyRotationRetryBackoff).
// ============================================================================

/// Recommended retry backoff when KEY_ROTATION_IN_PROGRESS is returned.
inline constexpr std::chrono::milliseconds kKeyRotationRetryBackoff{250};

// ============================================================================
// § 3  Access-Control Policy Evaluation Contract
//
// Policy evaluation is fail-closed by default:
//   - Missing policy → POLICY_DENY (never POLICY_ALLOW).
//   - Evaluation errors → ACCESS_DENIED (never silent pass).
//   - RBAC is evaluated before ABAC; an explicit RBAC deny short-circuits ABAC.
//
// Resolution order:
//   1. Deny-list (explicit deny always wins regardless of source).
//   2. RBAC role-permission matrix.
//   3. ABAC attribute-based rules.
//   4. Default deny (§ 3.1).
// ============================================================================

/// Maximum number of RBAC roles a single principal may hold.
/// Requests that exceed this are rejected with POLICY_MISCONFIGURED.
inline constexpr std::size_t kMaxRolesPerPrincipal = 256;

/// Maximum depth of ABAC policy rule inheritance chains.
inline constexpr int kMaxAbacPolicyDepth = 16;

/// Hard timeout for policy evaluation.  Evaluations that exceed this produce
/// ACCESS_DENIED (fail-closed) rather than a timeout error to the caller.
inline constexpr std::chrono::milliseconds kPolicyEvalHardTimeout{50};

// ============================================================================
// § 3.1  Fail-Closed Default
//
// The following MUST hold at every access-control decision point:
//   - An absent, unresolvable, or conflicted policy → deny.
//   - A policy-engine internal error → deny.
//   - Principal identity unverifiable → deny.
//
// Fail-OPEN for access control is NEVER permitted in production paths.
// ============================================================================

/**
 * @brief Returns true when the result mandates a fail-closed denial.
 *
 * All policy evaluation catch blocks should call this predicate to decide
 * whether to propagate ACCESS_DENIED regardless of exception detail.
 */
[[nodiscard]] inline constexpr bool isPolicyFailClosed(bool evalSucceeded) noexcept {
    return !evalSucceeded;
}

// ============================================================================
// § 4  Audit Logging Contract
//
// All security-relevant events are written to the audit log with the following
// guarantees:
//   a) Write-ordering: events are appended with a monotonically increasing
//      sequence number; no event may be written with a sequence number lower
//      than any previously written event.
//   b) Immutability: once written, audit records are not modified or deleted
//      within the audit retention window (kAuditRetentionMin).
//   c) Failure behavior: if the audit log is full or its backing store fails,
//      the triggering security operation is DENIED rather than logged silently.
//      The failure itself is reported via the operational metrics channel.
// ============================================================================

/// Minimum audit-log retention period.  Records must not be purged before this
/// duration from their write timestamp.
inline constexpr std::chrono::hours kAuditRetentionMin{24 * 365};  ///< 1 year.

/// Maximum batch size for bulk audit writes.  Larger batches are split.
inline constexpr std::size_t kAuditMaxBatchSize = 1000;

/// Maximum time an audit write may block the caller before returning
/// AUDIT_WRITE_FAILED.  The operation is NOT retried silently.
inline constexpr std::chrono::milliseconds kAuditWriteHardTimeout{200};

// ============================================================================
// § 5  Threat Detection Latency Contract
//
// The alert→response pipeline MUST satisfy:
//   - Detection latency (signal received → alert emitted): ≤ kThreatDetectLatency.
//   - Response action (alert emitted → block/quarantine applied): ≤ kThreatResponseLatency.
//
// If the detection pipeline is overloaded, it MUST shed load via sampling
// rather than silently dropping events without acknowledgement.
// ============================================================================

/// Maximum end-to-end detection latency from signal to alert emission.
inline constexpr std::chrono::milliseconds kThreatDetectLatency{500};

/// Maximum latency from alert emission to protective response action.
inline constexpr std::chrono::seconds kThreatResponseLatency{5};

// ============================================================================
// § 6  Error Taxonomy
//
// All security module operations map their failure outcomes to these canonical
// error codes.  Implementations MUST NOT surface internal codes to callers;
// they must translate to one of the values below.
//
// Codes < 1000 are transport/identity; 1000–1999 are key management;
// 2000–2999 are access control; 3000–3999 are audit; 4000+ are threat/other.
// ============================================================================

/**
 * @brief Canonical error codes for the ThemisDB security module.
 *
 * Every security operation that fails MUST return one of these codes so that
 * callers and operators can apply uniform handling and metrics collection.
 */
enum class SecurityErrorCode : int {
    // ── Transport / Identity ─────────────────────────────────────────────────
    /// TLS certificate failed signature or chain validation.
    CERT_VALIDATION_FAILED      = 100,
    /// Peer presented a certificate from an untrusted CA.
    CERT_UNTRUSTED_CA           = 101,
    /// Certificate has expired (notAfter in the past).
    CERT_EXPIRED                = 102,
    /// Certificate has been revoked (CRL or OCSP check).
    CERT_REVOKED                = 103,
    /// TLS handshake did not complete within kTlsHandshakeTimeout.
    TLS_HANDSHAKE_TIMEOUT       = 110,
    /// Generic transport security error (catch-all for TLS layer faults).
    TRANSPORT_SECURITY_ERROR    = 119,

    // ── Key Management ────────────────────────────────────────────────────────
    /// Requested key ID does not exist in any active key store.
    KEY_NOT_FOUND               = 1000,
    /// Key rotation is currently in progress; caller should retry.
    KEY_ROTATION_IN_PROGRESS    = 1001,
    /// Key has been permanently revoked and may not be used.
    KEY_REVOKED                 = 1002,
    /// Key generation operation exceeded kKeyGenerationTimeout.
    KEY_GENERATION_TIMEOUT      = 1003,
    /// Key store backend is unavailable (fail-closed: treat as KEY_NOT_FOUND).
    KEY_STORE_UNAVAILABLE       = 1004,
    /// Requested key size or algorithm is below security policy minimums.
    KEY_POLICY_VIOLATION        = 1005,
    /// Encryption or decryption operation failed (bad key/IV/tag).
    ENCRYPTION_FAILED           = 1010,
    /// Cryptographic signature verification failed.
    SIGNATURE_VERIFICATION_FAILED = 1011,

    // ── Access Control ────────────────────────────────────────────────────────
    /// Policy evaluation returned an explicit deny.
    POLICY_DENY                 = 2000,
    /// No applicable policy found; fail-closed default deny applied.
    POLICY_NOT_FOUND            = 2001,
    /// Policy evaluation engine encountered an internal error; deny applied.
    POLICY_EVAL_ERROR           = 2002,
    /// Caller is misconfigured (e.g. too many roles, cyclic ABAC chain).
    POLICY_MISCONFIGURED        = 2003,
    /// Principal identity could not be verified; access denied.
    ACCESS_DENIED               = 2010,
    /// RBAC role required for the operation is not assigned to the principal.
    RBAC_ROLE_MISSING           = 2011,

    // ── Audit ─────────────────────────────────────────────────────────────────
    /// Audit log write failed (disk full, I/O error, or timeout).
    AUDIT_WRITE_FAILED          = 3000,
    /// Audit log backing store is unavailable; triggering operation denied.
    AUDIT_STORE_UNAVAILABLE     = 3001,
    /// Audit record integrity check failed (tamper detected).
    AUDIT_INTEGRITY_VIOLATION   = 3002,

    // ── Threat Detection ──────────────────────────────────────────────────────
    /// A threat signal has been detected; protective response is in flight.
    THREAT_DETECTED             = 4000,
    /// Threat detection pipeline is overloaded; sampling in effect.
    THREAT_DETECTION_DEGRADED   = 4001,

    // ── Generic ───────────────────────────────────────────────────────────────
    /// Operation succeeded.
    OK                          = 0,
    /// Unclassified internal security error; always fail-closed.
    INTERNAL_ERROR              = 9999,
};

// ============================================================================
// § 7  Fail-Closed Classification Helper
//
// These helpers are used by all security components to decide whether a given
// error code mandates hard denial with no retry.
// ============================================================================

/**
 * @brief Returns true when @p code mandates immediate fail-closed denial.
 *
 * Fail-closed codes:
 *   - KEY_STORE_UNAVAILABLE, AUDIT_STORE_UNAVAILABLE, POLICY_EVAL_ERROR
 *   - INTERNAL_ERROR
 *   - ACCESS_DENIED, CERT_VALIDATION_FAILED, CERT_REVOKED
 */
[[nodiscard]] inline constexpr bool isHardDeny(SecurityErrorCode code) noexcept {
    switch (code) {
        case SecurityErrorCode::KEY_STORE_UNAVAILABLE:
        case SecurityErrorCode::AUDIT_STORE_UNAVAILABLE:
        case SecurityErrorCode::POLICY_EVAL_ERROR:
        case SecurityErrorCode::POLICY_NOT_FOUND:
        case SecurityErrorCode::INTERNAL_ERROR:
        case SecurityErrorCode::ACCESS_DENIED:
        case SecurityErrorCode::CERT_VALIDATION_FAILED:
        case SecurityErrorCode::CERT_REVOKED:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Returns true when @p code is a key-management transient error
 *        that the caller MAY retry after kKeyRotationRetryBackoff.
 */
[[nodiscard]] inline constexpr bool isKeyTransient(SecurityErrorCode code) noexcept {
    return code == SecurityErrorCode::KEY_ROTATION_IN_PROGRESS
        || code == SecurityErrorCode::KEY_GENERATION_TIMEOUT;
}

// ============================================================================
// § 8  Contract Conformance Notes
//
// Implementations that register themselves as security module components MUST:
//   1. Return SecurityErrorCode values (or wrap them in Expected<T, SecurityErrorCode>).
//   2. Never silently swallow errors that should produce an audit record.
//   3. Never bypass § 3.1 fail-closed behaviour based on caller identity.
//   4. Log every CERT_REVOKED, ACCESS_DENIED, THREAT_DETECTED event to the audit log.
//   5. Keep key-management state consistent across crash-restart (WAL/atomic swap).
// ============================================================================

}  // namespace security
}  // namespace themis
