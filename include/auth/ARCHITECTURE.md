> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/auth/ARCHITECTURE.md -->

# Auth Module — Public Header Architecture

**Module Path:** `include/auth/`
**Implementation:** `../../src/auth/`
**Canonical architecture doc:** [`../../src/auth/ARCHITECTURE.md`](../../src/auth/ARCHITECTURE.md)

---

## 1. Overview

`include/auth/` defines the **public identity, credential-verification, and authorization-policy contract** for ThemisDB. The 37 headers cover token validation, federated and enterprise authentication, MFA/passwordless flows, session and revocation handling, auth-rate protection, and auditable authorization decisions.

For runtime composition details — provider adapters, session/revocation internals, and trust/policy enforcement — see:
→ [`../../src/auth/ARCHITECTURE.md`](../../src/auth/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Token, Key, and Identity Validation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `jwt_validator.h` | `JWTValidator` | JWT/OIDC token parsing and validation |
| `jwks_validator.h` / `jwks_security.h` | `JWKSValidator`, `JWKSSecureFetcher` | JWKS discovery, fetch hardening, and key validation |
| `jwt_key_rotation_manager.h` | `JWTKeyRotationManager` | Signing-key rotation orchestration |
| `oidc_provider.h` | `OIDCProvider` | OIDC provider discovery and metadata |
| `principal_validator.h` | `PrincipalValidator` | Principal normalization and role mapping |

### 2.2 Federated and Enterprise Authentication

| Header | Public Type | Purpose |
|--------|------------|---------|
| `gssapi_authenticator.h` / `kerberos_security.h` | `GSSAPIAuthenticator`, `KerberosSecurityValidator` | Kerberos/GSSAPI authentication and ticket security |
| `ldap_authenticator.h` / `ldap_connection_pool.h` | `LDAPAuthenticator`, `LDAPConnectionPool` | LDAP/AD authentication and pooled connections |
| `saml_authenticator.h` | `SAMLAuthenticator` | SAML 2.0 authentication |
| `oauth_device_flow.h` / `oauth_pkce_flow.h` | `OAuthDeviceFlow`, `OAuthPKCEFlow` | OAuth 2.0 device and PKCE flows |
| `federated_identity_manager.h` | `FederatedIdentityManager` | Multi-provider identity federation |
| `eid_authenticator.h` | `EIDAuthenticator` | eID-based authentication |

### 2.3 MFA and Passwordless Authentication

| Header | Public Type | Purpose |
|--------|------------|---------|
| `mfa_authenticator.h` | `MFAAuthenticator` | TOTP enrollment and validation |
| `totp_replay_cache.h` / `totp_secret_encryption.h` | `TOTPReplayCache`, `TOTPSecretEncryption` | Replay prevention and secret protection |
| `webauthn_authenticator.h` / `passkey_authenticator.h` | `WebAuthnAuthenticator`, `PasskeyAuthenticator` | FIDO2/WebAuthn passkey authentication |
| `mtls_authenticator.h` | `MTLSAuthenticator` | Client-certificate authentication |
| `password_policy.h` | `PasswordPolicy` | Password complexity and lifecycle constraints |

### 2.4 Session, Revocation, and Backends

| Header | Public Type | Purpose |
|--------|------------|---------|
| `session_manager.h` | `SessionManager` | Session lifecycle and revocation checks |
| `token_blacklist.h` | `TokenBlacklist` | Revoked-token tracking interface |
| `redis_token_blacklist.h` / `rocksdb_token_blacklist.h` | `RedisTokenBlacklist`, `RocksDBTokenBlacklist` | Distributed and local revocation backends |
| `rate_limiter_backend.h` | `IRateLimiterBackend` | Shared storage backend contract for auth throttling |
| `secure_memory.h` | `SecureMemory` | Sensitive credential memory hardening |

### 2.5 Authorization, Trust, and Observability

| Header | Public Type | Purpose |
|--------|------------|---------|
| `authorization_policy.h` | `AuthorizationPolicy` | RBAC/ABAC-style authorization evaluation |
| `zero_trust_auth_verifier.h` | `ZeroTrustAuthVerifier` | Per-request trust verification |
| `auth_rate_limiter.h` | `AuthRateLimiter` | Brute-force / replay protection |
| `api_key_authenticator.h` | `ApiKeyAuthenticator` | Static API key authentication |
| `auth_audit_logger.h` / `auth_metrics.h` | `AuthAuditLogger`, `AuthMetrics` | Security-audit and metrics emission |
| `auth_event_bus.h` / `auth_worker_thread_pool.h` | `AuthEventBus`, `AuthWorkerThreadPool` | Async eventing and worker execution |
| `auth_error.h` | `AuthError` | Structured auth error contract |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::auth` | Shared authentication, session, and policy types |

---

## 4. Public Contract Notes

- Authentication adapters must fail closed for invalid credentials, malformed tokens, and unsupported provider states.
- Revocation and session headers define stable contracts for distributed invalidation, while backend-specific implementations remain swappable.
- Authorization and zero-trust headers expose auditable decision surfaces consumed by `include/server/` middleware and API handlers.
- Security-sensitive helpers such as `SecureMemory` and TOTP secret encryption remain part of the public contract because embedders may supply their own storage backends.
