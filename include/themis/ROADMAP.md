---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "unknown"
---

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: review | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/themis/ROADMAP.md -->

# Themis Core Identity Module — Public Header Roadmap

**Module Path:** `include/themis/`  
**Canonical implementation roadmap:** [`../../src/themis/ROADMAP.md`](../../src/themis/ROADMAP.md)

---

## Overview

Tracks public `themis` contract documentation, header-level coverage work,
and potential future public entry points. It does not declare API or ABI
stability. Runtime implementation work remains in:

→ [`../../src/themis/ROADMAP.md`](../../src/themis/ROADMAP.md)

---

## Current Status

The top-level headers expose build metadata, edition management, license
gating, module integrity, and export contracts. `include/themis/` also
contains public headers in subdirectories (for example `base/`, `gpu/`,
`llm/`, `network/`, and policy interfaces); this page does not claim that every
header or subdirectory is covered by the summary.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/themis/` implementation docs established
- [ ] Canonical layer association: not established by the current module sources; the earlier "Graph" label and `FUTURE_PLAN.md` reference are unverified.
- [x] Namespace layout (`themis`) documented

---

## In Progress

- [?] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3; status not revalidated)

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
