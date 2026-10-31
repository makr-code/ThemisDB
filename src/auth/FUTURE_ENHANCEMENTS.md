# Auth Module - Future Enhancements

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of authentication, revocation, federation, and trust verification flows
- expansion of deterministic reliability and observability behavior for provider integrations
- stricter benchmark-backed guardrails for token/session/policy hot paths
- **v1.2.0**: async/non-blocking operations and connection pooling for LDAP and HTTP
- **v1.3.0**: distributed token blacklist with cluster synchronization

## Design Constraints

- authentication contracts remain backward compatible within major release line.
- token/session validation remains fail-closed under malformed or unsupported states.
- distributed and provider-dependent paths remain bounded and observable.
- trust/policy decisions remain deterministic and auditable.
- async operations never block the caller's thread for network I/O.
- connection pooling reduces per-call overhead while maintaining deterministic behavior.

## Required Interfaces

| Interface | Requirement |
|---|---|
| token validation interfaces | deterministic claim/signature/revocation behavior |
| session/revocation interfaces | bounded lifecycle and consistent invalidation semantics |
| provider/federation interfaces | explicit capability checks and failure classification |
| trust/policy interfaces | clear allow/deny reasoning and auditability |
| async auth interfaces | non-blocking dispatch to worker threads, futures-based results |
| distributed blacklist interfaces | cluster-safe JTI revocation with atomic checks |

## Implementation Notes (v1.2.0 - v1.3.0)

### v1.2.0: Async Operations & Connection Pooling

**Implemented API surfaces (maturity and deployment readiness are not asserted):**

1. **AsyncHTTPAuth** (`http_auth_async.h/.cpp`)
   - Future-returning HTTP GET/POST helpers
   - Uses AuthWorkerThreadPool for concurrent operations
   - Retry, timeout, and certificate-verification settings are exposed in `HTTPAuthConfig`

2. **LDAPAuthenticator::authenticateAsync()**
   - Async wrapper for LDAP bind operations
   - Dispatches blocking calls to worker thread pool
   - Returns std::future<LDAPAuthResult>
   - No measured caller-latency guarantee is established by the API

3. **LDAPConnectionPool** (already implemented)
   - Configurable `pool_min_idle`, `pool_max_size`, and `pool_checkout_timeout_ms`

### v1.3.0: Distributed Token Blacklist

**Implemented components (not a production-readiness or cluster-consistency certification):**

1. **DistributedTokenBlacklist** (`distributed_token_blacklist.h/.cpp`)
   - [x] RocksDB persistence layer (extends RocksDBTokenBlacklist)
   - [x] Background purge thread for expired entries
   - [x] Background replication thread for cluster sync
   - [x] Local leader selection by node ID ordering
   - [x] Pull and push paths using the TBLK/v1 TCP protocol
   - [x] TCP server listener (`serveIncomingConnections` / `handlePeerConnection`) for inbound PUSH and PULL_REQ
   - [x] `getAllEntries()` — full RocksDB iterator scan, filtered to non-expired entries
   - [x] `applyEntries()` — batch writes received entries; expired entries are dropped

2. **TBLK/v1 Binary Wire Protocol**
   - Header layout: `magic (4 bytes, "TBLK") | version (1 byte, value 0x01) | type (1 byte) | count (4-byte big-endian)` = 10 bytes
   - Entry layout per entry: `jti_len[2 BE] | jti[jti_len] | expiry_unix_secs[8 BE int64]`
   - Message types: PUSH(0x01) = leader→follower, PULL_REQ(0x02) = follower→leader,
     PULL_RESP(0x03) = leader→follower, ACK(0x04) = any direction
   - Safety caps: max JTI length = 1024 bytes; max entries per message = 1,000,000
   - Timeout enforcement: non-blocking `connectWithTimeout()` via `select()` + SO_ERROR;
     SO_RCVTIMEO / SO_SNDTIMEO for data transfer, bounded by `peer_rpc_timeout_ms`
   - POSIX (`sendAll` uses `MSG_NOSIGNAL`) and Windows (`ioctlsocket` + `closesocket`) both supported

