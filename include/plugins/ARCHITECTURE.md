> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/plugins/ARCHITECTURE.md -->

# PLUGINS Module — Public Header Architecture

**Module Path:** `include/plugins/`
**Implementation:** `../../src/plugins/`
**Canonical architecture doc:** [`../../src/plugins/ARCHITECTURE.md`](../../src/plugins/ARCHITECTURE.md)

---

## 1. Overview

The `include/plugins/` directory contains the **public C++ header contract** for ThemisDB's plugin lifecycle orchestration, manifest/security validation, hot-plug monitoring, health/metrics reporting, and remote repository integration. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/plugins/ARCHITECTURE.md`](../../src/plugins/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::plugins`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Lifecycle and registry` | `plugin_api.h`, `plugin_interface.h`, `plugin_manager.h`... |
| `Security and validation` | `oci_manifest_signing.h`, `oci_registry_client.h`, `signed_plugin_repository.h` |
| `Monitoring and health` | `plugin_health_monitor.h`, `plugin_hot_plug_monitor.h`, `plugin_metrics.h`... |
| `Extension interfaces` | `audio_backend_interface.h`, `image_analysis_interface.h`, `image_analysis_manager.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_WASM` | wasm_component_model.h, wasm_host_api.h | WASM runtime host |
| `THEMIS_ENABLE_OCI` | oci_manifest_signing.h, oci_registry_client.h, signed_plugin_repository.h | OCI registry / manifest signing |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/plugins/ARCHITECTURE.md`](../../src/plugins/ARCHITECTURE.md)
- Module overview: [`../../src/plugins/README.md`](../../src/plugins/README.md)
- Roadmap: [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)
- Future enhancements: [`../../src/plugins/FUTURE_ENHANCEMENTS.md`](../../src/plugins/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
