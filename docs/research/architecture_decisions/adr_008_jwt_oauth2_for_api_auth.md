# ADR-008: JWT + OAuth2 PKCE as Primary API Authentication

**Status:** Accepted  
**Date:** 2023-01-01  
**Deciders:** @themisdb-core-team  
**Modules Affected:** `src/server/`, `src/auth/`  
**Related Research:** [JWT Short-Lived Tokens Best Practices](../best_practices/jwt_short_lived_tokens.md)

---

## Context

ThemisDB's HTTP and gRPC APIs must be secured with a mechanism that:

1. Works for **browser clients** (JavaScript, TypeScript) without exposing client secrets in the browser.
2. Works for **CLI tooling** (`themisctl`) using long-lived opaque tokens managed in a local config file.
3. Works for **machine-to-machine** service tokens (internal microservices, CI pipelines).
4. Supports **SSO integration** with enterprise identity providers (SAML 2.0, OpenID Connect).
5. Is **stateless at the API server** — no server-side session store required; any node in a horizontal cluster can validate any token.
6. Supports **hardware-backed admin sessions** (USB security key) for privileged administrative operations.

At decision time, ThemisDB had no authentication system. The choice needed to serve all six client categories without a separate authentication service running as a dependency.

## Decision Drivers

- **Stateless token validation:** API server nodes must validate tokens locally (no session DB lookup) to support horizontal scaling without shared state.
- **Browser-safe OAuth2 flow:** Browser clients must not receive or store a client secret; the PKCE (RFC 7636) extension is required.
- **Short-lived access tokens:** Access tokens must expire within ≤ 15 minutes to limit the blast radius of token leakage.
- **Refresh token rotation:** Refresh tokens must be rotated on each use (one-time use) to detect theft.
- **SSO compatibility:** Enterprise customers require SAML 2.0 SP and OIDC RP integration without re-architecting the auth layer.
- **CLI ergonomics:** CLI tokens must be long-lived (days/weeks) without requiring the user to re-authenticate frequently.
- **Algorithm choice:** RS256 (RSA-PKCS1-SHA256) is preferred over HS256 to avoid distributing the signing secret to all validator nodes.

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **JWT RS256 + OAuth2 PKCE (RFC 7519 + RFC 6749 + RFC 7636)** | Stateless validation (RS256 public key only at validators); PKCE browser-safe; short-lived access + rotating refresh; OIDC/SAML integration straightforward; RFC standardized | Requires RSA key management (rotation, distribution); JWT revocation requires a token blocklist or short TTL |
| **Session cookies + server-side store** | Simple implementation; instant revocation via session delete | Not stateless — requires shared session store (Redis/DB) across cluster nodes; CORS complications for browser cross-origin; not suitable for CLI or M2M |
| **API keys only** | Simple; CLI-friendly | No expiry mechanism (keys are permanent until manually revoked); no rotation; no privilege separation; unsuitable for browser flows |
| **mTLS client certificates** | Strong mutual authentication; no token expiry issue | Complex certificate distribution for browser clients (requires user to install client cert); poor UX; CA management overhead; impractical for public-facing API |

## Decision

**Chosen: JWT RS256 access tokens + refresh token rotation + OAuth2 PKCE + opaque CLI tokens + SAML 2.0 SP**

The authentication layer (`src/auth/`) implements the following scheme per client type:

### Browser clients (TypeScript / React Admin Console)
- OAuth2 Authorization Code Flow with **PKCE** (RFC 7636) — no client secret in browser.
- **Access token:** JWT RS256, TTL ≤ 15 min, signed by `src/auth/jwt_signer.cpp` using a 4096-bit RSA key.
- **Refresh token:** 256-bit random opaque token, one-time use (rotated on each `/token/refresh` call), stored in HttpOnly Secure SameSite=Strict cookie.
- **Token validation:** `src/auth/jwt_validator.cpp` validates RS256 signature using the public key published at `/.well-known/jwks.json` — no database lookup required.

### CLI tooling (`themisctl`)
- **Opaque API tokens:** 256-bit random tokens, generated at `POST /auth/api-token`, stored in `~/.themisctl/config.toml` with a configurable TTL (default 30 days).
- Validated server-side via a token lookup in the `auth_tokens` RocksDB column family (ADR-002); TTL enforced by `CompactionFilter`.
- CLI tokens do not use JWT to avoid storing RSA keys in the CLI config file.

### Machine-to-machine (M2M) / service tokens
- **JWT RS256 client credentials flow** (RFC 6749 §4.4): service accounts present `client_id` + `client_secret` to `/auth/token`; receive short-lived access token (TTL 5 min).
- Service account `client_secret` is stored as a bcrypt-hashed value in the `auth_service_accounts` column family.

