> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/plugins/ROADMAP.md -->

# PLUGINS Module — Public Header Roadmap

**Module Path:** `include/plugins/`
**Canonical implementation roadmap:** [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/plugins/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)

---

## Current Status

production plugin runtime with lifecycle management, manifest/signature validation, hot-plug monitoring, health metrics, and OCI/RPC/WASM integration. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `plugin_api.h` — lifecycle and registry contract
- [x] `plugin_interface.h` — lifecycle and registry contract
- [x] `plugin_manager.h` — lifecycle and registry contract
- [x] `plugin_registry.h` — lifecycle and registry contract
- [x] `plugin_dependency_resolver.h` — lifecycle and registry contract
- [x] `oci_manifest_signing.h` — security and validation contract
- [x] `oci_registry_client.h` — security and validation contract
- [x] `signed_plugin_repository.h` — security and validation contract
- [x] `plugin_health_monitor.h` — monitoring and health contract
- [x] `plugin_hot_plug_monitor.h` — monitoring and health contract
- [x] `plugin_metrics.h` — monitoring and health contract
- [x] `self_healing_plugin.h` — monitoring and health contract
- [x] `audio_backend_interface.h` — extension interfaces contract
- [x] `image_analysis_interface.h` — extension interfaces contract
- [x] `image_analysis_manager.h` — extension interfaces contract
- [x] `image_generation_interface.h` — extension interfaces contract
- [x] `huggingface_ingestion_plugin.h` — extension interfaces contract
- [x] `rpc_plugin_interface.h` — extension interfaces contract
- [x] `wasm_component_model.h` — extension interfaces contract
- [x] `wasm_host_api.h` — extension interfaces contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
