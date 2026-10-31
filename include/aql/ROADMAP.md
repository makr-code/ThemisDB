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

Exact model backend not verifiable from final document text alone.

<!-- Historical header-roadmap status; target dates and completion marks are not current verification. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/aql/ROADMAP.md -->

# AQL (AI Query Language) Module — Public Header Roadmap

**Module Path:** `include/aql/`  
**Canonical implementation roadmap:** [`../../src/aql/ROADMAP.md`](../../src/aql/ROADMAP.md)

---

## Overview

Tracks public `aql` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/aql/ROADMAP.md`](../../src/aql/ROADMAP.md)

---

## Current Status

The current public header group is mapped in [`ARCHITECTURE.md`](ARCHITECTURE.md).
The completion marks and target dates below are retained from an older planning
snapshot; **TODO:** reconcile them against current API/versioning policy before
treating them as active commitments.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/aql/` implementation docs established
- [x] Header grouping and implementation references documented
- [ ] Confirm namespace and API stability claims per individual header; namespace declarations are not uniform across every AQL header.

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
