# Auth Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production authentication runtime exists across JWT/OIDC, Kerberos, MFA, OAuth, SAML, LDAP, API-key, mTLS, WebAuthn, session/revocation, and zero-trust verification paths.

## In Progress

- [~] hardening of distributed revocation, federation, and policy-edge behavior (Target: Q3 2026)
- [~] benchmark and release-gate consolidation for token/session hot paths (Target: Q3 2026)
- [~] consistency hardening for async/provider-integration reliability (Target: Q3 2026)

## v1.2.0 Async Operations & Connection Pooling (Completed)

- [x] async/non-blocking LDAP authentication calls (authenticateAsync with AuthWorkerThreadPool)
- [x] async/non-blocking HTTP authentication calls (new AsyncHTTPAuth class)
- [x] LDAP connection pooling with health checks and reuse (LDAPConnectionPool)
- [x] HTTP retry logic with exponential backoff for transient failures
- [x] Thread-safe worker pool for concurrent auth operations

## v1.3.0 Token Blacklist Persistence & Distributed Support (In Progress)

- [x] Token blacklist persistence to RocksDB (RocksDBTokenBlacklist)
- [x] Leader election for distributed deployments (node-ID ordering, performLeaderElection)
- [x] Atomic blacklist validation during cluster sync (fail-closed isRevoked with RocksDB read)
- [~] Distributed token blacklist with cluster synchronization (DistributedTokenBlacklist — RPC layer stub, core storage complete)
- [x] Comprehensive test coverage for distributed scenarios (test_auth_distributed_blacklist.cpp, DBL-01..DBL-17)

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
- [ ] freeze authentication and principal-contract semantics for active major line (Target: Q3 2026)
- [ ] define explicit failure contracts per provider integration and policy gate (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete remaining hardening in revocation/federation/provider execution paths (Target: Q4 2026)
- [ ] align session/trust behavior to shared bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for malformed auth artifacts and degraded backends (Target: Q4 2026)
- [ ] unify error taxonomy and diagnostics across protocol adapters (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for concurrency, replay, and distributed-edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for provider/federation matrix permutations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for token/session/revocation hotspots (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core auth module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core auth surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for distributed/provider edge cases
- [ ] release-gate benchmark stabilization complete

## Known Issues and Limitations

- behavior remains partially capability-dependent on configured identity providers and backends.
- continued hardening is needed for multi-realm/distributed revocation edge profiles.
- benchmark coverage still requires tightening for certain policy and integration paths.

## Breaking Changes

No breaking auth-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.