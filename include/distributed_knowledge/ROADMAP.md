> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/distributed_knowledge/ROADMAP.md -->

# Distributed Knowledge Module — Public Header Roadmap

**Module Path:** `include/distributed_knowledge/`  
**Canonical implementation roadmap:** [`../../src/distributed_knowledge/ROADMAP.md`](../../src/distributed_knowledge/ROADMAP.md)

---

## Overview

Tracks public `distributed_knowledge` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/distributed_knowledge/ROADMAP.md`](../../src/distributed_knowledge/ROADMAP.md)

---

## Current Status

All headers in `include/distributed_knowledge/` are present and expose the federated RAG merging, LoRA federation, federated distillation, cross-shard feedback sync, and adapter capability announcements surface.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/distributed_knowledge/` implementation docs established
- [x] Layer association (**LLM/Tensor**) marked and verified against `FUTURE_PLAN.md`
- [x] Namespace layout (`themis::distributed_knowledge`) documented

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
