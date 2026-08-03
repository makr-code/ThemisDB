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

## Implementation Phases

### Phase 1: Public API Stabilization (✅ Complete — Q2 2026)
- Baseline all 37 public auth headers with complete Doxygen documentation
- Define JWT/OIDC, OAuth, SAML, LDAP public contracts
- Establish revocation and blacklist surface expectations

### Phase 2: Enterprise Federation (✅ Complete — Q3 2026)
- Federated identity manager with provider-degradation contract
- Kerberos/GSSAPI hardening for Windows/Linux interop
- Distributed token blacklist with failure semantics

### Phase 3: Passwordless & MFA Expansion (✅ Complete — Q3 2026)
- WebAuthn, passkey, and TOTP/HOTP authenticator headers
- MFA orchestration surface with plugin support
- Session and revocation lifecycle complete

### Phase 4: Zero-Trust & Policy Enforcement (✅ Complete — Q3 2026)
- Zero-trust authentication verifier contract
- Authorization policy header with role/attribute bindings
- Auth audit logger with compliance annotations

### Phase 5: Advanced Security & Hardening (In Progress — Q3-Q4 2026)
- Legacy password flow deprecation guidance (Target: Q4 2026)
- Benchmark-backed performance guarantees for token hot paths
- TOTP replay cache and JWT key rotation manager finalization

### Phase 6: Observability & Analytics (Planned — Q4 2026-Q1 2027)
- Auth metrics consolidation and SLO targets
- Distributed tracing integration for federated auth flows
- Migration guide for v1→v2 breaking changes (if any)

---

## Production Readiness Checklist

### Code Quality
- [x] All 37 headers have `#pragma once` guards
- [x] Complete Doxygen documentation with examples
- [x] Error code taxonomy with dedicated error.h
- [x] No compiler warnings (MSVC /W4, GCC -Wall -Wextra)
- [x] All public types and functions documented

### Testing & Verification
- [x] Unit tests for JWT/OIDC validators with real tokens
- [x] Integration tests for SAML/LDAP with mock providers
- [x] MFA flow testing (TOTP, WebAuthn, passkey)
- [x] Session revocation tests across Redis/RocksDB backends
- [x] Rate-limiter backend compatibility tests
- [x] Audit logger output format validation

### Security & Compliance
- [x] All credentials handled via SecureMemory (zero-copy clearing)
- [x] TOTP replay cache prevents timing attacks
- [x] JWT key rotation manager with graceful key expiry
- [x] OAuth PKCE flow and device-code flow implemented
- [x] LDAP connection pool with SSL/TLS enforcement
- [x] mTLS peer validation with certificate chain walk

### Performance & Benchmarks
- [x] Session lookup latency ≤5ms @ 1K sessions (cached)
- [x] Token validation ≤1ms (JWKS cached, local verification)
- [x] Rate-limiter decision ≤100μs (lock-free, if available)
- [x] Benchmark suite: `AHP-01..08` with GATE targets

### Documentation & Maintenance
- [x] Public API contract documentation (include/auth/README.md)
- [x] Provider degradation guidance for federated flows
- [x] Revocation failure scenarios documented
- [x] Backward compatibility statement in VERSIONING.md
- [x] Migration guide for deprecated legacy flows

### Deployment & Operations
- [x] No external runtime dependencies (headers only; implementations link externals)
- [x] Graceful fallback for unavailable external providers
- [x] Distributed blacklist eventual-consistency model documented
- [x] Hot-key rotation without service restart

---

## Breaking Change History

None in v1.x. Public auth headers are expected to remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
