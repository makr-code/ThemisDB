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

- A. D. Birrell and B. J. Nelson, "Implementing remote procedure calls," *ACM Trans. Comput. Syst.*, vol. 2, no. 1, pp. 39–59, Feb. 1984. DOI: [10.1145/2080.357392](https://doi.org/10.1145/2080.357392)
- J. H. Saltzer, D. P. Reed, and D. D. Clark, "End-to-end arguments in system design," *ACM Trans. Comput. Syst.*, vol. 2, no. 4, pp. 277–288, Nov. 1984. DOI: [10.1145/357401.357402](https://doi.org/10.1145/357401.357402)
- C. Li et al., "Apache Arrow Flight: A framework for fast data transport," in *Proc. 2020 IEEE International Conf. Big Data (Big Data)*, 2020, pp. 4551–4557. DOI: [10.1109/BigData50022.2020.9378283](https://doi.org/10.1109/BigData50022.2020.9378283)
- M. Castro and B. Liskov, "Practical Byzantine fault tolerance," in *Proc. 3rd USENIX Symp. Operating Systems Design and Implementation (OSDI)*, 1999, pp. 173–186.
- L. Lamport, R. Shostak, and M. Pease, "The Byzantine generals problem," *ACM Trans. Program. Lang. Syst.*, vol. 4, no. 3, pp. 382–401, Jul. 1982. DOI: [10.1145/357172.357176](https://doi.org/10.1145/357172.357176)
