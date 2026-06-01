> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
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

All headers in `include/importers/` are present and expose the multi-source data import (PostgreSQL, MySQL, Oracle, MongoDB, Kafka, S3, flat files, CRDT, MDM), schema inference, entity matching, federated learning, and e-government standards surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/importers/` implementation docs established
- [x] Layer association (**ANN/Tensor**) marked and verified against `FUTURE_PLAN.md`
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

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [ ] Deprecation annotations consistent
