# Auth Module Roadmap

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

> Roadmap checkmarks identify implementation or test artifacts cited below;
> they do not prove that a build, CI run, benchmark, or production deployment
> passed. CI status and hardware measurements are not inferred from source.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production authentication runtime exists across JWT/OIDC, Kerberos, MFA, OAuth, SAML, LDAP, API-key, mTLS, WebAuthn, session/revocation, and zero-trust verification paths.
The distributed blacklist has a TBLK/v1 TCP protocol, leader push/follower pull paths, a server listener, and DBL-01..DBL-17 test cases. The source does not establish conventional LWW conflict resolution or a cluster-wide convergence guarantee.

## In Progress

- [x] hardening of distributed revocation, federation, and policy-edge behavior (Target: Q3 2026)
- [x] benchmark and release-gate consolidation for token/session hot paths (Target: Q3 2026)
- [x] consistency hardening for async/provider-integration reliability (Target: Q3 2026)
- [?] Representative-hardware benchmark results and CI gate evidence (Target: Q4 2026)
  - Source defines `GATE-AHP-01..06` in `benchmarks/auth/bench_auth_hotpaths.cpp`; this source tree contains no result artifact establishing a passing run.
  - The status of previously dispatched CI runs is not verifiable from the source tree.

## v1.2.0 Async Operations & Connection Pooling (Completed)

- [x] async/non-blocking LDAP authentication calls (authenticateAsync with AuthWorkerThreadPool)
- [x] async/non-blocking HTTP authentication calls (new AsyncHTTPAuth class)
- [x] LDAP connection pooling with health checks and reuse (LDAPConnectionPool)
- [x] HTTP retry logic with exponential backoff for transient failures
- [x] Thread-safe worker pool for concurrent auth operations

## v1.3.0 Token Blacklist Persistence & Distributed Support (Completed)

- [x] Token blacklist persistence to RocksDB (RocksDBTokenBlacklist)
- [x] Leader election for distributed deployments (node-ID ordering, performLeaderElection)
- [x] Local blacklist lookup uses a RocksDB read and treats non-`NotFound` read errors as revoked; this is not a cross-node atomic transaction
- [x] Distributed token blacklist with cluster synchronization — TBLK/v1 binary TCP protocol:
  - TCP server listener on `local_node.rpc_port`; bind is non-fatal (outbound still works)
  - Leader PUSH: serializes all non-expired JTI entries and pushes to each follower
  - Follower PULL: sends PULL_REQ, receives PULL_RESP, and applies received entries
  - `performClusterSync()`: leader election → push to all peers (leader) or pull from leader (follower)
  - `pushRevisionsToFollower()` / `pullRevisionsFromLeader()`: full production implementation
  - `serveIncomingConnections()` / `handlePeerConnection()`: server-side PUSH and PULL_REQ dispatch
  - `getAllEntries()` / `applyEntries()`: RocksDB helpers; `applyEntries()` overwrites received keys without timestamp comparison
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
- [x] tighten fail-closed behavior for optional provider-degraded scenarios (Target: Q4 2026)
- [x] expand deterministic integration regressions across auth protocol matrixes (Target: Q4 2026)
- [x] improve operator diagnostics for policy/revocation/federation decision classes (Target: Q4 2026)

### Wave 8 — Auth Hardening Items (2026-08-26 scan, Q4 2026)
> Source: `src/MODULE_GAP_ANALYSIS_WAVE2.md §Wave 8`

- [x] **W8-15** `ldap_authenticator.cpp:699–722` — LDAP pagination error handling: add retry loop (max 3, exponential backoff) + `AuthAuditLogger` event on each transient LDAP error; current `break` returns partial group membership silently (Target: Q4 2026) ✅ **IMPLEMENTED 2026-08-26**
- [x] **W8-16** `federated_identity_manager.cpp:209–218` — Token cache DoS hardening: add `kTokenCacheMaxSize` cap + `std::list` LRU eviction + SHA-256(token) → hex string as cache key; current cache is unbounded and keyed on raw token strings (Target: Q4 2026) ✅ **IMPLEMENTED 2026-08-26**
- [x] **W8-17** `ldap_connection_pool.cpp:208–217` — Pool exhaustion audit: inject `AuthAuditLogger`; emit structured audit event on pool exhaustion before throw (LOW, Target: Q4 2026) ✅ **IMPLEMENTED 2026-08-26**
- [x] Test: `tests/auth/test_wave8_auth_hardening.cpp` — 10+ tests covering pagination retry, partial-result detection, cache eviction under size pressure, pool-exhaustion audit trail

