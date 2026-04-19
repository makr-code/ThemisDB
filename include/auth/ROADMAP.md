<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — Auth Module Public Headers

**Module Path:** `include/auth/`
**Implementation Roadmap:** `../../src/auth/ROADMAP.md`

---

## Current Status

Public headers at v1.9.0. JWT, OAuth 2.0 (device flow, PKCE), OIDC, LDAP, SAML,
Kerberos/GSSAPI, mTLS, WebAuthn, TOTP MFA, API key, federated identity, zero-trust,
session management, token blacklisting (Redis + RocksDB), and German eID Online-Ausweisfunktion are all stable.

---

## Completed Features

- [x] JWT validation with JWKS, key rotation, and TLS pinning
- [x] OAuth 2.0 device flow and PKCE
- [x] OIDC provider integration
- [x] LDAP authentication with connection pool
- [x] SAML 2.0 SSO
- [x] Kerberos/GSSAPI service ticket validation
- [x] Mutual TLS client certificate authentication
- [x] FIDO2/WebAuthn authentication
- [x] TOTP MFA with replay prevention and seed encryption
- [x] API key authentication
- [x] Federated identity management
- [x] Zero-trust continuous verification
- [x] Token blacklisting (Redis + RocksDB)
- [x] Session management with refresh
- [x] Secure memory for credential handling
- [x] Auth audit logging and rate limiting
- [x] German eID Online-Ausweisfunktion (`eid_authenticator.h`) — BSI TR-03130/TR-03110/TR-03127, eIDAS LoA HIGH

---

## Planned Features

- [x] `IPasskeyAuthenticator` for passkey (FIDO2 resident key) support (Target: Q3 2026)
- [x] `IAuthorizationPolicy` ABAC policy interface (Target: Q3 2026)
- [x] `IAuthEventBus` for auth event streaming to external SIEM (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Core Auth Protocols
- [x] JWT, JWKS, key rotation, OAuth, OIDC, LDAP, SAML, Kerberos

### Phase 2: Strong Auth Headers
- [x] mTLS, WebAuthn, TOTP MFA, API key

### Phase 3: Identity & Session Headers
- [x] Federated identity, session manager, token blacklist

### Phase 4: Zero-Trust & Rate Limiting
- [x] Zero-trust verifier, auth rate limiter, principal validator

### Phase 5: Future Auth
- [x] `IPasskeyAuthenticator` (Q3 2026)
- [x] `IAuthorizationPolicy` ABAC (Q3 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit docs present
- [ ] Doxygen fully annotated on all 33 headers

---

## Production Readiness Checklist

- [x] All major auth protocols covered
- [x] Secure memory header present
- [x] Audit logging required by all authenticators
- [x] TOTP replay prevention present
- [x] Zero-trust verification header present
- [x] ABAC policy interface published
- [x] Doxygen fully annotated
