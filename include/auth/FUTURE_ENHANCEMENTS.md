> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/auth/FUTURE_ENHANCEMENTS.md -->

# Auth Module — Public Header Future Enhancements

**Module Path:** `include/auth/`
**Canonical implementation enhancements:** [`../../src/auth/FUTURE_ENHANCEMENTS.md`](../../src/auth/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/auth/`. Runtime hardening, provider integration, and benchmark work remain tracked in:

→ [`../../src/auth/FUTURE_ENHANCEMENTS.md`](../../src/auth/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Token and credential-validation APIs must remain fail-closed.
- `[x]` Session/revocation headers must keep backend-agnostic interfaces for Redis/RocksDB swapability.
- `[x]` Authorization and zero-trust decisions must stay auditable through explicit result/error types.
- `[x]` Public headers must avoid leaking provider-specific transport internals.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `JWTValidator::parseAndValidate()` | `jwt_validator.h` | HTTP/gRPC auth middleware | ✅ Stable |
| `SessionManager` lifecycle APIs | `session_manager.h` | Session endpoints and middleware | ✅ Stable |
| `TokenBlacklist` backend contract | `token_blacklist.h` | Revocation services | ✅ Stable |
| `AuthorizationPolicy::evaluate()` | `authorization_policy.h` | Server policy gates | ✅ Stable |
| `ZeroTrustAuthVerifier` checks | `zero_trust_auth_verifier.h` | Per-request trust enforcement | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Add explicit backend-capability annotations for revocation and rate-limit storage contracts.
- Document degraded-provider/federation behavior uniformly across OAuth, LDAP, SAML, and Kerberos headers.
- Align passkey/WebAuthn and MFA header docs around shared user-verification terminology.

### Medium-Term (Q4 2026)

- Introduce `auth_decision_context.h` to standardize allow/deny evidence across auth middleware.
- Add deprecation guidance for weaker fallback flows once passkey-first rollout is complete.
- Expose benchmark-reference notes for token/session hot paths alongside public interfaces.

### Long-Term

- Unify protocol-specific authenticator result types behind a shared principal-context envelope.
- Add extension hooks for embedders to inject external risk-scoring / trust-evaluation providers.