### Mid-term (6-12 months)
- [x] reduce remaining proxy-like benchmark targets through dedicated auth microbenchmarks (Target: Q1 2027)
- [x] re-baseline auth p95/p99 envelopes on representative production profiles (Target: Q1 2027)
- [x] harden multi-realm and distributed trust-state synchronization paths (Target: Q1 2027)

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
- [x] Received entries are batch-written by `applyEntries()`; the implementation does not compare timestamps, so conventional LWW conflict resolution is not established
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
- [x] `isRevoked()` uses a local RocksDB point read; asymptotic and measured-latency guarantees are not claimed here
- [x] `add()` writes to RocksDB locally; a sub-millisecond latency is not established without a measured run
- [x] cluster sync every 30 s (configurable sync_interval_seconds); background thread
- [x] leader selection is local by node-ID ordering; a convergence-time guarantee is not established
- [x] lock benchmark-backed release gates for token/session/revocation hotspots (Target: Q3 2026)
  - Delivered: `benchmarks/auth/bench_auth_hotpaths.cpp` — AHP-02..11 registered cases with GATE-AHP-01..06; AHP-01 is not implemented/registered
    (blacklist hit/miss p99 ≤ 1 µs, session create p99 ≤ 5 ms, session validate p99 ≤ 1 ms,
     distributed add p99 ≤ 2 ms, distributed isRevoked warm p99 ≤ 1 µs)
  - Registered in `benchmarks/CMakeLists.txt` as `bench_auth_hotpaths`
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q3 2026)
  - Gate thresholds documented in AHP benchmark file and PERFORMANCE_EXPECTATIONS.md
  - The source exports p50/p95/p99 and gate counters; no measured artifact is recorded here.

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
- [?] Representative-hardware release-gate results are not evidenced in this source tree
- [x] PasskeyAuthenticator TODO stubs replaced with real CBOR/OpenSSL verification logic (2026-08-19)
- [x] Wave C test gates delivered: AUTH-Auth-01..08, AUTH-Token-01..08, AUTH-Provider-01..06, AUTH-AuthZ-01..08, AUTH-RateLimit-01..06 (2026-08-19)
- [x] Batch 5 gap closure: resource_leaked_in_exception (jwks_security.cpp RAII), unchecked_result (ldap_authenticator.cpp), catch_all_swallow (rate_limiter_backend.cpp, http_auth_async.cpp, auth_rate_limiter.cpp), circular_lock_ordering (auth_rate_limiter.cpp reset()) — delivered 2026-08-24
- [x] Audit logger header filename collision was resolved in the source tree; no auth benchmark execution result is implied
- [?] No representative-hardware benchmark result artifact is recorded in this repository; CI run status is unresolved
- [x] Short-term hardening complete: fail-closed provider-degraded, protocol matrix regression tests, operator diagnostics decision_class — delivered 2026-09-16
- [x] Mid-term code items: AHP-09..11 cases are present in `bench_auth_hotpaths.cpp` (`bench_auth_protocol_micro` is a label, not a separate executable); AHP-01 remains comment-only; design targets are listed in `PERFORMANCE_EXPECTATIONS.md`; `syncTrustState()` is declared in `federated_identity_manager.h`
- [x] Wave D contributions complete: high-cardinality stress tests (test_auth_highcardinality_stress.cpp), soak tests (test_auth_soak.cpp), operator runbook (RUNBOOK.md), operator_hint JSON field in 9420–9452 error registry entries — delivered 2026-09-16

## Known Issues and Limitations

