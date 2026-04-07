# Short-Lived JWT Access Tokens with Refresh Token Rotation

**Metadaten:**
- Source: RFC 7519 (JWT) + RFC 6749 (OAuth 2.0) + BCP 212 (OAuth 2.0 Security Best Current Practice)
- URL: https://www.rfc-editor.org/rfc/rfc7519 | https://www.rfc-editor.org/rfc/bcp/bcp212
- Tags: security, authentication
- ThemisDB-Versionen: v1.6.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Long-lived tokens are a major source of credential theft impact: a stolen token remains valid until its expiry, giving an attacker an extended window. BCP 212 (OAuth 2.0 Security Best Current Practice) and RFC 7519 both recommend short-lived access tokens (≤15 minutes) combined with rotating refresh tokens: each use of a refresh token issues a new access token *and* a new refresh token while invalidating the old one. Refresh token rotation means that if a token is stolen, its use by the attacker immediately invalidates the legitimate user's token and triggers a detectable anomaly (duplicate use detection).

ThemisDB's OAuth2 provider (`src/server/oauth2_provider.cpp`) implements this pattern with RS256-signed JWTs for access tokens and opaque random tokens (256-bit) for refresh tokens stored in the server-side session store.

## 🎯 Core Principles

- **Access token TTL ≤ 15 minutes**: The `exp` claim is set to `iat + 900` seconds maximum. Shorter TTLs (5 minutes) are used for high-privilege scopes.
- **Refresh token rotation on every use**: Each `/token` refresh request invalidates the presented refresh token and issues a new one. The token family tree is tracked for reuse detection.
- **Refresh token reuse detection**: If a previously-used (and already-rotated) refresh token is presented, the entire token family is immediately revoked (all refresh tokens in the family), alerting to a possible theft.
- **RS256 signing for access tokens**: Asymmetric signing allows any resource server to verify tokens using the public key (exposed via `/.well-known/jwks.json`) without needing the private key.
- **Short-lived refresh tokens for sensitive scopes**: Refresh tokens for admin scopes have a sliding expiry of 24 hours; for standard scopes, 30 days.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/server/oauth2_provider.cpp` — Token issuance, refresh, revocation, and introspection endpoints; token family tracking in-memory + Redis for distributed node sharing.
- `src/server/jwt_validator.cpp` — Stateless access token validation via JWKS; caches public key with `max-age` from JWKS `Cache-Control` header.
- `src/server/` middleware — `AuthMiddleware` validates the `Authorization: Bearer <token>` header using `JwtValidator` before routing.

### What Was Adopted?

- Access token `exp = iat + 900` (15 min); `iss` = server base URL; `sub` = user ID; `scope` = space-delimited granted scopes; `jti` = UUID v4 (unique per token).
- RS256 signing: 2048-bit RSA key pair; private key stored in encrypted key store; public key rotated every 90 days with a 7-day overlap window.
- Refresh token: 256-bit `RAND_bytes()` value, base64url-encoded, stored in Redis hash `refresh_tokens:<token_hash>` with TTL.
- On each `/oauth2/token?grant_type=refresh_token`: old token is deleted; new access + refresh tokens are issued; token family list is updated.
- Reuse detection: each token family stores all previously issued tokens; presenting an already-revoked token triggers `DELETE refresh_families:<family_id>` (full family revocation).
- `/.well-known/jwks.json` endpoint exposes public key as JWK; consumed by resource server middlewares.

### Deviations & Rationale

- **Opaque refresh tokens instead of signed JWTs**: BCP 212 recommends opaque refresh tokens (to allow server-side revocation). ThemisDB follows this exactly. Access tokens are signed JWTs (stateless verification); refresh tokens are opaque (server-side revocable).
- **No PKCE for server-side flows**: PKCE is implemented for public-client (browser/mobile) flows but is not required for confidential-client (server-to-server) flows per RFC 6749 §4.4. ThemisDB server-to-server clients use client_credentials grant without PKCE.
- **Refresh token absolute vs. sliding expiry**: BCP 212 permits both. ThemisDB uses sliding expiry for standard scopes (each use resets the 30-day clock) and absolute expiry for admin scopes (expires 24 hours after issuance regardless of activity).

## ⚠️ Trade-offs & Limitations

- **Logout on device with no token refresh**: If a user's device is offline for longer than the access token TTL, the next request will require a token refresh, which requires network access. This is expected and documented in client SDK documentation.
- **Redis dependency for refresh token state**: Server-side refresh token storage requires Redis availability. A Redis outage prevents token refresh; clients must re-authenticate. This dependency is mitigated by Redis replication and automatic failover.
- **Key rotation operational complexity**: RS256 key rotation requires coordinating JWKS cache invalidation across all resource servers. The 7-day overlap window and `Cache-Control: max-age=3600` header on the JWKS endpoint manage this.
- **Token size**: RS256-signed JWTs are ~400–600 bytes. For high-frequency API calls this adds bandwidth and parsing overhead compared to opaque tokens. The stateless verification benefit outweighs the size cost.

## 🔬 Validation

- [x] Code reviewed against RFC 7519, RFC 6749, and BCP 212 §2 (token theft mitigations)
- [x] Unit tests in `tests/server/oauth2_provider_test.cpp` verify token issuance, rotation, reuse detection, and revocation
- [x] Security test verifies that presenting a stolen (already-rotated) refresh token revokes the entire family
- [x] Module README linked (`src/server/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [TLS 1.3 Cipher Hardening](tls13_cipher_hardening.md)
- [Token Bucket Rate Limiting](token_bucket_rate_limiting.md)

---
**Last Updated:** 2026-04-06
