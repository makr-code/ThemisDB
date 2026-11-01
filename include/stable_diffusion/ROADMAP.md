---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone. Original creation dates
> could not be verified from available history; `Created` records this AI-assisted revision.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/stable_diffusion/ROADMAP.md -->

# Stable Diffusion Module — Public Header Roadmap

**Module Path:** `include/stable_diffusion/`  
**Canonical implementation roadmap:** [`../../src/stable_diffusion/ROADMAP.md`](../../src/stable_diffusion/ROADMAP.md)

---

## Overview

Tracks public `stable_diffusion` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/stable_diffusion/ROADMAP.md`](../../src/stable_diffusion/ROADMAP.md)

---

## Current Status

All headers in `include/stable_diffusion/` are present and expose the Stable Diffusion image generation plugin, configuration, prompt sanitisation, and plugin registration surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/stable_diffusion/` implementation docs established
- [!] Strategic layer association (**LLM**) needs verification: cited `FUTURE_PLAN.md` is absent;
  see the TODO in `FUTURE_ENHANCEMENTS.md`.
- [x] Namespace layout (`themis::imggen`) documented

---

## In Progress

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3 elapsed; replan needed)

---

## Planned

- [ ] API contract index for high-frequency entry points with usage examples (Target: 2026-Q3 elapsed; replan needed)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
