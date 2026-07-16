# Distributed API Gateway

**Status:** ✅ Implemented (v2.1.0)  
**Files:** `src/server/distributed_gateway.cpp`, `include/server/distributed_gateway.h`  
**Tests:** `tests/test_distributed_gateway.cpp` (41 unit tests)

---

## Overview

The Distributed API Gateway (`DistributedGateway`) extends the single-node
`APIGateway` with multi-node routing, Raft-based configuration replication, and
automatic leader failover.  It is designed for clusters of 3 or 5 gateway nodes
and transparently handles routing of all HTTP, WebSocket, and SSE traffic.

```
  Client → Load Balancer → [GatewayNode A]
                         → [GatewayNode B]  ← Raft cluster
                         → [GatewayNode C]
                               ↓
                         (config replicated via Raft log)
                               ↓
                         Backend services / shards
```

---

## Key Properties

| Property | Target |
|----------|--------|
| Leader failover | ≤ 500 ms |
| Config propagation (5-node LAN) | ≤ 100 ms |
| Per-request overhead vs single-node | None |
| Consistent-hash virtual nodes (default) | 150 per peer |

---

## Architecture

### Raft-based Configuration Replication

All routing rules (`GatewayRouteConfig`) and per-client rate limits are stored
in a `ClusterGatewayConfig` struct that is replicated through the Raft log.
Configuration mutations are only accepted by the current Raft leader.

```
Leader node:
  DistributedGateway::proposeConfig(new_config)
    → serialise to JSON
    → RaftConsensus::propose(entry)
    → committed entry broadcast to all followers
    → each node: DistributedGateway::applyConfigEntry(entry_json)
```

### Session Affinity (WebSocket / SSE)

Stateful connections (WebSocket upgrades and `text/event-stream` requests) are
pinned to the same gateway node via a `ConsistentHashRing`.  The ring uses
FNV-1a 64-bit hashing with 150 virtual nodes per physical node by default,
ensuring ≤ 1 % load imbalance under typical traffic patterns.

### Quorum-loss Degradation

When a cluster loses quorum (fewer than ⌈N/2⌉+1 nodes reachable):
- Ongoing requests continue to be served using the **last committed config**.
- A `CRITICAL`-level spdlog message is emitted once per quorum-loss event.
- Config mutations (calls to `proposeConfig`) are **rejected** immediately.
- As soon as quorum is restored, normal operation resumes and an `INFO` message
  is emitted.

---

## Configuration

```cpp
DistributedGateway::Config cfg;

// Node identity
cfg.node_id       = "gw-1";
cfg.bind_address  = "0.0.0.0";
cfg.bind_port     = 8080;

// Peer list (include this node)
cfg.cluster_nodes = {
    {"gw-1", "10.0.0.1", 8080},
    {"gw-2", "10.0.0.2", 8080},
    {"gw-3", "10.0.0.3", 8080},
};

// Raft tuning
cfg.election_timeout_min_ms = 150;   // ms
cfg.election_timeout_max_ms = 300;   // ms
cfg.heartbeat_interval_ms   = 50;    // ms
cfg.leader_failover_timeout = std::chrono::milliseconds{500};

// Consistent-hash ring
cfg.virtual_nodes_per_peer = 150;

// Graceful degradation on quorum loss
cfg.continue_on_quorum_loss = true;
```

---

## Usage

```cpp
#include "server/distributed_gateway.h"
#include "server/api_gateway.h"

// 1. Create the underlying single-node gateway
auto auth         = std::make_shared<themis::AuthMiddleware>();
auto rate_limiter = std::make_shared<themis::server::RateLimiter>();
auto load_shedder = std::make_shared<themis::server::LoadShedder>({});

APIGateway::Config gw_cfg;
gw_cfg.gateway_id = "my-gateway";
auto gateway = std::make_shared<themis::server::APIGateway>(
    gw_cfg, auth, rate_limiter, load_shedder);

// 2. Wrap with the distributed layer
themis::server::DistributedGateway::Config dist_cfg;
dist_cfg.node_id = "gw-1";
dist_cfg.cluster_nodes = {
    {"gw-1", "10.0.0.1", 8080},
    {"gw-2", "10.0.0.2", 8080},
    {"gw-3", "10.0.0.3", 8080},
};

themis::server::DistributedGateway dist_gw(dist_cfg, gateway);

// 3. Start Raft + hash ring
dist_gw.start();

// 4. Route incoming requests
auto response = dist_gw.handleRequest(req, local_handler);

// 5. Propose a config change (leader only)
themis::server::ClusterGatewayConfig new_cfg;
new_cfg.version = 1;
new_cfg.routes.push_back({"/api/v1/", "http://backend:9090", 5000, 2, true, 5});
bool committed = dist_gw.proposeConfig(new_cfg);

// 6. Query cluster status
nlohmann::json status = dist_gw.getClusterStatus();

// 7. Graceful shutdown
dist_gw.stop();
```

