# RPC Plugins – Roadmap

## Current Status

**Status:** ✅ Production-ready (gRPC backend)

Entry-point: `plugins/rpc/grpc/grpc_plugin.cpp`

| Backend | Status |
|---------|--------|
| gRPC | ✅ Production |

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

## Dependencies

- gRPC / Protobuf (ThemisDB vcpkg)
- `IRPCPlugin` (`include/plugins/rpc_plugin_interface.h`)
- mTLS certificate infrastructure (`certs/`)

## Open Questions

- [ ] Should the REST gateway be a separate plugin or part of the core HTTP server?
- [ ] What authentication mechanism should the WebSocket plugin use?

---

*See also: [`future_enhancements.md`](future_enhancements.md) · [RPC Architecture](../../docs/de/plugins/RPC_PLUGIN_ARCHITECTURE.md)*
