> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Auth Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready enterprise authentication with JWT/OpenID Connect, Kerberos/GSSAPI, TOTP-based MFA, and WebAuthn/FIDO2 hardware token support. RBAC and principal-to-role mapping are implemented.

## Completed ✅
- [x] JWT validation with OpenID Connect (Keycloak integration)
- [x] RS256 signature verification with JWKS caching
- [x] Clock skew tolerance for distributed environments
- [x] Kerberos/GSSAPI authentication for Active Directory SSO
- [x] TOTP-based Multi-Factor Authentication with recovery codes
- [x] Principal-to-role mapping and RBAC enforcement
- [x] Rate limiting for brute force and replay attack prevention
- [x] Configurable JWKS cache TTL and audience/issuer validation
- [x] Fallback from Kerberos to basic authentication
- [x] OIDC Provider Discovery and federated identity integration (`auth/oidc_provider.cpp`)
- [x] Federated identity across multiple realms (`auth/federated_identity_manager.cpp`)
- [x] Audit logging for all authentication events (`auth/auth_audit_logger.cpp`)
- [x] WebAuthn/FIDO2 hardware token support (`auth/webauthn_authenticator.cpp`)
- [x] Configurable password policy enforcement (`auth/password_policy.cpp`)
- [x] OAuth 2.0 device authorization flow (Target: Q2 2026)
- [x] SAML 2.0 identity provider integration (Target: Q2 2026)
- [x] Attribute-based access control (ABAC) engine (Target: Q3 2026) (Issue: #1542)
- [x] OAuth 2.0 PKCE flow for public clients (Issue: #1543)
- [x] API key authentication (static key + secret) (Issue: #1544)
- [x] Session management and revocation endpoint (Issue: #1983)
- [x] WebAuthn/FIDO2 hardware token support (Issue: #1533)
- [x] Audit logging for all authentication events (Issue: #1534)
- [x] Configurable password policy enforcement (Issue: #2013)
- [x] SAML 2.0 SP-initiated and IdP-initiated SSO
- [x] LDAP/Active Directory direct bind authentication (Issue: #1537)
- [x] Async/non-blocking LDAP and HTTP authentication calls via `AuthWorkerThreadPool`, `authenticateAsync()`, and `validateAsync()` (Target: v1.2.0) (Issue: #3836)
- [x] Token blacklist persistence and distributed backends via `ITokenBlacklist`, `RocksDBTokenBlacklist`, and `RedisTokenBlacklist` (Target: v1.3.0) (Issue: #3837)
- [x] LDAP connection pooling with `LDAPConnectionPool` and pool metrics (`pool_size`, `idle_connections`, `active_connections`) (Target: v1.2.0) (Issue: #3838)
- [x] Certificate-based mutual TLS (mTLS) authentication (Issue: #2370)
- [x] Federated identity across multiple realms (Issue: #1540)
- [x] Zero-trust access model with continuous verification (Issue: #1541)
- [x] EC Curve P-384 (ES384) and P-521 (ES512) JWT algorithm support

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)
- [P] Fine-grained ABAC with policy expressions (OPA integration) (Issue: #1538)

## Implementation Phases

### Phase 1: Enterprise Authentication Core (Status: Completed ✅)
- [x] JWT validation with OpenID Connect and Keycloak integration (`auth/jwt_validator.cpp`)
- [x] RS256 signature verification with JWKS caching and configurable TTL
- [x] Clock skew tolerance for distributed environments
- [x] Kerberos/GSSAPI authentication for Active Directory SSO (`auth/gssapi_authenticator.cpp`)
- [x] TOTP-based Multi-Factor Authentication with recovery codes (`auth/mfa_authenticator.cpp`)
- [x] Principal-to-role mapping and RBAC enforcement
- [x] Rate limiting for brute force and replay attack prevention
- [x] Fallback from Kerberos to basic authentication

### Phase 2: Extended Identity Protocols (Status: Completed ✅)
- [x] OAuth 2.0 device authorization flow (`auth/oauth_device_flow.cpp`, Target: Q2 2026)
- [x] SAML 2.0 identity provider integration (`auth/saml_authenticator.cpp`, Target: Q2 2026)
- [x] OIDC Provider Discovery and federated identity (`auth/oidc_provider.cpp`)
- [x] Attribute-based access control (ABAC) engine (Target: Q3 2026)
- [x] Federated identity across multiple realms (`auth/federated_identity_manager.cpp`)

### Phase 3: Zero-Trust & Modern AuthN (Status: Completed ✅)
- [x] OAuth 2.0 PKCE flow for public clients
- [x] API key authentication (static key + secret)
- [x] WebAuthn/FIDO2 hardware token support
- [x] Session management and revocation endpoint
- [x] Configurable password policy enforcement
- [x] Audit logging for all authentication events
- [x] Certificate-based mutual TLS (mTLS) authentication (`auth/mtls_authenticator.cpp`)

### Phase 4: RFC 7518 Algorithm Completeness (Status: Completed ✅)
- [x] EC Curve P-384 (ES384 / SHA-384) JWT algorithm (`auth/jwt_validator.cpp`)
- [x] EC Curve P-521 (ES512 / SHA-512) JWT algorithm (`auth/jwt_validator.cpp`)
- [x] RSA RS384 (SHA-384) JWT algorithm (`auth/jwt_validator.cpp`)
- [x] RSA RS512 (SHA-512) JWT algorithm (`auth/jwt_validator.cpp`)
- [x] Comprehensive test coverage: `tests/test_jwt_ec_curves_comprehensive.cpp`
## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1550)
- [x] Integration tests (JWT, Kerberos, MFA flows)
- [x] Focused standalone test targets registered in `tests/CMakeLists.txt` (31 test executables: JWT, API-key, MFA, TOTP, OAuth, SAML, mTLS, WebAuthn, session, zero-trust, anomaly detection, EC curves)
- [x] Performance benchmarks (token validation latency) (Issue: #1551)
- [x] Security audit (JWT validation, Kerberos keytab handling)
- [x] Documentation complete (configuration, flows, examples)
- [x] API stability guaranteed for JWT, Kerberos, and MFA
- [x] Full RFC 7518 (JWA) algorithm coverage: RS256/RS384/RS512, ES256/ES384/ES512, EdDSA

## Known Issues & Limitations
- Fine-grained ABAC with OPA/Rego policy expressions is not yet implemented; the current `PolicyEngine` evaluates structured JSON-based policies but does not integrate an OPA runtime (Issue: #1538, Target: Q3 2026).
- LDAP direct bind requires OpenLDAP (libldap) on Linux or WinLDAP on Windows; build with -DTHEMIS_ENABLE_LDAP=ON (default).
## Breaking Changes
- ABAC engine (`PolicyEngine`) API is additive to existing RBAC and backward-compatible.
- mTLS (`MtlsAuthenticator`) requires TLS layer configuration changes; see auth/mtls_authenticator.h for details.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### ✅ Aktiv (implementiert + externer Aufrufer bestätigt)

- `ApiKeyAuthenticator` – Authentifiziert HTTP-Requests via API-Key; genutzt in auth_middleware.cpp
- `constantTimeEqual` – Zeitkonstanter Byte-Vergleich gegen Timing-Side-Channel bei API-Key-Checks;
  genutzt in `api_key_authenticator.cpp`.
