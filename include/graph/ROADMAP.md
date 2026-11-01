---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: api-contract
Canonical source references:
  - ../../ROADMAP.md
  - ../../src/graph/ROADMAP.md
  - ./
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

> **Build:** see [`tests/graph/README.md`](../../tests/graph/README.md) for a test-enabled
> configuration; release preset settings are not suitable for focused tests.

<!-- Historical header-roadmap snapshot validated: 2026-06-01; dates below require owner review. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/graph/ROADMAP.md -->

# Graph Module — Public Header Roadmap

> The root [`ROADMAP.md`](../../ROADMAP.md) is the repository-level maturity aggregate; the
> header-specific dates and checkboxes below are an older planning snapshot, not current
> commitments or compatibility guarantees.

**Module Path:** `include/graph/`  
**Canonical implementation roadmap:** [`../../src/graph/ROADMAP.md`](../../src/graph/ROADMAP.md)

---

## Overview

Tracks public `graph` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/graph/ROADMAP.md`](../../src/graph/ROADMAP.md)

---

## Current Status

The directory contains graph-facing headers for traversal, planning, reasoning, ontology, tensor,
and related utility surfaces. A declared interface does not establish a concrete backend
implementation; consult its source and current CMake registration.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/graph/` implementation docs established
- [x] Layer association (**Graph Truth Layer**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::graph`) documented

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
