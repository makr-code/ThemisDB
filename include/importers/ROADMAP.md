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
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/importers/*.h; include/importers/*.hpp; src/importers/ROADMAP.md; VERSIONING.md"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

<!-- Header planning summary; no build or compatibility matrix is asserted. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/importers/ROADMAP.md -->

# Importers Module — Public Header Roadmap

**Module Path:** `include/importers/`  
**Canonical implementation roadmap:** [`../../src/importers/ROADMAP.md`](../../src/importers/ROADMAP.md)

---

## Overview

Tracks public `importers` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/importers/ROADMAP.md`](../../src/importers/ROADMAP.md)

---

## Current Status

The directory contains current declarations for importer interfaces, connectors,
plugins, and specialized helpers. Optional runtime availability and API
compatibility are not implied by the header inventory.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/importers/` implementation docs established
- [x] Namespace layout (`themis::importers`) documented

---

## In Progress

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3)

---

## Planned

- [ ] API contract index for high-frequency entry points with usage examples (Target: 2026-Q3)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Historical checklist (not a current readiness assessment)

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
