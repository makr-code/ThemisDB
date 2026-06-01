> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/acceleration/ROADMAP.md -->

# Acceleration Module — Public Header Roadmap

**Module Path:** `include/acceleration/`  
**Canonical implementation roadmap:** [`../../src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md)

---

## Overview

Tracks public `acceleration` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/acceleration/ROADMAP.md`](../../src/acceleration/ROADMAP.md)

---

## Current Status

All headers in `include/acceleration/` are present and expose the compute-backend dispatch, GPU/CPU/Vulkan/OpenCL acceleration, FAISS GPU KNN, tensor-core matmul, multi-GPU coordination, and kernel fallback surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/acceleration/` implementation docs established
- [x] Layer association (**ANN/Tensor**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::acceleration`) documented

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
