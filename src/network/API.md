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

# Network Module API Guide

## Scope and canonical interfaces

This page indexes the public network-facing contracts. The declarations in
[`include/network/`](../../include/network/) are authoritative; this guide
does not add a protocol compatibility promise beyond those declarations.
Application API endpoints and query language semantics are outside
`src/network/`.

## Principal interfaces

| Interface | Contract and caller responsibility |
|---|---|
| `WireProtocolServer` — [`wire_protocol_server.h`](../../include/network/wire_protocol_server.h) | Configure, preflight, start, stop, and inspect the native TCP wire server. Defaults include port 8766, 64 MiB maximum frame size, per-IP connection limits, request/auth timeouts, and authentication required. The server requires application dependencies; its `start()` preflight also requires a query engine and a geo-query backend. |
| `NetworkErrorCode` — [`network_api_contract.h`](../../include/network/network_api_contract.h) | Canonical error categories for protocol/frame, auth/session, transport, rate/backpressure, routing, connection-limit, and internal failures. Callers must treat fail-closed errors differently from retryable rate/backpressure responses. |
| `PacketParser` / `PacketBuilder` — [`wire_protocol.h`](../../include/network/wire_protocol.h) | Standalone stateless packet parser/builder for the `0xDB 0x01` framing format: two magic bytes, one type byte, a four-byte big-endian payload length, then payload. Parsing raises `ParseError` for malformed, truncated, or oversized input. Do not assume this helper’s frame layout is interchangeable with every `WireProtocolServer` session/opcode format. |
| `UDPServer` — [`udp_server.h`](../../include/network/udp_server.h) | Fire-and-forget ingestion for METRIC, LOG, EVENT, BATCH, and PING. Handler invocation is on an internal dispatch thread. ACKs are optional; UDP delivery and ordering are not guaranteed. |
| `QuicTransport` — [`quic_transport.h`](../../include/network/quic_transport.h) | Optional QUIC wire-protocol transport, compiled with `THEMIS_ENABLE_HTTP3`; uses TLS 1.3 and configured certificate/key material. Do not call start/stop concurrently. |
| `WireProtocolConnectionPool` — [`wire_protocol_connection_pool.h`](../../include/network/wire_protocol_connection_pool.h) | Client-side per-target TCP connection reuse. `acquireConnection()` returns a move-only RAII handle or throws on acquisition failure/timeout. The legacy `socket()` accessor is unavailable for SSL connections; use `socketWrapper()`. |
| `GeoTopologyRouter` — [`geo_topology_router.h`](../../include/network/geo_topology_router.h) | Chooses among healthy shard endpoints using configured locality, latency hints, or round-robin policy. Returns an empty string when no endpoint is eligible; callers own connection establishment. |
| `NetworkObservabilityRegistry` — [`network_observability.h`](../../include/network/network_observability.h) | Optional process-wide event sink and counters. Register a thread-safe sink before starting I/O; registration is non-owning, and the default null sink discards events. |

## Validation and error handling

For the network server, malformed/oversized frames are rejected before normal
handler dispatch. Authentication and request guards must remain before
protected operations. The exact wire-server configuration and validation are
declared in `WireProtocolServer::Config` and implemented in
`src/network/wire_protocol_server.cpp`; consult those sources when changing
defaults or error mapping.

`PacketParser::parse()` is a separate API contract: it validates its own
seven-byte minimum framing header, magic, payload size, and available bytes.
Do not treat that helper as a complete substitute for session authentication,
opcode dispatch, or transport lifecycle.

## Compatibility guidance

`network_api_contract.h` states that its contracts are stable within the v1.x
major line and that breaking changes require v2.0 with migration notes. Keep
wire behavior additive where possible and update the contract, focused tests,
and release/migration documentation together when compatibility changes.

See [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for lifecycle and
[`MODULE_GUIDE.md`](MODULE_GUIDE.md) for focused test commands.
