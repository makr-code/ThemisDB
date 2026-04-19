> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

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
**Status:** ✅ Implemented (v1.7.0)

`JWTValidatorConfig` now uses `std::optional<std::string>` for `expected_issuer` and `expected_audience` with `bool require_issuer_validation = true` / `bool require_audience_validation = true` flags. The constructor throws `std::runtime_error("Issuer validation not configured")` when a require flag is true but the field is unset.

**Implementation Notes:**
- `[x]` In `JWTValidator::Config` (`jwt_validator.h:91-99`), replaced `expected_issuer`/`expected_audience` plain strings with `std::optional<std::string>` and added `require_issuer_validation` / `require_audience_validation` flags
- `[x]` In the `JWTValidator(JWTValidatorConfig)` constructor, throw `std::runtime_error` if `require_issuer_validation` is true but `expected_issuer` is unset
- `[x]` Emit a `spdlog::warn` when either field is unset and the corresponding `require_*` flag is false
- `[x]` Unit tests: validate token with correct/wrong issuer, correct/wrong audience, missing issuer, missing audience

---

### 6. JWT JTI Replay Prevention Warning When JTI Is Absent

**Priority:** Medium (Security)
**Target Version:** v1.2.0
**Status:** ✅ Implemented (v1.7.0)

`JWTValidatorConfig` now has `bool require_jti = false`. When `token_blacklist_` is set and the incoming token has no `jti`, a one-time `spdlog::warn` is emitted (guarded by `warned_blacklist_no_jti_` atomic flag to prevent per-request log flooding).

**Implementation Notes:**
- `[x]` Added `bool require_jti = false` to `JWTValidator::Config` (`jwt_validator.h:100`)
- `[x]` When `require_jti` is true and token has no `jti`, reject with `std::runtime_error("Missing required jti claim")`
- `[x]` When `token_blacklist_` is set but token has no `jti`, emit one-time `spdlog::warn` via `warned_blacklist_no_jti_` atomic flag

---

### 7. Token Blacklist Persistence and Distributed Support

**Priority:** High
**Target Version:** v1.3.0
**Status:** ✅ Implemented (v1.7.0)

`ITokenBlacklist` abstract interface with three implementations: in-memory `TokenBlacklist` (Bloom filter pre-check, bounded `max_entries`), `RedisTokenBlacklist` (Redis `SET jti EX ttl NX`), and `RocksDBTokenBlacklist` (single-node persistence with background expiry thread).

**Implementation Notes:**
- `[x]` Defined abstract `ITokenBlacklist` interface in `include/auth/token_blacklist.h` with `add(jti, expiry)`, `isRevoked(jti)`, `purgeExpired()`
- `[x]` Implemented `RedisTokenBlacklist : ITokenBlacklist` backed by Redis `SET jti EX ttl NX` (`src/auth/redis_token_blacklist.cpp`)
- `[x]` Implemented `RocksDBTokenBlacklist : ITokenBlacklist` with dedicated CF and background expiry thread (`src/auth/rocksdb_token_blacklist.cpp`)
- `[x]` Added hand-rolled Bloom filter (FNV-1a + djb2 double-hashing, 7 hash probes, ~1% false-positive rate) for non-revoked fast path in `TokenBlacklist`
- `[x]` Bounded `TokenBlacklist` to `max_entries = 1,000,000` with earliest-expiry eviction

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
**Status:** ✅ Implemented (v1.8.0)

`jwt_validator.cpp` now supports ES384 (P-384/SHA-384), ES512 (P-521/SHA-512), RS384 (RSA/SHA-384), and RS512 (RSA/SHA-512) in addition to the existing ES256, RS256, and EdDSA.

**Implementation Notes:**
- `[x]` Refactored `verifySignatureES256()` to delegate to new `verifySignatureEC(alg, jwk)` dispatcher that selects curve (`NID_X9_62_prime256v1` / `NID_secp384r1` / `NID_secp521r1`), coordinate size (32 / 48 / 66 bytes), and digest (`EVP_sha256()` / `EVP_sha384()` / `EVP_sha512()`) based on the `alg` parameter
- `[x]` Added `verifySignatureRSA(alg, jwk)` dispatcher that selects SHA-256/384/512 for RS256/RS384/RS512; `verifySignatureRS256()` now delegates to it
- `[x]` Updated algorithm allow-list in `parseAndValidate()` to include RS384, RS512, ES384, ES512
- `[x]` Updated dispatch block to call `verifySignatureRSA` for RS256/RS384/RS512 and `verifySignatureEC` for ES256/ES384/ES512
- `[x]` Added comprehensive test coverage in `tests/test_jwt_ec_curves_comprehensive.cpp` (ES384 happy-path, ES512 happy-path, expired token, wrong signature, tampered payload, kid revocation, cross-curve attacks, RS384, RS512)

---

### 10. Secure Memory for Key Material in `jwks_security.cpp`

**Priority:** High (Security)
**Target Version:** v1.2.0
**Status:** ✅ Implemented (v1.7.0)

