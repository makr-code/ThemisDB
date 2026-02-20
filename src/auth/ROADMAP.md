# Auth Module Roadmap

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
- [ ] OAuth 2.0 device authorization flow (Target: Q2 2026)
- [ ] SAML 2.0 identity provider integration (Target: Q2 2026)
- [ ] Attribute-based access control (ABAC) engine (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] OAuth 2.0 PKCE flow for public clients
- [ ] API key authentication (static key + secret)
- [ ] Session management and revocation endpoint
- [ ] WebAuthn/FIDO2 hardware token support
- [ ] Audit logging for all authentication events
- [ ] Configurable password policy enforcement

### Long-term (6-12 months)
- [ ] SAML 2.0 SP-initiated and IdP-initiated SSO
- [ ] LDAP/Active Directory direct bind authentication
- [ ] Fine-grained ABAC with policy expressions (OPA integration)
- [ ] Certificate-based mutual TLS (mTLS) authentication
- [ ] Federated identity across multiple realms
- [ ] Zero-trust access model with continuous verification

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (JWT, Kerberos, MFA flows)
- [ ] Performance benchmarks (token validation latency)
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
