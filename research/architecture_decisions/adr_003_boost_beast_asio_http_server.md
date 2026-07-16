# ADR-003: Boost.Beast + Asio for HTTP/WebSocket/MQTT Server

**Status:** Accepted  
**Date:** 2022-09-01  
**Deciders:** @themisdb-core-team  
**Modules Affected:** `src/server/`  
**Related Research:** [Boost.Asio Async I/O Best Practices](../best_practices/boost_asio_async_io.md)

---

## Context

ThemisDB exposes its query and administration API over multiple transport protocols from a single server process:

- **HTTP/1.1** — primary REST API for CRUD, query submission, and health checks.
- **HTTP/2** — required for multiplexed gRPC-Web and for browser fetch streams.
- **HTTP/3 / QUIC** — roadmap target for latency-sensitive edge deployments.
- **WebSocket** — push-notification channel for CDC (change data capture) streaming and real-time subscriptions.
- **MQTT** — IoT ingestion endpoint for embedded device clients.

All five protocols must share a single async I/O event loop so that the thread count remains bounded and the in-process handler (`src/query/`) can be invoked directly — avoiding inter-process serialization. Target throughput is 50 K–200 K req/sec on an 8-core server with p50 < 5 ms.

ThemisDB's existing codebase already depends on Boost (filesystem, thread, program_options), so adding Boost.Asio/Beast incurs no new third-party vendor entry.

## Decision Drivers

- **Single unified I/O loop:** All protocols share one `io_context` pool; no per-protocol thread pool proliferation.
- **In-process handler embedding:** HTTP/WS/MQTT handlers call `src/query/` directly (function call, not IPC) to avoid serialization cost.
- **Zero-copy framing:** HTTP and WebSocket frame parsing must avoid unnecessary buffer copies on the hot path.
- **Throughput target:** ≥ 50 K req/sec HTTP/1.1 on 8 cores; ≥ 100 K req/sec with HTTP/2 multiplexing.
- **Boost umbrella:** ThemisDB already vendors Boost via vcpkg; adding Beast/Asio is a zero-cost dependency addition.
- **HTTP/2 and WebSocket in same binary:** Must not require a reverse proxy for protocol negotiation.

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **Boost.Beast + Boost.Asio** | Proactor model (io_context); Beast handles HTTP+WS framing at zero-copy; already in Boost umbrella; coroutine support via Asio; full HTTP/1.1 and HTTP/2 (via nghttp2 adapter); BSD license | Verbose API; Beast HTTP/2 requires nghttp2 integration; HTTP/3 not yet in Beast (roadmap) |
| **libevent + libevhtp** | Mature; used in Memcached; low overhead | Two separate libraries; no native WebSocket; HTTP/2 support incomplete; C API requires manual C++ wrapping |
| **uWebSockets** | Extremely high throughput (benchmark winner); clean C++ API | Limited HTTP/2 support; no built-in MQTT; AGPL license (incompatible); tight coupling between WS and HTTP |
| **Drogon** | High-level C++ web framework; ORM included; HTTP/2 | Forces ORM and controller architecture on a database engine; ORM coupling undesirable; extra binary size |
| **NGINX as reverse proxy** | Proven; HTTP/3 via QUIC; TLS termination | Separate process — in-process handler embedding impossible; adds ops complexity; complicates container deployments |

## Decision

**Chosen: Boost.Beast + Boost.Asio**

The proactor model of Boost.Asio maps directly to ThemisDB's query executor model: each accepted connection runs as an async coroutine on the thread pool without blocking a dedicated thread. Beast's zero-copy buffer chain (`beast::flat_buffer`, `beast::multi_buffer`) enables HTTP request parsing without intermediate allocation on the hot path.

Key design points adopted:

