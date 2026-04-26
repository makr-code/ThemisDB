> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Network Module

## Threat Model

### 1. Unauthenticated Connections
- **Risk:** Clients connect to the TCP/WebSocket/UDP/QUIC/gRPC endpoints without valid credentials.
- **Mitigation:** Token-based authentication enforced at the `AUTH` opcode stage of Wire Protocol V1 handshake. Connections that do not complete authentication within the configured timeout are terminated. The `auth_token` configuration key controls the expected credential.
- **Status:** ✅ Implemented

### 2. Man-in-the-Middle (MitM)
- **Risk:** Plaintext interception or session hijacking between client and server.
- **Mitigation:** TLS 1.3 is mandatory on all production transports. Mutual TLS (mTLS) is supported and recommended for service-to-service communication. Downgrade to older TLS versions is disallowed in the TLS context configuration.
- **Status:** ✅ Implemented

### 3. Distributed Denial of Service (DDoS)
- **Risk:** Flooding the server with connections or requests to exhaust resources.
- **Mitigation:**
  - Per-IP rate limiting enforced at the accept loop level.
  - Hard per-IP connection limits prevent socket exhaustion.
  - Backpressure handling in the connection pool causes new accept calls to yield when the server is saturated, applying natural back-pressure to upstream callers.
  - Adaptive circuit breaker (`adaptive_circuit_breaker.cpp`) trips to shed load under sustained failure conditions.
- **Status:** ✅ Implemented

### 4. Tenant Data Leakage
- **Risk:** One tenant's traffic or bandwidth consumption affects another tenant's isolation or data visibility.
- **Mitigation:** Per-tenant bandwidth quotas enforced via `qos_manager.cpp`. Tenant identity is established at the `AUTH` phase and propagated to all downstream handlers; no cross-tenant data paths exist in the frame dispatch logic.
- **Status:** ✅ Implemented

### 5. Replay Attacks
- **Risk:** A captured authentication token is replayed to gain unauthorized access.
- **Mitigation:** Connection-level authentication state is bound to the TCP session. Tokens are not reusable across connections without re-authentication. Stateful session tracking prevents replay within a session lifetime.
- **Status:** ✅ Implemented

### 6. Insecure Protocol Upgrades
- **Risk:** A client sends a crafted request to trigger an unintended protocol upgrade (e.g., HTTP → WebSocket → raw frames).
- **Mitigation:** Protocol detection is performed on the first bytes of a connection using the Wire Protocol magic bytes. Connections not presenting the expected magic are rejected before any protocol negotiation occurs.
- **Status:** ✅ Implemented

---

## Known Limitations

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| NET-SEC-01 | IPv6 CIDR-based policies are not yet implemented in `ZeroTrustPolicyEnforcer`. IPv6 clients are accepted but not subject to CIDR-level allow/deny rules. | Medium | Open |

---

## Security Configuration Reference

| Parameter | Description | Recommended Value |
|-----------|-------------|-------------------|
| `auth_token` | Shared secret used for Wire Protocol V1 token authentication | Strong random ≥ 32 bytes |
| `tls.enabled` | Enforce TLS on all TCP/WebSocket transports | `true` |
| `tls.mutual` | Require client certificates (mTLS) | `true` for internal mesh |
| `rate_limit.per_ip.max_rps` | Maximum requests per second per client IP | Tune per deployment |
| `rate_limit.per_ip.max_connections` | Maximum concurrent connections per IP | Tune per deployment |
| `circuit_breaker.enabled` | Enable adaptive circuit breaker | `true` |

---

## Security Review History

| Date | Reviewer | Scope | Outcome |
|------|----------|-------|---------|
| 2026-03-12 | Internal security review | TLS config, auth flow, rate limiting, DDoS mitigations | Passed; NET-SEC-01 filed |
