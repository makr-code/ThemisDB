> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Authentication Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/auth/`

---

## 1. Overview

The `auth` module implements ThemisDB's full authentication and authorization pipeline.
It supports JWT/OIDC, Kerberos/GSSAPI, TOTP MFA, OAuth 2.0 (device flow and PKCE), SAML 2.0,
mTLS, API key authentication, federated identity, zero-trust verification, and RBAC enforcement.
Every inbound request passes through this module before reaching any business logic.

---

## 2. Design Principles

- **Pluggable Authenticators** – each authentication method is an independent component
  that implements the same interface; adding a new method does not touch the others.
- **Defense in Depth** – multiple independent layers: token validation → principal
  extraction → role mapping → access decision.
- **JWKS Caching** – public keys are cached with configurable TTL to avoid per-request
  HTTPS round-trips to the IdP.
- **Replay Prevention** – TOTP codes and token nonces are tracked to prevent replay attacks.
- **Zero Trust Ready** – `zero_trust_auth_verifier.cpp` enables continuous verification
  beyond initial authentication.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `jwt_validator.cpp` | RS256 JWT validation, JWKS caching, claim extraction |
| `jwks_validator.cpp` / `jwks_security.cpp` | JWKS endpoint fetching and key security |
| `jwt_key_rotation_manager.cpp` | Automated key rotation for JWT signing |
| `gssapi_authenticator.cpp` | Kerberos/GSSAPI SSO (MIT, Heimdal, SSPI) |
| `kerberos_security.cpp` | Kerberos-specific security validations |
| `mfa_authenticator.cpp` | TOTP MFA (RFC 6238), QR provisioning, recovery codes |
| `totp_replay_cache.cpp` | One-time code replay prevention |
| `totp_secret_encryption.cpp` | Encrypted TOTP secret storage |
| `oauth_device_flow.cpp` | OAuth 2.0 Device Authorization Grant (RFC 8628) |
| `oauth_pkce_flow.cpp` | OAuth 2.0 Authorization Code + PKCE (RFC 7636) |
| `oidc_provider.cpp` | OIDC discovery and federated identity |
| `federated_identity_manager.cpp` | Multi-IdP identity federation |
| `saml_authenticator.cpp` | SAML 2.0 authentication |
| `mtls_authenticator.cpp` | Mutual TLS client certificate authentication |
| `api_key_authenticator.cpp` | API key validation and rate limiting |
| `session_manager.cpp` | Session lifecycle: creation, refresh, invalidation |
| `token_blacklist.cpp` | Revoked token tracking |
| `auth_rate_limiter.cpp` | Brute-force and replay attack rate limiting |
| `principal_validator.cpp` | Principal format and role mapping validation |
| `password_policy.cpp` | Password complexity and history enforcement |
| `zero_trust_auth_verifier.cpp` | Continuous trust re-verification |
| `auth_audit_logger.cpp` | Auth event audit logging (delegates to utils) |
| `auth_metrics.cpp` | Auth latency and success/failure metrics |
| `auth_error.cpp` | Structured auth error types |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Incoming Request                            │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                 Authentication Dispatcher                       │
│   detect method (Bearer JWT / Negotiate / TOTP / mTLS / APIKey) │
└──┬──────────┬──────────┬──────────┬──────────────┬─────────────┘
   │          │          │          │              │
┌──▼──┐  ┌───▼────┐  ┌──▼───┐  ┌──▼────────┐  ┌──▼───────┐
│ JWT │  │Kerberos│  │ MFA  │  │ OAuth 2.0 │  │  mTLS /  │
│Valid│  │GSSAPI  │  │ TOTP │  │ PKCE/Dev  │  │ API Key  │
└──┬──┘  └───┬────┘  └──┬───┘  └──┬────────┘  └──┬───────┘
   └─────────┴──────────┴──────────┴───────────────┘
                           │
           ┌───────────────▼──────────────────┐
           │      Principal Extraction         │
           │   sub, email, tenant, groups       │
           └───────────────┬──────────────────┘
                           │
           ┌───────────────▼──────────────────┐
           │      Role Mapping & RBAC          │
           │   principal → roles → permissions  │
           └───────────────┬──────────────────┘
                           │
           ┌───────────────▼──────────────────┐
           │     Zero Trust Verifier           │
           │  (continuous re-verification)      │
           └───────────────┬──────────────────┘
                           │
                    ✓ Authorized request
```

---

## 4. Data Flow

### 4.1 JWT Authentication Flow

