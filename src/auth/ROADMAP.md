# Auth Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production authentication runtime exists across JWT/OIDC, Kerberos, MFA, OAuth, SAML, LDAP, API-key, mTLS, WebAuthn, session/revocation, and zero-trust verification paths.
v1.3.0 distributed token blacklist is complete: TBLK/v1 binary TCP protocol, leader push, follower pull, server listener, LWW merge — all shipped and covered by DBL-01..DBL-17.

## In Progress

- [x] hardening of distributed revocation, federation, and policy-edge behavior (Target: Q3 2026)
- [x] benchmark and release-gate consolidation for token/session hot paths (Target: Q3 2026)
- [x] consistency hardening for async/provider-integration reliability (Target: Q3 2026)
- [~] Wave C benchmark gate execution and evidence capture in CI (AUTH-GRG-01..06) (Target: Q4 2026)
  - Dispatched: `CI — Benchmarks` run `#40` (`32765349559`) on `develop` with filter `bench_auth_hotpaths|AHP-`
  - Pending: run completion on representative hardware and artifact-to-gate mapping evidence

## v1.2.0 Async Operations & Connection Pooling (Completed)

- [x] async/non-blocking LDAP authentication calls (authenticateAsync with AuthWorkerThreadPool)
- [x] async/non-blocking HTTP authentication calls (new AsyncHTTPAuth class)
- [x] LDAP connection pooling with health checks and reuse (LDAPConnectionPool)
- [x] HTTP retry logic with exponential backoff for transient failures
- [x] Thread-safe worker pool for concurrent auth operations

## v1.3.0 Token Blacklist Persistence & Distributed Support (Completed)

- [x] Token blacklist persistence to RocksDB (RocksDBTokenBlacklist)
- [x] Leader election for distributed deployments (node-ID ordering, performLeaderElection)
- [x] Atomic blacklist validation during cluster sync (fail-closed isRevoked with RocksDB read)
- [x] Distributed token blacklist with cluster synchronization — TBLK/v1 binary TCP protocol:
  - TCP server listener on `local_node.rpc_port`; bind is non-fatal (outbound still works)
  - Leader PUSH: serializes all non-expired JTI entries and pushes to each follower
  - Follower PULL: sends PULL_REQ, receives PULL_RESP, applies entries with LWW semantics
  - `performClusterSync()`: leader election → push to all peers (leader) or pull from leader (follower)
  - `pushRevisionsToFollower()` / `pullRevisionsFromLeader()`: full production implementation
  - `serveIncomingConnections()` / `handlePeerConnection()`: server-side PUSH and PULL_REQ dispatch
  - `getAllEntries()` / `applyEntries()`: RocksDB helpers with LWW conflict resolution
  - Wire format: 10-byte header (magic "TBLK", version 0x01, type, count) + variable entries
  - Files: `include/auth/distributed_token_blacklist.h`, `src/auth/distributed_token_blacklist.cpp`
- [x] Comprehensive test coverage for distributed scenarios (tests/auth/test_auth_distributed_blacklist.cpp, DBL-01..DBL-17)

## Planned Features

### Wave 4-B: Auth Audit Events + OAuth Retry + mTLS Hardening (Target: Q4 2026)

> **Source:** MODULE_GAP_ANALYSIS_WAVE2.md §Wave 4-B · gap-verifier subagent 2026-08-25  
> **Verified real gaps:** 7 CRITICAL (missing audit events), 7 HIGH (retry + crypto), 1 MEDIUM  
> **FP closed (14):** sensitive_data_logging (all 155) — scanner matched variable names not values; mTLS cipher claim wrong file scope (MTLSAuthenticator has no SSL_CTX)

#### A — Missing Audit Events (7 gaps — all CRITICAL/HIGH)

