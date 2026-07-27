# Auth Module - Future Enhancements

<!-- Status: current | validated: 2026-07-27 -->
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

**Completed Components (v1.2.0 — production-ready):**

1. **AsyncHTTPAuth** (`http_auth_async.h/.cpp`)
   - Non-blocking HTTP GET/POST for OAuth, OIDC, SAML discovery
   - Uses AuthWorkerThreadPool for concurrent operations
   - Retry logic with exponential backoff for transient failures
   - Timeout configuration per request type
   - SSL certificate validation options

2. **LDAPAuthenticator::authenticateAsync()**
   - Async wrapper for LDAP bind operations
   - Dispatches blocking calls to worker thread pool
   - Returns std::future<LDAPAuthResult>
   - P99 latency target: ≤50ms visible to callers

3. **LDAPConnectionPool** (already implemented)
   - Pre-warmed pool of idle LDAP connections
   - Health checks on every checkout
   - Stale connection eviction and recreation
   - Configurable min_idle, max_size, checkout_timeout_ms

### v1.3.0: Distributed Token Blacklist

**In Progress Components (v1.3.0 — core storage complete, RPC layer stubbed):**

1. **DistributedTokenBlacklist** (`distributed_token_blacklist.h/.cpp`)
   - [x] RocksDB persistence layer (extends RocksDBTokenBlacklist)
   - [x] Background purge thread for expired entries
   - [x] Background replication thread for cluster sync
   - [x] Leader election using node ID ordering
   - [~] Pull-based synchronization from followers to leader (RPC stub — production RPC pending)

2. **Cluster Architecture**
   - Local node: persists JTI revocations to RocksDB
   - Peer nodes: maintain synchronized copy of blacklist
   - Leader: acts as the source of truth for revocation state
   - Followers: pull updates periodically (configurable interval)
   - Conflict resolution: Last-Write-Wins based on expiry timestamp

3. **Fault Tolerance**
   - Continues operating if peers are temporarily unavailable
   - Automatic leader re-election if current leader fails
   - Bounded RPC timeout to prevent cascading delays
   - Graceful degradation when cluster connectivity is lost

## Test Strategy

- protocol-matrix unit and integration suites across auth methods.
- replay/revocation and distributed-state regression scenarios.
- degraded-provider and failover/fallback deterministic tests.
- release-profile benchmark runs for mapped auth targets.
- **New for v1.2.0**: async non-blocking behavior validation
- **New for v1.2.0**: connection pool health checks and reuse metrics
- **New for v1.3.0**: distributed sync under concurrent load
- **New for v1.3.0**: cluster convergence and leader election tests

## Performance Targets

### v1.2.0
- LDAP bind latency P99 ≤ 50 ms visible to callers (backend may take 200 ms)
- OAuth token requests never block caller's thread
- OIDC discovery fetches run in background, cached results available immediately
- HTTP retry logic completes within configured timeout (default 30 sec)

### v1.3.0
- isRevoked() remains O(1) lookup in RocksDB (constant-time)
- add() to RocksDB + local cache negligible overhead (< 1 ms)
- Cluster sync every 30 seconds without blocking revocation checks
- Token validation hot path unaffected by replication activity
- Purge thread runs independently; does not compete with validation
- Leader election converges within 1 minute under normal conditions

## Security / Reliability

- maintain strict fail-closed behavior for credential/token/provider errors.
- preserve auditable decision paths for authn/authz and trust checks.
- enforce bounded resource behavior in rate-limiter and session/revocation components.
- keep diagnostics actionable for production incident response.
- **async ops**: exceptions propagated through futures, never silently dropped
- **connection pool**: stale connections detected and evicted before use
- **distributed blacklist**: RocksDB WAL ensures durability; no data loss on restart
- **cluster sync**: RPC failures cause follower to retry; leader continues accepting revocations

## Migration Path

Existing code using synchronous auth methods remains unchanged:
- `LDAPAuthenticator::authenticate()` continues to work as before
- `OIDCProvider::validateToken()` remains synchronous
- Token blacklist API is additive; new distributed variant is opt-in

New async code can opt-in:
- Call `authenticateAsync()` to get non-blocking future
- Configure AsyncHTTPAuth for OAuth/OIDC operations
- Enable `DistributedTokenBlacklist` for cluster deployments

No database migration required; RocksDB schema is auto-created on first use.