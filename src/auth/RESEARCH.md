---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-21
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Auth Module Research Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

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

The `JWTValidator` implementation does not trust a token on the basis of its presence alone. It decodes the token, validates issuer and audience when configured, applies JWKS selection and rotation rules, checks the signing key against the current trust set, and rejects invalid or expired state before returning a principal context.

The implementation also makes the trust configuration explicit in constructor validation:

- empty issuer or audience settings are normalized away when not required
- explicit validation requests throw if the required configuration is missing
- a warning is logged when the validator intentionally accepts a broad trust domain

This design keeps the token path deterministic and fail-closed rather than silently permissive.

### 2. Can session state remain bounded under pressure?

`SessionManager` enforces absolute and idle expiry windows, prunes expired sessions before creating new ones, and applies a per-user session cap before accepting additional sessions. It hashes session IDs before storage and uses a constant-time comparison helper for equality checks, which reduces timing leak risk in validation paths.

This is not merely convenience logic: it is part of identity-state control. Session creation, validation, termination, and purging are all explicit transitions with bounded resource usage.

### 3. How do we make revoked tokens and replay attempts visible and enforceable?

The module separates the local blacklist and distributed revocation path:

- `TokenBlacklist` provides a local, in-process or persistent revocation store
- `DistributedTokenBlacklist` extends that with leader/follower replication with RocksDB-backed persistence
- `TotpReplayCache` and related auth state helpers add replay protection for MFA flows

The implementation treats revocation state as an availability-sensitive but safety-critical input. A stale or missing blacklist does not silently grant access; the auth code is designed to deny or escalate rather than accept the unknown state.

### 4. Can external provider integrations degrade gracefully without producing unsafe auth decisions?

`FederatedIdentityManager`, `LDAPConnectionPool`, and the async HTTP/OIDC adapters all treat provider error modes as explicit failure domains. The source distinguishes between unknown realms, provider degradation, capability mismatch, and network-level issues. That classification matters: provider outages or malformed responses are mapped to typed auth errors rather than being melted into generic success/failure values.

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
