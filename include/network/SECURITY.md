<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Network Module

## Scope

This document covers the security posture of all public headers in `include/network/`. Implementation hardening details are in `../../src/network/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Man-in-the-middle on gRPC connections | High — query result interception | TLS 1.3 enforced in `GrpcTransport`; mTLS via `ServiceMeshClient` |
| Malformed wire frames (DoS via parse bomb) | High — CPU/memory exhaustion | `WireProtocolHelpers` validates length prefix and CRC32C before dispatch |
| UDP amplification / spoofed source | Medium — traffic amplification | `UDPServer` validates HMAC token per datagram; rate-limits per source IP |
| Raft log injection via `RaftLoadBalancer` | High — cluster takeover | Raft entries signed with cluster-shared HMAC-SHA256 |
| Compression oracle (CRIME-style) | Medium — data leakage | `ZstdDictionaryCompressor` uses per-connection dictionaries; TLS applied after compression |
| Circuit breaker bypass | Medium — cascading failure | `AdaptiveCircuitBreaker` state is per-endpoint with monotonic timers |
| xDS endpoint poisoning | High — routing hijack | `EnvoyXdsClient` validates xDS node ID and verifies server certificate |
| QoS misclassification | Low — priority inversion | `QosManager` enforces DSCP marking only on server-controlled sockets |

## Security Controls

1. **TLS 1.3** required on all gRPC, QUIC, and WebSocket transports.
2. **mTLS** enforced for inter-node service mesh paths via `ServiceMeshClient`.
3. **CRC32C** frame integrity check on every received frame (`WireProtocolHelpers`).
4. **HMAC-SHA256** on Raft log entries (`RaftLoadBalancer`).
5. **Per-source-IP rate limiting** in `UDPServer` and `QosManager`.
6. **Zero-copy path** (`ZeroCopyFrameBuilder`) does not expose internal memory to untrusted callers; mapped regions are `PROT_READ` only.

## Known Limitations

- `UdpFastPath` (io_uring SO_ZEROCOPY) requires kernel ≥ 5.15; falls back to standard sendmsg on older kernels — no security regression.
- DPDK bypass (planned Q3 2026) will require explicit security review before enabling in production.
- Envoy xDS certificate pinning is not yet implemented (tracked in ROADMAP.md).
