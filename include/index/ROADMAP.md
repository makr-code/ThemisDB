---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/index/ROADMAP.md -->

# Index Module — Public Header Roadmap

**Module Path:** `include/index/`  
**Canonical implementation roadmap:** [`../../src/index/ROADMAP.md`](../../src/index/ROADMAP.md)

---

## Overview

Tracks public `index` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/index/ROADMAP.md`](../../src/index/ROADMAP.md)

---

## Current Status

All headers in `include/index/` are present and expose the vector indexes (HNSW, ANN, GPU), graph indexes, secondary/spatial/temporal indexes, product quantisation, learned indexes, and multi-GPU support surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/index/` implementation docs established
- [x] Current API reference linked to source-checked behavior in `src/index/API.md`
- [!] Namespace placement must be read from each declaring header; a module-wide namespace claim is not verified.

---

## In Progress

- [?] Rebaseline the per-header deprecation/breaking-change review; the previous Q3 2026 target has elapsed and no replacement milestone is confirmed.

---

## Planned

- [x] Source-checked API reference for high-frequency entry points: `../../src/index/API.md`
- [ ] Compatibility matrix for the repository's C++20 baseline and supported platforms (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
