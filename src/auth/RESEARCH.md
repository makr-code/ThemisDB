---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

# Auth Module Research Notes

Exact model backend not verifiable from final document text alone.

This module is grounded in the concrete auth implementation under `src/auth`, the public contracts in `include/auth`, and the operational docs in `ARCHITECTURE.md`, `ROADMAP.md`, and `SECURITY.md`.

## Scope and evidence base

The auth module exists to answer one practical question: how can ThemisDB validate identity and session state without silently trusting misconfigured or hostile input?

The primary proof is in the following source files:

- `include/auth/jwt_validator.h` and `src/auth/jwt_validator.cpp` — JWT/JWKS validation, key refresh, and signer trust checks
- `include/auth/session_manager.h` and `src/auth/session_manager.cpp` — bounded session lifecycle and fail-closed invalidation rules
- `include/auth/token_blacklist.h` and `src/auth/token_blacklist.cpp` — revocation state and hot-path lookups
- `src/auth/distributed_token_blacklist.cpp` — cluster-wide distributed revocation state and leader/follower behavior
- `src/auth/federated_identity_manager.cpp` — external IdP validation, realm management, and token caching
- `src/auth/auth_rate_limiter.cpp` and `src/auth/rate_limiter_backend.cpp` — per-user and shared rate limiting policies
- `src/auth/auth_audit_logger.cpp` — security audit emission for auth decisions and revocation events
- `src/auth/jwt_key_rotation_manager.cpp` — key rotation and invalidation handling

## Research questions the implementation addresses

### 1. Can identity validation be performed without trusting unbounded caller input?

The `JWTValidator` implementation parses the token, validates its signature
using JWKS material, applies issuer/audience policy when enabled, and rejects
invalid or expired state before returning `JWTClaims`. It does not return a
`PrincipalContext`.

The implementation also makes the trust configuration explicit in constructor validation:

- empty issuer or audience settings are normalized away when not required
- the `JWTValidatorConfig` constructor defaults issuer/audience requirements to true and throws if expected values are missing
- the URL-only constructor disables those two requirements and logs a warning for the broad trust configuration

This design keeps the token path deterministic and fail-closed rather than silently permissive.

### 2. Can session state remain bounded under pressure?

`SessionManager` enforces absolute and idle expiry windows, prunes expired sessions before creating new ones, and applies a per-user session cap before accepting additional sessions. It hashes session IDs before storage and uses a constant-time comparison helper for equality checks, which reduces timing leak risk in validation paths.

This is not merely convenience logic: it is part of identity-state control. Session creation, validation, termination, and purging are all explicit transitions with bounded resource usage.

### 3. How do we make revoked tokens and replay attempts visible and enforceable?

The module separates the local blacklist and distributed revocation path:

- `TokenBlacklist` provides an in-memory revocation store
- Redis, RocksDB, and `DistributedTokenBlacklist` are separate implementations; the latter adds best-effort peer synchronization to RocksDB persistence
- `TotpReplayCache` and related auth state helpers add replay protection for MFA flows

Revocation only applies when the embedding caller configures and checks a
blacklist implementation. The source does not establish that every JWT or
session path is automatically wired to a revocation backend.

### 4. Can external provider integrations degrade gracefully without producing unsafe auth decisions?

`FederatedIdentityManager`, `LDAPConnectionPool`, and HTTP/OIDC adapters treat
provider errors as failure conditions, but failure representation is
component-specific. For example, federated validation classifies some provider
failures as `AuthException`; JWT validation uses `std::runtime_error`.

This means the module keeps the claim that provider availability is not equivalent to trust. In practice, degraded IdP endpoints either fail closed or return a specific error that can be traced back to the provider and the exact request path.

### 5. What prevents auth decisions from becoming opaque?

The module emits structured audit events via `AuthAuditLogger` and includes metrics/state surfaces in `auth_metrics.cpp` and `auth_event_bus.cpp`. This is part of the trust boundary: operator tools need to see whether a login, token validation, key rotation, or session invalidation succeeded or failed, and why.

## Constraints and assumptions

- External identity providers and storage backends can become unavailable or partially degraded.
- JWT and session trust are not the same as process-local authorization. Claims must be validated at the boundary and re-checked against the active policy context.
- A distributed environment introduces eventual consistency; the module therefore prefers explicit failure classification over hidden fallback to broad trust.
- Rate limits and session caps are part of the same trust model: without them, resource exhaustion becomes a login or token-validation problem.

## Trade-offs and failure modes

- Strong validation increases latency and may reject requests that a permissive system would carry forward.
- Distributed revocation improves recovery and failover, but requires leader election, replication state, and failure handling to remain deterministic.
- Strict key rotation and unknown-issuer handling are safer than best-effort compatibility but can temporarily block access to a valid but misconfigured realm.
- Audit and metrics add operational overhead, but they are necessary for incident response and trust verification.

## Validation path

The module is validated through the auth-focused test suites under `tests/auth`, the auth benchmarks under `benchmarks/auth`, and release-oriented doc and design artifacts in `src/auth/ROADMAP.md` and `src/auth/FUTURE_ENHANCEMENTS.md`. In practice, the implementation is intended to be hardened, observable, and fail-closed rather than permissive under uncertainty.

## Conclusion

The auth module is a trust and state control plane for ThemisDB. It does not treat identity as a single check but as a pipeline of validation, session control, policy gating, revocation, and auditability. The source code and module docs consistently favor explicit error classes, bounded state, and operator visibility over convenience-driven permissiveness.