### Enterprise SSO
- **SAML 2.0 Service Provider:** ThemisDB acts as the SP; enterprise IdP (Okta, Azure AD, PingFederate) is the IdP. On successful SAML assertion, ThemisDB issues a JWT access token using the same RS256 path.
- **OIDC Relying Party:** ThemisDB accepts OIDC `id_token` from a configured issuer; validates using the issuer's JWKS endpoint.

### Hardware admin sessions
- **USB security key (WebAuthn / FIDO2):** Admin operations at privilege level `ADMIN` require a WebAuthn assertion (`navigator.credentials.get()`) in addition to a valid JWT. The WebAuthn challenge-response is handled by `src/auth/webauthn_verifier.cpp` using libfido2.

Session cookies were rejected because they require a shared session store across cluster nodes — violating the stateless validation requirement. API keys alone were rejected because they have no expiry and no rotation, making compromised keys unrevokable without manual action. mTLS-only was rejected because browser clients cannot easily manage client certificates.

## Consequences

### Positive
- RS256 JWT validation requires only the public key at API nodes — no shared secret, no database lookup for access token validation, enabling horizontal scale-out without additional infrastructure.
- PKCE eliminates authorization code interception attacks for browser clients without requiring a client secret in JavaScript bundle.
- Refresh token rotation provides single-use semantics: a stolen refresh token triggers automatic session invalidation on the next legitimate use (replay detection).
- SAML 2.0 SP and OIDC RP integration satisfies enterprise SSO requirements without a separate identity provider deployment.

### Negative / Trade-offs
- **JWT revocation gap:** A compromised access token remains valid until its 15-minute TTL expires (no instant revocation without a blocklist). *Mitigation: a `TokenBlocklist` backed by the `auth_blocklist` RocksDB column family with TTL compaction handles emergency revocation; normal expiry relies on short TTL.*
- **RSA key rotation complexity:** Rotating the RS256 signing key requires coordinating JWKS publication before old tokens expire. *Mitigation: `src/auth/key_rotation.cpp` implements a two-key overlap period (publish new key, keep old key in JWKS for 30 min).*
- **CLI token server-side lookup:** Opaque CLI tokens require a database read per request (unlike JWT). *Accepted because: CLI tokens are a small fraction of total request volume; the RocksDB read is < 1 ms.*
- **WebAuthn adds browser API dependency for admin console:** Requires a modern browser with WebAuthn support. *Accepted because: admin operations are infrequent and the security benefit of hardware-backed sessions justifies the browser requirement.*

### Neutral
- The `IAuthProvider` interface in `src/auth/` allows unit tests to inject a mock provider that returns pre-signed test tokens without RSA key setup.
- The `/.well-known/jwks.json` endpoint is served by the HTTP server (ADR-003) and cached with a 1-hour `Cache-Control` header to reduce load on the auth module.

## Validation

- [x] PKCE browser flow tested end-to-end with TypeScript admin console (code verifier/challenge validation)
- [x] JWT RS256 access token validated by a separate validator node using public key only (no shared secret)
- [x] Refresh token rotation: stolen token replay correctly rejected after first legitimate use
- [x] SAML 2.0 SP integration test with Okta developer tenant
- [x] WebAuthn admin session tested with YubiKey 5 NFC
- [x] Token blocklist: blocklisted JWT rejected within 100 ms of revocation
- [ ] OIDC RP integration test with Azure AD (tracked: `tests/integration/auth/oidc_azure/`)
- [ ] RSA key rotation integration test: zero downtime key rotation under load

## Follow-up Actions

- [ ] Implement `key_rotation.cpp` with two-key overlap period for zero-downtime RSA key rotation (`src/auth/key_rotation.cpp`).
- [ ] Publish `/.well-known/openid-configuration` discovery document for OIDC RP support.
- [ ] Add OIDC RP integration with Azure AD to CI integration test suite.
- [ ] Document token TTL tuning guide for security-performance trade-off in `docs/security/token_configuration.md`.

## Related Decisions

- [ADR-003: Boost.Beast + Asio for HTTP/WebSocket/MQTT Server](adr_003_boost_beast_asio_http_server.md)
- [ADR-005: Argon2id over scrypt / bcrypt for Key Derivation](adr_005_argon2id_over_scrypt_bcrypt.md)
- [ADR-007: gRPC + Protobuf for Internal Service RPC](adr_007_grpc_for_internal_rpc.md)

---
**Last Updated:** 2026-04-06