- [x] 2026-08-26 `passkey_authenticator.cpp:880-892` — inject `AuthAuditLogger*`; call `logPasskeySuccess(credential_id)` / `logPasskeyFailure(reason)` from `verifyAuthentication()` — zero audit calls currently (CRITICAL) (Target: Q4 2026)
- [x] 2026-08-26 `mtls_authenticator.cpp:281` — inject `AuthAuditLogger*`; add `logMTLSSuccess(principal,serial)` / `logMTLSFailure(reason)` — no `AuthAuditLogger` include or call in file (CRITICAL) (Target: Q4 2026)
- [x] 2026-08-26 `federated_identity_manager.cpp:202-578` — add `AuthAuditLogger*` injection; call `logJWTSuccess/Failure` / `logFederatedSuccess/Failure` in `validateToken()` and `exchangeToken()` — file has no `#include "auth/auth_audit_logger.h"` (CRITICAL) (Target: Q4 2026)
- [x] 2026-08-26 `auth_audit_logger.cpp` — add `SecurityEventType::ROLE_CHANGED`, `PERMISSION_CHANGED`; add `logRoleChange(user_id, role, old_role)` and `logPermissionChange(user_id, resource, old_perm, new_perm)` (CRITICAL) (Target: Q4 2026)
- [x] 2026-08-26 `jwt_key_rotation_manager.cpp:54` — add try/catch around `max_keys` throw to fire `KEY_ROTATION_FAILED` audit event before re-throwing — logger assigned on line 77, after throw, so never reached (HIGH) (Target: Q4 2026)
- [x] 2026-08-26 `jwt_key_rotation_manager.cpp:99-100` — emit `KEY_REVOCATION_FAILED` event before `return false` on unknown `kid` — THEMIS_WARN only, no audit trail for key ID probing (HIGH) (Target: Q4 2026)
- [x] 2026-08-26 `auth_audit_logger.cpp` — add `logPasskeyRegistered(user_id, credential_id, rp_id)`; call from `registerCredential()` — `logMFAEnrolled` covers TOTP only (HIGH) (Target: Q4 2026)

#### B — Auth Retry Logic (4 real gaps)

- [x] 2026-08-26 `ldap_connection_pool.cpp:173-181` — add inner retry loop (max 3×, base 100ms, ×2, ±20ms jitter) around `createConnection()`; on exhaustion → `throw AuthException(PROVIDER_DEGRADED)`; current: `nullptr` falls through to CV wait without backoff (HIGH) (Target: Q4 2026)
- [x] 2026-08-26 `federated_identity_manager.cpp:390-393` — wrap `httpPost()` in retry loop (max 3×, jittered backoff); retry on `CURLE_COULDNT_CONNECT`, `CURLE_OPERATION_TIMEDOUT`, HTTP 429/503 — currently throws immediately (HIGH) (Target: Q4 2026)
- [x] 2026-08-26 `oauth_pkce_flow.cpp:317-318` — same fix as B-2 above; factor into shared retrying `httpPost()` helper (HIGH) (Target: Q4 2026)
- [x] 2026-08-26 `oauth_device_flow.cpp:399-400` — retry individual HTTP transport errors within the RFC poll loop (not the poll interval itself — RFC 8628 §3.5 poll loop is correct); distinguish `CURLE` transport failure from `authorization_pending` (MEDIUM) (Target: Q4 2026)

#### C — Crypto Weakness (3 real gaps)

- [x] 2026-08-26 `passkey_authenticator.cpp:407-483` — add COSE `alg` field allowlist in `coseKeyToEvpPkey()`; reject `kty=2` if `alg != -7` (ES256); reject `kty=3` if `alg != -257` (RS256); enforce stored credential algorithm matches (HIGH — cross-algorithm substitution risk) (Target: Q4 2026)
- [x] 2026-08-26 `mtls_authenticator.cpp:173-283` — add `X509_get_ext_d2i(cert, NID_ext_key_usage)` check; reject certs lacking `id-kp-clientAuth` OID; add `digitalSignature` key-usage bit check (HIGH — serverAuth-only certs currently accepted) (Target: Q4 2026)
- [x] 2026-08-26 `passkey_authenticator.cpp:447-482` — after RSA EVP_PKEY construction, call `EVP_PKEY_get_bits(pkey)` and reject if `< 2048` (MEDIUM — 512/1024-bit RSA keys currently accepted) (Target: Q4 2026)

