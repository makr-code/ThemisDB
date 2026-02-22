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

## In Progress 🚧
- [I] OAuth 2.0 device authorization flow (Target: Q2 2026) (Issue: #1527)
- [!] SAML 2.0 identity provider integration (Target: Q2 2026) (Issue: #2371)
- [I] Attribute-based access control (ABAC) engine (Target: Q3 2026) (Issue: #1542)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] OAuth 2.0 PKCE flow for public clients (Issue: #1543)
- [I] API key authentication (static key + secret) (Issue: #1544)
- [I] Session management and revocation endpoint (Issue: #1983)
- [I] WebAuthn/FIDO2 hardware token support (Issue: #1533)
- [I] Audit logging for all authentication events (Issue: #1534)
- [I] Configurable password policy enforcement (Issue: #2013)

### Long-term (6-12 months)
- [I] SAML 2.0 SP-initiated and IdP-initiated SSO (Issue: #1536)
- [I] LDAP/Active Directory direct bind authentication (Issue: #1537)
- [I] Fine-grained ABAC with policy expressions (OPA integration) (Issue: #1538)
- [!] Certificate-based mutual TLS (mTLS) authentication (Issue: #2370)
- [I] Federated identity across multiple realms (Issue: #1540)
- [I] Zero-trust access model with continuous verification (Issue: #1541)

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
- [P] OAuth 2.0 device authorization flow (`auth/oauth_device_flow.cpp`, Target: Q2 2026) (Issue: #1552)
- [~] SAML 2.0 identity provider integration (Target: Q2 2026)
- [ ] Attribute-based access control (ABAC) engine (Target: Q3 2026)

### Phase 3: Zero-Trust & Modern AuthN (Status: Planned 📋)
- [ ] OAuth 2.0 PKCE flow for public clients
- [ ] API key authentication (static key + secret)
- [ ] WebAuthn/FIDO2 hardware token support
- [ ] Session management and revocation endpoint
- [ ] Configurable password policy enforcement
- [ ] Audit logging for all authentication events
- [ ] Certificate-based mutual TLS (mTLS) authentication

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1550)
- [x] Integration tests (JWT, Kerberos, MFA flows)
- [I] Performance benchmarks (token validation latency) (Issue: #1551)
- [x] Security audit (JWT validation, Kerberos keytab handling)
- [x] Documentation complete (configuration, flows, examples)
- [x] API stability guaranteed for JWT, Kerberos, and MFA

## Known Issues & Limitations
- SAML 2.0 is not yet implemented
- WebAuthn support is planned but not started
- ABAC (attribute-based) access control is limited to role-based rules currently
- LDAP direct bind is not supported; only Kerberos-based AD integration

## Breaking Changes
- ABAC engine will introduce new policy evaluation APIs (additive to existing RBAC)
- mTLS support will require configuration changes at the TLS layer
