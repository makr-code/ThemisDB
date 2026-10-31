---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Historical validation annotation: 2026-06-01; current header inventory is source-cross-checked below. -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/core/ARCHITECTURE.md -->

# Core Module — Public Header Architecture

**Module Path:** `include/core/`
**Implementation:** `../../src/core/`
**Canonical architecture doc:** [`../../src/core/ARCHITECTURE.md`](../../src/core/ARCHITECTURE.md)

---

## 1. Overview

`include/core/` contains the **public C++ bootstrap and lifecycle contracts** for ThemisDB. These headers define the initialization and validation interfaces consumed by the server entrypoint and any embedder that composes a ThemisDB instance from its subsystems.

For threading model, subsystem wiring, and runtime data flows see:
→ [`../../src/core/ARCHITECTURE.md`](../../src/core/ARCHITECTURE.md)

---

## 2. Header Inventory

| Header | Public Type / Function | Purpose |
|--------|----------------------|---------|
| `config_hot_reloader.h` | `IConfigHotReloader`, `ConfigChange`, `HotReloadResult` | Configuration reload contract and result types |
| `config_validator.h` | `ConfigValidator` | Pre-start configuration validation |
| `health_probe.h` | `IHealthProbe`, `IHealthProbeRegistry`, health result types | Health probe contracts |
| `index_initialization.h` | `IndexManagerBuilder` | Index subsystem builder |
| `production_mode.h` | `ProductionMode` | Environment-based production-mode check |
| `query_engine_builder.h` | `QueryEngineBuilder` | Fluent builder for the query engine |
| `security_initialization.h` | `SecurityLayerBuilder` | Security-layer construction |
| `storage_initialization.h` | `StorageEngineBuilder` | Storage subsystem builder |
| `timestamp.h` | `HLCTimestamp` alias | Timestamp compatibility alias |

---

## 3. Namespace Layout

Namespaces vary by header: for example, `ProductionMode` is in `themis::core`,
while the initialization builders are declared in `themis`. See each header for
its exact namespace.

---

## 4. Initialization Sequence Contract

The headers expose independent builder and probe contracts. The current headers
do not establish the fixed cross-subsystem initialization sequence asserted in
older versions of this page. Follow each builder's result/error contract and the
owning application's startup order.

---

## 5. Build-Conditional Headers

None of the core headers carry optional feature guards. All are unconditionally compiled.

---

## 6. Architecture Boundary

This document inventories public headers only. Strategic subsystem architecture
and release status are outside this header inventory; use the corresponding
canonical module and root architecture sources instead.

---

## 7. Concerns Subdirectory

`include/core/concerns/` holds cross-cutting concern interfaces and their adapter
contracts, including logging, tracing, metrics, cache, secrets, and adapter
registration. Startup-builder declarations remain in the top-level `include/core/`
headers.