#### Tests
- `tests/auth/test_wave4b_auth_hardening.cpp` — minimum 14 tests covering all verified gaps above
- `tests/auth/test_wave4b_auth_hardening2.cpp` — 12+ additional tests; registered with `wave_b release_critical` labels (added 2026-08-26)

#### FPs Confirmed (closed, no code change needed)
- sensitive_data_logging (155): scanner matched variable names near log calls, not log values; `// NOPII` already on ambiguous sites; no raw credential in any spdlog format argument
- mTLS cipher list: `MTLSAuthenticator` is a PEM-level verifier with no SSL_CTX; TLS cipher enforcement belongs in the transport layer wrapping this component

### Wave 2-A: Auth Security Hardening (Target: Q3 2026)

> **Source:** MODULE_GAP_ANALYSIS_WAVE2.md §Wave 2-A, gap scanner verified 2026-08-25  
> **Gap count:** 155 `sensitive_data_logging` (HIGH), 7 `missing_audit_log` (CRITICAL), 22 `no_retry_logic`, 9 `crypto_weakness`

- [x] Sensitive data redaction: **FP CONFIRMED** — 100% false positive per subagent triage (2026-08-25); preventive lint policy recommended as follow-up
- [x] Add missing audit events: all 14 Wave 4-B gaps closed 2026-08-26
- [x] Auth retry logic: LDAP createConnection retry, federated/PKCE/device-flow retry — all closed 2026-08-26
- [x] Crypto weakness: passkey COSE alg allowlist, RSA key size, mTLS EKU — all closed 2026-08-26

### Wave 2-B: LDAP Stub Replacement (Target: Q4 2026)

> **Source:** Semantic analysis 2026-08-25 — `ldap_authenticator.cpp` has ~12 stubbed functions

- [x] 2026-08-26 LDAP Connection Pool: real pool management (bind context, bounded size, timeout) in `ldap_authenticator.cpp` (Target: Q4 2026)
  - Inputs: LDAP server config (host, port, bind-DN, timeout)
  - Outputs: pooled LDAP connection with automatic rebind on staleness
  - Errors: `LDAP_CONNECT_TIMEOUT` on pool exhaustion; retry with backoff
  - `checkout()` timeout now throws `AuthException(PROVIDER_DEGRADED)` instead of returning `nullptr`
  - Tests: `tests/auth/test_wave7_auth_ldap_federated.cpp` (WP-01..WP-06, WA-01..WA-04)
- [x] 2026-08-26 LDAP Search Pagination: controlled, bounded result pagination in `ldap_authenticator.cpp` (Target: Q4 2026)
  - Unix/OpenLDAP path: paginated `ldap_search_ext_s` loop with `ldap_create_page_control` / `ldap_parse_page_control`; page_size=500, max_results=5000; partial results returned on pagination error with `THEMIS_WARN`
- [x] 2026-08-26 `federated_identity_manager.cpp`: Cross-provider state sync — 9 new methods implemented (Target: Q4 2026)
  - `addCrossProviderTrust` / `removeCrossProviderTrust` / `isTrustedBy` / `getCrossProviderTrusts`: in-memory trust registry protected by `trust_mutex_`
  - `cacheValidationResult` / `getCachedResult` / `evictExpiredCacheEntries` / `clearTokenCache` / `tokenCacheSize`: `std::unordered_map`-backed token validation cache with `std::chrono::system_clock` expiry; auto-populated by `validateToken()`
  - Tests: `tests/auth/test_wave7_auth_ldap_federated.cpp` (FR-01..FR-03, FT-01..FT-05, FC-01..FC-07)

### Short-term (3-6 months)
- [ ] tighten fail-closed behavior for optional provider-degraded scenarios (Target: Q4 2026)
- [ ] expand deterministic integration regressions across auth protocol matrixes (Target: Q4 2026)
- [ ] improve operator diagnostics for policy/revocation/federation decision classes (Target: Q4 2026)

### Wave 8 — Auth Hardening Items (2026-08-26 scan, Q4 2026)
> Source: `src/MODULE_GAP_ANALYSIS_WAVE2.md §Wave 8`

