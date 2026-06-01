> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/exporters/ROADMAP.md -->

# Exporters Module — Public Header Roadmap

**Module Path:** `include/exporters/`  
**Canonical implementation roadmap:** [`../../src/exporters/ROADMAP.md`](../../src/exporters/ROADMAP.md)

---

## Overview

Tracks public `exporters` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/exporters/ROADMAP.md`](../../src/exporters/ROADMAP.md)

---

## Current Status

All headers in `include/exporters/` are present and expose the data export in Parquet, Arrow IPC, JSONL/LLM, HuggingFace, streaming, incremental, and encrypted formats surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/exporters/` implementation docs established
- [x] Layer association (**Tensor/Graph**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::exporters`) documented

---

## In Progress

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3)

---

## Planned

- [ ] API contract index for high-frequency entry points with usage examples (Target: 2026-Q3)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
