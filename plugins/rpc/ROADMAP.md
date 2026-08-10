# RPC Plugins – Roadmap

## Current Status

**Status:** ✅ Production-ready (gRPC backend)

Entry-point: `plugins/rpc/grpc/CMakeLists.txt` (compatibility shim) · implementation: `src/rpc_grpc/` · public API: `include/plugins/rpc/grpc_plugin.h`

| Backend | Status |
|---------|--------|
| gRPC | ✅ Production |

---

## In Progress

- [~] Integration tests for gRPC plugin: server startup, basic call, graceful shutdown
- [~] mTLS certificate rotation test without service restart

## Planned Features

- [ ] **REST / HTTP-JSON gateway** via gRPC-Gateway or separate adapter (Target: Q3 2026)
- [ ] **WebSocket plugin** – real-time bidirectional communication for browser clients (Target: Q3 2026)
- [ ] Connection pool monitoring metrics to Prometheus (Target: Q3 2026)
- [ ] **Apache Thrift backend** – alternative RPC for legacy enterprise integration (Target: Q4 2026)
- [ ] **AMQP / RabbitMQ backend** – async request/reply over message bus (Target: Q4 2026)
- [ ] Plugin-level circuit breaker and retry policy (Target: Q4 2026)
- [ ] Cross-datacenter replication via gRPC streaming (Target: 2027)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add integration tests for gRPC plugin: server startup, basic call, graceful shutdown.
- [ ] Verify mTLS certificate rotation works without service restart.
- [ ] Document all configuration options (port, TLS paths, max connections, timeouts).

## Mid-term Goals (1–3 months)

- [ ] **REST / HTTP-JSON gateway** – expose ThemisDB operations over plain HTTP/JSON using gRPC-Gateway or a separate adapter.
- [ ] **WebSocket plugin** – real-time bidirectional communication for browser clients.
- [ ] Connection pool monitoring: active connections, rejected connections metrics to Prometheus.
- [ ] Load-balancing hints (DNS round-robin, least-connections) documented and tested.

## Long-term Goals (3–12 months)

- [ ] **Apache Thrift backend** – alternative RPC protocol for legacy enterprise integration.
- [ ] **Message Queue backend** (AMQP / RabbitMQ) – async request/reply over message bus.
- [ ] Plugin-level circuit breaker and retry policy.
- [ ] Cross-datacenter replication via gRPC streaming.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| REST/HTTP-JSON gateway | TODO | 🔲 Planned |
| WebSocket plugin | TODO | 🔲 Planned |
| AMQP backend | TODO | 🔲 Planned |

## Implementation Phases

### Phase 1 – REST / HTTP-JSON Gateway
- [ ] Evaluate gRPC-Gateway vs. standalone HTTP adapter; decide ownership (plugin vs. core)
- [ ] Implement REST gateway that proxies HTTP/JSON requests to gRPC backend
- [ ] OpenAPI spec generated from `.proto` definitions
- [ ] Integration test: `curl` round-trip through REST gateway to gRPC backend

### Phase 2 – WebSocket Plugin
- [ ] Implement `WebSocketRPCPlugin` using Boost.Beast or uWebSockets
- [ ] Define authentication mechanism (JWT, mTLS, API key)
- [ ] Bidirectional streaming: subscribe to ThemisDB change feed over WebSocket
- [ ] Integration test: browser-compatible WebSocket client round-trip

### Phase 3 – Circuit Breaker & Retry
- [x] Implement circuit breaker: half-open state, configurable failure threshold and recovery window
- [ ] Per-call retry policy with exponential backoff and jitter
- [ ] Prometheus metrics: circuit state transitions, retry counts

### Phase 4 – Thrift & AMQP Backends
- [ ] Apache Thrift backend: IDL → generated server stub → `IRPCPlugin` adapter
- [ ] AMQP / RabbitMQ backend: request/reply pattern with configurable exchange and routing key
- [ ] Cross-datacenter replication via gRPC bidirectional streaming

---

## Dependencies

- gRPC / Protobuf (ThemisDB vcpkg)
- `IRPCPlugin` (`include/plugins/rpc_plugin_interface.h`)
- mTLS certificate infrastructure (`certs/`)

## Open Questions

- [ ] Should the REST gateway be a separate plugin or part of the core HTTP server?
- [ ] What authentication mechanism should the WebSocket plugin use?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| gRPC backend | ✅ Ready |
| mTLS support | ✅ Ready |
| Integration tests in CI | ❌ Pending |
| mTLS certificate rotation tested | ❌ Pending |
| REST / HTTP-JSON gateway | ❌ Not implemented |
| WebSocket plugin | ❌ Not implemented |
| Circuit breaker | ❌ Not implemented |
| Apache Thrift backend | ❌ Not implemented |
| AMQP / RabbitMQ backend | ❌ Not implemented |

## Known Issues & Limitations

- REST gateway ownership is unclear: separate plugin vs. part of the core HTTP server not decided
- WebSocket authentication mechanism is undefined
- No integration tests in CI; gRPC plugin is tested manually only
- mTLS certificate rotation has not been tested in an automated scenario

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) · [RPC Architecture](../../docs/de/plugins/RPC_PLUGIN_ARCHITECTURE.md)*
