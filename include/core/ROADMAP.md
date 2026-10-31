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

<!-- Historical validation annotation: 2026-06-01; header names cross-checked 2026-10-31. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/core/ROADMAP.md -->

# Core Module — Public Header Roadmap

**Module Path:** `include/core/`
**Canonical implementation roadmap:** [`../../src/core/ROADMAP.md`](../../src/core/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and breaking-change history for `include/core/`. Feature roadmap items that affect both implementation and headers are tracked in the canonical source-level document:

→ [`../../src/core/ROADMAP.md`](../../src/core/ROADMAP.md)

---

## Current Status

The current public bootstrap headers are present and `#pragma once` guarded. The headers do not define one fixed cross-subsystem initialization sequence; see [`ARCHITECTURE.md`](ARCHITECTURE.md).

---

## Completed ✅

- [x] `config_hot_reloader.h` — `IConfigHotReloader` hot-reload contracts
- [x] `config_validator.h` — `ConfigValidator` pre-start validation
- [x] `health_probe.h` — `IHealthProbe` / `IHealthProbeRegistry` contracts
- [x] `index_initialization.h` — `IndexManagerBuilder` bootstrap contract
- [x] `production_mode.h` — `ProductionMode` safety checks
- [x] `query_engine_builder.h` — `QueryEngineBuilder` fluent builder
- [x] `security_initialization.h` — `SecurityLayerBuilder` ACL/crypto bootstrap
- [x] `storage_initialization.h` — `StorageEngineBuilder` storage bootstrap
- [x] `concerns/` subdirectory — cross-cutting concerns API set (`ILogger`, `ITracer`, `IMetrics`, `ICache`, etc.)

---

## In Progress

- [ ] Link `QueryEngineBuilder` to the LLM/LoRA layer bootstrap path described in `FUTURE_PLAN.md` (Target: 2026-Q3)
- [ ] Add `TensorLayerInitializer` public header for Tensor Mid-Layer wiring (Target: 2026-Q3)

---

## Planned

- [ ] `bootstrap_orchestrator.h` — unified orchestrator that enforces ordered initialization across all subsystems (Target: 2026-Q3)
- [ ] `shutdown_coordinator.h` — graceful shutdown protocol (reverse initialization order) (Target: 2026-Q4)
- [ ] Compile-time check that `ProductionModeGuard` is instantiated before `HealthProbe::markReady()` (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Any breaking change requires a MAJOR version bump; see `VERSIONING.md`.
