<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Auth Module Public Headers

**Module Path:** `include/auth/`
**Implementation Security:** `../../src/auth/SECURITY.md`

---

## Scope

Security considerations for the auth module's public header API surface. The auth module
is ThemisDB's primary security boundary; all public headers in this module are
security-critical.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| JWT forgery | Unsigned or weak-signature JWT | `jwks_validator.h` — RS256/ES256 required; none/HS256 rejected |
| JWT algorithm confusion | `alg: none` header attack | `jwt_validator.h` — explicit algorithm allowlist |
| JWKS cache poisoning | MITM JWKS endpoint | `jwks_security.h` — TLS cert pinning + cache TTL bounds |
| TOTP replay | Reusing a valid TOTP code | `totp_replay_cache.h` — sliding-window replay prevention |
| Credential memory disclosure | Core dump or memory scan | `secure_memory.h` — zero-on-free `SecureBuffer` for all secrets |
| Session token hijacking | Token stolen from transport | `session_manager.h` — `SessionToken` is opaque; TLS required |
| Token blacklist bypass | Blacklist unavailable during revocation | `token_blacklist.h` — dual backend (Redis + RocksDB); fail-closed |
| OAuth CSRF | State parameter missing in OAuth flows | `oauth_pkce_flow.h` — PKCE challenge required; state validated |
| SAML replay | SAML assertion replayed | `saml_authenticator.h` — assertion ID + NotOnOrAfter enforced |
| Brute-force credentials | Password/API key brute force | `auth_rate_limiter.h` — per-IP + per-tenant lockout |
| Privilege escalation | Unvalidated principal claims | `principal_validator.h` + `zero_trust_auth_verifier.h` |
| Weak passwords | Password policy not enforced | `password_policy.h` — minimum entropy and rotation enforced |

---

## Security Controls

### Secure Credential Memory
`secure_memory.h` provides `SecureBuffer` — memory that is zeroed on destruction. All
credential-bearing types (JWT secrets, LDAP passwords, TOTP seeds) must use `SecureBuffer`.

### Algorithm Allowlist
`jwt_validator.h` rejects `alg: none` and `alg: HS256`; RS256 and ES256 are required.
`jwks_validator.h` enforces this via the JWKS key type.

### Replay Prevention
`totp_replay_cache.h` maintains a sliding-window cache of used TOTP codes; duplicate use
within the validity window returns `AuthErrorCode::TOTP_REPLAY`.

### Dual-Backend Blacklist
`token_blacklist.h` provides an abstract interface implemented by both `redis_token_blacklist.h`
and `rocksdb_token_blacklist.h`; the dual backend ensures revocation is durable even if
Redis is unavailable.

### Zero-Trust Continuous Verification
`zero_trust_auth_verifier.h` exposes `verify(principal, context)` for per-request posture
checks beyond initial authentication.

---

## Known Limitations

- Passkey (FIDO2 resident key) support is planned (Q3 2026); until then, WebAuthn with
  server-side credential storage is required.
- ABAC policy interface is planned (Q3 2026); current authorisation is claim-based only.
- mTLS certificate revocation (OCSP stapling) is operator-managed; the header contract
  does not enforce OCSP.
