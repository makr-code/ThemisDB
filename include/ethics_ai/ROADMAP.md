> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/ethics_ai/ROADMAP.md -->

# Ethics AI Module — Public Header Roadmap

**Module Path:** `include/ethics_ai/`  
**Canonical implementation roadmap:** [`../../src/ethics_ai/ROADMAP.md`](../../src/ethics_ai/ROADMAP.md)

---

## Overview

Tracks public `ethics_ai` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/ethics_ai/ROADMAP.md`](../../src/ethics_ai/ROADMAP.md)

---

## Current Status

All headers in `include/ethics_ai/` are present and expose the multi-school ethical reasoning, argument synthesis, discourse memory, profile registry, and LLM cascade routing surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/ethics_ai/` implementation docs established
- [x] Layer association (**LLM**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::ethics_ai`) documented

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
