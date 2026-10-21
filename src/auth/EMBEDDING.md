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

# Auth Module Embedding Guide

AI provenance note: Exact model backend not verifiable from final document text alone.

This document explains how to embed the auth module into higher-level ThemisDB components without violating its fail-closed semantics or revocation expectations.

## 1. Primary embedding surfaces

The auth module is designed to be consumed by runtime request-validation and policy code, not as an afterthought to business logic. The main embedding surfaces are:

- `include/auth/jwt_validator.h` and `include/auth/jwks_validator.h`
- `include/auth/session_manager.h`
- `include/auth/token_blacklist.h` and `include/auth/distributed_token_blacklist.h`
- `include/auth/auth_rate_limiter.h`
- `include/auth/federated_identity_manager.h`
- `include/auth/zero_trust_auth_verifier.h`
- `include/auth/auth_error.h`

These are the contracts that callers should treat as the production boundary.

## 2. Recommended insertion points

### HTTP/request boundary

The normal high-level pattern is:

1. authenticate or validate identity at the request boundary
2. check session or token state
3. enforce rate limiting or trust policy
4. continue only with a valid, auditable principal context

This pattern matches the server-side auth assumptions recorded in `src/auth/ARCHITECTURE.md` and the trust boundaries described in `src/auth/TRUST.md`.

### Service-level integration

For a service that needs auth context, the safer pattern is:

- validate credentials or JWTs before any database or business access
- reject unknown or degraded provider states rather than continuing in a partial trust state
- treat revocation checks as mandatory for security-sensitive operations
- record the auth result in the audit layer for later incident review

## 3. Example usage pattern

A minimal integration usually follows the following shape:

```cpp
JWTValidator validator(cfg);
SessionManager sessions;
AuthRateLimiter limiter;
TokenBlacklist revocations;

const auto auth_result = validator.validate(token);
if (!auth_result.is_valid) {
    return AuthErrorCode::INVALID_TOKEN;
}

const auto session_id = sessions.createSession(auth_result.user_id, device_fingerprint, ip, ua);
if (!sessions.validateSession(session_id, ip).valid) {
    return AuthErrorCode::SESSION_INVALID;
}

if (revocations.isRevoked(jti)) {
    return AuthErrorCode::TOKEN_REVOKED;
}
```

This pattern is intentionally conservative. A failed auth check should exit early rather than allowing partial downstream execution.

## 4. Dependency and lifecycle expectations

The auth module expects the caller to own lifecycle concerns for the external dependencies it uses:

- key sources and JWKS fetch endpoints must be configured and reachable
- timeouts and retry policies should be preserved when provider integration is used
- distributed state must be initialized with the correct node configuration if the cluster mode is enabled
- any custom policy or trust context must remain explicit and auditable

The module code assumes that identity state is not ambient. The caller must configure and validate that state before trusting it.

## 5. Edge cases that must remain explicit

The following conditions should never be silently downgraded by embedding callers:

- empty, malformed, or unknown token content
- expired or revoked session IDs
- lost connectivity to a JWKS or federation provider
- unknown realm or provider capability mismatch
- rate-limit exhaustion or a suddenly degraded backend

The source consistently reports these through typed error classes rather than generic exceptions or implicit fallbacks.

## 6. Operational safety rules

When the auth module is embedded in a higher-level component, keep these rules:

- do not permit a request to proceed after a deny result from the auth subsystem
- do not treat provider degradation as an authorization success
- do not bypass revocation or revalidation for privileged operations
- do not drop audit or metrics events for failed identity decisions
- do not widen trust by accepting unknown issuers or audiences without an explicit policy decision

## 7. Module boundaries

The auth module should remain the system boundary for:

- token validation
- key and JWKS trust
- session lifecycle and revocation
- rate limiting and auth event emission
- provider-specific protocol adapters

If a caller needs authorization decisions beyond these boundaries, the decision should pass through an explicit policy layer rather than being inferred inside business code.

## 8. Summary

Embedding the auth module means bringing in a security-sensitive control plane, not a thin convenience helper. The design is to validate aggressively, deny clearly, and expose enough state for operators to diagnose failures. This is what keeps the module usable in production without silently widening trust.
