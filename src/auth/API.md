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

# Auth Module API Reference

AI provenance note: Exact model backend not verifiable from final document text alone.

This page is a compact guide to the public auth surface and the contract semantics visible from `include/auth` and the module implementation under `src/auth`.

## 1. Core identity and session APIs

### JWT validation

Source: `include/auth/jwt_validator.h`, `include/auth/jwks_validator.h`

Purpose:

- validate JWT and OIDC tokens before they are accepted as user identity
- enforce issuer, audience, and key trust rules when configured
- refresh and cache JWKS material with bounded behavior

Key behavior and constraints:

- invalid or unsigned tokens are rejected
- missing issuer/audience configuration is treated as an explicit configuration error when validation is required
- JWKS refresh is single-flight and cache-aware to avoid concurrent fetch races

### Session lifecycle

Source: `include/auth/session_manager.h`

Purpose:

- create, validate, and terminate user sessions
- apply expiry and per-user session limits
- preserve fail-closed semantics under unknown or expired state

Core invariants:

- empty session IDs fail immediately
- expired sessions are pruned before new creation under the configured limits
- session validation returns explicit status rather than silently treating unknown state as success

## 2. Revocation and replay APIs

### Token blacklist

Source: `include/auth/token_blacklist.h`, `include/auth/distributed_token_blacklist.h`

Purpose:

- record revoked tokens or JTIs
- answer revocation lookups fast and deterministically
- enable distributed sync when multi-node auth is in use

These APIs are part of the authorization boundary. The module treats revocation as a safety-critical property instead of best-effort state.

### Replay protection

Source: `include/auth/totp_replay_cache.h`, related MFA and challenge flows

Purpose:

- prevent replay or duplicates in multi-step authentication flows
- keep authentication challenge usage bounded and auditable

## 3. Provider and protocol adapter APIs

The module exposes a wide range of protocol adapters, including:

- `ldap_authenticator.h` — LDAP bind and directory validation
- `oauth_device_flow.h` and `oauth_pkce_flow.h` — OAuth device and PKCE flows
- `oidc_provider.h` — OIDC discovery and token exchange paths
- `saml_authenticator.h` — SAML assertion handling
- `mfa_authenticator.h` — MFA challenge management and verification
- `webauthn_authenticator.h`, `passkey_authenticator.h` — WebAuthn and passkey validation
- `mtls_authenticator.h` — mTLS client certificate validation
- `gssapi_authenticator.h` and `kerberos_security.h` — Kerberos/GSSAPI flows
- `api_key_authenticator.h` — static API-key authentication

The major design rule across these interfaces is the same: malformed requests, provider failures, and unsupported capability states must be surfaced as typed failures.

## 4. Trust and policy APIs

Source: `include/auth/zero_trust_auth_verifier.h`, `include/auth/principal_validator.h`, `include/auth/authorization_policy.h`

Purpose:

- verify trust state before continuing execution
- validate principal identity and decision context
- keep policy decisions explicit and observable

This layer is responsible for the eventual authz boundary and is intentionally not a generic 'best effort' adapter.

## 5. Observability and error APIs

### Error taxonomy

Source: `include/auth/auth_error.h`

This is the canonical typed error layer for auth failures. The module uses it to distinguish:

- invalid token state
- provider outage
- session invalidity
- policy mismatch
- revocation or replay failures
- asynchronous and distributed backend timeouts

### Audit and metrics

Source: `include/auth/auth_audit_logger.h`, `include/auth/auth_metrics.h`, `include/auth/auth_event_bus.h`

These APIs provide the required operational signal for auth success/failure, key rotation, session lifecycle changes, and security-relevant events.

## 6. Compatibility posture

The public API is designed to be explicit and fail-closed. That means compatibility is generally additive and predictable, not permissive. An auth change that broadens trust or suppresses a typed failure should be treated as a release-impacting change.

## 7. Usage guidance

When using this API surface, callers should keep the following in mind:

- validate identity before business logic
- check session and revocation state for all privileged operations
- classify provider or network failures as typed auth errors
- avoid hidden fallback paths that would accept unknown trust state
- keep audit and metrics integrity alongside the decision itself

The auth module is designed around explicit trust decisions, not ambient trust.
