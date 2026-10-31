---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/exporters/*.h; src/exporters/ROADMAP.md"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: review | source checked 2026-10-31; milestone status requires maintainer review -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/exporters/ROADMAP.md -->

# Exporters Module — Public Header Roadmap

**Module Path:** `include/exporters/`  
**Canonical implementation roadmap:** [`../../src/exporters/ROADMAP.md`](../../src/exporters/ROADMAP.md)

---

## Overview

Tracks public `exporters` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/exporters/ROADMAP.md`](../../src/exporters/ROADMAP.md)

---

## Current Status

All headers in `include/exporters/` are present and expose the data export in Parquet, Arrow IPC, JSONL/LLM, HuggingFace, streaming, incremental, and encrypted formats surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/exporters/` implementation docs established
- [x] Namespace layout (`themis::exporters`) documented

---

## In Progress

- [ ] Per-header deprecation and breaking-change notices aligned with `VERSIONING.md` (Target: 2026-Q3)
  - TODO: Re-plan the target date; 2026-Q3 has passed.

---

## Planned

TODO: Re-plan the Q3 2026 items below; that target quarter has passed.

- [ ] API contract index for high-frequency entry points with usage examples (Target: 2026-Q3)
- [ ] Compatibility matrix (C++17/20 and platform coverage) per header group (Target: 2026-Q4)
- [ ] CI-enforced compile checks for all public headers (Target: 2026-Q4)

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
