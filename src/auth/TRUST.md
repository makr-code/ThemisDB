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

Exact model backend not verifiable from final document text alone.

# Auth Module Trust and Risk Model

This document defines the trust boundaries and risk posture for the auth module as implemented in `src/auth` and consumed by the broader ThemisDB runtime.

## 1. Trust boundaries

The auth module sits at the point where network, identity, and application trust meet. It is responsible for deciding whether a caller or token should be accepted as a valid identity for downstream business logic.

The trust boundaries are:

- external identity providers and federation endpoints
- client-supplied tokens and session identifiers
- local auth state such as session stores and blacklist entries
- internal service callers that depend on auth results
- audit, metrics, and failure-reporting surfaces used for investigation

The module assumes these boundaries are not inherently trusted. It treats identity as a property that must be proven, not assumed.

## 2. Key trust assumptions

The implementation requires the following assumptions to hold for safe operation:

- the signing key set used by JWT validation is current and not adversarially stale
- issuer and audience checks are configured when the module is expected to enforce them
- revocation state remains available or the system remains in a fail-closed mode
- session boundaries are bounded and not allowed to grow unobservably
- provider errors are classified and are not transformed into silent success
- issuer and audience behavior depends on the constructor: the URL-only `JWTValidator` disables both checks, while `JWTValidatorConfig` defaults both requirements to true and requires expected values
- operators account for the configured JWKS cache lifetime (600 seconds by default) when responding to issuer-key rotation or revocation

Violating these assumptions creates direct trust issues for downstream authorization and business execution.

## 3. Threat model and controls

### Forged or altered tokens

Control surface:

- `jwt_validator.cpp`
- `jwks_validator.cpp`
- `jwt_key_rotation_manager.cpp`

Impact reduction:

- verify signature and configured claim semantics against the current trust set
- reject unknown or stale signing keys
- reject expired or misissued tokens before principal creation

### Token replay and revocation bypass

Control surface:

- `token_blacklist.cpp`
- `distributed_token_blacklist.cpp`
- `totp_replay_cache.cpp`

Impact reduction:

- configure and check a revocation backend where the embedding flow requires revocation enforcement
- prefer explicit denial over ambiguous fallback
- keep distributed sync and local state consistent enough for accurate enforcement

### Provider degradation and confused identity

Control surface:

- `federated_identity_manager.cpp`
- `ldap_authenticator.cpp`
- `oauth_device_flow.cpp`
- `oauth_pkce_flow.cpp`

Impact reduction:

- classify provider failure states as typed auth errors
- keep realm and issuer checks explicit
- allow fallback or retry only under controlled conditions that do not broaden trust

### Session exhaustion and abuse

Control surface:

- `session_manager.cpp`
- `auth_rate_limiter.cpp`
- `rate_limiter_backend.cpp`

Impact reduction:

- cap per-user sessions and throttle abusive patterns
- keep resource usage bounded under load
- maintain audit data so operators can see whether the rate-limit or session cap is being triggered

## 4. Security controls and operator posture

The module keeps several design controls in place:

- explicit denial over silent default acceptance
- explicit failures using the error representation of the specific adapter (which is not uniform across the module)
- audit emission for security-relevant decisions
- bounded session lifetime and rate-limiting enforcement
- provider-specific capability and realm checks before trust is extended

These controls matter because the auth path is a gate to all sensitive actions in ThemisDB.

## 5. Incident handling expectations

If a trust incident occurs, the expected workflow is:

1. classify the failing contract: token, session, realm, revocation, or rate-limiting issue
2. isolate the failing backend or provider without broadening service trust
3. preserve auth failures as explicit deny decisions while the incident is active
4. inspect audit and metrics evidence for the exact decision path
5. restore the trust state only after the underlying issue is validated and the service is under control again

## 6. Residual risk

Trust remains a risk whenever external providers or distributed components participate in identity decisions: malformed provider metadata, stale keys, inconsistent session state, or backend degradation can widen the risk window if handled permissively.

The module’s documented design is to avoid that by keeping trust decisions explicit, bounded, and observable.

## 7. Summary

The auth module’s trust model is conservative by design. It treats identity, session state, and provider metadata as sensitive runtime assets and requires explicit proof before any downstream authorization or business execution proceeds.

## 8. Operational Response

If a token, signing key, provider, or revocation store is suspected compromised or unavailable, keep affected requests denied, revoke credentials at the issuer, invalidate relevant sessions, and retain audit evidence. Restore trust only after validating issuer, audience, key-set freshness, and revocation behavior. Use the private channel in [project SECURITY.md](../../SECURITY.md), with [SOP-05](../../SOP.md) for vulnerability response and [SOP-08](../../SOP.md) for operational incidents.
