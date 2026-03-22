<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Auth Module

All notable changes to the Auth module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Fine-grained ABAC with OPA policy expressions — PR open (Issue #1538)

## [1.8.0] — 2026-03-22
### Added
- EC Curve P-384 (ES384 / SHA-384) JWT algorithm support (`auth/jwt_validator.cpp`) — Feature #9
- EC Curve P-521 (ES512 / SHA-512) JWT algorithm support (`auth/jwt_validator.cpp`) — Feature #9
- RSA RS384 (SHA-384) and RS512 (SHA-512) JWT algorithm support (`auth/jwt_validator.cpp`) — Feature #9
- `verifySignatureEC()` dispatcher in `jwt_validator.cpp` handles ES256/ES384/ES512 parameterically (curve NID, coordinate size, digest)
- `verifySignatureRSA()` dispatcher in `jwt_validator.cpp` handles RS256/RS384/RS512 parameterically (digest selection)
- Comprehensive test coverage: `tests/test_jwt_ec_curves_comprehensive.cpp` (ES384 + ES512 happy-path, expired, wrong sig, tampered payload, kid revocation, cross-curve attacks; RS384 + RS512)

### Changed
- Algorithm allow-list in `JWTValidator::parseAndValidate()` extended from `{RS256, ES256, EdDSA}` to `{RS256, RS384, RS512, ES256, ES384, ES512, EdDSA}`
- `verifySignatureRS256()` now delegates to `verifySignatureRSA()` (backward compatible)
- `verifySignatureES256()` now delegates to `verifySignatureEC()` (backward compatible)

## [1.7.0] — 2026-03-09
### Added
- Certificate-based mutual TLS (mTLS) authentication (`auth/mtls_authenticator.cpp`) (Issue #2370)
- Zero-trust access model with continuous verification (Issue #1541)
- Session management and revocation endpoint (Issue #1983)
- Configurable password policy enforcement (`auth/password_policy.cpp`) (Issue #2013)
- Focused standalone test targets for 30 test executables: JWT, API-key, MFA, TOTP, OAuth, SAML, mTLS, WebAuthn, session, zero-trust, anomaly detection

## [1.6.0] — 2026-01-20
### Added
- Attribute-based access control (ABAC) engine (Issue #1542)
- OAuth 2.0 PKCE flow for public clients (Issue #1543)
- API key authentication: static key + secret (`auth/api_key_authenticator.cpp`) (Issue #1544)
- LDAP/Active Directory direct bind authentication (`auth/ldap_authenticator.cpp`) (Issue #1537)
- Federated identity across multiple realms (`auth/federated_identity_manager.cpp`) (Issue #1540)
- Auth rate limiter for brute force prevention (`auth/auth_rate_limiter.cpp`)
- Auth metrics collection (`auth/auth_metrics.cpp`)

## [1.5.0] — 2025-11-15
### Added
- OAuth 2.0 device authorization flow (`auth/oauth_device_flow.cpp`) — Target Q2 2026
- SAML 2.0 SP-initiated and IdP-initiated SSO (`auth/saml_authenticator.cpp`) — Target Q2 2026
- OIDC Provider Discovery and federated identity integration (`auth/oidc_provider.cpp`)
- WebAuthn/FIDO2 hardware token support (`auth/webauthn_authenticator.cpp`) (Issue #1533)
- Audit logging for all authentication events (`auth/auth_audit_logger.cpp`) (Issue #1534)
- JWT key rotation manager (`auth/jwt_key_rotation_manager.cpp`)
- JWKS security layer (`auth/jwks_security.cpp`) and JWKS validator (`auth/jwks_validator.cpp`)

### Changed
- Clock skew tolerance made configurable for distributed environment deployments
- JWKS cache TTL now configurable (default 5 minutes)

## [1.0.0] — 2024-01-01
### Added
- JWT validation with OpenID Connect (Keycloak integration) and RS256 signature verification
- JWKS caching with configurable TTL and audience/issuer validation
- Kerberos/GSSAPI authentication for Active Directory SSO (`auth/gssapi_authenticator.cpp`)
- TOTP-based Multi-Factor Authentication with recovery codes (`auth/mfa_authenticator.cpp`)
- Principal-to-role mapping and RBAC enforcement
- Rate limiting for brute force and replay attack prevention
- Fallback from Kerberos to basic authentication
- Auth error taxonomy (`auth/auth_error.cpp`)
