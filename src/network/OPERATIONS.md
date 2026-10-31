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

# Network Module Operations

## Purpose and operating boundary

This page describes the checks an operator or service owner should make when
deploying the network components. Listener enablement and lifecycle belong to
the server process that constructs them; this module does not provide a
standalone deployment controller or universal admin CLI.

## Configuration and startup

- Review `WireProtocolServer::Config` before enabling the native listener:
  bind host/port, connection and per-IP limits, frame size, auth and request
  timeouts, per-IP request limits, TLS files, auth configuration, optional
  WebSocket upgrade, IPv6/dual-stack, and listen backlog.
- Call `validateTransportSecurity()` before `WireProtocolServer::start()`.
  Startup also checks required integration backends. If the required query or
  geospatial backend is absent, correct the service wiring rather than
  weakening the preflight.
- Enable WebSocket, HTTP/3, and service-mesh surfaces only when their build
  definitions and runtime configuration are present:
  `THEMIS_ENABLE_WEBSOCKET`, `THEMIS_ENABLE_HTTP3`, and
  `THEMIS_ENABLE_SERVICE_MESH`.
- For QUIC, provision the certificate chain and private key securely and
  restrict key-file access. QUIC uses TLS 1.3; its encryption does not replace
  wire-protocol authentication or application authorization.
- UDP ingestion is connectionless and best-effort. Use it only for payloads
  whose loss/reordering behavior is acceptable; enable application ACKs only
  when the client uses the documented ACK protocol.

## Health and observability

The components expose local snapshots: `WireProtocolServer::getStats()`,
`UDPServer::getStats()`, `QuicTransport::getStats()`,
`WireProtocolConnectionPool::getStats()`,
`AdaptiveCircuitBreaker::getStats()`, and
`GeoTopologyRouter::getStats()`. Where these snapshots are not exported by the
owning process, use its existing health endpoint or logs; do not assume a
particular Prometheus metric name.

The optional `NetworkObservabilityRegistry` defaults to a null sink that
discards events. To collect spans, register a thread-safe sink before any
network I/O threads start. Registration is non-owning, so the sink must remain
alive until event production has stopped. Span labels and error details are
views into caller-owned storage and must be consumed during the callback.

Correlate counters by component and time window. A rise in connection rejects
can reflect configured limits, while a breaker rejection reflects the
breaker state; they are not interchangeable health signals.

## Graceful shutdown, restart, and rollback

The service owner must stop accepting new traffic before shutting down
dependent storage/query services. Call transport `stop()` methods through the
owner’s lifecycle path, wait for their threads/resources to be released, then
stop dependencies. Start in the reverse order so required backends are ready
before listeners accept requests.

Do not run concurrent `start()` and `stop()` calls on `UDPServer`,
`QuicTransport`, or `EnvoyXdsClient`; their public contracts explicitly
disallow it. If startup reports a bind or TLS error, correct the underlying
address, port, or credential issue and restart via the owning service manager.
Roll back a deployment when the failure tracks to a code/config release and
the prior configuration is known to satisfy the same authentication and
transport requirements.

## Capacity and change review

Treat frame size, connection limits, request rates, worker count, UDP batching,
QUIC stream/flow-control windows, and pool sizing as capacity controls with
memory, CPU, and latency consequences. Change one relevant dimension at a
time, monitor the component counters, and test the expected overload path.
Performance gates are described in [`RESEARCH.md`](RESEARCH.md); their target
values do not constitute a measured deployment capacity.

For incident diagnosis and recovery steps, see [`RUNBOOK.md`](RUNBOOK.md).
Security constraints are in [`SECURITY.md`](SECURITY.md) and
[`TRUST.md`](TRUST.md).
