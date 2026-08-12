/**
 * @file auth_principal_contract.h
 * @brief Frozen authentication and principal-contract semantics for the active v1.x line.
 *
 * @note **Header-Only Contract**: This file defines frozen semantics and invariants.
 *       No .cpp implementation needed. Consumers link to implementations of the contracts
 *       (e.g., JwtValidator, SessionManager, FederatedIdentityManager, etc.).
 *
 * This header defines the normative contract for authentication principals,
 * provider failure classification, and policy-gate behaviour that all auth
 * module components must honour in the current major release line.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB authentication pipeline:
 *   - Token validators (JWT, JWKS, OIDC, SAML, …)
 *   - Revocation backends (TokenBlacklist, RocksDBTokenBlacklist, DistributedTokenBlacklist)
 *   - Federation managers (FederatedIdentityManager)
 *   - Session managers (SessionManager)
 *   - Zero-trust verifiers (ZeroTrustAuthVerifier)
 *   - Async provider adapters (AsyncHTTPAuth, LDAPAuthenticator::authenticateAsync)
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/auth/ROADMAP.md — Phase 1 item
 * @see include/auth/ROADMAP.md — Phase 6 documentation item
 */

#pragma once

#include <string>
#include <chrono>

namespace themis {
namespace auth {

// ============================================================================
// § 1  Principal size constraints
//
// These limits are enforced at every inbound boundary (HTTP headers, TCP
// streams, gRPC metadata, internal API calls).  Inputs exceeding these limits
// are rejected immediately; the caller receives an AUTH_TOKEN_INVALID error.
// ============================================================================

/// Maximum accepted JWT token size in bytes.  Tokens exceeding this limit are
/// rejected before any cryptographic work, preventing memory-exhaustion attacks.
/// Kept in sync with MAX_JWT_TOKEN_SIZE in include/auth/jwt_validator.h (16 KiB).
inline constexpr std::size_t kMaxJwtTokenBytes = 16 * 1024;

/// Maximum subject / principal identifier length in UTF-8 bytes.
inline constexpr std::size_t kMaxPrincipalBytes = 512;

/// Maximum JTI (JWT ID) length in bytes accepted by all revocation backends.
/// Kept in sync with kMaxJtiLen in distributed_token_blacklist.cpp.
inline constexpr std::size_t kMaxJtiBytes = 1024;

/// Maximum session identifier length in bytes.
inline constexpr std::size_t kMaxSessionIdBytes = 256;

// ============================================================================
// § 2  Temporal contract
//
// All time comparisons use UTC epoch seconds.  Clock-skew tolerance is applied
// symmetrically at the validator boundary; downstream consumers receive
// already-validated claims.
// ============================================================================

/// Allowed clock-skew tolerance for JWT nbf/exp comparisons (both directions).
inline constexpr std::chrono::seconds kClockSkewTolerance{30};

/// Maximum accepted absolute session lifetime (30 days).
inline constexpr std::chrono::hours kMaxSessionLifetime{24 * 30};

/// Maximum idle timeout before a session is invalidated (8 hours).
inline constexpr std::chrono::hours kMaxSessionIdleTimeout{8};

// ============================================================================
// § 3  Failure classification
//
// All auth components must map their internal error states to one of these
// canonical failure classes.  This enables uniform operator diagnostics and
// consistent fail-closed/fail-open policy enforcement.
// ============================================================================

/**
 * @brief Canonical failure classes for auth provider errors.
 *
 * Every AuthException thrown by an auth component must carry one of these
 * classes (encoded as the high-level AuthErrorCode category) so that callers
 * can apply uniform policy regardless of the underlying adapter.
 */
enum class AuthFailureClass : int {
    /// Input is structurally malformed (bad format, size violation, encoding).
    MalformedArtifact   = 1,

    /// Credential is structurally valid but cryptographically invalid
    /// (bad signature, wrong key, untrusted issuer, revoked JTI, …).
    InvalidCredential   = 2,

    /// Credential has expired (exp/nbf check, session absolute timeout, …).
    ExpiredCredential   = 3,

    /// Caller lacks sufficient permissions for the requested resource.
    InsufficientPrivilege = 4,

    /// Provider backend is unreachable or returned an unexpected error.
    /// Treated as fail-closed: access DENIED unless a cached positive is valid.
    ProviderDegraded    = 5,

    /// The auth module received an internally inconsistent configuration or
    /// state that prevents a deterministic decision.
    ConfigurationError  = 6,

