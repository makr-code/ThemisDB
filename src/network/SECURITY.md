---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · TRUST.md -->

# Security - Network Module

> Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md). This page describes module controls, not product-wide certification.

## Threat Model

| Threat | Mitigation surface |
|---|---|
| Unauthenticated wire requests | session-level auth checks and early `401` error path in `wire_protocol_server.cpp` |
| Weak/invalid credential configuration | startup validation rejects empty/blank/invalid `auth_token` when auth is required |
| Frame abuse / oversized payloads | frame and payload size validation with explicit request rejection |
| Connection/request flooding | connection limits, rate-limiting checks, timeout manager and adaptive breaker |
| Timing side channels in token compare | constant-time compare path (`CRYPTO_memcmp`) in auth validation |
| Transport downgrade or unsafe deployment mode | transport security preflight via `validateTransportSecurity(...)` |
| Shell command injection via tc interface configuration | strict interface name validation (`isValidInterfaceName()`) + safe `posix_spawn()` with argv array (never shell-based `std::system()`) in `qos_manager.cpp` |

## Security Controls

- Session authentication gating before sensitive operation handlers.
- Configuration guardrails for auth token quality and frame size bounds.
- Per-connection/per-IP protection paths and timeout-based cleanup.
- Structured rejection/error responses for invalid payloads and unauthorized access.
- Network-side audit/event hooks for operational security monitoring.
- **Command Injection Prevention**: QoS manager validates interface names with whitelist-based character validation (alphanumeric, hyphen, underscore, dot; max 15 chars; no leading hyphen) and uses safe `posix_spawn()` with direct argv array (never shell metacharacter interpretation). All tc command arguments are passed as separate argv elements, preventing shell-based injection attacks.

## Known Limitations

- Security behavior depends on runtime deployment configuration and enabled transport surfaces.
- Some policy depth (for example, network-specific IPv6 CIDR governance) remains roadmap work and is not treated as fully enforced in this module documentation.
- Authentication is configuration-dependent: the wire server enforces its credential checks only when `require_auth` is enabled. Startup requires the `SCRAM-SHA-256` configuration label and a non-empty secret; the current AUTH handler compares a pre-shared JSON token rather than performing a SCRAM challenge-response exchange.
- Transport encryption is not authorization. QUIC pins TLS 1.3, but other listeners and TLS enablement depend on their transport configuration and deployment certificates.
- Treat authentication tokens, private keys, and certificate files as secrets; restrict their provisioning and filesystem permissions and never log or commit their contents.

## Incident Handling

On suspected listener, certificate, or credential compromise, disable the affected listener or revoke the credential, preserve available server logs and any audit events where `NetworkAuditLog` has been wired by the owning service, and verify transport and auth settings before reopening traffic. Do not assume the standalone audit API is automatically connected. Use [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Sourcecode Verification (Module: network/security)

- Verified files:
  - `src/network/wire_protocol_server.cpp`
  - `src/network/wire_protocol_server_ws.cpp`
  - `src/network/socket_timeout_manager.cpp`
  - `src/network/adaptive_circuit_breaker.cpp`
  - `src/network/network_audit_log.cpp`
  - `src/network/quic_transport.cpp`
  - `src/network/grpc_transport.cpp`
  - `src/network/qos_manager.cpp` (command injection mitigation)
- Verified controls:
  - auth/session checks and request rejection behavior
  - startup-time transport-security guardrails
  - rate/timeout/breaker overload protections
  - security event/audit logging surfaces
  - command injection prevention (interface name validation + safe posix_spawn)
- Test coverage:
  - `tests/network/test_qos_manager_command_injection.cpp` (QoS command-injection regression tests)

- Additional source references:
  - `src/network/wire_protocol_server.cpp`
  - `src/network/quic_transport.cpp`
  - [Trust and risk model](TRUST.md)
