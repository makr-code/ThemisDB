<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# Auth Module — Public Header Architecture

**Version:** 1.8.0
**Last Updated:** 2026-04-06
**Module Path:** `include/auth/`
**Implementation:** `../../src/auth/`

---

## 1. Overview

The `include/auth/` directory exposes public C++ headers for ThemisDB's authentication and
authorisation layer. This includes JWT validation and key rotation, OAuth 2.0 (device flow,
PKCE), OIDC provider, LDAP, SAML, Kerberos/GSSAPI, mTLS, WebAuthn, TOTP-based MFA, API key
authentication, federated identity management, and token blacklisting with both Redis and
RocksDB backends.

---

## 2. Design Principles

- **Pluggable Authenticators** – Each protocol (JWT, LDAP, SAML, Kerberos, mTLS, WebAuthn,
  API key) has an independent header; all implement a common `IAuthenticator` contract.
- **Defence in Depth** – `auth_rate_limiter.h`, `principal_validator.h`, and
  `zero_trust_auth_verifier.h` provide layered controls beyond initial authentication.
- **Secure Memory** – `secure_memory.h` exposes the zero-on-free memory interface used for
  credential handling; all credential-bearing types must use it.
- **Audit Trail** – `auth_audit_logger.h` is a required interface for all authentication
  events; silent auth failures are not allowed by the header contract.
- **Key Rotation** – `jwt_key_rotation_manager.h` and `jwks_validator.h` provide automated
  key rotation without downtime.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `jwt_validator.h` | `IJWTValidator`, `JWTClaims` | JWT signature verification and claims extraction |
| `jwks_validator.h` | `IJWKSValidator`, `JWKSConfig` | JWKS endpoint-based JWT key validation |
| `jwks_security.h` | `JWKSSecurity` | JWKS fetch security (TLS pinning, cache TTL) |
| `jwt_key_rotation_manager.h` | `IJWTKeyRotationManager` | Automated JWT key rotation |
| `oauth_device_flow.h` | `IOAuthDeviceFlow`, `DeviceAuthRequest` | OAuth 2.0 device authorisation flow |
| `oauth_pkce_flow.h` | `IOAuthPKCEFlow`, `PKCEChallenge` | OAuth 2.0 PKCE flow |
| `oidc_provider.h` | `IOIDCProvider`, `OIDCConfig` | OpenID Connect provider integration |
| `ldap_authenticator.h` | `ILDAPAuthenticator`, `LDAPConfig` | LDAP bind authentication |
| `ldap_connection_pool.h` | `ILDAPConnectionPool` | LDAP connection pool management |
| `saml_authenticator.h` | `ISAMLAuthenticator`, `SAMLAssertion` | SAML 2.0 SSO authentication |
| `kerberos_security.h` | `KerberosSecurity` | Kerberos/GSSAPI service ticket validation |
| `gssapi_authenticator.h` | `IGSSAPIAuthenticator` | Generic GSSAPI authentication |
| `mtls_authenticator.h` | `ImTLSAuthenticator`, `mTLSConfig` | Mutual TLS client certificate auth |
| `mfa_authenticator.h` | `IMFAAuthenticator`, `MFAChallenge` | Multi-factor authentication |
| `totp_replay_cache.h` | `ITOTPReplayCache` | TOTP one-time password replay prevention |
| `totp_secret_encryption.h` | `ITOTPSecretEncryption` | TOTP seed encryption at rest |
| `webauthn_authenticator.h` | `IWebAuthnAuthenticator`, `WebAuthnCredential` | FIDO2/WebAuthn authentication |
| `api_key_authenticator.h` | `IAPIKeyAuthenticator`, `APIKeyConfig` | API key validation |
| `federated_identity_manager.h` | `IFederatedIdentityManager`, `FederatedIdentity` | Federated identity aggregation |
| `session_manager.h` | `ISessionManager`, `SessionToken` | Session lifecycle management |
| `token_blacklist.h` | `ITokenBlacklist` | Abstract token blacklist interface |
| `redis_token_blacklist.h` | `IRedisTokenBlacklist` | Redis-backed token blacklist |
| `rocksdb_token_blacklist.h` | `IRocksDBTokenBlacklist` | RocksDB-backed token blacklist |
| `principal_validator.h` | `IPrincipalValidator`, `Principal` | Principal claims validation |
| `zero_trust_auth_verifier.h` | `IZeroTrustAuthVerifier` | Zero-trust continuous verification |
| `auth_rate_limiter.h` | `IAuthRateLimiter`, `AuthRatePolicy` | Auth endpoint rate limiting |
| `auth_worker_thread_pool.h` | `IAuthWorkerThreadPool` | Async auth operation thread pool |
| `auth_audit_logger.h` | `IAuthAuditLogger`, `AuthAuditEvent` | Auth event audit logging |
| `auth_metrics.h` | `AuthMetrics` | Auth operation metric descriptors |
| `auth_error.h` | `AuthErrorCode` enum | Canonical auth error taxonomy |
| `password_policy.h` | `IPasswordPolicy`, `PasswordPolicyConfig` | Password strength and rotation policy |
| `rate_limiter_backend.h` | `IRateLimiterBackend` | Pluggable rate limiter storage backend |
| `secure_memory.h` | `SecureMemory`, `SecureBuffer` | Zero-on-free credential memory |

> **Implementation details:** `../../src/auth/`