    /// Unclassified internal error; always fail-closed.
    InternalError       = 7,
};

// ============================================================================
// § 4  Fail-closed contract
//
// All auth decision paths MUST default to denial (fail-closed) when:
//   a) The failure class is ProviderDegraded or InternalError.
//   b) A mandatory claim is absent or its value cannot be verified.
//   c) The revocation backend is unreachable and no local cache is current.
//
// Fail-OPEN behaviour is ONLY allowed for explicitly whitelisted paths with
// documented justification and operator opt-in configuration.
// ============================================================================

/**
 * @brief Returns true when the given failure class mandates fail-closed denial.
 *
 * Use this predicate in catch blocks to decide whether to propagate the denial
 * or attempt a fallback:
 *
 * @code
 *   try {
 *       return provider->validateToken(token);
 *   } catch (const AuthException& ex) {
 *       if (isFailClosedClass(classifyError(ex.error().code()))) {
 *           throw;   // hard denial — no fallback
 *       }
 *       // structurally-invalid input — also deny but with a distinct error code
 *       throw;
 *   }
 * @endcode
 */
[[nodiscard]] inline constexpr bool isFailClosedClass(AuthFailureClass fc) noexcept {
    return fc == AuthFailureClass::ProviderDegraded
        || fc == AuthFailureClass::InternalError
        || fc == AuthFailureClass::ConfigurationError;
}

// ============================================================================
// § 5  Revocation backend availability contract
//
// Token revocation checks MUST satisfy all of the following:
//   - isRevoked() is always O(1) or amortised O(1) per implementation.
//   - isRevoked() never blocks caller threads for network I/O.
//   - If the backend becomes unavailable, isRevoked() returns true (deny) for
//     any JTI that CANNOT be positively confirmed as non-revoked from a
//     sufficiently fresh local cache.
// ============================================================================

/// Maximum staleness of a locally-cached revocation state that is still
/// considered authoritative when the primary backend is unreachable.
inline constexpr std::chrono::seconds kRevocationCacheMaxStaleness{120};

// ============================================================================
// § 6  Provider capability contract
//
// Federation and OIDC adapters MUST declare their network-capability
// requirement before performing any outbound call.  If the capability is not
// confirmed, the adapter must throw with AuthFailureClass::ProviderDegraded
// rather than blocking or timing out silently.
// ============================================================================

/**
 * @brief Capability flags that a provider adapter may require at runtime.
 *
 * Flags are OR-combinable.  Combine with bitwise-OR to declare a composite
 * capability requirement.
 */
enum class ProviderCapability : unsigned int {
    None          = 0u,
    /// Provider requires outbound HTTPS/TLS connectivity.
    NetworkTLS    = 1u << 0,
    /// Provider requires a valid, non-expired JWKS cache entry.
    JwksCache     = 1u << 1,
    /// Provider requires an active LDAP connection from the pool.
    LdapPool      = 1u << 2,
    /// Provider requires a reachable revocation backend (Redis / RocksDB cluster).
    RevocationBackend = 1u << 3,
};

[[nodiscard]] inline constexpr ProviderCapability operator|(
        ProviderCapability a, ProviderCapability b) noexcept {
    return static_cast<ProviderCapability>(
        static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}
[[nodiscard]] inline constexpr bool hasCapability(
        ProviderCapability set, ProviderCapability flag) noexcept {
    return (static_cast<unsigned int>(set) & static_cast<unsigned int>(flag)) != 0u;
}

// ============================================================================
// § 7  Async provider consistency contract
//
// Async provider adapters (AsyncHTTPAuth, LDAPAuthenticator::authenticateAsync)
// MUST obey all of the following:
//   - Exceptions are propagated through std::future; they are NEVER swallowed.
//   - A timeout (configurable, bounded) fires before the future is abandoned.
//   - After timeout: the future holds a ProviderDegraded exception; the caller
//     MUST treat this as fail-closed.
//   - Thread pool exhaustion is signalled via std::future with InternalError;
//     the caller MUST NOT interpret exhaustion as a positive grant.
// ============================================================================

/// Default per-request timeout for async provider calls.
inline constexpr std::chrono::milliseconds kAsyncProviderDefaultTimeout{30'000};

/// Hard maximum allowed async provider timeout (operator-configurable upper bound).
inline constexpr std::chrono::milliseconds kAsyncProviderMaxTimeout{120'000};

} // namespace auth
} // namespace themis
