> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/ingestion/ROADMAP.md -->

# Ingestion Module — Public Header Roadmap

**Module Path:** `include/ingestion/`  
**Canonical implementation roadmap:** [`../../src/ingestion/ROADMAP.md`](../../src/ingestion/ROADMAP.md)

---

## Overview

Tracks public `ingestion` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/ingestion/ROADMAP.md`](../../src/ingestion/ROADMAP.md)

---

## Current Status

All headers in `include/ingestion/` are present and expose the multi-connector ingestion pipeline (API, CDC, database, filesystem, HuggingFace, Kafka, S3, web crawler), entity assembly, semantic validation, and workflow orchestration surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/ingestion/` implementation docs established
- [x] Layer association (**ANN Frontdoor**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::ingestion`) documented

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
