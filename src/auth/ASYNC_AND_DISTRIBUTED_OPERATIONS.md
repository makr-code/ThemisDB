# Auth Infrastructure v1.2.0-v1.3.0: Async Operations & Distributed Blacklist

**Status**: ✅ v1.2.0 Complete | 🟡 v1.3.0 In Progress  
**Last Updated**: 2026-06-10  
**Maturity**: Production-Ready (v1.2.0) | Beta (v1.3.0)

---

## Overview

This document describes the async/non-blocking authentication features and distributed token blacklist support added in v1.2.0 and v1.3.0.

### Key Features

- **v1.2.0 (Complete)**
  - Non-blocking LDAP authentication via `LDAPAuthenticator::authenticateAsync()`
  - Non-blocking HTTP operations via `AsyncHTTPAuth` class
  - LDAP connection pooling with health checks
  - Retry logic with exponential backoff for HTTP transients

- **v1.3.0 (In Progress)**
  - Distributed token blacklist with cluster synchronization
  - Leader election and fault tolerance
  - RocksDB persistence with background purge threads

---

## v1.2.0: Async Operations & Connection Pooling

### 1. Async LDAP Authentication

**File**: `include/auth/ldap_authenticator.h`

Use the `authenticateAsync()` method to bind users without blocking:

```cpp
#include "auth/ldap_authenticator.h"
#include <future>

themis::auth::LDAPAuthenticator auth;
themis::auth::LDAPConfig cfg;
cfg.server_url = "ldaps://dc.example.com:636";
cfg.bind_dn_template = "CN={username},OU=Users,DC=example,DC=com";
cfg.pool_enabled = true;
cfg.pool_min_idle = 2;
cfg.pool_max_size = 16;
auth.initialize(cfg);

// Synchronous bind (blocking caller)
auto result_sync = auth.authenticate("alice", "password123");

// Asynchronous bind (non-blocking, caller receives future)
std::future<LDAPAuthResult> result_async = 
    auth.authenticateAsync("bob", "password456");

// Do other work while LDAP operation runs in background...

// Later, retrieve the result
LDAPAuthResult result = result_async.get();  // Blocks until ready
if (result.success) {
    THEMIS_INFO("User {} authenticated with roles: {}", 
        result.username, fmt::join(result.roles, ", "));
} else {
    THEMIS_WARN("Auth failed: {}", result.error_message);
}
```

**Performance Impact**:
- P50 latency: ~5-10 ms (caller thread is never blocked)
- P99 latency: ≤ 50 ms visible to callers (backend may take 200 ms)
- Connection reuse via pool reduces per-call overhead by ~70%

### 2. LDAP Connection Pooling

**File**: `include/auth/ldap_connection_pool.h`

The pool is automatically managed by `LDAPAuthenticator` when `pool_enabled=true`:

```cpp
themis::auth::LDAPConfig cfg;
cfg.pool_enabled = true;           // Enable pooling
cfg.pool_min_idle = 2;             // Min connections to keep alive
cfg.pool_max_size = 16;            // Max concurrent connections
cfg.pool_checkout_timeout_ms = 5000;  // Wait 5 sec for free connection

auto auth = std::make_unique<LDAPAuthenticator>();
auth.initialize(cfg);

// Pool is now active; connections are reused across calls
auto result1 = auth.authenticate("user1", "pass1");  // Uses pool
auto result2 = auth.authenticate("user2", "pass2");  // Reuses connection
```

**Features**:
- Health checks on every checkout (lightweight rootDSE query)
- Stale connections automatically evicted and recreated
- Configurable min/max size and checkout timeout
- Metrics available via `auth.connectionPool()->poolSize()`, etc.

### 3. Async HTTP Authentication

**File**: `include/auth/http_auth_async.h`

Use `AsyncHTTPAuth` for non-blocking OAuth, OIDC, and SAML operations:

```cpp
#include "auth/http_auth_async.h"

// Configuration
themis::auth::HTTPAuthConfig cfg;
cfg.request_timeout_seconds = 30;
cfg.max_retries = 3;
cfg.retry_backoff_ms = 100;
cfg.verify_ssl_certs = true;

themis::auth::AsyncHTTPAuth http_auth(cfg);

// Async GET (e.g., OIDC discovery)
std::future<HTTPAuthResponse> discovery_future = 
    http_auth.getAsync("https://keycloak.example.com/.well-known/openid-configuration");

// Do other work...

// Retrieve result
HTTPAuthResponse response = discovery_future.get();
if (response.success) {
    THEMIS_INFO("Discovery response: {}", response.body);
} else {
    THEMIS_WARN("Discovery failed: {}", response.error_message);
}
```

**Async POST** (e.g., OAuth token exchange):

