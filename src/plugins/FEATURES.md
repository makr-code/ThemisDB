---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Plugins module features

The module source implements plugin lifecycle and integration infrastructure.
Feature availability depends on the selected API path, build flags, and
deployment configuration.

| Capability | Source-backed boundary |
|---|---|
| Native lifecycle | Manifest discovery, dependency checks, load, initialize, unload, reload, and manager diagnostics via `PluginManager`. |
| Typed factories | Independent `PluginRegistry` factory registration/creation; this is not dynamic-library lifecycle management. |
| Policy checks | Manifest edition/license rules on the manifest-backed manager path; explicit-path/OCI loading do not parse `plugin.json`. |
| Trust/distribution | Separate binary verifier, manifest `.sig` hash check, pinned-key signed catalog, and OCI layer digest verification. These controls are not interchangeable. |
| Monitoring | Optional filesystem hot-plug monitoring, health/recovery interfaces, and process-local metrics collection. |
| RPC | A plugin RPC service registry with registration and lookup APIs. |
| WASM | The built-in loader path applies expected-hash and runtime/license checks; a registered `WasmPluginLoadFn` is consulted first and owns its own validation. The module does not claim general sandboxing. |

The module does not itself provide universal plugin sandboxing, durable audit
retention, embedding models, or semantic embedding-quality guarantees. See
[`ARCHITECTURE.md`](./ARCHITECTURE.md), [`API.md`](./API.md),
[`TRUST.md`](./TRUST.md), and [`EMBEDDING.md`](./EMBEDDING.md) for boundaries
and primary code references.
