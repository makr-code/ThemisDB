> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/ingestion/ROADMAP.md -->

# Ingestion Module — Public Header Roadmap

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

**Module Path:** `include/ingestion/`  
**Canonical implementation roadmap:** [`../../src/ingestion/ROADMAP.md`](../../src/ingestion/ROADMAP.md)

---

## Overview

Tracks public `ingestion` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/ingestion/ROADMAP.md`](../../src/ingestion/ROADMAP.md)

> **Status note (2026-10-31):** Checklist targets below are retained planning
> entries, not evidence that the work is still open or that it has shipped.
> Reconcile each item against current headers, CMake, and tests before using it
> for release planning. This file does not certify production readiness.

---

## Current Status

The directory currently contains public headers for source management,
connectors, workflow steps, validation, coordination, sink adapters, and
reindex/canary helper APIs. Connector behavior and maturity vary; header
presence alone does not demonstrate end-to-end implementation.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/ingestion/` implementation docs established
- [ ] TODO: identify a current authoritative architecture source before asserting a layer association; the previous **ANN Frontdoor** label is unverified.
- [x] Namespace layout (`themis::ingestion`) documented

---

## In Progress

- [ ] TODO: verify current deprecation annotations and migration notes against headers before retaining this target (historical target: 2026-Q3)

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
