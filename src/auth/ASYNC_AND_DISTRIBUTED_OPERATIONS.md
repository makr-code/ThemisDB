# Auth Async and Distributed Operations

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

This note documents the async APIs and distributed blacklist behavior currently
declared in `include/auth` and implemented in `src/auth`. It is not a latency
SLO or a deployment guide.

## Async LDAP

`LDAPAuthenticator::authenticateAsync(username, password)` returns
`std::future<LDAPAuthResult>` and dispatches the authentication operation to the
auth worker pool. Calling `future.get()` waits for the result. Pool use is
controlled by `LDAPConfig::pool_enabled` (default `true`); the public defaults
are `pool_min_idle = 2`, `pool_max_size = 16`, and
`pool_checkout_timeout_ms = 5000`. These are configuration defaults, not
capacity or latency guarantees.

```cpp
themis::auth::LDAPConfig config;
config.server_url = "ldaps://directory.example";
themis::auth::LDAPAuthenticator ldap(config);

auto pending = ldap.authenticateAsync("alice", password);
auto result = pending.get(); // waits until the LDAP result is ready
if (!result.success) {
    // Handle the returned LDAPAuthResult failure.
}
```

`LDAPAuthResult` reports success, username, DN, roles, groups, and an error
message. The header does not promise that `authenticateAsync()` itself can never
block during dispatch or that the remote operation completes within a fixed
time.

## Async HTTP helper

`AsyncHTTPAuth` offers `getAsync()`, `postAsync()`, and
`checkConnectivityAsync()`. The first two return
`std::future<HTTPAuthResponse>`; connectivity returns `std::future<bool>`.
`HTTPAuthConfig` declares a 30-second request timeout, three retries, a
100-millisecond retry backoff, HTTP/2 enabled, and certificate verification
enabled by default. These are configuration defaults; network latency and
success are environment-dependent.

The helper is generic HTTP infrastructure. Its presence does not establish that
every OAuth, OIDC, or SAML path uses it. Inspect the specific provider adapter
before assuming a flow is asynchronous or uses the same retry behavior.

## Distributed token blacklist

`DistributedTokenBlacklist` implements `ITokenBlacklist` with RocksDB storage
and optional TCP peer synchronization. Its configuration includes a local node,
peer nodes, `enable_cluster_sync` (default `true`), a 30-second sync interval,
and a 5-second peer RPC timeout. Replication and listener threads start only
when cluster sync is enabled and peers are configured. The local listener binds
the configured `local_node.rpc_port` (header default 9090); bind failure is
non-fatal, so the node may still make outbound connections.

Public distributed operations include:

- `syncWithCluster()`, returning `std::future<bool>`;
- `getReplicationStats()` and `isLeader()`;
- `waitForClusterConvergence(timeout)`;
- the inherited `add()`, `isRevoked()`, and `purgeExpired()` operations.

The implementation elects the peer with the lexicographically lowest node ID
locally. A leader pushes entries to peers; a follower pulls from the selected
leader and may push locally held entries. This is best-effort peer
synchronization, not quorum consensus.

### Important consistency limitation

`applyEntries()` writes received expiries to RocksDB without comparing them
with existing values. Therefore the source does not establish conventional
last-write-wins conflict resolution. `waitForClusterConvergence()` returns
after observing at least one successful sync, not after proving every peer has
the same complete state. No bounded convergence or failover-time guarantee is
verified; deployments needing stronger guarantees require an external
consistency design.

`isRevoked()` returns true for RocksDB errors other than `NotFound` and checks
the stored expiry for successful reads. A successful read with a malformed
expiry encoding throws during decoding. It is a local store lookup; it does not
wait for a synchronization round.

## Validation evidence and limits

- The distributed-blacklist API and behavior are covered by
  `tests/auth/test_auth_distributed_blacklist.cpp`.
- LDAP async and pool code is in `src/auth/ldap_authenticator.cpp` and
  `src/auth/ldap_connection_pool.cpp`; focused auth tests are registered from
  `tests/auth/CMakeLists.txt`.
- `benchmarks/auth/bench_auth_hotpaths.cpp` measures local hot-path cases; its
  thresholds are not network-provider performance measurements.
- No benchmark or test execution result is claimed here.

## References

- `include/auth/ldap_authenticator.h`
- `include/auth/http_auth_async.h`
- `include/auth/distributed_token_blacklist.h`
- `src/auth/ldap_authenticator.cpp`
- `src/auth/http_auth_async.cpp`
- `src/auth/distributed_token_blacklist.cpp`
- `tests/auth/test_auth_distributed_blacklist.cpp`
- `src/auth/PERFORMANCE_EXPECTATIONS.md`