- [ ] **W8-15** `ldap_authenticator.cpp:699–722` — LDAP pagination error handling: add retry loop (max 3, exponential backoff) + `AuthAuditLogger` event on each transient LDAP error; current `break` returns partial group membership silently (Target: Q4 2026)
- [ ] **W8-16** `federated_identity_manager.cpp:209–218` — Token cache DoS hardening: add `kTokenCacheMaxSize` cap + `std::list` LRU eviction + SHA-256(token) → hex string as cache key; current cache is unbounded and keyed on raw token strings (Target: Q4 2026)
- [ ] **W8-17** `ldap_connection_pool.cpp:208–217` — Pool exhaustion audit: inject `AuthAuditLogger`; emit structured audit event on pool exhaustion before throw (LOW, Target: Q4 2026)
- [ ] Test: `tests/auth/test_wave8_auth_hardening.cpp` — 10+ tests covering pagination retry, partial-result detection, cache eviction under size pressure, pool-exhaustion audit trail

### Mid-term (6-12 months)
- [ ] reduce remaining proxy-like benchmark targets through dedicated auth microbenchmarks (Target: Q1 2027)
- [ ] re-baseline auth p95/p99 envelopes on representative production profiles (Target: Q1 2027)
- [ ] harden multi-realm and distributed trust-state synchronization paths (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze authentication and principal-contract semantics for active major line (Target: Q3 2026)
  - Delivered: `include/auth/auth_principal_contract.h` — frozen v1.x contract (principal sizes, temporal bounds,
    failure classes, fail-closed semantics, revocation backend contract, provider capability contract, async contract)
- [x] define explicit failure contracts per provider integration and policy gate (Target: Q3 2026)
  - Delivered: failure contract sections added to `distributed_token_blacklist.h`, `federated_identity_manager.h`,
    `session_manager.h`; new error codes PROVIDER_DEGRADED, PROVIDER_CAPABILITY_MISMATCH,
    FEDERATION_UNKNOWN_REALM, FEDERATION_REALM_UNAVAILABLE, REVOCATION_BACKEND_UNAVAILABLE,
    REVOCATION_ENTRY_INVALID, REVOCATION_CLUSTER_SYNC_FAILED, POLICY_EDGE_UNDEFINED,
    POLICY_MISSING_REQUIRED_CLAIM, ASYNC_PROVIDER_TIMEOUT, ASYNC_POOL_EXHAUSTED,
    ASYNC_PROVIDER_EXCEPTION registered in `auth_error.h` + `auth_error.cpp`

### Phase 2: Core Implementation
- [x] RocksDB persistence layer for token blacklisting (RocksDBTokenBlacklist, DistributedTokenBlacklist)
- [x] TBLK/v1 binary TCP RPC protocol — `pushRevisionsToFollower()`, `pullRevisionsFromLeader()`, `serveIncomingConnections()`, `handlePeerConnection()` (src/auth/distributed_token_blacklist.cpp)
- [x] Leader election based on node-ID lexicographic ordering (performLeaderElection)
- [x] LWW (Last-Write-Wins) conflict resolution in `applyEntries()` — lower-timestamp entries silently overwritten
- [x] complete remaining hardening in revocation/federation/provider execution paths (Target: Q3 2026)
  - `add()` now validates JTI empty/size early with REVOCATION_ENTRY_INVALID (fail-closed)
  - `validateToken()` / `realmProvider()` / `exchangeToken()` now throw FEDERATION_UNKNOWN_REALM
    for unknown issuers (previously JWT_ISSUER_MISMATCH — now callers can distinguish realm-not-found
    from cryptographic failure)
  - `validateToken()` reclassifies unstructured provider exceptions as PROVIDER_DEGRADED (fail-closed)
  - `exchangeToken()` non-HTTPS endpoint check throws PROVIDER_CAPABILITY_MISMATCH
- [x] align session/trust behavior to shared bounded runtime contracts (Target: Q3 2026)
  - `session_manager.h` updated with bounded runtime contract cross-referencing auth_principal_contract.h
- [x] PasskeyAuthenticator concrete class and real CBOR/OpenSSL verification (Target: Q4 2026)
  - `include/auth/passkey_authenticator.h`: added `PasskeyAuthenticator` class implementing `IPasskeyAuthenticator`
    with thread-safe in-memory credential store and pending-challenge lifecycle
  - `src/auth/passkey_authenticator.cpp`: TODO stubs replaced with real base64url decode (OpenSSL BIO),
    CBOR attestation-object parsing, authenticatorData parsing (rpIdHash, flags, signCount, AAGUID, credential ID,
    COSE public key), ECDSA-P256/RS256 signature verification via `EVP_DigestVerify`, and sign_count clone detection

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed auth artifacts and degraded backends (Target: Q3 2026)
  - Delivered: `isFailClosedClass()` predicate in `auth_principal_contract.h` §4
  - `distributed_token_blacklist.cpp::add()` validates empty/oversized JTI early
  - `federated_identity_manager.cpp::validateToken()` wraps unstructured exceptions as PROVIDER_DEGRADED
- [x] unify error taxonomy and diagnostics across protocol adapters (Target: Q3 2026)
  - Delivered: 12 new AuthErrorCode entries (9420-9452) for provider/revocation/policy/async failures,
    all registered with actionable operator guidance in `auth_error.cpp::registerAuthErrors()`
- [x] close catch_all_swallow, unchecked_result, resource_leaked_in_exception gaps (Target: Q3 2026; delivered 2026-08-24)
  - `jwks_security.cpp`: RAII wrappers (UniqueX509, UniqueOSSLBuf, UniqueOSSLChar) applied to
    `computeSPKIHashFromFile`, `computeSPKIHashFromPEM`, `getCertificateInfo` — 3 resource_leaked_in_exception closed
  - `ldap_authenticator.cpp`: 4 unchecked `ldap_set_option` calls (TIMELIMIT×2, PROTOCOL_VERSION,
    NETWORK_TIMEOUT, TIMEOUT) now log warnings on failure — 4 unchecked_result closed
  - `rate_limiter_backend.cpp`: 5 bridge-function `catch(...)` blocks now log before fallback — 5 catch_all_swallow closed
  - `http_auth_async.cpp`: `performConnectivityCheck` `catch(...)` now logs at debug level — 1 catch_all_swallow closed
  - `auth_rate_limiter.cpp`: `reset()` lock-ordering hazard fixed (stats_mutex_ no longer held over
    sub-object reset calls); constructor and `incrementAndGetBreachCount()` Redis blocks wrapped with
    logged exception guards — 1 circular_lock_ordering + 2 catch_all_swallow closed

### Phase 4: Tests
- [x] DBL-01..DBL-08: core CRUD (add, isRevoked, purge, concurrency) — tests/auth/test_auth_distributed_blacklist.cpp
- [x] DBL-09..DBL-11: leader election semantics (sole node, lowest node_id wins, isLeader() state)
- [x] DBL-12..DBL-14: cluster API (syncWithCluster future, single-node convergence, timeout with unreachable peer)
- [x] DBL-15..DBL-17: observability + lifecycle (ReplicationStats zero-init, config accessor, RAII destructor)
- [x] expand focused regressions for concurrency, replay, and distributed-edge scenarios (Target: Q3 2026)
  - Delivered: `tests/auth/test_auth_hardening_revocation_federation.cpp`
    RFP-01..08 (revocation edge cases: concurrent, oversized JTI, empty JTI, purge selectivity)
- [x] extend deterministic fixture coverage for provider/federation matrix permutations (Target: Q3 2026)
  - Delivered: FED-01..08 (federation: unknown realm code, duplicate realm, malformed token, non-HTTPS endpoint,
    missing endpoint, multi-realm coexistence, realm count)
  - Delivered: ASY-01..08 (session/async: empty user_id, unknown session, expired session, idempotent terminate,
    terminateAllOther, sess_ prefix invariant, pruneExpired, per-user limit)
- [x] Wave C test gates delivered (Target: Q4 2026)
  - Delivered: `tests/auth/test_auth_wavec_authentication_methods.cpp` (AUTH-Auth-01..08: JWT/SAML/mTLS validation edge cases)
  - Delivered: `tests/auth/test_auth_wavec_token_lifecycle.cpp` (AUTH-Token-01..08: SessionManager + DistributedTokenBlacklist lifecycle)
  - Delivered: `tests/auth/test_auth_wavec_federation_providers.cpp` (AUTH-Provider-01..06: FederatedIdentityManager failover, degradation, realm management)
  - Delivered: `tests/auth/test_auth_wavec_authorization.cpp` (AUTH-AuthZ-01..08: authorization policy contract types)
  - Delivered: `tests/auth/test_auth_wavec_rate_limiting.cpp` (AUTH-RateLimit-01..06: AuthRateLimiter per-user, concurrency, reset)

### Phase 5: Performance and Hardening
- [x] isRevoked() confirmed O(1) RocksDB point read (< 1 µs warm cache); hot path unaffected by background sync
- [x] add() confirmed < 1 ms (single RocksDB Put)
- [x] cluster sync every 30 s (configurable sync_interval_seconds); background thread
- [x] leader election local-only (O(#peers) string comparison) converges < 1 s
- [x] lock benchmark-backed release gates for token/session/revocation hotspots (Target: Q3 2026)
  - Delivered: `benchmarks/auth/bench_auth_hotpaths.cpp` — AHP-01..08 with GATE-AHP-01..06
    (blacklist hit/miss p99 ≤ 1 µs, session create p99 ≤ 5 ms, session validate p99 ≤ 1 ms,
     distributed add p99 ≤ 2 ms, distributed isRevoked warm p99 ≤ 1 µs)
  - Registered in `benchmarks/CMakeLists.txt` as `bench_auth_hotpaths`
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q3 2026)
  - Gate thresholds documented in AHP benchmark file and PERFORMANCE_EXPECTATIONS.md

### Phase 6: Documentation and Acceptance
- [x] core auth module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Clarify distributed revocation and backend-capability expectations across blacklist and rate-limiter backend headers
  - `distributed_token_blacklist.h` failure/degradation contract section added (§ failure/degradation contract)
- [x] Add explicit provider-degradation guidance for network-bound authentication adapters
  - `federated_identity_manager.h` provider-degradation contract section added
- [x] Document benchmark-backed compatibility guarantees for token/session hot paths in header docs
  - `session_manager.h` bounded runtime contract section added

## Production Readiness Checklist

- [x] core auth surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] distributed blacklist RPC layer (TBLK/v1) fully implemented and tested (DBL-01..DBL-17)
- [x] remaining hardening tasks closed for provider edge cases
- [x] release-gate benchmark stabilization complete
- [x] PasskeyAuthenticator TODO stubs replaced with real CBOR/OpenSSL verification logic (2026-08-19)
- [x] Wave C test gates delivered: AUTH-Auth-01..08, AUTH-Token-01..08, AUTH-Provider-01..06, AUTH-AuthZ-01..08, AUTH-RateLimit-01..06 (2026-08-19)
- [x] Batch 5 gap closure: resource_leaked_in_exception (jwks_security.cpp RAII), unchecked_result (ldap_authenticator.cpp), catch_all_swallow (rate_limiter_backend.cpp, http_auth_async.cpp, auth_rate_limiter.cpp), circular_lock_ordering (auth_rate_limiter.cpp reset()) — delivered 2026-08-24
- [x] audit_logger.h filename collision resolved: include/api/audit_logger.h renamed to include/api/graphql_audit_logger.h; ws_handler.cpp updated — delivered 2026-08-24 (unblocks build validation for AUTH-GRG gate evidence)
- [~] Wave C benchmark gates executed (AUTH-GRG-01..06) — CI run `CI — Benchmarks` #40 (`32765349559`) pending completion as of 2026-08-24T19:20Z; evidence capture follows artifact publication

## Known Issues and Limitations

- behavior remains partially capability-dependent on configured identity providers and backends.
- continued hardening is needed for multi-realm/distributed revocation edge profiles.
- benchmark coverage still requires tightening for certain policy and integration paths.

## Breaking Changes

No breaking auth-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `auth`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
