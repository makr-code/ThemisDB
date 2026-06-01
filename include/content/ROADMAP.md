> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/content/ROADMAP.md -->

# Content Processing Module — Public Header Roadmap

**Module Path:** `include/content/`  
**Canonical implementation roadmap:** [`../../src/content/ROADMAP.md`](../../src/content/ROADMAP.md)

---

## Overview

Tracks public `content` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/content/ROADMAP.md`](../../src/content/ROADMAP.md)

---

## Current Status

All headers in `include/content/` are present and expose the multi-format content ingestion and processing (PDF, Office, images, video, audio, CAD, HTML, Markdown, OCR), abuse detection, PII redaction, deduplication, embedding pipelines, and geo/language processing surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/content/` implementation docs established
- [x] Layer association (**ANN/Tensor**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::content`) documented

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