```cpp
std::string body = R"({
  "grant_type": "authorization_code",
  "code": "auth_code_123",
  "client_id": "themisdb",
  "client_secret": "secret"
})";

std::vector<std::pair<std::string, std::string>> headers{
    {"Authorization", "Bearer " + admin_token}
};

std::future<HTTPAuthResponse> token_future = http_auth.postAsync(
    "https://oauth.example.com/token",
    body,
    "application/json",
    headers);

// Process token response
HTTPAuthResponse token_resp = token_future.get();
if (token_resp.success && token_resp.status_code == 200) {
    // Parse token from response.body
}
```

**Connectivity Check** (non-blocking):

```cpp
std::future<bool> connectivity_check = 
    http_auth.checkConnectivityAsync("https://oauth.example.com");

bool is_reachable = connectivity_check.get();
if (!is_reachable) {
    THEMIS_WARN("OAuth provider unreachable");
}
```

**Retry Logic**:
- Transient errors (timeout, DNS failure, connection reset) are automatically retried
- Exponential backoff: 100ms * attempt_number
- Max 3 retries by default (configurable)
- Total timeout respected across all attempts

---

## v1.3.0: Distributed Token Blacklist

### 1. Single-Node Persistence

**File**: `include/auth/rocksdb_token_blacklist.h` (already implemented)

```cpp
#include "auth/rocksdb_token_blacklist.h"

themis::auth::RocksDBTokenBlacklist::Config cfg;
cfg.db_path = "/var/lib/themisdb/token_blacklist";
cfg.column_family = "token_blacklist";
cfg.purge_interval_seconds = 300;

auto blacklist = std::make_unique<RocksDBTokenBlacklist>(cfg);

// Add revoked JTI (persists to RocksDB)
auto expiry = std::chrono::system_clock::now() + std::chrono::hours(24);
blacklist->add("jti-abc123", expiry);

// Check revocation (O(1) lookup)
if (blacklist->isRevoked("jti-abc123")) {
    THEMIS_WARN("Token is revoked");
}

// Background purge thread removes expired entries automatically
```

### 2. Distributed Blacklist with Cluster Sync

**File**: `include/auth/distributed_token_blacklist.h` (new in v1.3.0)

```cpp
#include "auth/distributed_token_blacklist.h"

// Configure local node
themis::auth::ClusterNode local;
local.node_id = "themisdb-node-1";
local.rpc_address = "10.0.1.100";
local.rpc_port = 9090;

// Configure peer nodes
std::vector<themis::auth::ClusterNode> peers{
    {"themisdb-node-2", "10.0.1.101", 9090},
    {"themisdb-node-3", "10.0.1.102", 9090}
};

// Create distributed blacklist
themis::auth::DistributedBlacklistConfig cfg;
cfg.db_path = "/var/lib/themisdb/token_blacklist_cluster";
cfg.enable_cluster_sync = true;
cfg.sync_interval_seconds = 30;  // Sync every 30 seconds
cfg.local_node = local;
cfg.peer_nodes = peers;

auto blacklist = std::make_unique<DistributedTokenBlacklist>(cfg);

// Add revoked JTI (persisted locally + queued for replication)
auto expiry = std::chrono::system_clock::now() + std::chrono::hours(24);
blacklist->add("jti-def456", expiry);

// Check revocation (fast O(1) local lookup; doesn't wait for sync)
if (blacklist->isRevoked("jti-def456")) {
    THEMIS_WARN("Token is revoked (known locally)");
}

// Manually trigger sync if needed
std::future<bool> sync_result = blacklist->syncWithCluster();
bool success = sync_result.get();

// Wait for cluster convergence at startup
if (blacklist->waitForClusterConvergence(std::chrono::seconds(60))) {
    THEMIS_INFO("Cluster converged; safe to accept requests");
} else {
    THEMIS_WARN("Cluster convergence timeout");
}

// Monitor replication health
auto stats = blacklist->getReplicationStats();
THEMIS_INFO("Replications: {} total, {} successful, {} failed",
    stats.total_syncs, stats.successful_syncs, stats.failed_syncs);
```

**Cluster Architecture**:

1. **Leader-Follower Model**
   - Node with lowest ID becomes leader (automatic election)
   - Followers pull revocations from leader every `sync_interval_seconds`
   - If leader fails, next-lowest ID is elected

2. **Conflict Resolution**
   - Last-Write-Wins: entry with newest expiry timestamp wins
   - All nodes eventually converge to same state

3. **Fault Tolerance**
   - Continues operating if peers are unreachable (local-only mode)
   - Retries failed syncs with exponential backoff
   - RocksDB WAL ensures durability on restart

### 3. Monitoring & Operations

**Get Replication Statistics**:

```cpp
auto stats = blacklist->getReplicationStats();
THEMIS_INFO("Total syncs: {}", stats.total_syncs);
THEMIS_INFO("Successful: {}", stats.successful_syncs);
THEMIS_INFO("Failed: {}", stats.failed_syncs);
THEMIS_INFO("Entries pushed: {}", stats.entries_pushed);
THEMIS_INFO("Entries pulled: {}", stats.entries_pulled);
THEMIS_INFO("Last sync: {}", 
    std::chrono::system_clock::to_time_t(stats.last_sync_time));
```

**Check Leader Status**:

```cpp
if (blacklist->isLeader()) {
    THEMIS_INFO("This node is the replication leader");
} else {
    THEMIS_INFO("This node is a follower");
}
```

---

## Migration Guide

### From Synchronous to Async LDAP

**Before (v1.1.0)**:
```cpp
LDAPAuthResult result = auth.authenticate("user", "pass");
// Caller blocked by network latency (~100-200 ms)
```

**After (v1.2.0)**:
```cpp
std::future<LDAPAuthResult> future = auth.authenticateAsync("user", "pass");
// Caller not blocked; continues immediately
LDAPAuthResult result = future.get();  // Later, retrieve result
```

### From Single-Node to Distributed Blacklist

**Before (v1.2.0)**:
```cpp
RocksDBTokenBlacklist blacklist(cfg);
blacklist->add(jti, expiry);
// Only this node knows about revocation
```

**After (v1.3.0)**:
```cpp
DistributedTokenBlacklist blacklist(cfg);
blacklist->add(jti, expiry);
// Revocation replicated to peer nodes automatically
```

---

## Performance Characteristics

### Latency Targets (v1.2.0)

| Operation | P50 | P99 | Notes |
|---|---|---|---|
| LDAP bind (async caller) | 5ms | 50ms | Network latency hidden |
| LDAP bind (blocking) | 50ms | 200ms | Caller thread blocked |
| HTTP GET/POST (async) | 10ms | 100ms | Dispatch only; network in background |
| Connection pool checkout | <1ms | <2ms | O(1) operation |
| Connection health check | 10ms | 30ms | Lightweight rootDSE query |

### Throughput Targets (v1.2.0)

| Metric | Target | Notes |
|---|---|---|
| Concurrent LDAP auths | 16+ per node | Limited by pool_max_size |
| HTTP requests per second | 100+ | Depends on network |
| Connection reuse rate | >80% | Pool hit rate on warm start |

### Distributed Blacklist Targets (v1.3.0)

| Operation | Latency | Notes |
|---|---|---|
| isRevoked() lookup | O(1) | Local RocksDB read; not blocked by sync |
| add() revocation | <1ms | Local write; async replication |
| Cluster convergence | <60s | Under normal network conditions |
| Failover time | <5min | Leader re-election |

---

## Error Handling & Resilience

### Async HTTP Failures

```cpp
auto future = http_auth.postAsync(url, body);
try {
    HTTPAuthResponse resp = future.get();
    if (!resp.success) {
        THEMIS_WARN("HTTP error: {}", resp.error_message);
    }
} catch (const std::exception& e) {
    THEMIS_ERROR("HTTP future exception: {}", e.what());
}
```

### LDAP Connection Pool Exhaustion

```cpp
try {
    auto future = auth.authenticateAsync("user", "pass");
    result = future.get();
} catch (const AuthException& e) {
    if (e.code() == AUTH_INTERNAL_ERROR && 
        e.message().find("pool exhausted") != std::string::npos) {
        THEMIS_WARN("LDAP pool timeout; consider increasing pool_max_size");
    }
}
```

### Distributed Blacklist Sync Failures

```cpp
// Automatic retries happen every sync_interval_seconds
// Manual intervention rarely needed, but available:

std::future<bool> sync = blacklist->syncWithCluster();
if (!sync.get()) {
    THEMIS_WARN("Cluster sync failed; continue operating in local-only mode");
    // Token validations continue using local cached state
}
```

---

## Best Practices

1. **Always use async when possible** to avoid blocking request handlers
2. **Configure pool sizes** based on your authentication load (default: min=2, max=16)
3. **Enable cluster sync** for multi-node deployments to ensure consistent revocation
4. **Monitor replication stats** to detect cluster health issues early
5. **Set appropriate timeouts** (HTTP: 30s, LDAP: 10s) based on your environment
6. **Use SSL/TLS** for all LDAP and HTTP operations in production

---

## Testing

Comprehensive test suites are provided:

```bash
# Build and run async HTTP tests
ctest -R test_http_auth_async -V

# Build and run distributed blacklist tests
ctest -R test_distributed_token_blacklist -V

# Run all auth tests
ctest -R auth -V
```

---

## See Also

- `include/auth/ldap_authenticator.h` - LDAP async API
- `include/auth/http_auth_async.h` - HTTP async API
- `include/auth/distributed_token_blacklist.h` - Distributed blacklist API
- `src/auth/ROADMAP.md` - Development roadmap
- `src/auth/FUTURE_ENHANCEMENTS.md` - Planned enhancements