```
Client sends: Authorization: Bearer <token>
    │
    ▼
jwt_validator.cpp: decode header → extract kid
    │
    ▼
jwks_validator.cpp: fetch/cache public key from JWKS endpoint
    │
    ▼
Verify RS256 signature with OpenSSL
    │
    ├─ invalid signature → 401
    ├─ expired (exp check with clock skew) → 401
    └─ valid → extract claims (sub, email, tenant_id, roles)
               → principal_validator.cpp
               → rbac_enforcer (in governance/server)
               → ✓ proceed
```

### 4.2 TOTP MFA Flow

```
Step 1: user authenticates with primary method (JWT/password)
    │
Step 2: server challenges for TOTP code
    │
    ▼
Client sends: X-MFA-Code: 123456
    │
    ▼
mfa_authenticator.cpp: TOTP validation (RFC 6238)
    ├─ check totp_replay_cache.cpp → reject replay
    ├─ compute HOTP(secret, time_window ±1)
    └─ match → add code to replay cache → ✓ proceed
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/api/` | Auth middleware before request routing |
| **Called by** | `src/server/` | Handler-level authorization checks |
| **Uses** | `src/security/` | Secret storage, key management |
| **Uses** | `src/utils/` | Audit logging via `auth_audit_logger.cpp` |
| **Uses** | `src/observability/` | Auth metrics and tracing |
| **Provides to** | `src/governance/` | Authenticated principal for RBAC |

---

## 6. Threading & Concurrency Model

- All authenticators are stateless and safe for concurrent invocation.
- `token_blacklist.cpp` uses a concurrent hash map with read-write lock.
- `totp_replay_cache.cpp` uses a lock-free ring buffer with TTL expiry.
- `jwks_validator.cpp` key cache is protected by a shared mutex (many readers, one writer).
- `session_manager.cpp` uses a single `std::mutex` protecting the session map.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| JWKS key caching | Public keys cached with configurable TTL (default: 5 min) |
| Token blacklist | In-memory hash set with periodic persistence to RocksDB |
| TOTP replay cache | Lock-free ring buffer (O(1) insert/lookup) |
| Session map | `std::unordered_map` under `std::mutex`; expired entries pruned on access |

---

## 8. Security Considerations

- **Clock skew tolerance**: JWT `exp` checked with ±60 s tolerance for distributed clocks.
- **PKCE**: prevents authorization code interception (RFC 7636).
- **TOTP replay prevention**: each TOTP code is single-use within its time window.
- **Brute force protection**: `auth_rate_limiter.cpp` enforces progressive delays and
  lockout after N failures.
- **Secret encryption**: TOTP secrets are encrypted at rest (`totp_secret_encryption.cpp`).
- **mTLS**: client certificates validated against the configured CA chain.
- **Zero trust**: optional continuous re-verification on sensitive operations.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `auth.jwt.jwks_ttl_s` | 300 | JWKS cache TTL in seconds |
| `auth.jwt.clock_skew_s` | 60 | Allowed clock skew for JWT `exp` |
| `auth.totp.window` | 1 | TOTP time windows checked (±N × 30 s) |
| `auth.rate_limit.max_attempts` | 5 | Login attempts before lockout |
| `auth.rate_limit.lockout_s` | 300 | Lockout duration in seconds |
| `auth.session.idle_timeout_s` | 28800 | Session idle timeout in seconds (8 hours) |
| `auth.session.absolute_timeout_s` | 2592000 | Session absolute lifetime in seconds (30 days) |
| `auth.session.max_per_user` | 10 | Maximum concurrent sessions per user |
| `auth.zero_trust.enabled` | false | Enable continuous trust verification |

---

## 10. Error Handling

| Error | HTTP Code | Description |
|---|---|---|
| Invalid/expired token | 401 | Log event; return WWW-Authenticate header |
| Insufficient permissions | 403 | Log access denial; return error |
| Rate limited | 429 | Return Retry-After header |
| MFA required | 401 | Return X-MFA-Required: true |
| SAML / OIDC provider unreachable | 503 | Fail open or closed per config |

---

## 11. Known Limitations & Future Work

- SAML 2.0 (`saml_authenticator.cpp`) is in progress; full IdP federation planned.
- OAuth 2.0 Device Flow is implemented but not yet fully tested at scale.
- Zero trust continuous verification is experimental; threshold tuning is manual.
- Hardware security key (FIDO2/WebAuthn) support is planned.

---

## 12. References

- `src/auth/README.md` — module overview and configuration guide
- `docs/AUTH_IMPLEMENTATION_SUMMARY.md` — implementation history
- `docs/KERBEROS_IMPLEMENTATION_SUMMARY.md` — Kerberos/GSSAPI guide
- `docs/security/` — security architecture overview
- `ARCHITECTURE.md` (root) — full system architecture
