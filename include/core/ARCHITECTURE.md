> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
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
| `config_hot_reloader.h` | `ConfigHotReloader` | Live configuration reload without restart |
| `config_validator.h` | `ConfigValidator` | Pre-start configuration validation |
| `health_probe.h` | `HealthProbe` | Readiness / liveness probe interface |
| `index_initialization.h` | `IndexInitializer` | Index subsystem bootstrap |
| `production_mode.h` | `ProductionModeGuard` | Production-safety flag enforcement |
| `query_engine_builder.h` | `QueryEngineBuilder` | Fluent builder for the query engine |
| `security_initialization.h` | `SecurityInitializer` | Security subsystem bootstrap |
| `storage_initialization.h` | `StorageInitializer` | Storage subsystem bootstrap |

---

## 3. Namespace Layout

All types reside in `themis::core::`.

---

## 4. Initialization Sequence Contract

The public headers encode the following expected call order:

```
ConfigValidator::validate()          // fail-fast on bad config
SecurityInitializer::initialize()    // crypto / ACL bootstrap
StorageInitializer::initialize()     // RocksDB / WAL open
IndexInitializer::initialize()       // ANN / graph index warm-up
QueryEngineBuilder::build()          // query layer wiring
HealthProbe::markReady()             // signal readiness
```

Callers must not call later stages if an earlier stage returns an error.

---

## 5. Build-Conditional Headers

None of the core headers carry optional feature guards. All are unconditionally compiled.

---

## 6. Relationship to Strategic Architecture

The `core/` module implements the bootstrap path for all four strategic layers described in `FUTURE_PLAN.md`:

- ANN Frontdoor: `IndexInitializer` wires HNSW/DiskANN indices
- Tensor Mid-Layer: future `TensorLayerInitializer` will be added here
- Graph Truth Layer: `StorageInitializer` opens the graph-truth RocksDB store
- LLM/LoRA Final Layer: `QueryEngineBuilder` can register LLM backends

---

## 7. Concerns Subdirectory

`include/core/concerns/` holds policy-level interfaces (e.g., `IInitializationConcern`) that allow plugging additional startup validators without modifying core bootstrap code.
