> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/plugins/ROADMAP.md -->

# Plugins Module — Public Header Roadmap

**Module Path:** `include/plugins/`
**Canonical implementation roadmap:** [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)

---

## Overview

Tracks public plugin API contract stability, header coverage, and future public entry points. Runtime plugin sandboxing, OCI mechanics, WASM instantiation, and self-healing loop work remain in:

→ [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)

---

## Current Status

All 20 plugin headers are present. Public entry points exist for the core plugin lifecycle contract, plugin manager and registry, dependency resolution, health monitoring, hot-plug, metrics, self-healing, OCI registry and manifest signing, signed plugin repository, RPC/image/audio/HuggingFace domain plugins, and WASM component model and host API.

---

## Completed ✅

- [x] `plugin_interface.h`, `plugin_api.h`, `plugin_manager.h`, `plugin_registry.h`, `plugin_dependency_resolver.h` — core plugin lifecycle contract
- [x] `plugin_health_monitor.h`, `plugin_hot_plug_monitor.h`, `plugin_metrics.h`, `self_healing_plugin.h` — health, monitoring, and self-healing
- [x] `oci_registry_client.h`, `oci_manifest_signing.h`, `signed_plugin_repository.h` — OCI distribution and signing
- [x] `rpc_plugin_interface.h`, `huggingface_ingestion_plugin.h`, `image_analysis_interface.h`, `image_analysis_manager.h`, `image_generation_interface.h`, `audio_backend_interface.h` — domain plugin interfaces
- [x] `wasm_component_model.h`, `wasm_host_api.h` — WASM runtime contract

---

## In Progress

- [ ] Document OCI manifest signing fail-closed semantics for unsigned artefacts in `oci_manifest_signing.h` (Target: 2026-Q3)
- [ ] Clarify WASM memory isolation guarantees and host-function capability limits in `wasm_host_api.h` (Target: 2026-Q3)

---

## Planned

- [ ] `plugin_policy.h` — per-plugin resource, capability, and security-policy contract (Target: 2026-Q4)
- [ ] Add `IPlugin` version-negotiation protocol to support plugin API evolution (Target: 2026-Q4)
- [ ] Expose benchmark load/unload latency targets for hot-plug and WASM instantiation hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Plugin headers maintain backward compatibility within the active major line; `IPlugin` API and `PluginAPI` host-surface changes require major-version bumps, migration notes, and changelog updates.
