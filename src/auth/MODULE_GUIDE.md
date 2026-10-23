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

# Auth Module Guide

AI provenance note: Exact model backend not verifiable from final document text alone.

This guide is the entry point for engineers working inside `src/auth`. It connects the module’s public contracts, implementation files, and operational evidence without repeating the full roadmap or architecture narrative.

## 1. Module purpose

The auth module is responsible for identity validation, session and principle state, provider-specific authentication and federation, and the operational controls required to keep trust decisions safe under load and degraded infrastructure.

The main sources of truth are:

- `include/auth/*.h` — public API and contracts
- `src/auth/*.cpp` — concrete logic and safety checks
- `src/auth/ARCHITECTURE.md` — boundaries and runtime flows
- `src/auth/ROADMAP.md` — current implementation status and roadmap
- `src/auth/SECURITY.md` — threat model and security controls
- `src/auth/RUNBOOK.md` — operator follow-up for common auth incidents

## 2. Recommended reading order

1. `README.md` — module purpose and high-level boundaries
2. `ARCHITECTURE.md` — execution planes and trust boundaries
3. `API.md` — public interface summary and contract expectations
4. `IMPLEMENTATION.md` — how the code enforces behavior in practice
5. `RESEARCH.md` — design assumptions and trade-offs
6. `ROADMAP.md` / `MODULE_GAPS.md` — current work and residual gaps
7. `RUNBOOK.md` or `OPERATIONS.md` — diagnosis and recovery flow

## 3. Ownership map

- public API: `include/auth/`
- implementation logic: `src/auth/`
- protocol adapters and state helpers: `src/auth/*.cpp`
- operational evidence: `src/auth/RUNBOOK.md`, `src/auth/SECURITY.md`, `src/auth/AUDIT.md`
- tests: `tests/auth/`
- benchmarks: `benchmarks/auth/`

## 4. High-value entry points

### Identity and key trust

- `jwt_validator.cpp`
- `jwks_validator.cpp`
- `jwt_key_rotation_manager.cpp`
- `federated_identity_manager.cpp`

### Session and revocation

- `session_manager.cpp`
- `token_blacklist.cpp`
- `distributed_token_blacklist.cpp`
- `totp_replay_cache.cpp`

### Provider-specific flows

- `ldap_authenticator.cpp`
- `oauth_device_flow.cpp`
- `oauth_pkce_flow.cpp`
- `saml_authenticator.cpp`
- `mtls_authenticator.cpp`
- `passkey_authenticator.cpp`
- `webauthn_authenticator.cpp`

### Security and monitoring

- `auth_audit_logger.cpp`
- `auth_event_bus.cpp`
- `auth_metrics.cpp`
- `auth_rate_limiter.cpp`
- `zero_trust_auth_verifier.cpp`

## 5. Change checklist

Before merging auth-related work, confirm:

- token validation remains fail-closed in misconfigured or malformed states
- session and revocation lookups are still explicit and bounded
- no hidden fallback converts provider failure into an allow decision
- audit logs and metrics still reflect the auth outcome
- tests cover the changed auth path and its edge cases

## 6. Common failure modes to check

- JWT validation with unknown or rotated keys
- session expiry, invalidation, or per-user session overflow
- revocation backend availability or distributed sync timeout
- LDAP or IdP connectivity issues
- rate-limit exhaustion during login storms
- fail-open drift in policy or trust verification paths

## 7. Summary

The auth module is a security-sensitive boundary. The safe pattern is to validate early, deny clearly, and leave a deterministic operator trail behind every auth decision.
