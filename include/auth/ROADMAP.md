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

- [ ] Clarify distributed revocation and backend-capability expectations across blacklist and rate-limiter backend headers (Target: 2026-Q3)
- [ ] Add explicit provider-degradation guidance for network-bound authentication adapters (Target: 2026-Q3)

---

## Planned

- [ ] `auth_decision_context.h` — shared decision object for authn/authz/trust pipelines (Target: 2026-Q4)
- [ ] Mark legacy password-only flows as discouraged once passkey/WebAuthn rollout guidance is complete (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility guarantees for token/session hot paths in header docs (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public auth headers are expected to remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