- **Thread pool:** `net::io_context` instance shared across `std::thread` pool sized to `hardware_concurrency()`. Each protocol listener (HTTP, WS, MQTT) posts accepted sockets onto the same executor.
- **HTTP/1.1 and HTTP/2:** HTTP/1.1 handled natively by Beast; HTTP/2 handled via an nghttp2 adapter wrapping Beast's async I/O layer — both reuse the same SSL context and `io_context`.
- **WebSocket upgrade:** Beast's `websocket::stream<tcp_stream>` handles upgrade negotiation inline, allowing CDC subscription handlers to be co-located with REST handlers in the same compilation unit.
- **MQTT:** A lightweight MQTT 3.1.1 broker layer (`src/server/mqtt_session.cpp`) is implemented atop raw `tcp::socket` using the same `io_context`; Beast is not involved for MQTT framing (custom parser).
- **TLS:** `ssl::context` (Asio) with SNI routing; Let's Encrypt ACME renewal via a background timer coroutine.

uWebSockets was rejected due to AGPL licensing incompatibility. Drogon was rejected because embedding an ORM-coupled web framework inside a database engine inverts the dependency graph. NGINX was rejected because it would require IPC (Unix socket or loopback) between NGINX and ThemisDB's query engine, adding latency and deployment complexity.

## Consequences

### Positive
- Single `io_context` pool eliminates context-switch overhead between protocol layers; all I/O is non-blocking with bounded thread count.
- Beast's zero-copy buffer chain reduces HTTP/1.1 p50 latency to < 1 ms for small payloads on loopback.
- Boost umbrella membership means vcpkg install is a no-op addition (Boost already required).
- WebSocket and HTTP share the same TCP listener and TLS context — no extra port or SSL certificate needed for subscriptions.

### Negative / Trade-offs
- **Verbose API:** Beast's coroutine-based HTTP session handler requires ~150 lines of boilerplate per session type. *Mitigation: `src/server/http_session_base.hpp` provides CRTP base class reducing per-handler boilerplate to ~20 lines.*
- **HTTP/3 not yet available in Beast:** QUIC support is deferred to a future release of Beast/Asio. *Mitigation: NGINX or Envoy can be optionally fronted for HTTP/3 termination without changing the in-process handler path.*
- **nghttp2 integration complexity:** HTTP/2 requires a custom adapter (~400 lines) on top of Beast. *Accepted because: HTTP/2 multiplexing is necessary for gRPC-Web browser clients.*

### Neutral
- The `IHttpServer` interface in `src/server/` allows unit tests to inject a mock server that invokes handlers synchronously, bypassing the real `io_context`.
- MQTT broker is an independent session type reusing only the Asio executor; it can be disabled at compile time (`-DTHEMIS_ENABLE_MQTT=OFF`).

## Validation

- [x] 50 K HTTP/1.1 req/sec verified on 8-core i7-12700 under wrk benchmark
- [x] p50 < 2 ms for 1 KB JSON payload on loopback HTTP/1.1
- [x] WebSocket CDC subscription end-to-end test passing (subscribe → insert → receive event)
- [x] HTTP/2 multiplexed gRPC-Web client test passing (TypeScript client)
- [x] MQTT publish/subscribe round-trip test passing with mosquitto_pub
- [ ] HTTP/3 QUIC path integration (deferred — tracked in roadmap Q3 2026)
- [ ] Load test at 200 K req/sec with TLS enabled on production hardware class

## Follow-up Actions

- [ ] Extract `HttpSessionBase` CRTP template to reduce per-protocol boilerplate (`src/server/http_session_base.hpp`).
- [ ] Implement ACME (Let's Encrypt) auto-renewal coroutine for TLS certificates (`src/server/acme_renewal.cpp`).
- [ ] Investigate Asio stackless coroutines (C++20 `co_await`) to replace callback chains in MQTT session handler.
- [ ] Add HTTP/3 QUIC via ngtcp2 adapter when Boost.Beast merges upstream QUIC support.

## Related Decisions

- [ADR-007: gRPC + Protobuf for Internal Service RPC](adr_007_grpc_for_internal_rpc.md)
- [ADR-008: JWT + OAuth2 PKCE as Primary API Authentication](adr_008_jwt_oauth2_for_api_auth.md)

---
**Last Updated:** 2026-04-06
