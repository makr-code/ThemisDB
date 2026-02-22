# RPC Plugins – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

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

## Research / References

- [ ] TODO: Add reference – *Cap'n Proto: Insanely Fast Data Interchange Format* (URL placeholder)
- [ ] TODO: Add reference – *Apache Arrow Flight: Fast Data Transport* (URL placeholder)
- [ ] TODO: Add reference – *NATS: The Connective Technology for Adaptive Edge & Cloud Native Systems* (URL placeholder)
- [ ] TODO: Add reference – *OpenTelemetry Specification* (URL placeholder)
