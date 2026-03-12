<!-- Status: current | validated: 2026-06-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · src/auth/FUTURE_ENHANCEMENTS.md -->

# Auth Module - Future Enhancements

## Scope

The ThemisDB authentication module (`src/auth/`, `include/auth/`) is a full-stack identity and access control subsystem covering: JWT/OIDC bearer-token validation and issuance, Kerberos/GSSAPI enterprise authentication, TOTP and WebAuthn/FIDO2 multi-factor authentication, OAuth 2.0 (authorization-code, device, PKCE, client-credentials), SAML 2.0 SP/IdP-initiated SSO, LDAP/AD directory bind, mTLS service-identity verification, federated identity bridging, session management, token blacklisting, rate limiting, audit logging, and zero-trust continuous verification. The module consists of 29 source files and a matching set of public headers.

---

## Design Constraints

- `[ ]` JWT validation hot path must remain stateless — no database or network call when the JWKS cache is warm (`jwt_validator.cpp:fetchJWKS()` already caches, but the cache itself is unprotected by a mutex)
- `[ ]` All synchronous LDAP/HTTP calls must not block the event-loop thread — needs either a dedicated thread pool or an async wrapper
- `[ ]` All token/secret comparisons must use constant-time primitives (`CRYPTO_memcmp`) — `api_key_authenticator.cpp` already complies; remaining callers must follow
- `[ ]` Token blacklist must survive process restart — currently in-memory only (`token_blacklist.cpp`)
- `[ ]` Token cache (JWKS, session store) must be bounded — no hard memory cap enforced today
- `[ ]` No secret material in plain-text config structs — `jwks_security.cpp:274-276` passes `client_key_password` directly via `CURLOPT_KEYPASSWD`
- `[ ]` LDAP filter and DN values must be properly escaped before use — `ldap_authenticator.cpp:buildUserDN()` performs raw string substitution with no LDAP escape
- `[ ]` `expected_issuer` and `expected_audience` should be required, not optional — silent skip when empty is a misconfiguration footgun (`jwt_validator.cpp:506`, `jwt_validator.cpp:345`)

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ITokenValidator` | Query engine, HTTP gateway | Validates JWT/OIDC bearer tokens; returns `Claims` or error |
| `IKerberosAuthenticator` | Enterprise connector | Accepts GSSAPI token, returns authenticated principal |
| `IMFAProvider` | Login service | TOTP / WebAuthn challenge-response |
| `IOAuthFlowHandler` | REST API, CLI | Authorization-code, device, and client-credentials flows |
| `ISAMLHandler` | SSO bridge | Parses/validates SAML assertions, maps attributes to internal roles |
| `IRBACEvaluator` | Query engine | Role-permission matrix for principal + resource |
| `IABACEvaluator` | Query engine | Attribute-based policies (XACML-style) |
| `ImTLSVerifier` | gRPC/HTTP server | Validates client certificate chain; maps to service identity |
| `ITokenBlacklist` | JWT validator, session manager | Revocation store; must support distributed backends |

---

## Implemented Features (Preserved)

| Feature | Source File | Status |
|---------|-------------|--------|
| OAuth 2.0 Device Flow (RFC 8628) | `src/auth/oauth_device_flow.cpp` | ✅ Production-Ready |
| OAuth 2.0 PKCE Flow (RFC 7636) | `src/auth/oauth_pkce_flow.cpp` | ✅ Production-Ready |
| WebAuthn/FIDO2 hardware token support | `src/auth/webauthn_authenticator.cpp` | ✅ Production-Ready |
| SAML 2.0 SP- and IdP-initiated SSO | `src/auth/saml_authenticator.cpp` | ✅ Production-Ready |
| Concurrent session management and remote logout | `src/auth/session_manager.cpp` | ✅ Production-Ready |
| Constant-time API key comparison via `CRYPTO_memcmp` | `src/auth/api_key_authenticator.cpp:272` | ✅ Production-Ready |
| `alg:none` rejection in JWT validation | `src/auth/jwt_validator.cpp:413` | ✅ Production-Ready |
| `exp`, `nbf`, `iat`, `iss`, `aud` claim validation | `src/auth/jwt_validator.cpp:458-518` | ✅ Production-Ready |
| JTI-based per-token revocation against blacklist | `src/auth/jwt_validator.cpp:558-564` | ✅ Production-Ready |
| LDAP username/password/DN length bounds checking | `src/auth/ldap_authenticator.cpp:133-173` | ✅ Production-Ready |
| TOTP replay protection with mutex-guarded cache | `src/auth/totp_replay_cache.cpp` | ✅ Production-Ready |

---

## Planned Features

### 1. Thread-Safety: Add Mutex to `JWTValidator` JWKS Cache

**Priority:** Critical  
**Target Version:** v1.1.0

`include/auth/jwt_validator.h` declares `jwks_cache_` (line 192) and `jwks_cache_time_` (line 193) as plain non-atomic member fields. There is **no `mutable std::mutex`** guarding them in the header or in `jwt_validator.cpp`. When multiple threads call `JWTValidator::validate()` concurrently and the cache expires, they all race into `fetchJWKS()` simultaneously — writing `jwks_cache_` and `jwks_cache_time_` from multiple threads is a data race (undefined behaviour under C++11 and later).

**Implementation Notes:**
- `[x]` Add `mutable std::shared_mutex jwks_cache_mutex_` to `jwt_validator.h` alongside `jwks_cache_` (`jwt_validator.h:192`)
- `[x]` Wrap all reads of `jwks_cache_` in `fetchJWKS()` with `std::shared_lock` and all writes with `std::unique_lock` (`jwt_validator.cpp:98-174`)
- `[x]` Implement "double-checked locking" pattern: acquire shared lock first, check staleness, upgrade to unique lock only if refresh is needed, then re-check to avoid thundering-herd on cache expiry
- `[x]` Add unit test: spawn 32 threads each calling `validate()` concurrently with cache TTL of 0 — verify no crash under Thread Sanitizer (TSAN)

**Performance Targets:**
- Zero-overhead on warm-cache path (shared_lock is reader-writer; concurrent readers proceed in parallel)
- At most one actual HTTP fetch per cache expiry under concurrent load

---

### 2. Async / Non-Blocking LDAP and HTTP Authentication Calls

**Priority:** High  
**Target Version:** v1.2.0

`ldap_authenticator.cpp` uses exclusively synchronous blocking calls: `ldap_simple_bind_s()` (line 222), `ldap_search_s()` (line 257), `ldap_search_ext_s()` (line 379), `ldap_start_tls_s()` (line 333). `jwt_validator.cpp:132` calls `curl_easy_perform()` synchronously with an inline `std::this_thread::sleep_for` retry loop (lines 118, 145). `oidc_provider.cpp:230` and `oauth_pkce_flow.cpp:214` and `oauth_device_flow.cpp:198` each call `curl_easy_perform()` / `httpPost()` on the caller's thread. This means any network timeout or LDAP server slowdown stalls the entire calling thread.

**Implementation Notes:**
- `[x]` Introduce a dedicated `AuthWorkerThreadPool` (min 4, max 32 threads) in `ldap_authenticator.cpp` so that `authenticate()` dispatches to the pool and returns a `std::future<LDAPAuthResult>`
- `[x]` Replace `curl_easy_perform()` in `jwt_validator.cpp:fetchJWKS()`, `oidc_provider.cpp:httpGet()`, `oauth_pkce_flow.cpp`, and `oauth_device_flow.cpp` with `curl_multi_perform()` calls on a shared multi-handle. `curl_multi_info_read()` is used to retrieve per-transfer `CURLcode` results.
- `[x]` Move the HTTP fetch in `fetchJWKS()` entirely outside `jwks_cache_mutex_`. Only a brief exclusive lock is taken to write the result. A single-flight mutex (`jwks_refresh_mutex_`) prevents thundering-herd stampedes.  `std::this_thread::sleep_for` back-off remains on the worker thread (not on the caller).
- `[x]` Expose async variants (`authenticateAsync()`, `validateAsync()`) on existing public interfaces so callers can use `co_await` / `std::future`

**Status:** `[x]` Implemented in v1.2.0

**Performance Targets:**
- LDAP bind latency P99 ≤ 50 ms visible to callers even when backend latency is 200 ms (no head-of-line blocking)
- JWT JWKS refresh never blocks the validation hot path for more than 1 ms

---

### 3. LDAP DN and Filter Injection Prevention

**Priority:** Critical (Security)  
**Target Version:** v1.1.0

`ldap_authenticator.cpp:buildUserDN()` (lines 90-97) substitutes the raw `username` string into a DN template by replacing the `{username}` placeholder with no escaping at all. An attacker supplying a username containing DN special characters (`,`, `=`, `+`, `<`, `>`, `#`, `;`, `\`, `"`) can manipulate the constructed DN to bind as a different directory entry. This is a textbook LDAP injection vulnerability.