3. **Cluster Architecture**
   - Local node: persists JTI revocations to RocksDB
   - Peer nodes: can receive synchronized blacklist entries when peer communication succeeds
   - Leader: node with lexicographically lowest `node_id`; acts as source of truth
   - Followers: pull updates from leader via PULL_REQ / PULL_RESP exchange
   - Leader also pushes: leader initiates PUSH to each follower in `performClusterSync()`
   - `applyEntries()` overwrites received keys without comparing timestamps; conventional last-write-wins conflict resolution is not established

4. **Fault Tolerance**
   - Continues operating if peers are temporarily unavailable (bind failure is non-fatal)
   - Leader re-election on each `performClusterSync()` call (stateless, purely local)
   - Bounded `peer_rpc_timeout_ms` prevents cascading delays
   - `performClusterSync()` reports whether any peer sync succeeded; no quorum or complete-peer success is established

## Test Strategy

- protocol-matrix unit and integration suites across auth methods.
- replay/revocation and distributed-state regression scenarios.
- degraded-provider and failover/fallback deterministic tests.
- release-profile benchmark runs for mapped auth targets.
- **New for v1.2.0**: async non-blocking behavior validation
- **New for v1.2.0**: connection pool health checks and reuse metrics
- **v1.3.0 delivered** (tests/auth/test_auth_distributed_blacklist.cpp, DBL-01..DBL-17):
  - DBL-01..08: core CRUD (add, isRevoked, future/past expiry, purgeExpired, concurrency, idempotent re-add)
  - DBL-09..11: leader election (sole node, lowest node_id wins, isLeader() post-election state)
  - DBL-12..14: cluster API (syncWithCluster future, single-node convergence, timeout with unreachable peer)
  - DBL-15..17: observability + lifecycle (ReplicationStats zero-init, config accessor round-trip, RAII destructor)

## Performance Targets

### v1.2.0
- The future-returning LDAP and HTTP APIs expose async dispatch; no latency SLO is established here.
- `HTTPAuthConfig` defaults `request_timeout_seconds` to 30; this is a configured request timeout, not a completion guarantee.

### v1.3.0
- `isRevoked()` performs a local RocksDB point read; no constant-time or measured-latency guarantee is established.
- `add()` performs a local RocksDB write; no sub-millisecond guarantee is established.
- Cluster sync every 30 seconds without blocking revocation checks (configurable sync_interval_seconds)
- Replication runs on background threads; contention and token-validation latency under concurrent writes have not been measured here
- Purge thread runs independently; does not compete with validation
- Leader selection is local by node-ID ordering; no election convergence-time guarantee is established.
- `waitForClusterConvergence()` returns after at least one successful sync and does not prove that every peer has converged.
- `pushRevisionsToFollower()` / `pullRevisionsFromLeader()` bounded by `peer_rpc_timeout_ms` (default 5 s)

## Security / Reliability

- maintain strict fail-closed behavior for credential/token/provider errors.
- preserve auditable decision paths for authn/authz and trust checks.
- enforce bounded resource behavior in rate-limiter and session/revocation components.
- keep diagnostics actionable for production incident response.
- **async ops**: inspect each future-returning method's result and exception contract; behavior is API-specific
- **connection pool**: stale connections detected and evicted before use
- **distributed blacklist**: RocksDB persistence is used; restart durability and recovery guarantees are not certified here
- **cluster sync**: failed sync attempts are retried by later background cycles; there is no quorum guarantee

## Migration Path

Existing code using synchronous auth methods remains unchanged:
- `LDAPAuthenticator::authenticate()` continues to work as before
- `OIDCProvider::validateToken()` remains synchronous
- Token blacklist API is additive; new distributed variant is opt-in

New async code can opt-in:
- Call `authenticateAsync()` to get non-blocking future
- Configure AsyncHTTPAuth for OAuth/OIDC operations
- Enable `DistributedTokenBlacklist` for cluster deployments

The RocksDB implementation creates its database/column family when configured to
do so. Cross-version schema migration compatibility is not established here.