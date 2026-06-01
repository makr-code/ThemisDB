> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/chaos/ROADMAP.md -->

# Chaos Engineering Module — Public Header Roadmap

**Module Path:** `include/chaos/`  
**Canonical implementation roadmap:** [`../../src/chaos/ROADMAP.md`](../../src/chaos/ROADMAP.md)

---

## Overview

Tracks public `chaos` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/chaos/ROADMAP.md`](../../src/chaos/ROADMAP.md)

---

## Current Status

All headers in `include/chaos/` are present and expose the fault injection, chaos experiment orchestration, and resilience testing surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/chaos/` implementation docs established
- [x] Layer association (**LLM/Graph**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::chaos`) documented

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