- behavior remains partially capability-dependent on configured identity providers and backends.
- Hardware-measured p95/p99 benchmark baselines are design-target-only pending a dedicated benchmark hardware run (see `PERFORMANCE_EXPECTATIONS.md §Re-baseline procedure`).
- Representative-hardware benchmark gate evidence is not present in the source tree; CI run status is unresolved.

## Breaking Changes

No breaking auth-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `auth`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
  - Validated: PROVIDER_DEGRADED fail-closed on empty sub, HTTP 5xx, and provider exceptions; syncTrustState() for multi-realm distribution; all revocation backends are fail-closed by default.
- [?] Benchmark-backed p95/p99 baselines on representative hardware (Target: Q1 2027)
  - Source thresholds are defined for GATE-AHP-01..06; hardware-measured baselines are not recorded in `PERFORMANCE_EXPECTATIONS.md`.
- [~] Operator diagnostics and runbook coverage (Target: Q1 2027)
  - `src/auth/RUNBOOK.md` documents source-backed JWT/JWKS, blacklist, session, provider, and rate-limiter triage limits. The auth source does not establish a universal metrics/alert schema or admin CLI.

## Research-Based Release Plan: v2.5.0 and v3.0.0

This is a module-level proposal, not a release commitment. Root release governance
and `VERSIONING.md` remain authoritative: v2.5.0 work must preserve compatible
contracts; any incompatible API or wire-protocol change belongs behind the v3.0.0
major-version migration process. Effort estimates are engineering-weeks (EW),
exclude external-provider and representative-hardware wait time, and have roughly
±50% uncertainty until Wave 1 inventories deployment profiles.

### Research basis and current-state assessment

This is a qualitative assessment, not a certification or standardized maturity
score.

| Area | Evidence-based assessment |
|---|---|
| Authentication coverage and application fit | Broad runtime coverage exists for JWT/OIDC, OAuth, SAML, LDAP, Kerberos, API keys, mTLS, MFA/passkeys, sessions, federation, and policy checks. Provider configuration and deployment remain caller-specific; source coverage does not prove interoperability with every IdP. |
| Security and resilience | Bounded inputs, failure classes, fail-closed rules, session limits, replay controls, provider timeouts, audit, and rate limiting are represented in the [principal contract](../../include/auth/auth_principal_contract.h) and tests. The [distributed blacklist implementation](distributed_token_blacklist.cpp) is best-effort replication: the local node-ID leader choice, periodic full-entry sync, and overwrite-on-apply behavior do not establish quorum, conflict ordering, or a cluster-wide convergence bound. |
| Performance and scale | [`bench_auth_hotpaths.cpp`](../../benchmarks/auth/bench_auth_hotpaths.cpp) registers AHP-02..11 and defines six gates for AHP-02..07; AHP-01 is described but not registered. LDAP AHP-09 is explicitly a simulation, and AHP-08/10/11 do not establish provider-network performance. [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) records target thresholds but no measured hardware baseline. Therefore release performance and scaling claims remain unverified. |
| Tests, governance, and operations | Focused auth tests are registered in [`tests/auth/CMakeLists.txt`](../../tests/auth/CMakeLists.txt), including the [protocol-matrix suite](../../tests/auth/test_auth_protocol_matrix_regression.cpp), [high-cardinality stress suite](../../tests/auth/test_auth_highcardinality_stress.cpp), and [soak suite](../../tests/auth/test_auth_soak.cpp). The high-cardinality stress tests use in-process stubs, and the soak suite defaults to three seconds; neither is production-provider or sustained-load evidence. The [auth runbook](RUNBOOK.md) and structured audit/metrics surfaces exist, but there is no source-backed universal auth SLO or compliance attestation. |

Use the following primary technical references to build a traceable requirements
matrix; they inform test design but do not imply that ThemisDB conforms to them:

