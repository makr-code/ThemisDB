> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/plugins/FUTURE_ENHANCEMENTS.md -->

# Plugins Module — Public Header Future Enhancements

**Module Path:** `include/plugins/`
**Canonical implementation enhancements:** [`../../src/plugins/FUTURE_ENHANCEMENTS.md`](../../src/plugins/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/plugins/`. Runtime plugin sandboxing, OCI pull/push mechanics, WASM instantiation internals, and benchmark work remain tracked in:

→ [`../../src/plugins/FUTURE_ENHANCEMENTS.md`](../../src/plugins/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` `IPlugin` must remain the mandatory base contract; all plugins must implement it.
- `[x]` `PluginAPI` must remain a stable host-side surface; breaking changes require major-version bumps.
- `[x]` OCI signing headers must fail closed for unsigned or untrusted artefacts.
- `[x]` WASM host-function headers must not expose sandboxing or memory-isolation internals to plugin consumers.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `IPlugin` load / unload / execute | `plugin_interface.h` | Plugin manager and host | ✅ Stable |
| `PluginAPI` host-side API | `plugin_api.h` | All loaded plugins | ✅ Stable |
| `OCIManifestSigning` verify | `oci_manifest_signing.h` | Plugin distribution pipeline | ✅ Stable |
| `WASMComponentModel` instantiate | `wasm_component_model.h` | WASM plugin runtime | ✅ Stable |
| `PluginHealthMonitor` check API | `plugin_health_monitor.h` | Orchestration and observability | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document OCI manifest signing fail-closed semantics for unsigned artefacts in `oci_manifest_signing.h`.
- Clarify WASM memory isolation guarantees and host-function capability limits in `wasm_host_api.h`.
- Add explicit stability annotations for domain plugin interfaces (`IImageAnalysisPlugin`, `IAudioBackend`).

### Medium-Term (Q4 2026)

- Introduce `plugin_policy.h` to provide per-plugin resource quotas, capability restrictions, and security-policy contract.
- Add `IPlugin` version-negotiation protocol to `plugin_interface.h` to support graceful API evolution.
- Expose benchmark-reference load/unload latency targets for hot-plug and WASM instantiation hot paths.

### Long-Term

- Unify domain plugin result types behind a shared plugin-context envelope for consistent host-side handling.
- Add extension hooks for embedders to register custom distribution backends alongside the OCI registry client.
- Provide plugin capability-introspection APIs via `plugin_registry.h` to support dynamic feature negotiation.
