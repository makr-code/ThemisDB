---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Network Module - Architecture Guide

<!-- Status: review | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Version:** 1.3
**Last Updated:** 2026-09-09
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
3. Dispatch supported opcodes to configured storage/query/index integration hooks; unavailable integrations return structured errors.
4. Emit framed responses and maintain connection stats/error counters.
5. Apply timeout/rate/backpressure/circuit-breaker behavior on overload/failure paths.

## 4. Integration Boundaries

| Direction | Integration |
|---|---|
| Delegates to | storage/query/index/process/time-series integration pointers configured in `WireProtocolServer` |
| Used by | native clients and internal node-to-node transport paths |
| Complements | HTTP/REST and higher-level gRPC service layers outside this module |

## 5. Concurrency Model

- Each session uses asynchronous socket operations on the server's shared Asio I/O context; it does not own a dedicated connection thread.
- Heavy handlers are posted to the server worker pool with per-dispatch payload storage so the I/O thread can continue reading safely.
- Shared counters/registries and connection maps use explicit synchronization and atomic state in hot paths.
- Timeout and breaker helpers run independently and feed rejection/close decisions back into connection/session flow.

## 6. Known Limits (Source-Visible)

- Feature-rich transport surfaces exist, but availability can depend on build/runtime wiring.
- Query/geospatial execution over wire protocol is integration-dependent (`query_engine_`, spatial index/bridge callback).
- Network-specific release/routing benchmark gates are in `benchmarks/network/`; they exercise in-process helper paths. The protocol-path cases are stubs, and no live-transport hardware baseline is established (see `RESEARCH.md`).
- `NetworkAuditLog` has a standalone implementation, but production call-site integration is not verified in the inspected build/source wiring.

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

## 8. EnvoyXdsClient — xDS REST polling

`EnvoyXdsClient` in `include/network/envoy_xds.h` is compiled with
`THEMIS_ENABLE_SERVICE_MESH`. It polls an xDS v3 control plane using separate
HTTP/1.1 JSON POST requests; it does not maintain a gRPC streaming connection.
The current implementation uses a plain TCP stream, so the control-plane
connection is not TLS-protected by this client.

`start()` launches one polling thread. In each pass it requests enabled LDS,
CDS, EDS, and RDS resources, parses changed responses, updates per-resource
versions, and invokes the corresponding configured callback. `stop()` signals
the polling thread and joins it. The defaults are a 15-second poll interval,
a 5-second wait after a pass with no updates or errors, a 10-second per-request
timeout, and control-plane port 15010; see `EnvoyXdsClient::Config` for the
runtime switches and values.

Callbacks run on the polling thread while `callbacks_mutex_` is held. Keep
callbacks short and do not call callback-registration methods from them, since
those methods acquire the same mutex. Do not call `stop()` from a callback;
it would attempt to join the current polling thread. Consumers must apply
received configuration themselves: `ServiceMeshIntegration` provides health
probes and does not automatically connect the xDS client to the topology
router or load balancer.

Request or parse failures are counted in the per-resource statistics; the
client waits for the configured reconnect interval before the next polling
pass. The implementation does not perform exponential backoff or send a
stream-level ACK/NACK.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` | Logging, audit, and thread-pool helpers for connection and session management |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/network/wire_protocol_server.h` (`WireProtocolServer`) | Server layer binds the wire-protocol runtime for native client connections |
| (node-to-node) | internal transport APIs | Internal replication and distributed coordination paths use network transport adapters |

## Integration Points

### Critical Integration: Wire Protocol Server ↔ Server Layer
**Files:** `src/network/wire_protocol_server.cpp` ↔ `server/`
**Contract:** `WireProtocolServer` dispatches opcodes to storage/query/index integration hooks configured at server startup; handler pointers must remain valid for the server lifetime.
**Thread Safety:** Sessions use the server's shared Asio I/O context and may dispatch heavy handlers to its worker pool. Shared application dependencies must remain valid and synchronized for concurrent handler access.

### Envoy xDS / Service Mesh
**Files:** `src/network/envoy_xds.cpp`, `src/network/service_mesh.cpp`
**Contract:** `EnvoyXdsClient` exposes parsed xDS responses through callbacks. The service-mesh probe server is a separate component; applying xDS updates to routing remains the callback consumer's responsibility.
**Thread Safety:** xDS callbacks execute on the polling thread with `callbacks_mutex_` held. Keep them short and avoid reentering callback-registration or lifecycle methods (see §8).