---

## API Reference

### DistributedGateway

| Method | Description |
|--------|-------------|
| `DistributedGateway(config, gateway)` | Construct with config and non-null APIGateway |
| `start()` | Start Raft consensus and hash ring |
| `stop()` | Gracefully stop |
| `handleRequest(req, local_handler)` | Route an HTTP request |
| `resolveAffinityNode(session_key)` | Determine affinity node for a key |
| `proposeConfig(new_config)` | Propose a cluster-wide config change (leader only) |
| `getCurrentConfig()` | Return last-committed config |
| `registerHandler(pattern, handler)` | Register a local handler (delegates to APIGateway) |
| `registerDeprecation(endpoint, info)` | Register a deprecated endpoint |
| `isLeader()` | True if this node is the Raft leader |
| `hasQuorum()` | True if cluster has quorum |
| `getLeaderId()` | Return current leader node ID |
| `getClusterStatus()` | JSON status snapshot for monitoring |
| `applyConfigEntry(entry_json)` | Apply a replicated Raft log entry (also exposed for testing) |

### ClusterGatewayConfig

| Field | Type | Description |
|-------|------|-------------|
| `version` | `uint64_t` | Monotonically increasing config version |
| `routes` | `vector<GatewayRouteConfig>` | Ordered routing rules |
| `rate_limits` | `unordered_map<string, uint32_t>` | Per-client-key limit (req/s) |
| `global_rate_limit_rps` | `uint32_t` | Cluster-wide default req/s |
| `updated_by` | `string` | Node that committed this version |
| `updated_at` | `time_point` | Commit timestamp |

### GatewayRouteConfig

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `path_prefix` | `string` | `""` | Path prefix to match |
| `upstream_url` | `string` | `""` | Target upstream URL |
| `timeout_ms` | `uint32_t` | `30000` | Per-request timeout (ms) |
| `retry_count` | `uint32_t` | `2` | Retry attempts on transient errors |
| `circuit_breaker_enabled` | `bool` | `true` | Enable circuit breaker |
| `circuit_breaker_failure_threshold` | `uint32_t` | `5` | Failure count to open circuit |

---

## Error Handling

| Scenario | Behaviour |
|----------|-----------|
| `gateway` is null in constructor | `std::invalid_argument` thrown |
| `proposeConfig` called on non-leader | Returns `false`, logs `WARN` |
| `proposeConfig` called with no quorum | Returns `false`, logs `ERROR` |
| Commit times out | Returns `false`, logs `WARN` with timeout duration |
| `applyConfigEntry` receives invalid JSON | Returns `false`, logs `ERROR` |
| `applyConfigEntry` receives stale version | Returns `true` (idempotent, silent) |
| Quorum lost during operation | Logs `CRITICAL`, continues with last-known config |
| Quorum restored | Logs `INFO`, resumes normal operation |

---

## Testing

The full unit-test suite is in `tests/test_distributed_gateway.cpp` (41 tests):

```bash
# Build and run the focused test target
cmake --build build --target test_distributed_gateway_focused
ctest --test-dir build -R DistributedGatewayFocusedTests --output-on-failure
```

Test suites covered:

- `GatewayRouteConfigTest` – JSON round-trip and defaults
- `ClusterGatewayConfigTest` – JSON round-trip and empty-JSON safety
- `ConsistentHashRingTest` – empty ring, single node, deterministic routing, distribution, remove/re-route, flap stability
- `DistributedGatewayTest` – construction, lifecycle (start/stop idempotency), cluster status, config apply, session-affinity detection, request delegation, handler/deprecation registration, quorum-loss flag, concurrent apply safety

---

## See Also

- [`include/server/distributed_gateway.h`](../include/server/distributed_gateway.h) – Public API
- [`src/server/distributed_gateway.cpp`](../src/server/distributed_gateway.cpp) – Implementation
- [`src/server/api_gateway.cpp`](../src/server/api_gateway.cpp) – Single-node gateway (wrapped)
- [`include/sharding/raft_consensus.h`](../include/sharding/raft_consensus.h) – Raft engine
- [`src/server/ROADMAP.md`](../src/server/ROADMAP.md) – Server module roadmap
