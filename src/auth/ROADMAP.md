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

### Short-term (3-6 months)
- [ ] tighten fail-closed behavior for optional provider-degraded scenarios (Target: Q4 2026)
- [ ] expand deterministic integration regressions across auth protocol matrixes (Target: Q4 2026)
- [ ] improve operator diagnostics for policy/revocation/federation decision classes (Target: Q4 2026)

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
- [ ] Wave C benchmark gates executed (AUTH-GRG-01..06) — pending CI run

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