`SecureString` and `SecureVector` wrappers in `include/auth/secure_memory.h` use `OPENSSL_cleanse()` on destruction and call `mlock()` / `VirtualLock()` to prevent key material from being swapped to disk.

**Implementation Notes:**
- `[x]` `SecureString` / `SecureVector<T>` wrappers in `include/auth/secure_memory.h` with `OPENSSL_cleanse()` in destructor
- `[x]` `mlock()` (Linux/macOS) and `VirtualLock()` (Windows) called in `secure_mlock()` helper
- `[x]` Key material fields in `JWKSSecurityConfig`, `JWTKeyRotationManager`, and `TOTPSecretEncryption` use `SecureString`
- `[x]` `client_key_password` excluded from any serialised or logged structs

---

### 11. TOTP/MFA: Configurable Window and Audit on Drift

**Priority:** Medium
**Target Version:** v1.2.0
**Status:** ✅ Implemented (v1.7.0)

`MFAAuthenticator::Config` now has `uint8_t max_window_steps = 1` capped at an absolute hard limit of 2. The constructor rejects configurations where `time_window > max_window_steps` via `std::invalid_argument`. Non-zero step offsets emit audit log entries via `auth_audit_logger.cpp`.

**Implementation Notes:**
- `[x]` Added `uint8_t max_window_steps = 1` to `MFAAuthenticator::Config` (`mfa_authenticator.h:78`)
- `[x]` Constructor enforces `time_window <= std::min(max_window_steps, uint8_t{2})`
- `[x]` Non-zero TOTP step offset emits audit event with subject, offset, and timestamp
- `[x]` `totp_drift_histogram` counter exposed in `auth_metrics.cpp`

---

### 12. Rate Limiter: Distributed State Synchronisation

**Priority:** Medium
**Target Version:** v1.3.0
**Status:** ✅ Implemented (v1.7.0)

`IRateLimiterBackend` abstract interface in `include/auth/rate_limiter_backend.h` with two built-in implementations: `InMemoryRateLimiterBackend` (single-node default) and `RedisRateLimiterBackend` (multi-node; atomic Lua sliding-window script).

**Implementation Notes:**
- `[x]` Defined `IRateLimiterBackend` interface with `increment(key, window)`, `getCount(key, window)`, `reset(key)` (`include/auth/rate_limiter_backend.h`)
- `[x]` Implemented `RedisRateLimiterBackend` using atomic Lua sorted-set script (ZREMRANGEBYSCORE + ZADD + EXPIRE + ZCARD), avoiding TOCTOU
- `[x]` `InMemoryRateLimiterBackend` kept as default for single-node deployments
- `[x]` Integration test in `tests/test_auth_rate_limiter_distributed.cpp`: two instances sharing a backend observe combined request count

---

### 13. Credential Stuffing Detection: Persistent Cross-Session State

**Priority:** Medium
**Target Version:** v1.3.0

`auth_rate_limiter.cpp:463` gates credential stuffing detection on `config_.enable_credential_stuffing_detection` but the underlying counters are in-memory only. Cross-session detection (tracking a user across multiple login sessions over hours) requires persisted, time-windowed counters. Currently, process restart resets all detection state.

**Implementation Notes:**
- `[x]` Store credential-stuffing counters in the same `IRateLimiterBackend` (Redis) with a dedicated key namespace `cs:{user_id}:{day}`
- `[x]` Implement exponential back-off lock-out: first breach triggers CAPTCHA requirement, second triggers email OTP, third triggers 24-hour account lock (`auth_rate_limiter.cpp:282-300`)
- `[x]` Expose `credential_stuffing_attempts_total` metric counter in `auth_metrics.cpp` with labels `{user_id, ip, outcome}`

---

### 14. Zero-Trust Continuous Verification: Async Policy Re-evaluation

**Priority:** Medium
**Target Version:** v1.4.0

`zero_trust_auth_verifier.cpp` currently performs synchronous policy evaluation. For long-lived connections (WebSocket, gRPC streaming, DB connection pool), the zero-trust posture of a session must be re-evaluated periodically without dropping the connection.

**Implementation Notes:**
- `[x]` Add `std::chrono::seconds re_evaluation_interval{300}` to `ZeroTrustConfig` in `include/auth/zero_trust_auth_verifier.h`
- `[x]` Implement background re-evaluation loop: per-session timer fires every `re_evaluation_interval`; if policy check fails, signal the session manager to revoke the session via `session_manager.cpp:terminateSession()`
- `[x]` Re-evaluation must not block the data-plane thread; dispatch to `AuthWorkerThreadPool` (see Feature 2)
- `[x]` Emit audit event `zero_trust/re_evaluation_failed` via `auth_audit_logger.cpp` when continuous check revokes an active session

**Status:** `[x]` Implemented in v1.4.0

---

### 15. SAML Assertion Encryption Support

**Priority:** Low
**Target Version:** v1.4.0
**Status:** ✅ Implemented (v1.4.0)

Encrypted SAML assertions (`<EncryptedAssertion>`) are now supported. The implementation uses OpenSSL (AES-128-CBC / AES-256-CBC with RSA-OAEP or RSA-PKCS1-v1.5 key transport) and the pugixml parser that is already a codebase dependency, avoiding the need for a separate xmlsec library.

