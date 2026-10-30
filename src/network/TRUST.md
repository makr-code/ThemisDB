---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Network Trust Boundaries and Risk Model

**Scope:** `src/network`; source-of-truth domain: security. This document describes transport and protocol controls, not a guarantee that every listener is encrypted or authenticated.

## Trust boundaries

- Client to listener: bytes, frames, and credentials are untrusted until parsed and authenticated.
- Listener to identity/policy services: transport admission is distinct from application authorization.
- Network configuration to host services: certificate paths, tokens, interface names, and listener settings are security-sensitive inputs.
- Network events to operators: audit/error logs support triage but may contain client and connection metadata.

## Controls and operating conditions

- `WireProtocolServer::start()` runs transport-safety validation and rejects invalid frame, connection, timeout, or auth configuration.
- Wire-protocol authentication is conditional on `require_auth`; when required, startup accepts the configured `SCRAM-SHA-256` mechanism and rejects empty/blank secrets. Keep this option enabled on untrusted networks.
- QUIC configures TLS 1.3 only. Certificate/key loading depends on the configured paths; validate certificates and private-key permissions at deployment.
- Frame bounds, timeouts, per-IP connection limits, rate limits, and safe argument passing for QoS commands reduce parser and resource-abuse risk.
- TLS protects a transport channel, not the application permission decision. The server/auth layer must still authorize each protected operation.

## Residual risks

- Optional auth or TLS settings can leave a listener exposed; configuration and enabled protocol surfaces determine the effective boundary.
- A leaked bearer token or private key can impersonate a client until revoked.
- Rate/connection controls reduce abuse but do not guarantee availability under all traffic or upstream-resource failures.

## Incident handling

Disable or isolate the affected listener first; revoke exposed credentials/certificates, retain connection/audit evidence, identify impacted protocol and time range, and restore only after validating the corrected listener configuration and authentication path. Report vulnerabilities through [project SECURITY.md](../../SECURITY.md), using [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Source references

- `src/network/wire_protocol_server.cpp`
- `src/network/quic_transport.cpp`
- `src/network/qos_manager.cpp`
- `src/network/socket_timeout_manager.cpp`
- `src/network/network_audit_log.cpp`
- [Module security summary](SECURITY.md)
