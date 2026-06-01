> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/tensor/ROADMAP.md -->

# Tensor Mid-Layer Module — Public Header Roadmap

**Module Path:** `include/tensor/`  
**Canonical implementation roadmap:** [`../../src/tensor/ROADMAP.md`](../../src/tensor/ROADMAP.md)

---

## Overview

Tracks public `tensor` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/tensor/ROADMAP.md`](../../src/tensor/ROADMAP.md)

---

## Current Status

All headers in `include/tensor/` are present and expose the tensor index management, encoder interfaces, HNSW tensor bridges, hyper-index construction, butterfly operators, tensor fingerprints, mmap bridges, and TT decomposition surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/tensor/` implementation docs established
- [x] Layer association (**Tensor Mid-Layer**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::tensor`) documented

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
