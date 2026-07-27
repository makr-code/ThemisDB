> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/auth/ROADMAP.md -->

# Auth Module — Public Header Roadmap

**Module Path:** `include/auth/`
**Canonical implementation roadmap:** [`../../src/auth/ROADMAP.md`](../../src/auth/ROADMAP.md)

---

## Overview

Tracks public auth API contract stability, header-level hardening work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/auth/ROADMAP.md`](../../src/auth/ROADMAP.md)

---

## Current Status

All 37 auth headers are present and public entry points exist for JWT/OIDC, Kerberos, OAuth, SAML, LDAP, API-key, mTLS, MFA, passkeys, session/revocation, and authorization policy enforcement.

---

## Completed ✅

- [x] `jwt_validator.h`, `jwks_validator.h`, `oidc_provider.h` — token and provider-validation contract
- [x] `gssapi_authenticator.h`, `ldap_authenticator.h`, `saml_authenticator.h` — enterprise/federated authentication headers
- [x] `mfa_authenticator.h`, `webauthn_authenticator.h`, `passkey_authenticator.h` — MFA and passwordless flows
- [x] `session_manager.h`, `token_blacklist.h`, `redis_token_blacklist.h`, `rocksdb_token_blacklist.h` — revocation and session lifecycle surfaces
- [x] `authorization_policy.h`, `zero_trust_auth_verifier.h`, `auth_rate_limiter.h` — policy, trust, and abuse-control surfaces
- [x] `auth_audit_logger.h`, `auth_metrics.h`, `auth_error.h` — audit, metrics, and error contracts

---

## In Progress

- [x] Clarify distributed revocation and backend-capability expectations across blacklist and rate-limiter backend headers (Target: 2026-Q3)
  - Done: `distributed_token_blacklist.h` failure/degradation contract section added; 3 new REVOCATION_* error codes registered
- [x] Add explicit provider-degradation guidance for network-bound authentication adapters (Target: 2026-Q3)
  - Done: `federated_identity_manager.h` provider-degradation contract section added; PROVIDER_DEGRADED / PROVIDER_CAPABILITY_MISMATCH / FEDERATION_* codes registered

---

## Planned

- [x] `auth_decision_context.h` — shared decision object deferred; covered by auth_principal_contract.h ProviderCapability and AuthFailureClass (Target: 2026-Q4 → partially addressed Q3 2026)
- [ ] Mark legacy password-only flows as discouraged once passkey/WebAuthn rollout guidance is complete (Target: 2026-Q4)
- [x] Document benchmark-backed compatibility guarantees for token/session hot paths in header docs (Target: 2026-Q4 → delivered Q3 2026)
  - Done: `session_manager.h` bounded runtime contract section added; AHP-01..08 benchmarks with GATE-AHP-01..06

---

## Breaking Change History

None in v1.x. Public auth headers are expected to remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
