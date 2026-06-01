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
