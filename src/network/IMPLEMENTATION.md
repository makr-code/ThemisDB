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

Exact model backend not verifiable from final document text alone.

# Network Module Implementation Notes

## Purpose and ownership

This document is for contributors implementing or changing `src/network/`.
Public contracts are declared under [`include/network/`](../../include/network/)
and take precedence over this implementation guide. The module owns wire
sessions, transport adapters, connection-pool mechanics, and network-side
resilience controls; the HTTP/REST API service and application-level query
semantics are owned elsewhere.

## Wire server lifecycle and dispatch

`WireProtocolServer` defaults to TCP port 8766 and maintains separate Asio I/O
threads and a request worker pool. A session reads a frame, validates it,
applies authentication/rate and connection guards, dispatches the opcode, and
queues the response. Heavy handlers are posted with per-dispatch payload
storage so that the I/O thread can continue reading without reusing the
worker’s input buffer. Preserve that ownership boundary when changing frame
dispatch; the payload being parsed by a worker must not be overwritten by the
next read.

Before calling `start()`:

1. Configure the listener and limits in `WireProtocolServer::Config`.
2. Call `validateTransportSecurity()` and stop startup if it returns false.
3. Supply the runtime dependencies checked by `start()`: a `QueryEngine` and
   at least one geospatial backend (`SpatialIndexManager` or the registered
   `GeoQueryFn` bridge).
4. If authentication is required, use the supported `SCRAM-SHA-256`
   configuration and a non-blank, valid token. Do not copy secrets into
   source or logs.

`stop()` is the lifecycle boundary for releasing the listener, sessions, and
worker resources. Do not call start/stop concurrently. Handler objects passed
to the server must remain valid for the server’s runtime; the owning server
layer is responsible for the lifetime and synchronization of shared
dependencies.

## Other transport implementation boundaries

- `UDPServer` accepts the documented METRIC, LOG, EVENT, BATCH, and PING
  packet types. It validates packet framing, applies source-IP rate limits and
  bounded sequence-number deduplication, and optionally sends application ACKs.
  Its callback runs on an internal dispatch thread and must be thread-safe.
- `QuicTransport` is compiled under `THEMIS_ENABLE_HTTP3`. It uses QUIC with
  TLS 1.3 and a binary wire-protocol ALPN; its `start()`/`stop()` calls must
  not run concurrently. Configure certificate/key paths for a real deployment.
- WebSocket upgrade is conditional on `THEMIS_ENABLE_WEBSOCKET` and
  `enable_websocket_upgrade`; it shares the wire-protocol listener rather
  than being a separate guaranteed listener.
- xDS/service-mesh code is conditional on `THEMIS_ENABLE_SERVICE_MESH`.
  `EnvoyXdsClient` surfaces parsed configuration to callbacks; applying that
  configuration remains the caller’s responsibility.

## Invariants for changes

- Reject malformed or oversized input before application dispatch.
- Keep authentication and request rejection ahead of protected handlers.
- Bound retry count and delay; retry only where replay is safe.
- Keep callback and sink lifetimes valid while background threads may invoke
  them. `NetworkObservabilityRegistry::setSink()` does not take ownership, and
  the sink must outlive its registration.
- Preserve existing build guards. A feature present in a header is not
  necessarily enabled in every build.
- Do not describe the stub protocol-path benchmarks as production transport
  measurements; see [`RESEARCH.md`](RESEARCH.md).

## Focused regression evidence

The CMake module tests are generated from `tests/network/test_*.cpp`; their
target names and CTest registrations are defined in
[`tests/network/CMakeLists.txt`](../../tests/network/CMakeLists.txt).
Select the tests matching the changed contract, such as
`test_network_contract_hardening_focused`,
`test_network_transport_resilience_focused`, or
`test_network_lifecycle_guardrails_focused`. Update module docs when a public
contract, build guard, error behavior, or operator procedure changes.

See [`API.md`](API.md), [`MODULE_GUIDE.md`](MODULE_GUIDE.md), and
[`ARCHITECTURE.md`](ARCHITECTURE.md).
