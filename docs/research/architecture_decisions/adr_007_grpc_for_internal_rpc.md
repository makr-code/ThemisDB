# ADR-007: gRPC + Protobuf for Internal Service RPC

**Status:** Accepted  
**Date:** 2022-10-01  
**Deciders:** @themisdb-core-team  
**Modules Affected:** `src/rpc_grpc/`, `src/server/`  
**Related Research:** [OpenTelemetry Tracing Best Practices](../best_practices/opentelemetry_tracing.md)

---

## Context

ThemisDB's modular architecture separates concerns into independently deployable components: the query engine, replication coordinator, CDC (change data capture) publisher, and the distributed gateway. These components need efficient, strongly-typed RPC to communicate:

- **Replication:** Leader streams `ReplicateRequest` to followers; followers stream acknowledgements back.
- **CDC:** The CDC publisher streams `ChangeEvent` records to downstream consumers.
- **Distributed gateway:** The gateway fans out sub-queries to shard nodes and aggregates results.
- **Admin API:** CLI tooling (`themisctl`) sends administrative commands to the running server.

Requirements at decision time:

- Bidirectional streaming for replication and CDC without polling.
- Code generation for client SDKs in TypeScript (browser and Node.js), Python (for data pipelines), and Go (for sidecar tooling).
- Binary wire format for efficiency — JSON overhead at replication throughput (> 500 MB/s) is unacceptable.
- Strong schema evolution (add fields without breaking old clients).
- HTTP/2 transport (multiplexed, flow-controlled) to reuse the Boost.Asio connection pool (ADR-003).
- Interceptor hooks for OpenTelemetry distributed tracing integration.
- Envoy xDS API compatibility for future service mesh integration.

## Decision Drivers

- **Bidirectional streaming:** Replication and CDC require server-push and client-acknowledgement streams simultaneously.
- **Multi-language code generation:** TypeScript, Python, Go clients must be generated from a single `.proto` schema with no hand-written glue.
- **Binary wire format:** At 500 MB/s replication throughput, JSON serialization overhead would consume > 30 % of CPU. Protobuf binary encoding must be used.
- **Schema evolution:** Field additions and deprecations must be backward-compatible across minor versions.
- **OpenTelemetry interceptors:** Every RPC call must propagate trace context (W3C TraceContext header) without per-method boilerplate.
- **Envoy xDS:** Future service mesh integration requires xDS v3 API compatibility — a gRPC-native protocol.

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **gRPC + Protobuf (Google)** | Bidirectional streaming; HTTP/2 transport; codegen for 10+ languages; Protobuf schema evolution; interceptor API for OpenTelemetry; Envoy xDS native; Apache 2.0 | Complex build setup (protoc + grpc plugin); larger dependency footprint |
| **Apache Thrift** | Multi-language codegen; compact binary format; IDL similar to proto | Weaker streaming support (no true bidirectional); smaller ecosystem than gRPC; no native HTTP/2 transport; OpenTelemetry integration is manual |
| **Cap'n Proto (Sandstorm)** | Zero-copy; extremely fast serialization; promise pipelining | Smaller ecosystem; fewer language targets (no official TypeScript); schema registry tooling immature; complex build |
| **REST + JSON** | Universal; no codegen; human-readable | JSON overhead prohibitive at replication throughput; no bidirectional streaming; no native schema evolution; no typed client generation |
| **ZeroMQ** | Extremely low latency; flexible topology (pub/sub, req/rep, push/pull) | No built-in schema — schema evolution must be implemented manually; no HTTP/2; no code generation; no interceptor API |

## Decision

**Chosen: gRPC + Protobuf**

gRPC satisfies every decision driver:

1. **Bidirectional streaming:** Proto service definitions use `stream` on both request and response sides for the replication (`StreamReplicate`) and CDC (`StreamChanges`) RPCs.
2. **Code generation:** `scripts/gen_grpc_web_ts.py` generates TypeScript gRPC-Web stubs from `.proto` files; Python stubs are generated via `grpcio-tools`; Go stubs via `protoc-gen-go-grpc`. All stubs are checked in to `sdks/` and regenerated in CI on proto changes.
3. **Binary wire format:** Protobuf binary encoding at replication throughput adds < 3 % CPU overhead (measured); equivalent JSON encoding consumed 35 % CPU on the same hardware.
4. **Schema evolution:** Protobuf `reserved` fields and field number stability rules are enforced by `buf lint` in CI, preventing accidental breaking changes.
5. **OpenTelemetry interceptors:** A single `OtelGrpcInterceptor` (in `src/rpc_grpc/otel_interceptor.cpp`) extracts or injects W3C TraceContext on every incoming and outgoing RPC without per-method code — required per the OpenTelemetry best practice.
6. **Envoy xDS:** ThemisDB's distributed gateway exposes an xDS v3 management API (`proto/xds/`) compatible with Envoy and Istio, enabling zero-config service mesh integration.

