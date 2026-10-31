---
Author: ThemisDB Team
Created: 2026-06-01
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | header inventory reviewed: 2026-10-31 -->
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

The directory currently contains public headers for ethics types/plugin contracts, profile routing, LDM helper APIs, memory/compression, validation, synthesis, tournament selection, and cascade routing. See `ARCHITECTURE.md` for the inventory. Public declarations do not establish runtime integration; LDM pipeline wiring remains an implementation follow-up in `../../src/ethics_ai/ROADMAP.md`.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/ethics_ai/` implementation docs established
- [x] Layer association (**LLM**) recorded in module documentation; the prior `FUTURE_PLAN.md` reference is not present in the current tree
- [x] Namespace layout (`themis::plugins::ethics`) documented

---

## In Progress

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3)

---

> The Q3/Q4 2026 target labels above are original planning dates. Q3 has elapsed; no revised milestone is asserted here. Current header inventory and source/runtime status are in `ARCHITECTURE.md` and `../../src/ethics_ai/ROADMAP.md`.

## Planned

- [ ] API contract index for high-frequency entry points with usage examples (Target: 2026-Q3)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
