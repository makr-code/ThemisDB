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

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/projects/ROADMAP.md -->

# Projects Module — Public Header Roadmap

**Module Path:** `include/projects/`  
**Canonical implementation roadmap:** [`../../src/projects/ROADMAP.md`](../../src/projects/ROADMAP.md)

---

## Overview

Tracks public `projects` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/projects/ROADMAP.md`](../../src/projects/ROADMAP.md)

---

## Current Status

Headers declare project lifecycle, versioning, diff/merge, collaboration,
templates, metrics, and audit-log APIs. `project_bundle.h` declares only an
abstract interface; this module has no concrete bundle implementation.
`projects_api_contract.h` currently defines error-code values only.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/projects/` implementation docs established
- [x] Layer association (**Graph**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::projects`) documented

---

## In Progress

- [!] Review per-header deprecation and breaking-change notices against `VERSIONING.md` (Target: TBD; prior 2026-Q3 target has passed)

---

## Planned

- [!] API contract index for high-frequency entry points with usage examples (Target: TBD; prior 2026-Q3 target has passed)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
