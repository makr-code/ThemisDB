> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Auth Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Auth module is the security foundation of ThemisDB. It provides authentication (JWT/OIDC, Kerberos/GSSAPI, SAML, WebAuthn/FIDO2, mTLS, API keys, LDAP) and authorization (RBAC, ABAC, zero-trust). All other modules delegate identity verification and access decisions to this module.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| JWT token forgery | RS256 signature verification with JWKS; key rotation via `JWTKeyRotationManager`; audience and issuer validation |
| JWT replay attack | Rate limiting middleware; short token TTL; revocation via session management endpoint |
| Kerberos keytab exposure | Keytab files restricted to service account permissions; keytab path validated before loading |
| TOTP brute force | Rate limiting per principal; account lockout after configurable failed attempts |
| WebAuthn credential cloning | WebAuthn/FIDO2 uses hardware-bound credentials; attestation verified during registration |
| Password spraying | `AuthRateLimiter` enforces per-IP and per-principal request limits |
| SAML assertion injection | SAML responses are XML-signature-validated; `InResponseTo` correlation enforced for SP-initiated flows |
| OAuth PKCE bypass | PKCE code verifier enforced for all public client flows; authorization codes are single-use |
| mTLS certificate spoofing | Client certificates validated against trusted CA chain; certificate revocation (CRL/OCSP) checked |
| LDAP injection | LDAP queries constructed with escaped values; no string interpolation of user-supplied DN components |
| Privilege escalation via role mapping | Principal-to-role mapping is policy-controlled; no self-service role assignment |
| Federated identity trust extension | Each federated realm has an explicit trust configuration; cross-realm token acceptance requires explicit allowlist |

## Security Controls

### JWT Validation
- RS256 asymmetric verification with JWKS endpoint caching.
- `JWTKeyRotationManager` handles automatic key rotation without service restart.
- Clock skew tolerance is configurable (default ±30s) for distributed environments.
- Audience, issuer, expiry, and not-before claims all validated.

### Multi-Factor Authentication
- TOTP with RFC 6238 time-based codes; recovery codes generated on enrollment, hashed with bcrypt.
- WebAuthn/FIDO2 hardware token authentication with full attestation verification.
- MFA enforcement can be required globally or per-principal via RBAC policy.

### Session Management
- Session revocation endpoint allows immediate invalidation of issued tokens.
- Sessions are tracked per-principal; concurrent session limit is configurable.

### Zero-Trust Model
- Continuous verification: tokens are re-validated on sensitive operations, not only at login.
- ABAC engine evaluates access decisions based on request context attributes (IP, time, device) in addition to RBAC roles.

### Audit Logging
- All authentication events (success, failure, MFA, session creation, revocation) are recorded in `AuthAuditLogger` with timestamp, principal, event type, and source IP.
- Audit log entries are append-only and cannot be modified by the auth module.

## Data Handling

- Passwords are never stored; only bcrypt hashes of TOTP recovery codes.
- JWKS public keys are cached in memory; private keys are never loaded or stored by this module.
- Kerberos keytab files are read from disk at authentication time; not cached in process memory beyond the negotiation lifetime.
- Audit log entries include principal IDs and IP addresses; these are PII and subject to retention policies configured in the governance module.
- WebAuthn credentials contain public keys only; private keys remain in hardware tokens.

## Known Limitations

- Fine-grained OPA-based ABAC policy expressions are in progress (PR open, Issue #1538); current ABAC is expression-based without full Rego policy evaluation.
- SAML metadata federation (automatic metadata refresh from federation operators) is not yet implemented.
- Session anomaly detection is implemented but threshold tuning is manual.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| OpenSSL | JWT RS256, mTLS, SAML XML-sig | Keep patched; critical dependency |
| libkrb5 | Kerberos/GSSAPI | System-provided; follow OS security advisories |
| libxml2 | SAML XML parsing | Input is schema-validated; keep patched |
| libldap | LDAP/AD authentication | Input escaping enforced; keep patched |
| libwebauthn | WebAuthn/FIDO2 | Attestation verification enabled |
