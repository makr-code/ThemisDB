> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Network Module - Architecture Guide

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Version:** 1.3
**Last Updated:** 2026-05-31
**Module Path:** `src/network/`

## 1. Overview

The network module provides the wire-protocol server runtime, transport adapters, connection lifecycle helpers, and network-side reliability/security guards.

## 2. Architecture Surfaces

| Surface | Source files |
|---|---|
| Core wire protocol server/session handling | `wire_protocol_server.cpp`, `wire_protocol_helpers.cpp` |
| Connection reuse and pooling | `wire_protocol_connection_pool.cpp` |
| Protocol variants and transport adapters | `wire_protocol_v2.cpp`, `wire_protocol_server_ws.cpp`, `udp_fast_path.cpp`, `udp_server.cpp`, `quic_transport.cpp`, `grpc_transport.cpp` |
| Timeout, backpressure, and failure control | `socket_timeout_manager.cpp`, `adaptive_circuit_breaker.cpp` |
| Throughput/serialization optimizations | `wire_protocol_batch.cpp`, `wire_protocol_zero_copy.cpp`, `connection_compression.cpp` |
| Routing/topology/load balancing | `geo_topology_router.cpp`, `raft_load_balancer.cpp` |
| Mesh/audit/runtime-adaptive helpers | `service_mesh.cpp`, `envoy_xds.cpp`, `network_audit_log.cpp`, `adaptive_io_scaler.h` |

## 3. Runtime Control Flow

1. Accept connection and parse frames in wire session context.
2. Enforce auth/session checks and request guards before operation execution.
3. Dispatch supported opcodes to storage/query/index integration hooks.
4. Emit framed responses and maintain connection stats/error counters.
5. Apply timeout/rate/backpressure/circuit-breaker behavior on overload/failure paths.

## 4. Integration Boundaries

| Direction | Integration |
|---|---|
| Delegates to | storage/query/index/process/time-series integration pointers configured in `WireProtocolServer` |
| Used by | native clients and internal node-to-node transport paths |
| Complements | HTTP/REST and higher-level gRPC service layers outside this module |

## 5. Concurrency Model

- Session-level wire handling and asynchronous response writes are coordinated inside `wire_protocol_server.cpp`.
- Shared counters/registries and connection maps use explicit synchronization and atomic state in hot paths.
- Timeout and breaker helpers run independently and feed rejection/close decisions back into connection/session flow.

## 6. Known Limits (Source-Visible)

- Feature-rich transport surfaces exist, but availability can depend on build/runtime wiring.
- Query/geospatial execution over wire protocol is integration-dependent (`query_engine_`, spatial index/bridge callback).
- Dedicated transport benchmark coverage is partial; stream-protocol benchmarks currently provide the strongest direct module signal.

## 7. Sourcecode Verification (Module: network/architecture)

- Verified files:
  - `src/network/wire_protocol_server.cpp`
  - `src/network/wire_protocol_helpers.cpp`
  - `src/network/wire_protocol_connection_pool.cpp`
  - `src/network/wire_protocol_v2.cpp`
  - `src/network/wire_protocol_server_ws.cpp`
  - `src/network/udp_fast_path.cpp`
  - `src/network/udp_server.cpp`
  - `src/network/quic_transport.cpp`
  - `src/network/grpc_transport.cpp`
  - `src/network/socket_timeout_manager.cpp`
  - `src/network/adaptive_circuit_breaker.cpp`
  - `src/network/wire_protocol_batch.cpp`
  - `src/network/wire_protocol_zero_copy.cpp`
  - `src/network/connection_compression.cpp`
  - `src/network/geo_topology_router.cpp`
  - `src/network/raft_load_balancer.cpp`
  - `src/network/network_audit_log.cpp`
- Verified interfaces/behaviors:
  - session/auth/frame validation and opcode dispatch
  - transport and routing surfaces
  - timeout/backpressure/circuit-breaker behavior
  - batching/zero-copy/compression optimization paths

## 8. EnvoyXDSClient — xDS Resource Subscription Lifecycle

`EnvoyXDSClient` (implemented in `src/network/envoy_xds.cpp`,
public header `include/network/envoy_xds.h`) provides dynamic service-mesh
configuration updates via the xDS (Discovery Service) protocol used by Envoy
Proxy, Istio, and compatible control planes.

### Subscription Lifecycle

```
  Start
    │
    ├─ connect()  ──────→  TCP/gRPC stream established to xDS management server
    │                        (configurable host:port, TLS optional)
    │
    ├─ subscribe(type)  ──→  DiscoveryRequest sent for resource type:
    │                          - Cluster Discovery Service  (CDS)  — backend clusters
    │                          - Listener Discovery Service (LDS)  — front-door listeners
    │                          - Endpoint Discovery Service (EDS)  — per-cluster endpoints
    │                          - Route Configuration Service (RDS) — routing rules
    │
    ├─ onResponse(DiscoveryResponse)
    │    ├─ Parse resources from response (TypeUrl dispatch)
    │    ├─ Invoke registered callback per resource type
    │    └─ Send ACK (DiscoveryRequest with same version_info) or NACK on parse error
    │
    ├─ Reconnect loop (on stream error)
    │    ├─ Exponential backoff: 100 ms → 200 ms → 400 ms … max 30 s
    │    └─ Re-subscribe all active resource types after reconnect
    │
    └─ disconnect() ──────→  Stream closed; no further callbacks fired
```

### Resource Callback Contract

- Callbacks registered via `setClusterCallback()`, `setListenerCallback()`,
  `setEndpointCallback()`, and `setRouteCallback()`.
- Callbacks are invoked from the I/O thread while the internal lock is **not**
  held — it is safe to call `subscribe()` / `disconnect()` from within a callback.
- Callbacks must not block: delegate heavy processing to a separate thread.
- A NACK (parse failure) does NOT invoke the callback; the previous resource
  state is retained.

### Thread Safety

- `connect()`, `disconnect()`, and `subscribe()` are safe to call from any
  thread (internal mutex protects stream state).
- Multiple concurrent `subscribe()` calls for the same resource type are
  idempotent (only one DiscoveryRequest is sent per inflight round-trip).

### Error Handling

| Condition                     | Behaviour                                      |
|-------------------------------|------------------------------------------------|
| xDS server unreachable        | Reconnect loop with exponential backoff        |
| DiscoveryResponse parse error | NACK sent; previous resource state retained    |
| Resource version conflict     | Version mismatch logged; full re-subscribe     |
| Stream reset by server        | Reconnect loop triggered                       |

### Integration with ServiceMesh

`ServiceMesh` holds an `EnvoyXDSClient` instance and wires the cluster/endpoint
callbacks directly into `GeoTopologyRouter` and `RaftLoadBalancer` for
live topology updates without a restart.
