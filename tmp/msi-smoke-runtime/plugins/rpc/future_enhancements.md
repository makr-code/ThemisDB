# RPC Plugins – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`ROADMAP.md`](ROADMAP.md) for committed near-term work.

---

## Scope

- Enhancements to RPC transport plugins: new protocols (Cap'n Proto, NATS, Arrow Flight), security hardening (mTLS, JWT/OAuth2), and observability (OpenTelemetry traces, per-RPC metrics).
- Entry-point: `plugins/rpc/grpc/CMakeLists.txt` (compatibility shim) · canonical implementation: `src/rpc_grpc/`.
- New protocol backends should follow Themis RPC plugin interfaces (`IRPCPlugin` / `IRPCServer`).
- Out of scope: changes to ThemisDB query engine internals; this plugin only handles transport serialisation and connection lifecycle.
- Covers adaptive flow control, compression negotiation, and multiplexed streaming for high-throughput clients.

## Design Constraints

- [ ] Every RPC backend MUST integrate via `IRPCPlugin` and provide an `IRPCServer` implementation.
- [ ] gRPC streaming MUST support at least 50,000 events/s before back-pressure triggers.
- [ ] TLS MUST be enabled by default; plaintext connections MUST require explicit `allow_insecure=true` configuration.
- [ ] JWT validation MUST use asymmetric keys (RS256 or ES256); symmetric HS256 is prohibited.
- [ ] Compression algorithm MUST be negotiated per-call; default zstd level 3; fallback gzip.
- [ ] All RPC backends MUST emit OpenTelemetry spans with `rpc.method`, `rpc.status_code`, and `rpc.response_size` attributes.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IRPCPlugin` / `IRPCServer` | PluginManager, ThemisDB server | RPC plugin contract + server lifecycle |
| `ITLSConfigProvider` | RPC backend impls | Loads cert/key/CA; validates cert chain at startup |
| `IJWTValidator` | RPC backend impls | Validates RS256/ES256 tokens; returns principal + claims |
| `ICompressionNegotiator` | RPC backend impls | Selects zstd/gzip per call based on `Accept-Encoding` |
| OpenTelemetry `Tracer` | All backends | Span per RPC call with method, status, response-size attributes |

## Idea Backlog

### Additional RPC Protocols

- [ ] **Cap'n Proto** – zero-copy serialisation, lower overhead than Protobuf.
- [ ] **FlatBuffers** – direct memory access without parsing step.
- [ ] **Apache Arrow Flight** – columnar data transfer optimised for analytics.
- [ ] **NATS** – lightweight pub/sub messaging.
- [ ] **ZeroMQ** – brokerless messaging for low-latency IPC.

### Security

- [ ] **JWT / OAuth2 authentication** – token-based auth for gRPC calls.
- [ ] **IP allowlist / denylist** – connection-level access control.
- [ ] **gRPC metadata encryption** – protect sensitive request metadata.

### Observability

- [ ] **OpenTelemetry traces** – distributed tracing for cross-shard calls.
- [ ] **gRPC reflection** – enable runtime service discovery for debugging tools.
- [ ] **Structured logging** – JSON-formatted RPC access logs.

### Performance

- [ ] **Multiplexed streaming** – use a single gRPC stream for multiple logical queries.
- [ ] **Compression negotiation** – gzip / zstd per-call compression.
- [ ] **Adaptive flow control** – back-pressure handling for high-throughput clients.

---

## Test Strategy

- Unit tests for `IRPCServer` implementations with a loopback server; assert round-trip latency ≤ 1 ms for 1 KB payload.
- TLS enforcement tests: assert that a plaintext connection is rejected when `allow_insecure=false` (default).
- JWT validation tests: expired token, wrong algorithm (HS256), and invalid signature must all return `UNAUTHENTICATED`.
- Throughput benchmark: gRPC unary 10,000 req/s with 100-byte payload sustained for 30 s; assert zero errors.
- Streaming throughput benchmark: 50,000 events/s over a single gRPC server-stream for 10 s; assert zero dropped events.
- Compression tests: assert zstd-compressed payload is smaller than uncompressed for text payloads ≥ 256 bytes.

## Performance Targets

- gRPC unary throughput ≥ 10,000 req/s (100-byte payload, TLS enabled, single connection).
- gRPC p99 latency ≤ 5 ms for payloads ≤ 4 KB on loopback.
- gRPC server-streaming throughput ≥ 50,000 events/s (single stream, 64-byte events).
- TLS handshake overhead ≤ 2 ms per new connection (session resumption ≤ 0.5 ms).
- JWT validation overhead ≤ 0.2 ms per call (ES256, cached public key).

## Research / References

- A. D. Birrell and B. J. Nelson, "Implementing remote procedure calls," *ACM Trans. Comput. Syst.*, vol. 2, no. 1, pp. 39–59, Feb. 1984. DOI: [10.1145/2080.357392](https://doi.org/10.1145/2080.357392)
- J. H. Saltzer, D. P. Reed, and D. D. Clark, "End-to-end arguments in system design," *ACM Trans. Comput. Syst.*, vol. 2, no. 4, pp. 277–288, Nov. 1984. DOI: [10.1145/357401.357402](https://doi.org/10.1145/357401.357402)
- C. Li et al., "Apache Arrow Flight: A framework for fast data transport," in *Proc. 2020 IEEE International Conf. Big Data (Big Data)*, 2020, pp. 4551–4557. DOI: [10.1109/BigData50022.2020.9378283](https://doi.org/10.1109/BigData50022.2020.9378283)
- M. Castro and B. Liskov, "Practical Byzantine fault tolerance," in *Proc. 3rd USENIX Symp. Operating Systems Design and Implementation (OSDI)*, 1999, pp. 173–186.
- L. Lamport, R. Shostak, and M. Pease, "The Byzantine generals problem," *ACM Trans. Program. Lang. Syst.*, vol. 4, no. 3, pp. 382–401, Jul. 1982. DOI: [10.1145/357172.357176](https://doi.org/10.1145/357172.357176)
