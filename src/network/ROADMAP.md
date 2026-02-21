# Network Module Roadmap

## Current Status
v1.x – Production-grade networking layer. Binary wire protocol server, connection pooling, TLS/mTLS, circuit breaker, and rate limiting are fully implemented.

## Completed ✅
- [x] WireProtocolServer – high-performance binary TCP server (port 8766)
- [x] Dedicated I/O thread pool + worker thread pool separation
- [x] Connection pool management (client-side and server-side)
- [x] TLS 1.3 and mutual TLS (mTLS) support
- [x] Per-IP rate limiting (requests/sec and requests/min)
- [x] Connection limits (global and per-IP)
- [x] Circuit breaker pattern for socket timeouts
- [x] Protocol buffer wire format helpers (lightweight parser/serializer)
- [x] Authentication (token-based) with configurable auth timeout
- [x] Health checking and keepalive mechanisms
- [x] Automatic retry logic with configurable back-off
- [x] Transport security validation (`validateTransportSecurity`)
- [x] Prometheus metrics for connection and request statistics

## In Progress 🚧
- [ ] WebSocket upgrade support on wire protocol port (Target: Q2 2026)
- [ ] UDP-based fast-path for read-only queries (Target: Q3 2026)
- [ ] QUIC/HTTP3 transport layer integration (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Connection multiplexing (multiple logical streams per TCP connection)
- [ ] Adaptive I/O thread scaling based on connection load
- [ ] Per-tenant network bandwidth quotas
- [ ] Connection-level compression (LZ4, Zstd)
- [ ] Structured network audit log (connection open/close/auth events)

### Long-term (6-12 months)
- [ ] Service mesh integration (Istio/Envoy sidecar compatibility)
- [ ] RDMA support for ultra-low-latency inter-node communication
- [ ] IPv6 dual-stack support
- [ ] gRPC native transport (separate from server module)
- [ ] Network topology-aware routing for geo-distributed clusters

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (TLS handshake, rate limiting, failover)
- [ ] Performance benchmarks (connections/sec, throughput, latency p99)
- [ ] Security audit (TLS configuration, DoS resilience)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- WebSocket and UDP transports are not yet implemented.
- gRPC server is handled by the server module; this module provides only the binary wire protocol.
- Service mesh integration is a future enhancement.

## Breaking Changes
- Wire protocol frame format is versioned; v2 frame format planned with extended metadata fields.
- `WireProtocolServer::Config` may gain new fields; defaults remain backward-compatible.
