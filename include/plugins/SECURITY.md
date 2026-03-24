<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Plugins Module

## Scope

Covers all public headers in `include/plugins/`. Implementation hardening in `../../src/plugins/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Malicious plugin code execution | Critical — host RCE | Ed25519 signature verification (`SignedPluginRepository`) before any plugin is loaded |
| WASM escape / host-call abuse | High — sandbox bypass | `WasmHostApi` allowlist of host calls; WASM verifier rejects malformed modules |
| Dependency confusion attack | High — supply-chain attack | `PluginDependencyResolver` resolves only from configured trusted registries |
| OCI registry man-in-the-middle | High — plugin tampering | `OciRegistryClient` uses TLS 1.3 + digest pinning for all registry operations |
| Hot-reload race condition | Medium — partial state | `PluginHotPlugMonitor` uses atomic swap; old plugin finalizes before new one activates |
| Plugin crash DoS | Medium — service degradation | `SelfHealingPlugin` isolates crashes; failed plugins quarantined after N restarts |
| Plugin privilege escalation | High — host process control | Plugins run in separate thread context; no access to PluginManager internals |
| Health monitor SSRF | Low — internal network probe | `PluginHealthMonitor` probes are local IPC only; no external HTTP |

## Security Controls

1. **Mandatory Ed25519 signing** — all plugins from non-localhost sources must pass `SignedPluginRepository::verify()`.
2. **WASM host-call allowlist** — `WasmHostApi` only exposes explicitly allowlisted host functions.
3. **Digest pinning** — `OciRegistryClient` verifies image digest (SHA-256) after pull.
4. **Quarantine policy** — `SelfHealingPlugin` quarantines after 3 consecutive crashes within 60 s.
5. **RBAC for plugin load** — `PluginManager::load()` checks operator-role token before accepting plugins.
6. **TLS 1.3 for OCI** — `OciRegistryClient` enforces TLS 1.3 with certificate validation.

## Known Limitations

- WASM Component Model (planned Q3 2026) will introduce new host-call surface requiring separate security review.
- Linux namespace isolation for native plugins not yet implemented — tracked in ROADMAP.md.