**Implementation Notes:**
- `[x]` Implement `decryptAssertion(const pugi::xml_node& encrypted_assertion_node)` using OpenSSL EVP APIs (RSA-OAEP + AES-CBC) in `saml_authenticator.cpp`
- `[x]` Load SP private key via `SAMLConfig::sp_private_key_loader` callback (not a plain-text PEM config field) — callers supply a closure that loads the key from their HSM, KMS, or secrets manager
- `[x]` After decryption, the plaintext assertion XML is re-parsed with pugixml and passed through the existing signature verification path in `processResponseImpl()`
- `[x]` All decryption failures throw `AuthErrorCode::SAML_DECRYPTION_FAILED` with an explicit diagnostic message — no silent empty return

---

### 16. Federated Identity Manager: Token Exchange (RFC 8693)

**Priority:** Low
**Target Version:** v1.4.0
**Status:** ✅ Implemented (v1.4.0)

`FederatedIdentityManager::exchangeToken()` implements RFC 8693 (OAuth 2.0 Token Exchange) for service-to-service impersonation and delegation in federated scenarios.

**Implementation Notes:**
- `[x]` Implement `exchangeToken(subject_token, subject_token_type, requested_token_type)` in `federated_identity_manager.cpp` calling the IdP's `token_endpoint` with `grant_type=urn:ietf:params:oauth:grant-type:token-exchange`
- `[x]` Validate the returned token through the existing `JWTValidator` pipeline
- `[x]` Scope the exchanged token to the minimum required permissions for the target service

---

## Production Readiness Checklist

- `[x]` Thread-sanitizer (TSAN) clean under 64-thread concurrent `JWTValidator::validate()` load (Feature 1)
- `[x]` LDAP injection fuzz-test passes with 1,000,000 adversarial username inputs (Feature 3)
- `[x]` All secret comparison paths verified constant-time via valgrind/memcheck + microbenchmark (Feature 4)
- `[x]` Token blacklist survives process restart and is synchronised across 2+ nodes (Feature 7)
- `[x]` LDAP connection pool stress-tested at 500 concurrent authentications (Feature 8)
- `[x]` All `expected_issuer`/`expected_audience` empty-string misconfiguration cases covered by integration tests (Feature 5)
- `[x]` Key material pages locked with `mlock()` / `VirtualLock()` — verified via `/proc/self/smaps` (Feature 10)
- `[x]` Rate limiter cross-node bypass attack tested with two nodes sharing Redis backend (Feature 12)
- `[x]` ES384 (P-384), ES512 (P-521), RS384, RS512 JWT validation tested with comprehensive test suite (Feature 9)

---

## Known Issues & Limitations

- **`JWTValidator` JWKS cache is mutex-protected** — `std::shared_mutex jwks_cache_mutex_` guards all reads/writes; double-checked locking via `jwks_refresh_mutex_` prevents thundering-herd stampedes. ✅ Fixed in v1.7.0.
- **LDAP calls are non-blocking** — `ldap_simple_bind_s` / `ldap_search_ext_s` execute on the `AuthWorkerThreadPool`; callers receive `std::future<LDAPAuthResult>`. ✅ Fixed in v1.7.0.
- **LDAP DN injection** — `buildUserDN()` in `ldap_authenticator.cpp` now uses RFC 4514 `escapeLDAPDNComponent()` and RFC 4515 `escapeLDAPFilterValue()`. ✅ Fixed in v1.7.0.
- **Token blacklist is now persistent** — `RedisTokenBlacklist` and `RocksDBTokenBlacklist` provide cross-restart and cross-node persistence. ✅ Fixed in v1.7.0.
- **`expected_issuer` / `expected_audience` are now mandatory by default** — `require_issuer_validation = true` / `require_audience_validation = true` throw at construction if the field is unset. Deployments using the legacy URL-only constructor have both require flags set to `false` for backward compatibility. ✅ Fixed in v1.7.0.
- **EC JWT validation now supports P-256, P-384 (ES384), and P-521 (ES512)** as well as RSA RS384/RS512. ✅ Fixed in v1.8.0.
- **SAML encrypted assertions return empty silently** — no error surfaced to caller (`saml_authenticator.cpp:428`, `443`). ✅ Fixed in v1.4.0 (see Feature 15).

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
- ✅ RFC 7518 (JWA) — RS256, RS384, RS512, ES256, ES384, ES512, EdDSA
- ✅ RFC 4120 (Kerberos v5)
- ✅ OpenID Connect Core 1.0
- ✅ RFC 7636 (PKCE)
- ✅ RFC 8628 (Device Flow)
- `[ ]` RFC 8693 (Token Exchange) — partially stubbed in `federated_identity_manager.cpp:187`
- `[x]` RFC 4514 (LDAP DN string representation / escaping) — `escapeLDAPDNComponent()` in `ldap_authenticator.cpp`
- `[x]` RFC 4515 (LDAP filter string representation / escaping) — `escapeLDAPFilterValue()` in `ldap_authenticator.cpp`
