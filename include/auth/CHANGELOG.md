<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Auth Module Public Headers

All notable changes to public headers in `include/auth/`.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.9.1] — 2026-04-08
### Changed
- `tests/CMakeLists.txt`: Registered 13 previously unregistered test executables covering federated identity, token blacklist persistence, password policy, Kerberos security validator, server rate limiting, mTLS client/pool, API auth config, API key management, and PKI/eIDAS placeholder. All targets are gated with `if(EXISTS ...)` guards consistent with existing patterns.

## [1.9.0] — 2026-03-24
### Added
- `eid_authenticator.h`: `EIDAttributeType`, `EIDAttribute`, `EIDAssuranceLevel`, `EIDIdentity`, `EIDAuthConfig`, `EIDAuthResult`, `IEIDAuthenticator`, `InMemoryEIDAuthenticator` — integration with the German Online-Ausweisfunktion (eID) per BSI TR-03130 v3.3 / TR-03110 v2.21 / TR-03127 v1.20; supports initialize, beginAuthSession (returns redirect URL), completeAuthSession (SAML assertion validation simulation), revokeSession, EIDIdentity attribute access, fullName helper, eIDAS Level of Assurance; 30 tests in `tests/test_eid_authenticator.cpp`; CI: `eid-authenticator-ci.yml`

## [1.8.0] — 2026-03-22
### Added
- `zero_trust_auth_verifier.h`: `IZeroTrustAuthVerifier` for continuous zero-trust posture checks
- `totp_secret_encryption.h`: `ITOTPSecretEncryption` for TOTP seed encryption at rest
- `rate_limiter_backend.h`: `IRateLimiterBackend` pluggable storage for rate limiting
- `gssapi_authenticator.h`: `IGSSAPIAuthenticator` for generic GSSAPI authentication
- `oauth_pkce_flow.h`: `IOAuthPKCEFlow` and `PKCEChallenge` for OAuth 2.0 PKCE

### Changed
- `jwt_validator.h`: `JWTClaims` extended with `custom_claims` map
- `session_manager.h`: Added `refreshSession()` method to `ISessionManager`
- `auth_rate_limiter.h`: `AuthRatePolicy` now supports per-IP and per-tenant independent limits

## [1.7.0] — 2026-03-09
### Added
- `federated_identity_manager.h`: `IFederatedIdentityManager` for multi-IdP identity aggregation
- `rocksdb_token_blacklist.h`: `IRocksDBTokenBlacklist` as local persistent blacklist backend
- `principal_validator.h`: `IPrincipalValidator` and `Principal` for claims-level validation
- `auth_worker_thread_pool.h`: `IAuthWorkerThreadPool` for async auth operation scheduling

### Changed
- `token_blacklist.h`: `ITokenBlacklist` interface made backend-agnostic (Redis/RocksDB)
- `webauthn_authenticator.h`: `WebAuthnCredential` extended with `aaguid` and `attestation_type`

## [1.6.0] — 2026-02-01
### Added
- Initial public header set: `jwt_validator.h`, `jwks_validator.h`, `jwks_security.h`,
  `jwt_key_rotation_manager.h`, `oauth_device_flow.h`, `oidc_provider.h`
- `ldap_authenticator.h`, `ldap_connection_pool.h`
- `saml_authenticator.h`, `kerberos_security.h`
- `mtls_authenticator.h`, `mfa_authenticator.h`, `totp_replay_cache.h`
- `webauthn_authenticator.h`, `api_key_authenticator.h`
- `session_manager.h`, `token_blacklist.h`, `redis_token_blacklist.h`
- `auth_rate_limiter.h`, `auth_audit_logger.h`, `auth_metrics.h`, `auth_error.h`
- `password_policy.h`, `secure_memory.h`
