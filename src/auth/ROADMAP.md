# Auth Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready enterprise authentication with JWT/OpenID Connect, Kerberos/GSSAPI, and TOTP-based MFA. RBAC and principal-to-role mapping are implemented.

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
- [x] Configurable password policy enforcement (`auth/password_policy.cpp`)

## In Progress 🚧
- [x] OAuth 2.0 device authorization flow (Target: Q2 2026)
- [x] SAML 2.0 identity provider integration (Target: Q2 2026)
- [x] Attribute-based access control (ABAC) engine (Target: Q3 2026) (Issue: #1542)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] OAuth 2.0 PKCE flow for public clients (Issue: #1543)
- [x] API key authentication (static key + secret) (Issue: #1544)
- [x] Session management and revocation endpoint (Issue: #1983)
- [x] WebAuthn/FIDO2 hardware token support (Issue: #1533)
- [x] Audit logging for all authentication events (Issue: #1534)
- [x] Configurable password policy enforcement (Issue: #2013)

### Long-term (6-12 months)
- [x] SAML 2.0 SP-initiated and IdP-initiated SSO
- [I] LDAP/Active Directory direct bind authentication (Issue: #1537)
- [P] Fine-grained ABAC with policy expressions (OPA integration) (Issue: #1538)
- [x] Certificate-based mutual TLS (mTLS) authentication (Issue: #2370)
- [x] Federated identity across multiple realms (Issue: #1540)
- [x] Zero-trust access model with continuous verification (Issue: #1541)

## Implementation Phases

### Phase 1: Enterprise Authentication Core (Status: Completed ✅)
- [x] JWT validation with OpenID Connect and Keycloak integration (`auth/jwt_validator.cpp`)
- [x] RS256 signature verification with JWKS caching and configurable TTL
- [x] Clock skew tolerance for distributed environments
- [x] Kerberos/GSSAPI authentication for Active Directory SSO (`auth/kerberos_auth.cpp`)
- [x] TOTP-based Multi-Factor Authentication with recovery codes (`auth/totp_mfa.cpp`)
- [x] Principal-to-role mapping and RBAC enforcement
- [x] Rate limiting for brute force and replay attack prevention
- [x] Fallback from Kerberos to basic authentication

### Phase 2: Extended Identity Protocols (Status: In Progress 🚧)
- [x] OAuth 2.0 device authorization flow (`auth/oauth_device_flow.cpp`, Target: Q2 2026)
- [x] SAML 2.0 identity provider integration (`auth/saml_authenticator.cpp`, Target: Q2 2026)
- [x] OIDC Provider Discovery and federated identity (`auth/oidc_provider.cpp`)
- [x] Attribute-based access control (ABAC) engine (Target: Q3 2026)
- [x] Federated identity across multiple realms (`auth/federated_identity_manager.cpp`)

### Phase 3: Zero-Trust & Modern AuthN (Status: Planned 📋)
- [x] OAuth 2.0 PKCE flow for public clients
- [x] API key authentication (static key + secret)
- [x] WebAuthn/FIDO2 hardware token support
- [x] Session management and revocation endpoint
- [x] Configurable password policy enforcement
- [x] Audit logging for all authentication events
- [x] Certificate-based mutual TLS (mTLS) authentication (`auth/mtls_authenticator.cpp`)
## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1550)
- [x] Integration tests (JWT, Kerberos, MFA flows)
- [I] Performance benchmarks (token validation latency) (Issue: #1551)
- [x] Security audit (JWT validation, Kerberos keytab handling)
- [x] Documentation complete (configuration, flows, examples)
- [x] API stability guaranteed for JWT, Kerberos, and MFA

## Known Issues & Limitations
- ABAC (attribute-based) access control is limited to role-based rules currently
- WebAuthn support is planned but not started
- LDAP direct bind is not supported; only Kerberos-based AD integration

## Breaking Changes
- ABAC engine introduces new policy evaluation APIs (additive to existing RBAC, backward-compatible)
- mTLS support will require configuration changes at the TLS layer