Thrift was rejected because its streaming model requires long-polling workarounds and has no native HTTP/2 transport. Cap'n Proto was rejected due to the absence of an official TypeScript target (a hard requirement for the browser-based admin console). ZeroMQ was rejected because it has no schema evolution mechanism — a breaking requirement for a database system that must maintain backward compatibility across minor releases.

## Consequences

### Positive
- Single `.proto` schema is the source of truth for all RPC interfaces; generated stubs for TypeScript, Python, and Go eliminate hand-written client code.
- HTTP/2 multiplexing allows hundreds of concurrent streams over a single TCP connection, reducing connection setup overhead for high-fanout distributed gateway queries.
- gRPC interceptors provide a clean hook for OpenTelemetry trace propagation, authentication middleware (JWT validation, ADR-008), and rate limiting — all without modifying individual service handlers.
- Envoy xDS compatibility enables Kubernetes sidecar proxy (Istio/Envoy) deployment without a custom control plane.

### Negative / Trade-offs
- **Build complexity:** The protoc compiler and language-specific plugins must be installed in the build environment. *Mitigation: `scripts/gen_grpc_web_ts.py` and a `cmake/GrpcGenerate.cmake` helper automate generation; Docker build image pre-installs all required plugins.*
- **gRPC-Web limitation in browsers:** Native gRPC (HTTP/2 trailers) is not supported by all browsers; gRPC-Web requires an Envoy or grpcwebproxy translation layer. *Mitigation: Envoy is the recommended production deployment topology; for development, `grpcwebproxy` is included as a docker-compose service.*
- **Large Protobuf dependency:** The gRPC C++ library adds ~12 MB to the binary. *Accepted because: the functionality provided (streaming, codegen, interceptors) would require > 12 MB of hand-written equivalent code.*

### Neutral
- Internal RPC between co-located components (same process) uses direct C++ function calls, not gRPC, to avoid serialization overhead; gRPC is only used for cross-process and cross-host communication.
- The `proto/` directory is the canonical location for all `.proto` files; changes require a PR review from the `@themisdb-api-team` CODEOWNERS group.

## Validation

- [x] Bidirectional streaming replication tested: leader → follower stream + acknowledgement stream
- [x] CDC `StreamChanges` tested: insert triggers ChangeEvent delivery within 10 ms
- [x] TypeScript gRPC-Web client generated and tested against live server
- [x] Python client generated and tested in data pipeline integration test
- [x] OpenTelemetry trace propagation verified: parent span ID visible in follower trace
- [x] `buf lint` enforces Protobuf breaking-change rules in CI
- [ ] Envoy xDS v3 management API tested with Istio sidecar injection (tracked: `tests/integration/service_mesh/`)
- [ ] gRPC reflection endpoint enabled for development tooling (grpcurl support)

## Follow-up Actions

- [ ] Enable gRPC server reflection endpoint for development (`grpcurl` and BloomRPC compatibility).
- [ ] Implement `AuthInterceptor` to validate JWT tokens (ADR-008) on all non-public RPCs (`src/rpc_grpc/auth_interceptor.cpp`).
- [ ] Add Envoy xDS v3 management API integration test with a real Envoy proxy.
- [ ] Publish proto schema changelog to `proto/CHANGELOG.md` on each release.

## Related Decisions

- [ADR-003: Boost.Beast + Asio for HTTP/WebSocket/MQTT Server](adr_003_boost_beast_asio_http_server.md)
- [ADR-004: Native Multi-Model Data Model](adr_004_multi_model_data_model.md)
- [ADR-008: JWT + OAuth2 PKCE as Primary API Authentication](adr_008_jwt_oauth2_for_api_auth.md)

---
**Last Updated:** 2026-04-06
