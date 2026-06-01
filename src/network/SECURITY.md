> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security - Network Module

> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation surface |
|---|---|
| Unauthenticated wire requests | session-level auth checks and early `401` error path in `wire_protocol_server.cpp` |
| Weak/invalid credential configuration | startup validation rejects empty/blank/invalid `auth_token` when auth is required |
| Frame abuse / oversized payloads | frame and payload size validation with explicit request rejection |
| Connection/request flooding | connection limits, rate-limiting checks, timeout manager and adaptive breaker |
| Timing side channels in token compare | constant-time compare path (`CRYPTO_memcmp`) in auth validation |
| Transport downgrade or unsafe deployment mode | transport security preflight via `validateTransportSecurity(...)` |

## Security Controls

- Session authentication gating before sensitive operation handlers.
- Configuration guardrails for auth token quality and frame size bounds.
- Per-connection/per-IP protection paths and timeout-based cleanup.
- Structured rejection/error responses for invalid payloads and unauthorized access.
- Network-side audit/event hooks for operational security monitoring.

## Known Limitations

- Security behavior depends on runtime deployment configuration and enabled transport surfaces.
- Some policy depth (for example, network-specific IPv6 CIDR governance) remains roadmap work and is not treated as fully enforced in this module documentation.

## Sourcecode Verification (Module: network/security)

- Verified files:
  - `src/network/wire_protocol_server.cpp`
  - `src/network/wire_protocol_server_ws.cpp`
  - `src/network/socket_timeout_manager.cpp`
  - `src/network/adaptive_circuit_breaker.cpp`
  - `src/network/network_audit_log.cpp`
  - `src/network/quic_transport.cpp`
  - `src/network/grpc_transport.cpp`
- Verified controls:
  - auth/session checks and request rejection behavior
  - startup-time transport-security guardrails
  - rate/timeout/breaker overload protections
  - security event/audit logging surfaces
