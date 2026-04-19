> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md (root) -->

# Security Policy — Plugins Module

## Supported Versions

| Version | Security Fixes |
|---------|---------------|
| 1.3.x   | ✅ Active      |
| 1.2.x   | ✅ Active      |
| < 1.2   | ❌ EOL         |

## Threat Model

### T1 — Native Plugin Crash Corrupting Server Process

- **Risk:** High — native plugins run in-process; a crash or memory corruption propagates to the host
- **Mitigation (implemented):** `plugin_health_monitor.cpp` detects plugin crashes and triggers automatic restart; hot-reload with rollback prevents bad plugin versions from persisting
- **Mitigation (planned):** WASM sandbox via Wasmtime will isolate plugin execution address space (Target: Q3 2027)
- **Residual risk:** In-process native execution remains until WASM isolation is complete

### T2 — Ed25519 Signature Bypass

- **Risk:** High — a bypass would allow unsigned or tampered plugins to execute
- **Mitigation (implemented):** Ed25519 signature verification is enforced at load time in `signed_plugin_repository.cpp`; unsigned plugins are rejected unconditionally
- **Key rotation:** OCI registry client supports key rotation via manifest metadata
- **Residual risk:** Low — verification is mandatory and not bypassable via configuration

### T3 — Privilege Escalation via Capability

- **Risk:** Medium — a malicious plugin could attempt to claim capabilities beyond its declared manifest
- **Mitigation (implemented):** `PluginCapabilityNegotiator` enforces declared capabilities at load time; undeclared capabilities are denied
- **Residual risk:** Runtime capability escalation (post-load re-negotiation) is not yet blocked programmatically; mitigated operationally by RBAC in the server layer

### T4 — Malicious Plugin Manifest

- **Risk:** Medium — a crafted manifest could trigger parser exploits or inject unexpected metadata
- **Mitigation (implemented):** Manifests are validated against JSON Schema v2 before any fields are consumed; schema validation errors abort the load sequence
- **Residual risk:** Low — structural validation prevents injection; schema coverage is reviewed on each release

### T5 — Supply Chain Attacks

- **Risk:** High — a compromised plugin distributed via registry could introduce backdoors
- **Mitigation (implemented):** OCI registry client verifies signed manifests at download time; key rotation prevents stale trust anchors
- **Residual risk:** Community plugin repository scanning and trust scoring are planned (see Unreleased in CHANGELOG)

## Known Limitations

| ID    | Description                                                     | Target Fix   |
|-------|-----------------------------------------------------------------|--------------|
| KL-01 | In-process native plugin execution; WASM sandbox not yet live  | Q3 2027      |
| KL-02 | Runtime capability escalation not blocked programmatically     | Q4 2026      |
| KL-03 | Community repo scanning not implemented                         | Q4 2027      |

## Reporting a Vulnerability

Report security vulnerabilities via the project's private security disclosure channel (see root `SECURITY.md`).
Do **not** open public issues for security vulnerabilities.

Response SLA: acknowledgement within 2 business days; severity assessment within 5 business days.
