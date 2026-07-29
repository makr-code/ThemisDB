> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/document/ROADMAP.md -->

# Document Module — Public Header Roadmap

**Module Path:** `include/document/`  
**Canonical implementation roadmap:** [`../../src/document/ROADMAP.md`](../../src/document/ROADMAP.md)

---

## Overview

Tracks public `document` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/document/ROADMAP.md`](../../src/document/ROADMAP.md)

---

## Current Status

All headers in `include/document/` are present and expose the document lifecycle management, versioning, schema evolution, encrypted entities, round-trip editing, and XDOMEA connector surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/document/` implementation docs established
- [x] Layer association (**Graph**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::document`) documented

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
