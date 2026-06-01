> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/process/ROADMAP.md -->

# Process Mining Module — Public Header Roadmap

**Module Path:** `include/process/`  
**Canonical implementation roadmap:** [`../../src/process/ROADMAP.md`](../../src/process/ROADMAP.md)

---

## Overview

Tracks public `process` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/process/ROADMAP.md`](../../src/process/ROADMAP.md)

---

## Current Status

All headers in `include/process/` are present and expose the BPM/BPMN/CMMN/DMN process modelling, EPK, XPDL, OCEL, object-centric tracing, LLM process descriptors, and RAG integration surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/process/` implementation docs established
- [x] Layer association (**Graph/LLM**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::process`) documented

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
