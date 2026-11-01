---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`
>
> The dated task targets below are retained from an older planning snapshot. They are proposals, not current schedule commitments or implementation/test evidence; target dates already passed should be re-planned before being treated as active.

<!-- Status: planning snapshot | header inventory checked: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/toolbox/ROADMAP.md -->

# Toolbox Module — Public Header Roadmap

**Module Path:** `include/toolbox/`  
**Canonical implementation roadmap:** [`../../src/toolbox/ROADMAP.md`](../../src/toolbox/ROADMAP.md)

---

## Overview

Tracks public `toolbox` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/toolbox/ROADMAP.md`](../../src/toolbox/ROADMAP.md)

---

## Current Status

All headers in `include/toolbox/` are present and expose the shared text-processing, language detection, chunking, normalisation, quality scoring, fingerprinting, and content-toolbox bridging utilities surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/toolbox/` implementation docs established
- [x] Layer association (**ANN/Tensor**) recorded in this planning document; strategic-plan alignment not independently verified here
- [x] Namespace layout (`themis::toolbox`) documented

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

> Exact model backend not verifiable from final document text alone.