- NIST SP 800-63B-4 (final, July 2025), *Digital Identity Guidelines: Authentication
  and Authenticator Management*: [NIST publication](https://csrc.nist.gov/pubs/sp/800/63/b/4/final).
  Apply only the requirements relevant to the product's chosen assurance profile.
- IETF RFC 9700 (January 2025), *Best Current Practice for OAuth 2.0 Security*:
  [RFC 9700](https://www.rfc-editor.org/rfc/rfc9700.html). Use its threat and flow
  requirements to assess the OAuth/OIDC client paths, including redirect handling
  and protections against code/token misuse.
- W3C Web Authentication Level 3 (Recommendation, 2026-08-25):
  [WebAuthn Level 3](https://www.w3.org/TR/2026/REC-webauthn-3-20260825/).
  Use the stable specification and its implementation report to select
  interoperability cases for passkey/WebAuthn paths.
- OWASP [Authentication Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Authentication_Cheat_Sheet.html):
  use as implementation guidance for MFA, automated-attack throttling, and
  authentication failure handling, not as a compliance standard.

#### v2.5.0 — measurable hardening without breaking contracts

**Release constraint:** Keep public API and wire compatibility. Prioritize closing
evidence gaps and operationally validating existing controls before adding
protocol surface.

##### Wave 1 — Requirements and reproducible baseline (Priority: P0; Estimate: 1–2 EW)

- **Goal:** Replace source-defined performance targets and broad protocol claims with traceable, repeatable evidence.
- **Tasks:**
  - [ ] Map each applicable NIST/RFC/W3C control to an auth component, owner, test, and evidence artifact; record non-applicability with rationale (Target: v2.5.0).
  - [ ] Capture release-profile p50/p95/p99 and throughput for every registered AHP case on representative hardware; correct the AHP-01 inventory mismatch and label simulation-only cases (Target: v2.5.0).
- **Dependencies:** Auth test/benchmark targets and benchmark-capable hardware; security owner confirms the applicable product assurance profile.
- **Risk / trade-off:** Measurements can expose regressions or require tighter target definitions; single-machine results must not be generalized to all deployment classes.
- **KPIs:** 100% of applicable requirements have a code/test/evidence mapping; all six existing AHP gates have recorded measured results; zero missing or misidentified registered benchmark cases.
- **Validation:** Run focused auth tests and `bench_auth_hotpaths` in the release profile; archive machine, compiler, configuration, and raw JSON results; review the mapping against the cited primary standards.

##### Wave 2 — Protocol and abuse-control conformance (Priority: P1; Estimate: 2–4 EW)

- **Goal:** Close only the OAuth/OIDC, WebAuthn, login-throttling, and error-handling gaps confirmed by Wave 1.
- **Tasks:**
  - [ ] Add regression cases for applicable RFC 9700 redirect/flow protections, credential replay, and provider failure behavior without weakening existing fail-closed rules (Target: v2.5.0).
  - [ ] Add W3C WebAuthn interoperability vectors for supported credential algorithms, RP/origin validation, challenge lifecycle, and authenticator counter behavior; document unsupported profiles (Target: v2.5.0).
  - [ ] Verify throttling, generic externally visible failures, and audit redaction across enabled authentication methods against the selected NIST/OWASP profile (Target: v2.5.0).
- **Dependencies:** Wave 1 applicability matrix; existing `tests/auth/test_auth_protocol_matrix_regression.cpp`, passkey-focused tests, rate-limiter tests, and audit tests.
- **Risk / trade-off:** Stricter validation may reject previously tolerated but invalid provider configuration; do not change defaults or public contracts without migration review.
- **KPIs:** 100% of applicable high-risk matrix cases have automated regressions; zero accepted malformed/replayed credentials in the targeted suite; zero credential or token values in tested audit/log output.
- **Validation:** Run the focused protocol, passkey, input-validation, rate-limit, and sensitive-data-redaction suites; security review any changed validation decision.

##### Wave 3 — Revocation and provider-degradation evidence (Priority: P1; Estimate: 3–5 EW)

- **Goal:** Demonstrate bounded, fail-closed behavior across multi-node revocation and degraded identity providers.
- **Tasks:**
  - [ ] Add multi-node tests for restart/rejoin, duplicate and reordered updates, partition/recovery, unreachable peers, and simultaneous revocations; document current TBLK/v1 limits (Target: v2.5.0).
  - [ ] Measure revocation propagation under a defined healthy three-node profile and set an approved operational bound; retain fail-closed behavior when the configured freshness bound is exceeded (Target: v2.5.0).
  - [ ] Exercise provider timeouts, retries, pool exhaustion, and recovery through production adapter code rather than simulation-only fixtures (Target: v2.5.0).
- **Dependencies:** Wave 1 hardware/configuration profile; a three-node test environment or deterministic network fault fixture; Wave 2 regression coverage.
- **Risk / trade-off:** Replication semantics may reveal that the current best-effort protocol cannot meet the desired bound; a redesign must be escalated to v3 rather than hidden behind a v2.5 behavior change.
- **KPIs:** Zero authorization when a mandatory revocation check is indeterminate; p99 propagation at or below 2× the configured sync interval in the healthy test profile (candidate target, to be approved in Wave 1); 100% deterministic outcomes for the documented fault matrix.
- **Validation:** Run distributed-blacklist and provider integration tests with repeatable fault injection; publish propagation distributions, timeout/recovery outcomes, and restart/rejoin results.

##### Wave 4 — Release gates and operator readiness (Priority: P1; Estimate: 2–3 EW)

- **Goal:** Make auth release readiness auditable without implying certification or universal deployment behavior.
- **Tasks:**
  - [ ] Convert the accepted Wave 1 baselines into release-profile regression gates; keep simulated benchmark/test results explicitly separate from production-path evidence (Target: v2.5.0).
  - [ ] Define auth-specific operator indicators and runbook actions for provider degradation, revocation lag, key rollover, rate limiting, and audit-sink failure (Target: v2.5.0).
  - [ ] Record security review outcomes, unresolved applicability questions, and compatibility/migration status in the release evidence bundle (Target: v2.5.0).
- **Dependencies:** Waves 1–3; root `release_critical` gate and release evidence workflow.
- **Risk / trade-off:** New alerting and gates may expose deployment configuration gaps or add CI/runtime cost; establish thresholds from measured behavior rather than guesses.
- **KPIs:** All auth `release_critical` checks pass on the release candidate; no mapped benchmark regresses more than 10% against its accepted baseline; every high-severity operator scenario has an actionable runbook and owner.
- **Validation:** Run auth `release_critical`, focused soak and distributed-failure suites, benchmark regression checks, documentation/link checks, and security sign-off; retain the exact artifacts used for approval.

#### v3.0.0 — versioned contracts and stronger distributed guarantees

**Release constraint:** Use the major release only for explicitly approved incompatible
contracts or wire changes. Preserve a documented migration path from v2.5.0; do not
assume that this roadmap itself approves a breaking change.

##### Wave 1 — Identity and authorization contract redesign (Priority: P0; Estimate: 3–5 EW)

- **Goal:** Decide whether to consolidate provider results, principal claims, policy decisions, and failure semantics into a versioned cross-provider contract.
- **Tasks:**
  - [ ] Specify principal/claim normalization, issuer/audience boundaries, authorization context, error mapping, and ownership/lifetime rules across all supported adapters (Target: v3.0.0).
  - [ ] Identify each incompatible API/configuration change, its v2.5.0 compatibility behavior, deprecation period, and migration example before implementation (Target: v3.0.0).
- **Dependencies:** v2.5.0 requirements matrix and production-path test results; architecture and security-owner approval.
- **Risk / trade-off:** A normalized contract can simplify callers but may erase provider-specific semantics; retain explicit capability and failure information rather than silently coercing it.
- **KPIs:** 100% of supported adapters represented in the contract matrix; every proposed breaking change has an approved migration test and owner; no unreviewed compatibility shim.
- **Validation:** API/compatibility review, threat-model review, generated/public API documentation audit, and migration tests against representative v2.5.0 clients.

##### Wave 2 — Revocation protocol consistency and recovery (Priority: P0; Estimate: 5–8 EW)

- **Goal:** Determine and, if approved, implement explicit distributed revocation consistency, peer-authentication, and recovery semantics beyond best-effort TBLK/v1 replication.
- **Tasks:**
  - [ ] Select and document the consistency model, conflict ordering, peer trust/authentication, replay protection, bounded message handling, and partition behavior before changing the wire protocol (Target: v3.0.0).
  - [ ] If the approved design requires incompatible wire behavior, implement a versioned protocol and an explicit v2-to-v3 migration/rollout procedure (Target: v3.0.0).
  - [ ] Add repeatable multi-node chaos tests covering partition, clock skew, restart, data restoration, and peer rejoin (Target: v3.0.0).
- **Dependencies:** Wave 1 contract decisions; threat model, supported topology, and operator availability objectives; a multi-node fault-injection environment.
- **Risk / trade-off:** Stronger consistency can increase latency or reduce availability during partitions; the system must state which trade-off applies and deny when a required security decision cannot be established.
- **KPIs:** Zero revoked-token resurrection in 100 repeated restart/rejoin scenarios; p99 propagation within the approved healthy-cluster SLO; 100% of partition tests produce the documented fail-closed or explicitly approved result.
- **Validation:** Protocol conformance and negative tests, 3+ node fault-injection runs, restart/recovery verification, load tests, and independent security review of peer trust and wire parsing.

##### Wave 3 — Lifecycle and federation interoperability (Priority: P1; Estimate: 4–6 EW)

- **Goal:** Make credential enrollment, renewal, revocation, and recovery predictable across supported passkey and federated-provider deployments.
- **Tasks:**
  - [ ] Define supported WebAuthn Level 3 and OAuth/OIDC profiles, provider metadata/key rollover behavior, and account-recovery boundaries (Target: v3.0.0).
  - [ ] Implement only profile gaps confirmed by conformance/interoperability testing; publish operator and client migration guidance for changed defaults (Target: v3.0.0).
- **Dependencies:** Wave 1 public-contract decisions; Wave 2 revocation lifecycle semantics; representative external test providers/authenticators.
- **Risk / trade-off:** Ecosystem implementations may differ in optional features; explicitly scope supported profiles and avoid claiming universal interoperability.
- **KPIs:** 100% pass rate for the declared profile's mandatory conformance vectors; zero lost or resurrected credentials in lifecycle/recovery regressions; every changed default has migration and rollback guidance.
- **Validation:** Run conformance vectors and interoperability tests against at least two independent implementations per selected profile; test key rollover, credential revocation, and recovery end-to-end.

##### Wave 4 — Assurance, operability, and major-release acceptance (Priority: P1; Estimate: 3–5 EW)

- **Goal:** Establish evidence and ownership for operating the v3 auth contract at its declared security and availability profile.
- **Tasks:**
  - [ ] Complete an applicability/evidence map for the selected NIST profile and relevant OAuth/WebAuthn standards; record exceptions and evidence owners without asserting certification (Target: v3.0.0).
  - [ ] Define SLOs, alert thresholds, audit retention/redaction rules, recovery procedures, and support boundaries for each supported deployment topology (Target: v3.0.0).
  - [ ] Complete migration, rollback, security review, and release sign-off for every approved breaking API or wire change (Target: v3.0.0).
- **Dependencies:** Waves 1–3; organization-selected assurance/compliance scope; operations and security sign-off.
- **Risk / trade-off:** Compliance scope, retention rules, or topology requirements may remain undecided; do not advertise compliance or production guarantees until evidence and accountable approval exist.
- **KPIs:** 100% of applicable controls have current evidence or an approved exception; zero unowned critical/high security findings at release approval; all supported topologies have tested recovery and rollback instructions.
- **Validation:** Independent security assessment, release-critical suite, benchmark and chaos evidence review, documentation/governance audit, and recorded human release approval.

### Open assumptions and decision gates

- The product's target assurance level, regulatory scope, supported cluster size, provider matrix, and availability SLO have not been established by the inspected auth sources; Wave 1 must obtain owners and decisions.
- The v2.5.0 propagation threshold is a proposed test target, not a current guarantee. Approve or replace it using measured deployment data before making it a release gate.
- The current source establishes auth components and test registrations, not successful execution of every test, performance gate, standards conformance, or production deployment.
- Root roadmap entries still track auth fail-closed/protocol-matrix and benchmark evidence as program gates. Keep their release status and this module plan aligned when test-run artifacts become available.