**Implementation Notes:**
- `[x]` Implement `escapeLDAPDNComponent(const std::string& value)` in `ldap_authenticator.cpp` following RFC 4514 Section 2.4: escape characters `,`, `+`, `"`, `\`, `<`, `>`, `;`, and leading/trailing spaces and `#`
- `[x]` Implement `escapeLDAPFilterValue(const std::string& value)` following RFC 4515 Section 3: escape `*`, `(`, `)`, `\`, NUL
- `[x]` Call `escapeLDAPDNComponent()` on `username` inside `buildUserDN()` before string substitution (line 96)
- `[x]` Call `escapeLDAPFilterValue()` on all user-controlled values inserted into LDAP search filter strings (lines 257, 379)
- `[x]` Add `LDAP_OPT_REFERRALS = LDAP_OPT_OFF` to both the Windows path (line 208) and the POSIX path (line 317) — referral chasing with attacker-controlled usernames can redirect authentication to a rogue LDAP server
- `[x]` Add fuzz test (libFuzzer) targeting `buildUserDN()` with adversarial username inputs

**Performance Targets:**
- Escaping adds < 5 µs overhead per authentication call

---

### 4. Constant-Time Comparison for Recovery Codes and Session IDs

**Priority:** High (Security)  
**Target Version:** v1.1.0

`api_key_authenticator.cpp:272` already uses `CRYPTO_memcmp()` for secret comparison — correct. However, other comparators in the module are not constant-time:

- `mfa_authenticator.cpp:173`: recovery code lookup uses `std::find` / iterator comparison (`it != enrollment.recovery_codes.end()`). An attacker who can measure sub-microsecond timing differences can deduce the position of the matching recovery code in the list via early-exit short-circuit.
- `session_manager.cpp:205`: session ID lookup uses `unordered_map::find`, which compares `std::string` keys with `operator==` — subject to timing oracle for session token brute-force.
- `totp_replay_cache.cpp`: TOTP code comparison inside `markUsed` uses `std::unordered_set` integer hash lookup — effectively constant-time for integers, but worth documenting explicitly.

**Implementation Notes:**
- `[x]` In `mfa_authenticator.cpp`, replace `std::find` over recovery codes with a loop using `CRYPTO_memcmp()` that always iterates all entries regardless of match, then returns true/false after full traversal (prevents early-exit timing leak) (line 173)
- `[x]` In `session_manager.cpp`, store session IDs as their SHA-256 hash in the lookup map; compare incoming tokens by hashing them first, which normalises comparison time regardless of input content (`session_manager.cpp:205`, `sessions_` member)
- `[x]` Add microbenchmark that measures TOTP/recovery-code verification latency variance under ThreadSanitizer to confirm constant-time behaviour

**Performance Targets:**
- Recovery code verification time must vary by < 100 ns regardless of match position in a list of 10 codes (production hardware target; CI gate uses < 100 µs to accommodate sanitizer/scheduler overhead)

---

### 5. Mandatory JWT Issuer and Audience Validation

**Priority:** High (Security)  
**Target Version:** v1.1.0

`jwt_validator.cpp:506` validates `iss` only if `cfg_.expected_issuer` is non-empty, and `jwt_validator.cpp:345` validates `aud` only if `cfg_.expected_audience` is non-empty. A misconfigured deployment (empty strings, which is the default) silently accepts tokens from **any** issuer and **any** audience. In a multi-tenant or microservice environment this allows token substitution attacks.

**Implementation Notes:**
- `[ ]` In `JWTValidator::Config` (`jwt_validator.h:83-84`), replace `expected_issuer`/`expected_audience` plain strings with `std::optional<std::string>` and add a `bool require_issuer_validation = true` / `bool require_audience_validation = true` flag pair
- `[ ]` In `JWTValidator::validate()`, throw `std::runtime_error("Issuer validation not configured")` at startup (constructor) if `require_issuer_validation` is true but `expected_issuer` is empty (`jwt_validator.cpp:506`)
- `[ ]` Emit a `spdlog::warn` (audit-level) when either field is unset and the corresponding `require_*` flag is false, so operator misconfiguration is visible in logs
- `[ ]` Add unit test: validate token with correct issuer/audience, wrong issuer, wrong audience, missing issuer, missing audience — all permutations

---

### 6. JWT JTI Replay Prevention Warning When JTI Is Absent

**Priority:** Medium (Security)  
**Target Version:** v1.2.0

`jwt_validator.cpp:446` extracts `jti` but only connects it to the blacklist check at line 559 when `!claims.jti.empty()`. Tokens without a `jti` claim bypass per-token revocation entirely with no warning. This is spec-compliant (JTI is optional per RFC 7519) but dangerous in deployments where revocation is expected.

**Implementation Notes:**
- `[ ]` Add `bool require_jti = false` to `JWTValidator::Config`; when `true`, reject tokens missing `jti` with `throw std::runtime_error("Missing required jti claim")` (`jwt_validator.cpp:446`)
- `[ ]` When `token_blacklist_` is set but incoming token has no `jti`, emit `spdlog::warn("JWT has no jti; per-token revocation impossible for this token")` — operators need visibility (`jwt_validator.cpp:558`)
- `[ ]` Document the `require_jti` flag and its security implications in the module README

---

### 7. Token Blacklist Persistence and Distributed Support

**Priority:** High  
**Target Version:** v1.3.0

`token_blacklist.cpp` stores revoked tokens in `std::unordered_set<std::string> blacklist_` (pure in-memory). On process restart all revoked tokens are forgotten — previously revoked JWT tokens become valid again until they expire naturally. In a multi-node deployment each node maintains an independent blacklist with no cross-node synchronisation.

**Implementation Notes:**
- `[ ]` Define abstract interface `ITokenBlacklist` in `include/auth/token_blacklist.h` with `add(jti, expiry)`, `isRevoked(jti)`, `purgeExpired()` methods
- `[ ]` Implement `RedisTokenBlacklist : ITokenBlacklist` backed by Redis `SET jti EX ttl NX` — use hiredis or redis-plus-plus; see `include/auth/token_blacklist.h`
- `[ ]` Implement `RocksDBTokenBlacklist : ITokenBlacklist` for single-node deployments with persistence — write jti+expiry to a dedicated CF, background thread purges expired entries
- `[ ]` Add Bloom filter pre-check (`libbloom` or hand-rolled) in the in-memory path to reduce hash-map lookups on non-revoked tokens (hot path is `isRevoked` returning `false`)
- `[ ]` Bound in-memory blacklist to `max_entries` (configurable, default 1 million) — evict by earliest expiry when capacity is reached, log a warning
- `[ ]` Unit test: revoke a token, restart process, verify token is still rejected (persistence test); revoke on node A, check on node B (distribution test)

**Performance Targets:**
- `isRevoked()` hot path (non-revoked token, warm Bloom filter): ≤ 1 µs
- Redis-backed `isRevoked()`: ≤ 2 ms P99 on local network

---

### 8. LDAP Connection Pooling

**Priority:** High  
**Target Version:** v1.2.0  
**Status:** ✅ Implemented

`ldap_authenticator.cpp` opens a new LDAP connection (TCP + TLS handshake + bind) for **every authentication call** (`performBind()`, lines 188-286 on Windows; lines 307-395 on POSIX). LDAP connection setup including TLS typically takes 10–50 ms. Under load (e.g., 500 concurrent logins) this exhausts file descriptors and introduces severe latency.

**Implementation Notes:**
- `[x]` Implement `LDAPConnectionPool` class in new `ldap_connection_pool.cpp`: pool of pre-bound `LDAP*` handles, protected by `std::mutex` + `std::condition_variable`, configurable `min_idle`, `max_size`, and `checkout_timeout`
- `[x]` On `authenticate()`, checkout a connection from the pool (blocking up to `checkout_timeout`), perform bind/search, return connection to pool (RAII via `PooledConnection` wrapper)
- `[x]` Implement connection health check: on checkout, test the connection with `ldap_search_ext_s` to `""` base with scope `LDAP_SCOPE_BASE` requesting `supportedLDAPVersion`; evict and re-create if stale
- `[x]` Expose `pool_size`, `idle_connections`, `active_connections` via `auth_metrics.cpp` counters

**Performance Targets:**
- Average LDAP authentication latency reduced from ~30 ms to < 5 ms under sustained load via connection reuse

---

### 9. EC Curve Support: P-384 and P-521 in JWT Validator

**Priority:** Medium  
**Target Version:** v1.3.0

`jwt_validator.cpp:242` hard-checks `crv == "P-256"` for EC keys and returns `false` for any other curve. RFC 7518 defines `ES384` (P-384 / SHA-384) and `ES512` (P-521 / SHA-512) as standard algorithms. Several enterprise IdPs issue ES384 tokens; the current code silently rejects them.

**Implementation Notes:**
- `[ ]` Extend `verifySignatureES256()` into a `verifySignatureEC(algorithm, crv, hash)` dispatcher in `jwt_validator.cpp:239`
- `[ ]` Add P-384 path: `NID_secp384r1`, `EVP_sha384()`, 48-byte r/s coordinates (96-byte raw signature), alg label `ES384`
- `[ ]` Add P-521 path: `NID_secp521r1`, `EVP_sha512()`, 66-byte r/s coordinates (132-byte raw signature), alg label `ES512`
- `[ ]` Update algorithm allow-list check at `jwt_validator.cpp:413` to include `ES384` and `ES512`
- `[ ]` Update `verifySignatureRS256()` to also support `RS384` (SHA-384) and `RS512` (SHA-512) for completeness

---

### 10. Secure Memory for Key Material in `jwks_security.cpp`

**Priority:** High (Security)  
**Target Version:** v1.2.0

`jwks_security.cpp:276` passes `impl_->config.client_key_password` as a plain `std::string` to `CURLOPT_KEYPASSWD`. `std::string` stores content in allocator-managed heap memory that may be swapped to disk, appear in core dumps, or be left in freed pages readable by a later allocation. Similarly, private key bytes loaded into memory in `jwt_key_rotation_manager.cpp` and `totp_secret_encryption.cpp` are held in plain `std::string` or `std::vector<uint8_t>`.

**Implementation Notes:**
- `[ ]` Introduce `SecureString` wrapper (or use `sodium_malloc` / `sodium_mlock` from libsodium) for all password and private-key fields in `JWKSSecurityConfig` (`include/auth/jwks_security.h`) and `TOTPSecretEncryption` (`include/auth/totp_secret_encryption.h`)
- `[ ]` Call `OPENSSL_cleanse()` (or `sodium_memzero()`) on key buffers in destructors of `JWKSSecurityImpl`, `JWTKeyRotationManager`, and `TOTPSecretEncryption` before freeing memory
- `[ ]` Ensure `mlockall(MCL_CURRENT)` or per-allocation `mlock()` is called for pages holding key material on Linux; document Windows equivalent (`VirtualLock`)
- `[ ]` Remove `client_key_password` from any struct that may be serialised, logged, or copied by value

---

### 11. TOTP/MFA: Configurable Window and Audit on Drift

**Priority:** Medium  
**Target Version:** v1.2.0

`mfa_authenticator.cpp:82` enforces only code lengths of 6 or 8 digits. The TOTP time window (number of ± intervals accepted) is not explicitly validated against a minimum/maximum in the public configuration path. A misconfiguration accepting a very wide window (e.g., ±5 steps = ±150 seconds) substantially weakens TOTP replay resistance beyond the `totp_replay_cache.cpp` mitigations.

**Implementation Notes:**
- `[ ]` Add `uint8_t max_window_steps = 1` to `TOTPConfig` in `include/auth/mfa_authenticator.h`; reject configurations where `time_step_window > 2` with `std::invalid_argument` in the constructor (`mfa_authenticator.cpp:82`)
- `[ ]` When a TOTP code validates against a non-zero time step offset (i.e., `step != 0`), emit an audit log entry via `auth_audit_logger.cpp` recording the subject, offset, and timestamp — large sustained offsets indicate a misconfigured device clock
- `[ ]` Expose `totp_drift_histogram` counter in `auth_metrics.cpp` (label: `step_offset`) for operational visibility

---

### 12. Rate Limiter: Distributed State Synchronisation

**Priority:** Medium  
**Target Version:** v1.3.0

`auth_rate_limiter.cpp` uses in-process sliding-window counters protected by `std::mutex`. In a multi-node deployment each node tracks independent counters, so an attacker can bypass per-user or per-IP rate limits by spreading requests across nodes (horizontal bypass).

**Implementation Notes:**
- `[ ]` Define `IRateLimiterBackend` interface with `increment(key) -> count` and `reset(key)` operations
- `[ ]` Implement `RedisRateLimiterBackend` using a Lua atomic increment+expire script (avoids TOCTOU) for centrally consistent sliding-window counts across nodes
- `[ ]` Keep `InMemoryRateLimiterBackend` (current implementation) as the default for single-node deployments
- `[ ]` Add integration test: two in-process rate limiter instances sharing a Redis backend both observe the combined request count

---

### 13. Credential Stuffing Detection: Persistent Cross-Session State

**Priority:** Medium  
**Target Version:** v1.3.0

`auth_rate_limiter.cpp:463` gates credential stuffing detection on `config_.enable_credential_stuffing_detection` but the underlying counters are in-memory only. Cross-session detection (tracking a user across multiple login sessions over hours) requires persisted, time-windowed counters. Currently, process restart resets all detection state.

**Implementation Notes:**
- `[ ]` Store credential-stuffing counters in the same `IRateLimiterBackend` (Redis) with a dedicated key namespace `cs:{user_id}:{day}`
- `[ ]` Implement exponential back-off lock-out: first breach triggers CAPTCHA requirement, second triggers email OTP, third triggers 24-hour account lock (`auth_rate_limiter.cpp:282-300`)
- `[ ]` Expose `credential_stuffing_attempts_total` metric counter in `auth_metrics.cpp` with labels `{user_id, ip, outcome}`

---

### 14. Zero-Trust Continuous Verification: Async Policy Re-evaluation

**Priority:** Medium  
**Target Version:** v1.4.0

`zero_trust_auth_verifier.cpp` currently performs synchronous policy evaluation. For long-lived connections (WebSocket, gRPC streaming, DB connection pool), the zero-trust posture of a session must be re-evaluated periodically without dropping the connection.

**Implementation Notes:**
- `[ ]` Add `std::chrono::seconds re_evaluation_interval{300}` to `ZeroTrustConfig` in `include/auth/zero_trust_auth_verifier.h`
- `[ ]` Implement background re-evaluation loop: per-session timer fires every `re_evaluation_interval`; if policy check fails, signal the session manager to revoke the session via `session_manager.cpp:terminateSession()`
- `[ ]` Re-evaluation must not block the data-plane thread; dispatch to `AuthWorkerThreadPool` (see Feature 2)
- `[ ]` Emit audit event `zero_trust/re_evaluation_failed` via `auth_audit_logger.cpp` when continuous check revokes an active session

---

### 15. SAML Assertion Encryption Support

**Priority:** Low  
**Target Version:** v1.4.0

`saml_authenticator.cpp` validates signed SAML assertions but currently returns `{}` (empty) in the `extractAttributes()` path (lines 428, 443) when the assertion is encrypted. Encrypted SAML assertions (`<EncryptedAssertion>`) are the default in high-security SAML deployments.

**Implementation Notes:**
- `[ ]` Implement `decryptAssertion(xmlDocPtr doc, const std::string& sp_private_key_pem)` using `xmlSecOpenSSLInit()` and `xmlSecEncCtxDecrypt()` in `saml_authenticator.cpp`
- `[ ]` Load SP private key from HSM/key-store, not from plain-text PEM config field
- `[ ]` After decryption, pass the plaintext assertion through the existing signature verification path (`saml_authenticator.cpp:326-407`)
- `[ ]` Return `{}` with an explicit error code (not silently) when decryption fails — current silent empty return hides misconfiguration

---

### 16. Federated Identity Manager: Token Exchange (RFC 8693)

**Priority:** Low  
**Target Version:** v1.4.0

`federated_identity_manager.cpp:187` returns `false` in the token exchange path. RFC 8693 (OAuth 2.0 Token Exchange) is required for service-to-service impersonation and delegation in federated scenarios.

**Implementation Notes:**
- `[ ]` Implement `exchangeToken(subject_token, subject_token_type, requested_token_type)` in `federated_identity_manager.cpp` calling the IdP's `token_endpoint` with `grant_type=urn:ietf:params:oauth:grant-type:token-exchange`
- `[ ]` Validate the returned token through the existing `JWTValidator` pipeline
- `[ ]` Scope the exchanged token to the minimum required permissions for the target service

---

## Production Readiness Checklist

- `[ ]` Thread-sanitizer (TSAN) clean under 64-thread concurrent `JWTValidator::validate()` load (Feature 1)
- `[ ]` LDAP injection fuzz-test passes with 1,000,000 adversarial username inputs (Feature 3)
- `[ ]` All secret comparison paths verified constant-time via valgrind/memcheck + microbenchmark (Feature 4)
- `[ ]` Token blacklist survives process restart and is synchronised across 2+ nodes (Feature 7)
- `[ ]` LDAP connection pool stress-tested at 500 concurrent authentications (Feature 8)
- `[ ]` All `expected_issuer`/`expected_audience` empty-string misconfiguration cases covered by integration tests (Feature 5)
- `[ ]` Key material pages locked with `mlock()` / `VirtualLock()` — verified via `/proc/self/smaps` (Feature 10)
- `[ ]` Rate limiter cross-node bypass attack tested with two nodes sharing Redis backend (Feature 12)

---

## Known Issues & Limitations

- **`JWTValidator` JWKS cache is not mutex-protected** — data race under concurrent validation when cache expires (`jwt_validator.cpp:192-193`). Workaround: use one `JWTValidator` instance per thread until fixed.
- **LDAP calls are synchronous** — `ldap_simple_bind_s` / `ldap_search_ext_s` block the calling thread. Workaround: call LDAP auth from a dedicated thread pool at the consumer side.
- **LDAP DN injection** — `buildUserDN()` in `ldap_authenticator.cpp` inserts raw username without RFC 4514 escaping. Do not expose LDAP authentication endpoints to untrusted networks without upstream input validation until Feature 3 is complete.
- **Token blacklist is not persistent** — revoked tokens become valid after process restart until natural expiry. Workaround: keep JWT `exp` TTL short (≤ 15 minutes) for sensitive operations.
- **`expected_issuer` / `expected_audience` are optional** — a misconfigured empty string silently skips validation (`jwt_validator.cpp:506`, `jwt_validator.cpp:345`). Always set both fields in production configuration.
- **EC JWT validation limited to P-256** — ES384 and ES512 tokens from enterprise IdPs are silently rejected (`jwt_validator.cpp:242`).
- **SAML encrypted assertions return empty silently** — no error surfaced to caller (`saml_authenticator.cpp:428`, `443`).

---

## Breaking Changes

- Feature 5 (mandatory issuer/audience): adding `require_issuer_validation = true` as default is a **breaking configuration change** — deployments with empty `expected_issuer` will fail to start. Must be gated behind an explicit opt-in flag for one release cycle.
- Feature 7 (ITokenBlacklist interface): changes `TokenBlacklist` from a concrete class to an interface. Callers that construct `TokenBlacklist` directly must be updated to use factory methods.
- Feature 2 (async LDAP): `LDAPAuthenticator::authenticate()` signature change from synchronous return to `std::future<LDAPAuthResult>` is a breaking API change for all consumers.

---

## RFC / Standard Compliance Targets

- ✅ RFC 6238 (TOTP)
- ✅ RFC 7519 (JWT)
- ✅ RFC 7517 (JWK)
- ✅ RFC 7518 (JWA) — RS256, ES256, EdDSA; **`[ ]` ES384, ES512, RS384, RS512 pending**
- ✅ RFC 4120 (Kerberos v5)
- ✅ OpenID Connect Core 1.0
- ✅ RFC 7636 (PKCE)
- ✅ RFC 8628 (Device Flow)
- `[ ]` RFC 8693 (Token Exchange) — partially stubbed in `federated_identity_manager.cpp:187`
- `[ ]` RFC 4514 (LDAP DN string representation / escaping)
- `[ ]` RFC 4515 (LDAP filter string representation / escaping)
