> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
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
- [x] Layer association (**ANN/Tensor**) marked and verified against `FUTURE_PLAN.md`
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
